 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2015 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*
 * ExternalSortFilter.txx
 *
 *  Created on: 14/01/2015
 *      Author: alex
 */

#ifndef __otbExternalSortFilter_txx
#define __otbExternalSortFilter_txx

#include "nmlog.h"
#include "otbSortFilter.h"
#include "otbExternalSortFilter.h"
#include "otbStreamingRATImageFileWriter.h"
//#include "itkImageRegionIterator.h"
//#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkExceptionObject.h"
#include "itkDataObject.h"
#include "itkImageSource.h"


#ifdef _WIN32
    #define CHAR_PATHDEVIDE "\\"
#else
    #define CHAR_PATHDEVIDE "/"
#endif

namespace otb {

template <class TInputImage, class TOutputImage>
ExternalSortFilter<TInputImage, TOutputImage>
::ExternalSortFilter()
     : m_SortAscending(false),
       m_MaxChunkSize(128)
#ifdef BUILD_RASSUPPORT
      ,m_Rasconn(0)
#endif
{
    this->SetNumberOfRequiredInputs(0);
}

template <class TInputImage, class TOutputImage>
ExternalSortFilter<TInputImage, TOutputImage>
::~ExternalSortFilter()
{
}

#ifdef BUILD_RASSUPPORT
template <class TInputImage, class TOutputImage>
void
ExternalSortFilter<TInputImage, TOutputImage>
::SetRasdamanConnector(RasdamanConnector* rascon)
{
    this->m_Rasconn = rascon;
}
#endif

template <class TInputImage, class TOutputImage>
void ExternalSortFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
    os << indent << "ExternalSortFilter - sorting "
       << this->m_FileNames.size()
		<< " layers." << std::endl;
}

template <class TInputImage, class TOutputImage>
void ExternalSortFilter<TInputImage, TOutputImage>
::SetNthFileName(int idx, string filename)
{
    m_FileNames.resize(idx+1);
    m_FileNames[idx] = filename;
}

template <class TInputImage, class TOutputImage>
void ExternalSortFilter<TInputImage, TOutputImage>
::GenerateOutputFileNames(std::vector<string> &outputFN)
{
    std::string sortIndicator = "_desc";
    if (m_SortAscending)
    {
        sortIndicator = "_asc";
    }

    std::string idxFN;
    for (int f=0; f < m_FileNames.size(); ++f)
    {
        std::string outFN = m_FileNames[f];
        std::string::size_type pos = m_FileNames[f].rfind('.');

        // look after rasterman names
        if (pos == std::string::npos)
        {
            // extract the collection name and
            // append the sort indicator string
            pos = m_FileNames[f].rfind(':');
            if (pos != std::string::npos)
            {
                outFN = outFN.substr(0, pos);
            }
            outFN.append(sortIndicator);
            outputFN.push_back(outFN);
            continue;
        }

        outFN.insert(pos, sortIndicator);
        outputFN.push_back(outFN);

        if (f == 0)
        {
            idxFN = outFN;
            idxFN.insert(pos, "_idx");
        }
    }

    // add a name for the output index image
    outputFN.push_back(idxFN);
}


template <class TInputImage, class TOutputImage>
void ExternalSortFilter<TInputImage, TOutputImage>
::GenerateData(void)
{
    NMDebugCtx(ctxExternalSortFilter, << "...")
    if (m_FileNames.size() == 0)
    {
        throw itk::ExceptionObject(__FILE__, __LINE__,
                             "No input file names specified!",
                                   __FUNCTION__);
        NMDebugCtx(ctxExternalSortFilter, << "done!")
        return;
    }

    // number of inputs plus one index image as output
    int numOutputs = m_FileNames.size()+1;
    this->SetNumberOfIndexedOutputs(numOutputs);

    // =============================================================
    // READERS & WRITERS
    // =============================================================

    // initiate image readers
    std::vector<ImageReaderPointerType> readers;
    for (int f=0; f < m_FileNames.size(); ++f)
    {
        ImageReaderPointerType r = ImageReaderType::New();
#ifdef BUILD_RASSUPPORT
        if (    this->m_Rasconn
            &&  m_FileNames.at(f).find('.') == std::string::npos
           )
        {
            r->SetRasdamanConnector(this->m_Rasconn);
        }
#endif
        r->SetFileName(m_FileNames.at(f));
        readers.push_back(r);
    }

    // set up regions for writing image chunks
    InputImageType* img0 = static_cast<InputImageType*>(readers.at(0)->GetOutput());
    img0->UpdateOutputInformation();
    InputImageRegionType lpr = img0->GetLargestPossibleRegion();

    itk::ImageIORegion wior(lpr.ImageDimension);
    for (int d=0; d < lpr.ImageDimension; ++d)
    {
        wior.SetIndex(d, lpr.GetIndex(d));
        wior.SetSize(d, lpr.GetSize(d));
    }


    // initiate image writers
    std::vector<std::string> outNames;
    this->GenerateOutputFileNames(outNames);

    std::vector<ImageWriterPointerType> writers;
    for (int wr=0; wr < outNames.size()-1; ++wr)
    {
        ImageWriterPointerType iw = ImageWriterType::New();
#ifdef BUILD_RASSUPPORT
        if (readers.at(wr)->GetRasdamanConnector() != 0)
        {
            iw->SetRasdamanConnector(readers.at(wr)->GetRasdamanConnector());
        }
#endif
        iw->SetForcedLargestPossibleRegion(wior);
        writers.push_back(iw);
    }

    IndexImageWriterPointerType idxWriter = IndexImageWriterType::New();
#ifdef BUILD_RASSUPPORT
    if (readers.at(0)->GetRasdamanConnector() != 0)
    {
        idxWriter->SetRasdamanConnector(readers.at(0)->GetRasdamanConnector());
    }
#endif
    idxWriter->SetForcedLargestPossibleRegion(wior);

    // =======================================================
    // PRE-SORT CHUNKS
    // =======================================================

    // work out the chunck size for processing image files
    int pixsize   = readers.at(0)->GetImageIO()->GetComponentSize();
    int imagechunk = (m_MaxChunkSize * 1024*1024) / (numOutputs*2) / pixsize;

    std::vector<std::string> chunknames =
            PreSortChunks(outNames, readers, writers, idxWriter, lpr, wior, imagechunk);

    if (chunknames.size() == 0)
    {
        NMDebugAI(<< "Pre-sorting of image chunks failed or aborted!" << endl);
        NMDebugCtx(ctxExternalSortFilter, << "done!");
    }

    // =======================================================
    // FINE-SORT CHUNKS
    // =======================================================

    bool sorted = false;
    if (!this->GetAbortGenerateData())
    {
        sorted = FineSortChunks(chunknames, outNames, writers, idxWriter, lpr, wior, imagechunk);
    }

    if (sorted)
    {
        NMDebugAI( << "ALL SORTED MATE !!!" << endl);
    }
    else
    {
        NMDebugAI( << "BACK TO THE DRAWING BOARD ... " << endl);
    }

    NMDebugCtx(ctxExternalSortFilter, << "done!")
}

template <class TInputImage, class TOutputImage>
bool
ExternalSortFilter<TInputImage, TOutputImage>
::FineSortChunks(std::vector<std::string>& chunkNames,
                 std::vector<std::string>& outNames,
                 std::vector<ImageWriterPointerType>& writers,
                 IndexImageWriterPointerType& idxWriter,
                 InputImageRegionType& lpr,
                 itk::ImageIORegion& wior,
                 int imagechunk
                 )
{
    NMDebugCtx(ctxExternalSortFilter, << "...");

    // BOOK KEEPING  - PRE-SORTED IMAGES
    std::vector< std::vector< ChunkReaderPointerType > > imgChkReaders;
    std::vector< std::vector< OutputImagePixelType* > >  imgChkBuffers;
    std::vector< InputImageRegionType >   imgChkLPR;
    std::vector< int >                    imgChkOffset;
    std::vector< int >                    imgChkLength;
    std::vector< int >                    imgChkSplitNum;
    std::vector< int >                    imgChkNumSplitSplits;

    // BOOK KEEPING - INDEX IMAGE
    std::vector< IndexReaderPointerType > idxChkReaders;
    std::vector< IndexImagePixelType* >   idxChkBuffers;
    std::vector< IndexImagePixelType >    idxChkOffset;     // since index image chunks hold
                                                            // only 'local' chunk index rather
                                                            // the global one, we generate a
                                                            // buffer storing the appropriate
                                                            // index offset to this chunk, so
                                                            // we can piece everything together
                                                            // as it should be


    // BOOK KEEPING - OUTPUTIMAGES
    std::vector< OutputImagePixelType* >  outBuffers;
    outBuffers.resize(writers.size(), 0);
    IndexImagePixelType*                  idxOutBuffer;

    // chunk region stuff
    itk::NMImageRegionSplitterMaxSize::Pointer splitter =
            itk::NMImageRegionSplitterMaxSize::New();
    int numSplits = splitter->GetNumberOfSplits(lpr, imagechunk);
    InputImageRegionType chunkRegion;

    // probably, we've got to split the chunk regions since we're accessing
    // all of them at the same time (may prove to be utterly slow because of lots
    // of reading and disk access bottleneck ... we'll find out!)
    itk::NMImageRegionSplitterMaxSize::Pointer ssp =
            itk::NMImageRegionSplitterMaxSize::New();
    long maxsplitsize = ((double)imagechunk / (double)(numSplits)) + 0.5;
    long splitsize = maxsplitsize;
    int numSplitSplits = 1;

    // ==============================================================
    // VECTOR OF READERS FOR EACH IMAGE AND FOR EACH IMAGE CHUNK
    // ==============================================================

    NMDebugAI(<< "============================" << endl);
    NMDebugAI(<< "PREP WORK READERS & WRITERS" << std::endl);
    NMDebugAI(<< "============================" << endl);

    // ------------------------------------------------------
    // INIT READERS + other book keeping preparations
    // ------------------------------------------------------

    // for each image prepare numSplits readers ...
    // except for the index readers, they've got a different type
    for (int r=0; r < chunkNames.size(); ++r)
    {
        std::vector< ChunkReaderPointerType > chkReaders;
        std::vector< OutputImagePixelType* >  chkBuffers;
        for (int split=0; split < numSplits; ++split)
        {
            chunkRegion = lpr;
            splitter->GetSplit(split, imagechunk, chunkRegion);

            // calc the max split region (chunk region) size
            // to get a feeling for how much memory is
            // available for each chunk region
            if (r == 0)   // only need to do this once
            {
                splitsize = chunkRegion.GetNumberOfPixels() * sizeof(IndexImagePixelType);
                if (maxsplitsize < splitsize)
                {
                    numSplitSplits = ssp->GetNumberOfSplits(chunkRegion, maxsplitsize);
                    imgChkNumSplitSplits.push_back(numSplitSplits);
                }
                else
                {
                    imgChkNumSplitSplits.push_back(1);
                }
            }

            if (r < chunkNames.size()-1)
            {
                ChunkReaderPointerType cr = ChunkReaderType::New();
#ifdef BUILD_RASSUPPORT
                cr->SetRasdamanConnector(writers.at(r)->GetRasdamanConnector());
#endif
                cr->SetFileName(chunkNames.at(r));
                cr->GetOutput()->SetLargestPossibleRegion(chunkRegion);
                chkReaders.push_back(cr);
                chkBuffers.push_back(0);
            }
            else
            {
                IndexReaderPointerType ir = IndexReaderType::New();
#ifdef BUILD_RASSUPPORT
                ir->SetRasdamanConnector(idxWriter->GetRasdamanConnector());
#endif
                ir->SetFileName(chunkNames.at(r));
                ir->GetOutput()->SetLargestPossibleRegion(chunkRegion);
                idxChkReaders.push_back(ir);
                idxChkBuffers.push_back(0);
                imgChkOffset.push_back(0);
                imgChkLength.push_back(0);
                imgChkSplitNum.push_back(-1);
                imgChkLPR.push_back(chunkRegion);

                // create index offset per chunk to map between stored 'local'
                // index and proper whole image index
                if (split == 0)
                {
                    idxChkOffset.push_back(0);
                }
                else
                {
                    IndexImagePixelType tr1 = imgChkLPR.at(split-1).GetNumberOfPixels();
                    tr1 += idxChkOffset[split-1];
                    idxChkOffset.push_back(tr1);
                }
            }
        }

        // add to the vector of readers of image chunks
        if (r < chunkNames.size()-1)
        {
            imgChkReaders.push_back(chkReaders);
            imgChkBuffers.push_back(chkBuffers);
        }
    }
    NMDebugAI(<< "maxsplitsize   = " << maxsplitsize << endl);
    NMDebugAI(<< "splitsize      = " << splitsize << endl);
    NMDebugAI(<< "numSplitSplits = " << numSplitSplits << endl);
    NMDebug(<< endl);


    // ------------------------------------------------------
    // INIT WRITERS
    // ------------------------------------------------------
    for (int on=0; on < outNames.size()-1; ++on)
    {
        OutputImageType* oldImg = const_cast<OutputImageType*>(writers.at(on)->GetInput());

        OutputImagePointer outImg = OutputImageType::New();
        outImg->CopyInformation(oldImg);

        writers.at(on)->SetFileName(outNames.at(on));
        writers.at(on)->SetUpdateMode(true);
        writers.at(on)->SetInput(outImg);
    }

    IndexImageType* oldIdx = const_cast<IndexImageType*>(
                idxWriter->GetInput());

    IndexImagePointer idxImg = IndexImageType::New();
    idxImg->CopyInformation(oldIdx);

    idxWriter->SetFileName(outNames.at(outNames.size()-1));
    idxWriter->SetUpdateMode(true);
    idxWriter->SetInput(idxImg);


    // ==============================================================
    // FINE SORT
    // ==============================================================

    NMDebugAI(<< "=========================" << endl);
    NMDebugAI(<< "FINE SORT ... " << std::endl);
    NMDebugAI(<< "=========================" << endl);
    OutputImagePixelType tmp, val;

    int outputSplit = -1;
    int outputLength = 0;
    int outputOffset = 0;

    long numpix = lpr.GetNumberOfPixels();
    long pixcnt = 0;
    int valChkIdx = -1;

    while (pixcnt < numpix && !this->GetAbortGenerateData())
    {
        if (this->m_SortAscending)
        {
            val = itk::NumericTraits<OutputImagePixelType>::max();
        }
        else
        {
            val = itk::NumericTraits<OutputImagePixelType>::NonpositiveMin();
        }
        valChkIdx = -1;

        // find the next biggest value out of all chunks of the
        // primary 'sort image'
        for (int chunk=0; chunk < numSplits; ++chunk)
        {
            // since each chunk is split into further chunk, we
            // make sure there is a next pixel available;
            if (imgChkOffset.at(chunk) >= imgChkLength.at(chunk))
            {
                if (imgChkSplitNum.at(chunk) < imgChkNumSplitSplits.at(chunk)-1)
                {
                    forwardChunkSplit(chunk, maxsplitsize, ssp,
                                      idxChkReaders, idxChkBuffers,
                                      imgChkReaders, imgChkBuffers,
                                      imgChkLPR,
                                      imgChkOffset, imgChkLength, imgChkSplitNum);
                    NMDebugAI(<< "pixel: " << pixcnt << std::endl);
                }
                // if we've processed all pixels of this chunk, we skip it and
                // go to the next
                else
                {
                    //NMDebugAI(<<"nope - done this chunk ..." << std::endl);
                    continue;
                }
            }

            // is this pixel bigger than the ones we've looked at?
            // if yes, remember which chunk it was
            tmp = imgChkBuffers.at(0).at(chunk)[imgChkOffset.at(chunk)];
            if (this->m_SortAscending)
            {
                if (tmp <= val)
                {
                    val = tmp;
                    valChkIdx = chunk;
                }
            }
            else
            {
                if (tmp >= val)
                {
                    val = tmp;
                    valChkIdx = chunk;
                }
            }
        }

        // make sure we've got an appropriate output region allocated
        if (outputOffset >= outputLength)
        {
            forwardOutputSplit(outputSplit,
                               numSplits,
                               imagechunk,
                               splitter,
                               writers, idxWriter,
                               idxOutBuffer, outBuffers,
                               outputOffset, outputLength);
            NMDebugAI(<< "pixel: " << pixcnt << std::endl);
        }

        for (int i=0; i < writers.size(); ++i)
        {
            outBuffers[i][outputOffset] =
                    static_cast<OutputImagePixelType>(
                     imgChkBuffers[i][valChkIdx][imgChkOffset[valChkIdx]]);
        }
        idxOutBuffer[outputOffset] = static_cast<IndexImagePixelType>(
                    idxChkBuffers[valChkIdx][imgChkOffset.at(valChkIdx)]
                    + idxChkOffset[valChkIdx]);
        ++imgChkOffset.at(valChkIdx);
        ++outputOffset;
        ++pixcnt;
    }

    // write the final pieces
    for (int wr=0; wr < writers.size(); ++wr)
    {
        writers.at(wr)->Update();
    }
    idxWriter->Update();

    NMDebugCtx(ctxExternalSortFilter, << "done!");
    return true;
}


template <class TInputImage, class TOutputImage>
void
ExternalSortFilter<TInputImage, TOutputImage>
::forwardOutputSplit(int& outputSplit,
        const int& numSplits,
        const int& maxSplitSize,
        itk::NMImageRegionSplitterMaxSize::Pointer& splitter,
        std::vector< ImageWriterPointerType >& writers,
        IndexImageWriterPointerType&           idxWriter,
        IndexImagePixelType* &                  idxOutBuffer,
        std::vector<OutputImagePixelType*>&    outBuffers,
        int&                    outputOffset,
        int&                    outputLength
        )
{
    // ============================================================
    // WRITE PREVIOUS REGION
    // ============================================================

    if (outputSplit >= 0)
    {
        for (int wr=0; wr < writers.size(); ++wr)
        {
            writers.at(wr)->Update();
        }
        idxWriter->Update();
    }

    if (outputSplit >= numSplits)
    {
        NMDebugAI(<< "no more write splits ..." << std::endl);
        return;
    }
    ++outputSplit;
    // ============================================================
    // FORWARD REGION
    // ============================================================
    itk::ImageIORegion ioRegion = idxWriter->GetForcedLargestPossibleRegion();
    OutputImageRegionType region;

    // advance outer split region by one
    // set new inner split region back to first region (i.e. idx=0)
    for (int d=0; d < region.GetImageDimension(); ++d)
    {
        region.SetIndex(d, ioRegion.GetIndex(d));
        region.SetSize(d, ioRegion.GetSize(d));
    }

    splitter->GetSplit(outputSplit, maxSplitSize, region);


    NMDebugAI(<< "WS: " << outputSplit << " : "
              << region.GetIndex(0) << ", " << region.GetIndex(1) << " "
              << region.GetSize(0) << "x" << region.GetSize(1) << " = "
              << region.GetNumberOfPixels() << std::endl);


    // copy current region settings for the io region for writers
    for (int d=0; d < ioRegion.GetImageDimension(); ++d)
    {
        ioRegion.SetIndex(d, region.GetIndex(d));
        ioRegion.SetSize(d, region.GetSize(d));
    }

    // ============================================================
    // ALLOCATE NEW OUTPUT FOR NEW REGION
    // ============================================================
    outputOffset = 0;
    outputLength = region.GetNumberOfPixels();

    // set-up new output buffers
    // allocate new output buffers
    for (int on=0; on < writers.size(); ++on)
    {
        OutputImageType* oldImg = const_cast<OutputImageType*>(
                    writers.at(on)->GetInput());

        OutputImagePointer outImg = OutputImageType::New();
        outImg->CopyInformation(oldImg);
        outImg->SetLargestPossibleRegion(region);
        outImg->SetRequestedRegion(region);
        outImg->SetBufferedRegion(region);
        outImg->Allocate();

        //NMDebugAI(<< "writer #" << on << ": new out image..." << endl);
        //outImg->Print(std::cout, itk::Indent(nmlog::nmindent));

        writers.at(on)->SetInput(outImg);
        writers.at(on)->SetUpdateRegion(ioRegion);

        OutputImagePixelType* outBuffer = static_cast<OutputImagePixelType*>(
                    outImg->GetBufferPointer());
        outBuffers.at(on) = outBuffer;
    }

    IndexImageType* oldIdx = const_cast<IndexImageType*>(
                idxWriter->GetInput());

    IndexImagePointer idxImg = IndexImageType::New();
    idxImg->CopyInformation(oldIdx);
    idxImg->SetLargestPossibleRegion(region);
    idxImg->SetRequestedRegion(region);
    idxImg->SetBufferedRegion(region);
    idxImg->Allocate();

    idxWriter->SetInput(idxImg);
    idxWriter->SetUpdateRegion(ioRegion);

    idxOutBuffer = static_cast<IndexImagePixelType*>(idxImg->GetBufferPointer());
}


template <class TInputImage, class TOutputImage>
void
ExternalSortFilter<TInputImage, TOutputImage>
::forwardChunkSplit(const int& splitNum,
        const int& maxSplitSplitSize,
        itk::NMImageRegionSplitterMaxSize::Pointer& ssp,
        std::vector< IndexReaderPointerType >&                idxChkReaders,
        std::vector< IndexImagePixelType* >&                  idxChkBuffers,
        std::vector< std::vector< ChunkReaderPointerType > >& imgChkReaders,
        std::vector< std::vector< OutputImagePixelType* > >&  imgChkBuffers,
        std::vector<InputImageRegionType> &imgChkLPR,
        std::vector<int>& imgChkOffset,
        std::vector<int>& imgChkLength,
        std::vector<int>& imgChkSplitNum
        )
{
    // get largest possible region as input for the region splitter
    OutputImageType* img = static_cast<OutputImageType*>(
                           imgChkReaders.at(0).at(splitNum)->GetOutput());
    OutputImageRegionType ssreg = imgChkLPR.at(splitNum);
            //img->GetLargestPossibleRegion();

    // forward the splitsplit number and fetch associated new region
    int splitsplit = imgChkSplitNum.at(splitNum) + 1;
    ssp->GetSplit(splitsplit, maxSplitSplitSize, ssreg);

    NMDebugAI(<< "RS: " << splitNum << "-" << splitsplit << " : "
              << ssreg.GetIndex(0) << ", " << ssreg.GetIndex(1) << " "
              << ssreg.GetSize(0) << "x" << ssreg.GetSize(1) << " = "
              << ssreg.GetNumberOfPixels() << std::endl);

    for (int i=0; i < imgChkReaders.size(); ++i)
    {
        OutputImageType* img = static_cast<OutputImageType*>(
                               imgChkReaders.at(i).at(splitNum)->GetOutput());

        img->SetRequestedRegion(ssreg);
        img->Update();

        imgChkBuffers[i][splitNum]  = static_cast<OutputImagePixelType*>(img->GetBufferPointer());
    }
    imgChkOffset[splitNum] = 0;
    imgChkLength[splitNum] = ssreg.GetNumberOfPixels();
    imgChkSplitNum[splitNum] = splitsplit;

    IndexImageType* idxImg = static_cast<IndexImageType*>(
                        idxChkReaders.at(splitNum)->GetOutput());
    idxImg->SetRequestedRegion(ssreg);
    idxImg->Update();

    idxChkBuffers[splitNum] = static_cast<IndexImagePixelType*>(idxImg->GetBufferPointer());
}


template <class TInputImage, class TOutputImage>
std::vector<std::string>
ExternalSortFilter<TInputImage, TOutputImage>
::PreSortChunks(std::vector<std::string>& outNames,
        std::vector<ImageReaderPointerType>& readers,
        std::vector<ImageWriterPointerType>& writers,
        IndexImageWriterPointerType& idxWriter,
        InputImageRegionType& lpr,
        itk::ImageIORegion& wior,
        int imagechunk
        )
{
//    NMDebugCtx(ctxExternalSortFilter, << "...");

    std::vector<std::string> chunknames;

    // ==========================================================
    // CREATE CHUNKNAMES
    // ==========================================================

    std::string fn;
    for (int wr=0; wr < outNames.size(); ++wr)
    {
        fn = outNames.at(wr);
        std::string::size_type pos = fn.rfind(CHAR_PATHDEVIDE);
        std::stringstream chkstr;
        chkstr << "tmpick" << "_";
        if (pos == std::string::npos)
        {
            pos = 0;
        }
        else
        {
            ++pos;
        }
        fn.insert(pos, chkstr.str());
        chunknames.push_back(fn);

        if (wr < outNames.size()-1)
        {
            writers.at(wr)->SetFileName(fn);
            writers.at(wr)->SetUpdateMode(true);
        }
    }
    idxWriter->SetFileName(chunknames[chunknames.size()-1]);
    idxWriter->SetUpdateMode(true);

    itk::NMImageRegionSplitterMaxSize::Pointer splitter = itk::NMImageRegionSplitterMaxSize::New();
    int numSplits = splitter->GetNumberOfSplits(lpr, imagechunk);

    NMDebugAI(<< "Pre-sorting " << numSplits << " chunks ..." << endl);

    InputImageRegionType procRegion = lpr;
    for (int s=0; s < numSplits; ++s)
    {
        procRegion = lpr;
        splitter->GetSplit(s, imagechunk, procRegion);

        for (int d=0; d < lpr.ImageDimension; ++d)
        {
            wior.SetIndex(d, procRegion.GetIndex(d));
            wior.SetSize(d, procRegion.GetSize(d));
        }

        // ===========================================================
        // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
        NMDebugAI(<< "#" << s << " - Index: ");
        for (int i=0; i < lpr.ImageDimension; ++i)
        {
            NMDebug(<< procRegion.GetIndex(i) << " ");
        }

        NMDebug(<< " Size: ");
        for (int i=0; i < lpr.ImageDimension; ++i)
        {
            NMDebug(<< procRegion.GetSize(i) << " ");
        }
        NMDebug(<< std::endl);
        // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
        // ===========================================================

        // ===========================================================
        // PRIME THE SORTER & SORT
        // ===========================================================

        typename SortFilterType::Pointer sorter = SortFilterType::New();
        sorter->SetSortAscending(this->m_SortAscending);

        for (int i=0; i < readers.size(); ++i)
        {
            if (readers.at(i)->GetOutput() == 0)
            {
                std::stringstream msg;
                msg << "Couldn't read input image '"
                    << m_FileNames.at(s) << "'!";
                throw itk::ExceptionObject(__FILE__, __LINE__,
                                           msg.str(),
                                           __FUNCTION__);
                NMDebugCtx(ctxExternalSortFilter, << "done!");
                chunknames.clear();
                return chunknames;
            }

            readers.at(i)->GetOutput()->SetRequestedRegion(procRegion);
            readers.at(i)->Update();
            InputImagePointer img = readers.at(i)->GetOutput();
            img->DisconnectPipeline();

            sorter->SetInput(i, img);
        }

        sorter->Update();

        // ===========================================================
        // WRITE SORTED CHUNKS
        // ===========================================================

        for (int wr = 0; wr < chunknames.size()-1; ++wr)
        {
            writers.at(wr)->SetInput(sorter->GetOutput(wr));
            writers.at(wr)->SetUpdateRegion(wior);
            writers.at(wr)->Update();
        }
        idxWriter->SetInput(sorter->GetIndexImage());
        idxWriter->SetUpdateRegion(wior);
        idxWriter->Update();
    }

//    NMDebugCtx(ctxExternalSortFilter, << "done!");

    return chunknames;
}

} // end namespace

#endif
