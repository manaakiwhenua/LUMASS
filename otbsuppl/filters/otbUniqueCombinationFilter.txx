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
      m_OutputImageFileName(""),
      m_OutIdx(0)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    m_UVTable = otb::SQLiteTable::New();

    // seed the random number generator
    std::srand(std::time(0));
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
::SetInput(unsigned int idx, const TInputImage* image)
{
    //this->SetInput(idx, const_cast<TInputImage *>( image ));
    typename TInputImage::Pointer img = TInputImage::New();
    img = const_cast<TInputImage*>(image);// ->Graft(const_cast<TInputImage*>(image));
    if (idx < this->m_InputImages.size())
    {
        m_InputImages[idx] = img;
    }
    else
    {
        m_InputImages.push_back(img);
    }
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
//    typename TInputImage::ConstPointer inImg = this->GetInput();
//    typename TOutputImage::Pointer outImg = this->GetOutput();
//    outImg->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
//    outImg->SetBufferedRegion(inImg->GetBufferedRegion());
//    outImg->SetRequestedRegion(inImg->GetRequestedRegion());
    //outImg->Allocate();

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::Update()
{
    // the code below is an adapted part of the
    // original itk::ProcessObject::UpdateOutputData() code

    this->InvokeEvent(itk::StartEvent());

    this->SetAbortGenerateData(false);
    this->SetProgress(0.0f);

    try
    {
        this->GenerateData();
    }
    catch(itk::ProcessAborted&)
    {
        this->InvokeEvent(itk::AbortEvent());
        throw;
    }
    catch(...)
    {
        throw;
    }

    if (this->GetAbortGenerateData())
    {
        this->UpdateProgress(1.0f);
    }

    this->InvokeEvent(itk::EndEvent());

}


template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::GenerateData()
{
    NMDebugCtx(ctx, << "...");

    unsigned int nbInputs = m_InputImages.size(); //this->GetNumberOfIndexedInputs();
    unsigned int nbRAT = this->m_vInRAT.size();
    if (nbInputs < 2 || nbRAT < 2)
    {
        NMDebugCtx(ctx, << "done!");
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
    inputSize[0] = m_InputImages.at(0)->GetLargestPossibleRegion().GetSize(0); //this->GetInput(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = m_InputImages.at(0)->GetLargestPossibleRegion().GetSize(1); //this->GetInput(0)->GetLargestPossibleRegion().GetSize(1);

    // NMDebugAI(<< "nbInputs = " << nbInputs << std::endl);
    for (unsigned int p=0; p < nbInputs; ++p)
    {
            //        if((inputSize[0] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(0))
            //           || (inputSize[1] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(1)))
        if((inputSize[0] != m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(0))
           || (inputSize[1] != m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(1)))
          {
          itkExceptionMacro(<< "Input images must have the same dimensions." << std::endl
                            << "image #1 is [" << inputSize[0] << ";" << inputSize[1] << "]" << std::endl
                            << "image #" << p+1 << " is ["
                            << m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(0) << ";"
                            << m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(1) << "]");
//                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(0) << ";"
//                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(1) << "]");

          itk::ExceptionObject e(__FILE__, __LINE__);
          e.SetLocation(ITK_LOCATION);
          e.SetDescription("Input regions don't match in size!");
          NMDebugCtx(ctx, << "done!");
          throw e;
          }
    }

    // ======================================================================
    // ALLOCATE THE OUTPUT IMAGE
    // ======================================================================
    //    typename TInputImage::ConstPointer inImg = this->GetInput();
    //    typename TOutputImage::Pointer outImg = this->GetOutput();
    //this->InternalAllocateOutput();

    //IndexType lprPixNum = m_InputImages.at(p)->GetLargestPossibleRegion().GetNumberOfPixels();
    //typename TOutputImage::RegionType outRegion = outImg->GetBufferedRegion();


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

    typename ReaderType::Pointer iterReader = 0;
    typename CombFilterType::Pointer ctFilter = CombFilterType::New();
    otb::SQLiteTable::Pointer uvTable = otb::SQLiteTable::New();
    uvTable->SetUseSharedCache(false);
    otb::SQLiteTable::Pointer uvTable2 = 0;


    std::string temppath = std::tmpnam(0);//"/home/alex/garage/testing/LENZ25/";
    int posDiv = 0;
#ifndef _WIN32
    posDiv = temppath.rfind('/');
#else
    posDiv = temppath.rfind('\\');
#endif
    temppath = temppath.substr(0, posDiv+1);
    std::stringstream uvTableName;
    uvTableName << temppath << "uv_" << this->getRandomString(5) << ".ldb";
    if (uvTable->CreateTable(uvTableName.str()) == otb::SQLiteTable::ATCREATE_ERROR)
    {
        itkExceptionMacro(<< "Combinatorial analysis failed!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    int numIter = 1;
    OutputPixelType accIdx = static_cast<OutputPixelType>(m_vInRAT.at(0)->GetNumRows());
    int fstImg = 0;
    int lastImg = this->nextUpperIterationIdx(static_cast<unsigned int>(fstImg), accIdx);
    int pos = fstImg;
    std::vector<long long> nodata;
    std::vector<std::string> names;
    std::vector<std::string> doneNames;

    float progrChunk = 0.33f / (float)nbRAT;

    while (lastImg < nbRAT && !this->GetAbortGenerateData())
    {
        // ------------------------------------------------------------------------
        //      SET UP THE COMBINE-TWO-FILTER  - THE WORKHORSE OF THE ANALYSIS
        // ------------------------------------------------------------------------

        // do  the combinatorial analysis ...
        NMDebugAI( << ">>>>> Iteration " << numIter << " <<<<<<" << std::endl);
        NMDebugAI( << "  combining imgs #" << fstImg << " to #" << lastImg << std::endl);
        // set up the combination filter

        for (int i=fstImg; i <= lastImg; ++i, ++pos)
        {
            ctFilter->SetInput(pos, m_InputImages.at(i));
            ctFilter->setRAT(pos, m_vInRAT.at(i));

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
        ctTableNameStr << temppath << "cttab" << numIter << "_" << this->getRandomString(5) << ".ldb";
        ctFilter->SetOutputTableFileName(ctTableNameStr.str());
        //ctFilter->SetReleaseDataFlag(true);

        typename WriterType::Pointer ctWriter = WriterType::New();
        std::stringstream ctImgNameStr;
        ctImgNameStr << temppath << "ctimg" << numIter << "_"  << this->getRandomString(5) << ".kea";
        ctWriter->SetFileName(ctImgNameStr.str());
        ctWriter->SetResamplingType("NONE");
        ctWriter->SetInput(ctFilter->GetOutput());
        //ctWriter->SetInputRAT(ctFilter->getRAT(0));
        //ctWriter->SetUpdateMode(true);
        //ctWriter->SetReleaseDataFlag(true);
        NMDebugAI( << "  do combinatorial analysis ..." << std::endl);
        ctWriter->Update();

        otb::AttributeTable::Pointer tempUvTable = ctFilter->getRAT(0);
        accIdx = ctFilter->GetNumUniqueCombinations();

        this->UpdateProgress(((float)(lastImg+1))*progrChunk);

        // ------------------------------------------------------------------------
        //          CREATE/UDATE THE NORMALISED UNIQUE VALUE ATTRIBUTE TABLE
        // ------------------------------------------------------------------------

        if (this->GetAbortGenerateData())
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }
        // .........................
        // some prep work
        // .........................

        // update the final table
        NMDebugAI( << "  do table magic ..." << std::endl);
        otb::SQLiteTable::Pointer sqlTemp = static_cast<otb::SQLiteTable*>(tempUvTable.GetPointer());

        // create some shortcut strings for programmer's readability ....
        // most recent table incl. 'uvimg'
        std::string tt = sqlTemp->GetTableName();
        // tab from prev. iter cont. rowidx: -> tt.uvimg = uv.rowidx
        std::string uv = uvTable->GetTableName();

        // list of images already combined ...
        std::stringstream sql;
        std::stringstream uv_doneNamesStr;
        std::stringstream tt_newNamesStr;

        for (int n=0; n < names.size(); ++n)
        {
            tt_newNamesStr << tt << "." << names.at(n);
            if (n < names.size()-1)
            {
                tt_newNamesStr << ", ";
            }
        }

        for (int n=0; n < doneNames.size(); ++n)
        {
            uv_doneNamesStr << uv << "." << doneNames.at(n);
            if (n < doneNames.size()-1)
            {
                uv_doneNamesStr << ", ";
            }
        }


        // .........................
        // join new and previous results
        // .........................

        // if this is the second round of combinations, join the
        // signature (= index columns denoted by images' name)
        // of previously processed images to the result set of the
        // current round
        if (numIter > 1)
        {
            // we just close the uv table here for a minute,
            // so we ('unlock' the database and) don't have
            // a locked data base later when we try to
            // re-create/update the unique value attribute table
            uvTable->CloseTable();

            if (!sqlTemp->AttachDatabase(uvTableName.str(), "uvdb"))
            {
                itkExceptionMacro(<< "Combinatorial analysis failed!");
                return;
            }

            NMDebugAI(<< "JOINING CURRENT & PREVIOUS RESULTS ... " << std::endl);
            sql.str("");
            sql << "BEGIN TRANSACTION;";
            sql << "CREATE TEMP TABLE _uv_tmp_ AS "
                <<  "SELECT "
                    << tt << ".rowidx, " << tt << ".UvId, "
                    << uv_doneNamesStr.str() << ", " << tt_newNamesStr.str()
                << " FROM " << tt <<  " INNER JOIN " << uv
                << " ON " << tt << ".uvimg = "<< uv << ".rowidx; ";
            sql << "DROP TABLE " << tt << "; ";
            sql << "CREATE TABLE " << tt << " AS SELECT * FROM _uv_tmp_;";
            sql << "DROP TABLE _uv_tmp_;";
            sql << "END TRANSACTION;";

            // ", "<< uv <<

            NMDebugAI(<< sql.str() << std::endl);

            if (!sqlTemp->SqlExec(sql.str()))
            {
                NMDebugAI(<< "Failed joining previous with current results!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed!");
                return;
            }

            if (!sqlTemp->DetachDatabase("uvdb"))
            {
                NMDebugAI(<< "Failed detaching uvdb database!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed!");
                return;
            }
        }
        // add the current image names to the list of already processed ones
        // note: we don't add 'uvimg' since we're only interested in the
        // original normalised image values of the input layers
        int startid = numIter == 1 ? 0 : 1;
        for (int n=startid; n < names.size(); ++n)
        {
            doneNames.push_back(names.at(n));
        }


        // .........................
        // (re-)create the normalised unique combination attribute table
        // .........................

        NMDebugAI(<< "\nCREATING NORMALISED UV TABLE ... \n");
        std::stringstream tt_doneColsStr;
        for (int n=0; n < doneNames.size(); ++n)
        {
            tt_doneColsStr << tt << "." << doneNames.at(n);
            if (n < doneNames.size()-1)
            {
                tt_doneColsStr << ", ";
            }
        }


        if (    !uvTable->CreateTable(uvTableName.str())
            ||  !uvTable->AttachDatabase(sqlTemp->GetDbFileName(), "sqltmp")
           )
        {
            NMDebugAI(<< "Failed attaching sqltmp database!\n");
            itkExceptionMacro(<< "Combinatorial analysis failed!");
            return;
        }

        // create a normalised unique value table including results from
        // all previous combination iterations
        sql.str("");
        sql << "BEGIN TRANSACTION;";
        sql << "DROP TABLE IF EXISTS main." << uv << ";";
        sql << "CREATE TABLE main." << uv << " AS "
             << " SELECT UvId as rowidx, " << (numIter == 1 ? tt_newNamesStr.str() : tt_doneColsStr.str())
             << " FROM sqltmp." << tt << ";";
        sql << "END TRANSACTION;";

        NMDebugAI(<< sql.str() << std::endl);

        if (!uvTable->SqlExec(sql.str()))
        {
            NMDebugAI(<< "Failed updating normalised unique value table!\n");
            itkExceptionMacro(<< "Combinatorial analysis failed!");
            return;
        }

        if (!uvTable->DetachDatabase("sqltmp"))
        {
            {
                NMDebugAI(<< "Detaching database uvdb from sqlTemp failed!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed!");
                return;
            }
        }

        // repopulate the table admin structures
        uvTable->PopulateTableAdmin();

        this->UpdateProgress(((float)(lastImg+1))*2.0f*progrChunk);

        // ------------------------------------------------------------------------
        //      CREATE THE NORMALISED RESULT IMAGE
        // ------------------------------------------------------------------------

        if (this->GetAbortGenerateData())
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }

        // do the normalisation

        // no exece the normalisation pipeline
        typename ReaderType::Pointer imgReader = ReaderType::New();
        imgReader->SetReleaseDataFlag(true);
        imgReader->SetFileName(ctImgNameStr.str());
        //imgReader->RATSupportOn();
        //imgReader->SetRATType(otb::AttributeTable::ATTABLE_TYPE_RAM);

        typename MathFilterType::Pointer normFilter = MathFilterType::New();
        normFilter->SetReleaseDataFlag(true);
        normFilter->SetNthInput(0, imgReader->GetOutput());
        std::vector<std::string> vColumns;
        vColumns.push_back("UvId");
        //normFilter->SetNthAttributeTable(0, imgReader->GetAttributeTable(1), vColumns);
        normFilter->SetNthAttributeTable(0, tempUvTable, vColumns);
        normFilter->SetExpression("b1__UvId");

        // if this is the last iteration, we use the user specified
        // image file name for the final output (if specified)
        // otherwise we just keep using temp filenames!
        std::stringstream normImgNameStr;
        if (    this->nextUpperIterationIdx(lastImg+1, accIdx) >= nbRAT
            &&  !m_OutputImageFileName.empty()
           )
        {
            normImgNameStr << m_OutputImageFileName;
        }
        else
        {

            normImgNameStr << temppath << "norm_" << numIter << this->getRandomString(5) << ".kea";
        }

        typename WriterType::Pointer normWriter = WriterType::New();
        normWriter->SetReleaseDataFlag(true);
        //        std::stringstream normImgNameStr;
        //        normImgNameStr << temppath << "norm_" << numIter << this->getRandomString(5) << ".kea";
        normWriter->SetFileName(normImgNameStr.str());
        normWriter->SetResamplingType("NEAREST");
        normWriter->SetInput(normFilter->GetOutput());
        normWriter->SetInputRAT(uvTable);
        NMDebugAI( << "  normalise the image ..." << std::endl);
        normWriter->Update();

        this->UpdateProgress(((float)(lastImg+1))*3.0f*progrChunk);

        // ------------------------------------------------------------
        //      PREPARE THE NEXT ITERATION STEP
        // ------------------------------------------------------------

        fstImg = lastImg+1;
        lastImg = this->nextUpperIterationIdx(fstImg, accIdx);
        ++numIter;

        iterReader = ReaderType::New();
        iterReader->SetReleaseDataFlag(true);
        iterReader->RATSupportOn();
        iterReader->SetRATType(otb::AttributeTable::ATTABLE_TYPE_SQLITE);
        iterReader->SetFileName(normImgNameStr.str());

        ctFilter = CombFilterType::New();
        ctFilter->SetReleaseDataFlag(true);
        ctFilter->SetInput(0, iterReader->GetOutput());
        ctFilter->setRAT(0, static_cast<otb::AttributeTable*>(uvTable.GetPointer()));

        nodata.clear();
        names.clear();
        nodata.push_back(0);
        names.push_back("uvimg");
        pos = 1;
    }

    this->UpdateProgress(1.0f);
    NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
unsigned int
UniqueCombinationFilter< TInputImage, TOutputImage >
::nextUpperIterationIdx(unsigned int idx, OutputPixelType &accIdx)
{
    unsigned int cnt = idx;
    unsigned int nbRAT = m_vInRAT.size();
    if (idx >= nbRAT)
    {
        return nbRAT;
    }

    //    cnt = cnt + 1 < nbRAT ? cnt + 1 : nbRAT;
    //    return cnt;

    //IndexType maxIdx = itk::NumericTraits<IndexType>::max();
    OutputPixelType maxIdx = itk::NumericTraits<OutputPixelType>::max();
    while (   cnt < nbRAT
           && (accIdx <= maxIdx / (m_vInRAT.at(cnt)->GetNumRows() > 0
                                    ? m_vInRAT.at(cnt)->GetNumRows()
                                    : 1)
              )
          )
    {
        accIdx *= m_vInRAT.at(cnt)->GetNumRows();
        ++cnt;
    }

    return cnt-1;
}

template< class TInputImage, class TOutputImage >
std::string
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRandomString(int length)
{
    if (length < 1)
    {
        return "";
    }

    char nam[length+1];
    for (int i=0; i < length; ++i)
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
    nam[length] = '\0';
    std::string ret = nam;

    return ret;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
//    NMDebugCtx(ctx, << "...");

//    m_StreamingProc = false;

//    if (this->m_UVTable.IsNotNull())
//    {
//        if (m_DropTmpTables)
//        {
//            this->m_UVTable->CloseTable(true);
//        }
//        else
//        {
//            this->m_UVTable->CloseTable(false);
//        }
//    }
//    m_UVTable = 0;
//    m_UVTable = SQLiteTable::New();
//    m_OutIdx = 0;
//    m_UVTableIndex = 0;

//    Superclass::ResetPipeline();
//    NMDebugCtx(ctx, << "done!")
}


}   // end namespace otb

#endif /* __otbUniqueCombinationFilter_txx */
