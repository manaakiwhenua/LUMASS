/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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

#ifndef __otbUniqueCombinationFilter_txx
#define __otbUniqueCombinationFilter_txx

#include "nmlog.h"
#include "otbUniqueCombinationFilter.h"
#include "otbCombineTwoFilter.h"
#include "otbNMImageReader.h"
#include "otbStreamingRATImageFileWriter.h"
#include "otbRATBandMathImageFilter.h"
#include "otbSQLiteTable.h"


#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

#include <ctime>

namespace otb
{

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::UniqueCombinationFilter()
    : m_StreamingProc(false),
      m_DropTmpTables(true),
      m_UVTable(0),
      m_UVTableIndex(0),
      m_UVTableName(""),
      m_WorkingDirectory(""),
      m_OutIdx(0)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    m_UVTable = otb::SQLiteTable::New();
}

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::~UniqueCombinationFilter()
{
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetNthInput(unsigned int idx, const TInputImage* image)
{
    this->SetInput(idx, const_cast<TInputImage *>( image ));
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    AttributeTable::Pointer tab = m_UVTable.GetPointer();
    return tab;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer table)
{
    // the user is always right
    for (int i=this->m_vInRAT.size(); i <= idx; ++i)
    {
        this->m_vInRAT.push_back(0);
    }

    this->m_vInRAT[idx] = table;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetInputNodata(const std::vector<long long>& inNodata)
{
    m_InputNodata.clear();
    for (int i=0; i < inNodata.size(); ++i)
    {
        m_InputNodata.push_back(static_cast<InputPixelType>(inNodata.at(i)));
    }
}

//template< class TInputImage, class TOutputImage >
//AttributeTable::Pointer
//UniqueCombinationFilter< TInputImage, TOutputImage >
//::getRAT(unsigned int idx)
//{
//    if (idx < this->m_vOutRAT.size())
//    {
//        return this->m_vOutRAT.at(idx);
//    }
//    return 0;
//}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetUVTableName(const std::string& name)
{
    if (name.empty())
    {
        m_DropTmpTables = true;
    }
    else
    {
        m_DropTmpTables = false;
    }
    this->m_UVTableName = name;
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::InternalAllocateOutput()
{
    typename TInputImage::ConstPointer inImg = this->GetInput();
    typename TOutputImage::Pointer outImg = this->GetOutput();
    outImg->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
    outImg->SetBufferedRegion(inImg->GetBufferedRegion());
    outImg->SetRequestedRegion(inImg->GetRequestedRegion());
    outImg->Allocate();

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::GenerateData()
{
    NMDebugCtx(ctx, << "...");

    unsigned int nbInputs = this->GetNumberOfIndexedInputs();
    unsigned int nbRAT = this->m_vInRAT.size();
    if (nbInputs < 2 || nbRAT < 2)
    {
        itkExceptionMacro(<< "Need at least two input layers to work!");
        return;
    }

    // ======================================================================
    // IMAGE AXES DIMENSION CHECK
    // ======================================================================

    // check, whether all inputs have the same dimension
    // (... and hopefully also the same projection - but
    // we don't check that ...)
    IndexType inputSize[2];
    inputSize[0] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(1);

    // NMDebugAI(<< "nbInputs = " << nbInputs << std::endl);
    for (unsigned int p=0; p < nbInputs; ++p)
    {
        if((inputSize[0] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(0))
           || (inputSize[1] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(1)))
          {
          itkExceptionMacro(<< "Input images must have the same dimensions." << std::endl
                            << "image #1 is [" << inputSize[0] << ";" << inputSize[1] << "]" << std::endl
                            << "image #" << p+1 << " is ["
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(0) << ";"
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(1) << "]");

          itk::ExceptionObject e(__FILE__, __LINE__);
          e.SetLocation(ITK_LOCATION);
          e.SetDescription("Input regions don't match in size!");
          throw e;
          }
    }

    // ======================================================================
    // ALLOCATE THE OUTPUT IMAGE
    // ======================================================================
    typename TInputImage::ConstPointer inImg = this->GetInput();
    typename TOutputImage::Pointer outImg = this->GetOutput();
    this->InternalAllocateOutput();

    IndexType lprPixNum = inImg->GetLargestPossibleRegion().GetNumberOfPixels();
    typename TOutputImage::RegionType outRegion = outImg->GetBufferedRegion();


    /// here's what we do:
    /// - determine the number of images for the initial iteration
    /// - according to the above iterate over internal pipeline
    ///   which does the following:
    ///   - take the first n images and combine them using otbCombineTwo
    ///     - set a name for the resulting table
    ///   - normalise the resulting image of the above:
    ///     use a another internal pipeline: reader->BandMath->writer
    ///     - use b_1__UvId as the output image values
    ///   - join the CombineTwo result table onto the new BandMath output
    ///
    ///   - determine the next n images for the next iteration

    // set up objects for the first pipline
    typedef typename otb::CombineTwoFilter<TInputImage, TOutputImage> CombFilterType;
    typedef typename otb::RATBandMathImageFilter<TOutputImage> MathFilterType;
    typedef typename otb::NMImageReader<TOutputImage> ReaderType;
    typedef typename otb::StreamingRATImageFileWriter<TOutputImage> WriterType;

    std::string temppath = "/home/alex/garage/testing/";

    int numIter = 1;
    OutputPixelType accIdx = static_cast<OutputPixelType>(m_vInRAT.at(0)->GetNumRows());
    int fstImg = 0;
    int lastImg = this->nextUpperIterationIdx(static_cast<unsigned int>(fstImg), accIdx);
    while (lastImg+1 <= nbRAT)
    {
        // ------------------------------------------------------------------------
        // do  the combinatorial analysis ...
        NMDebugAI( << "Iteration 1 ..." << std::endl);
        NMDebugAI( << "  combining imgs #" << fstImg << " to #" << lastImg << std::endl);

        // set up the combination filter
        std::vector<long long> nodata;
        std::vector<std::string> names;
        typename CombFilterType::Pointer ctFilter = CombFilterType::New();
        for (int i=fstImg; i <= lastImg; ++i)
        {
            ctFilter->SetInput(i, this->GetInput(i));
            ctFilter->setRAT(i, m_vInRAT.at(i));

            if (i < m_InputNodata.size())
            {
                nodata.push_back(static_cast<long long>(m_InputNodata.at(i)));
            }
            else
            {
               nodata.push_back(static_cast<long long>(m_InputNodata.at(m_InputNodata.size()-1)));
            }

            if (i < m_ImageNames.size())
            {
                names.push_back(m_ImageNames.at(i));
            }
            else
            {
                std::stringstream n;
                n << "L" << i+1;
                names.push_back(n.str());
            }
        }
        ctFilter->SetInputNodata(nodata);
        ctFilter->SetImageNames(names);
        std::stringstream ctTableNameStr;
        ctTableNameStr << temppath << "cttab_" << this->getRandomString() << ".ldb";
        ctFilter->SetOutputTableFileName(ctTableNameStr.str());

        typename WriterType::Pointer ctWriter = WriterType::New();
        std::stringstream ctImgNameStr;
        ctImgNameStr << temppath << "ctimg_" << numIter << this->getRandomString() << ".kea";
        ctWriter->SetFileName(ctImgNameStr.str());
        ctWriter->SetResamplingType("NONE");
        ctWriter->SetUpdateMode(true);
        ctWriter->SetInputRAT(ctFilter->getRAT(0));
        ctWriter->SetInput(ctFilter->GetOutput());
        NMDebugAI( << "  do combinatorial analysis ..." << std::endl);
        ctWriter->Update();

        otb::AttributeTable::Pointer uvTable = ctFilter->getRAT(0);
        accIdx = ctFilter->GetNumUniqueCombinations();

        // ------------------------------------------------------------------------
        // do the normalisation

        // ...................................
        // tweak the ctTable into the normTable
        // and write it out with the normWriter

        // ...................................

        // no exece the normalisation pipeline
        typename ReaderType::Pointer imgReader = ReaderType::New();
        imgReader->SetFileName(ctImgNameStr.str());
        imgReader->SetRATSupport(true);
        imgReader->SetRATType(otb::AttributeTable::ATTABLE_TYPE_RAM);

        typename MathFilterType::Pointer normFilter = MathFilterType::New();
        normFilter->SetNthInput(0, imgReader->GetOutput());
        std::vector<std::string> vColumns;
        vColumns.push_back("UvId");
        normFilter->SetNthAttributeTable(0, uvTable, vColumns);
        normFilter->SetExpression("b1__UvId");

        typename WriterType::Pointer normWriter = WriterType::New();
        std::stringstream normImgNameStr;
        normImgNameStr << temppath << "norm_" << numIter << this->getRandomString() << ".kea";
        normWriter->SetResamplingType("NEAREST");
        normWriter->SetInput(normFilter->GetOutput());
        normWriter->SetFileName(normImgNameStr.str());
        NMDebugAI( << "  normalise the image ..." << std::endl);
        normWriter->Update();

        // prepare for the next iteration
        // get the normalised accIdx from the normalised image's RAT
        accIdx = static_cast<OutputPixelType>(uvTable->GetNumRows());
        uvTable = 0;

        fstImg = lastImg+1;
        lastImg = this->nextUpperIterationIdx(fstImg, accIdx);
        ++numIter;
    }

    NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
unsigned int
UniqueCombinationFilter< TInputImage, TOutputImage >
::nextUpperIterationIdx(unsigned int idx, OutputPixelType &accIdx)
{
    unsigned int cnt = idx;
    unsigned int nbRAT = m_vInRAT.size();
    //IndexType maxIdx = itk::NumericTraits<IndexType>::max();
    OutputPixelType maxIdx = itk::NumericTraits<OutputPixelType>::max();
    while (   cnt+1 < nbRAT
           && (accIdx <= maxIdx / (m_vInRAT.at(cnt+1)->GetNumRows() > 0 ? m_vInRAT.at(cnt+1)->GetNumRows() : 1))
          )
    {
        accIdx *= m_vInRAT.at(cnt)->GetNumRows();
        ++cnt;
    }

    return cnt;
}

template< class TInputImage, class TOutputImage >
std::string
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRandomString(int length)
{
    int len = length;
    if (len < 8)
        len = 8;

    std::srand(std::time(0));
    char nam[len];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (::rand() % 2 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else
            {
                nam[i] = ::rand() % 26 + 97;
            }
        }
        else
        {
            if (::rand() % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (::rand() % 5 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else if (::rand() % 3 == 0)
            {
                nam[i] = ::rand() % 26 + 97;
            }
            else
            {
                nam[i] = ::rand() % 10 + 48;
            }
        }
    }

    std::string ret = nam;

    return ret;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
    NMDebugCtx(ctx, << "...");

    m_StreamingProc = false;

    if (this->m_UVTable.IsNotNull())
    {
        if (m_DropTmpTables)
        {
            this->m_UVTable->CloseTable(true);
        }
        else
        {
            this->m_UVTable->CloseTable(false);
        }
    }
    m_UVTable = 0;
    m_UVTable = SQLiteTable::New();
    m_OutIdx = 0;
    m_UVTableIndex = 0;

    Superclass::ResetPipeline();
    NMDebugCtx(ctx, << "done!")
}


}   // end namespace otb

#endif /* __otbUniqueCombinationFilter_txx */
