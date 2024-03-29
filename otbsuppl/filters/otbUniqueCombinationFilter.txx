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

/*  ACKNOWLEDGEMENT
 *
 *  This implementation is based on a conceptual algorithm for
 *  computing unique combinations developed by my brillant colleague
 *
 *  Robbie Price (Manaaki Whenua - Landcare Research New Zealand Ltd)
 *
 *
 */

#ifndef __otbUniqueCombinationFilter_txx
#define __otbUniqueCombinationFilter_txx

#include "nmlog.h"
#include "otbUniqueCombinationFilter.h"
#include "otbCombineTwoFilter.h"
#include "otbNMImageReader.h"
#include "otbStreamingRATImageFileWriter.h"
//#include "otbRATBandMathImageFilter.h"
#include "otbRATValExtractor.h"
#include "otbSQLiteTable.h"


#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"
#include "itkNMLogEvent.h"

#include <ctime>
#include <random>

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
      m_Workspace(""),
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
    catch(itk::ExceptionObject& eo)
    {
        NMProcErr(<< eo.GetDescription());
        throw;
    }
    catch(std::exception& se)
    {
        NMProcErr(<< se.what());
        throw;
    }
    catch(...)
    {
        NMProcErr(<< "Unknown Error!");
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
::determineProcOrder()
{
    std::multimap<long, int> orderMap;
    for (int i=0; i < m_vInRAT.size(); ++i)
    {
        orderMap.insert(std::pair<long ,int>(m_vInRAT.at(i)->GetNumRows(), i));
    }

    m_ProcOrder.clear();
    std::multimap<long, int>::iterator it = orderMap.begin();
    while (it != orderMap.end())
    {
        m_ProcOrder.push_back(it->second);
        ++it;
    }
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::GenerateData()
{
    NMDebugCtx(ctx, << "...");

    unsigned int nbInputs = m_InputImages.size();
    unsigned int nbRAT = this->m_vInRAT.size();
    if (nbInputs < 2 || nbRAT < 2)
    {
        NMDebugCtx(ctx, << "done!");
        itkExceptionMacro(<< "Need at least two input layers with raster attribute table to work!");
        return;
    }

    // check for valid RATs
    for (int t=0; t < m_vInRAT.size(); ++t)
    {
        AttributeTable::Pointer tab = m_vInRAT.at(t);
        if (tab.IsNull())
        {
            NMDebugCtx(ctx, << "done!");
            itkExceptionMacro(<< "Table at index=" << t
                              << " is NULL! Please only provide"
                              << " categorical images with raster"
                              << " attribute tables!");
            return;
        }
    }

    if (m_InputNodata.size() == 0)
    {
        this->InvokeEvent(itk::NMLogEvent("No 'InputNodata' values defined!",
                                          itk::NMLogEvent::NM_LOG_WARN));
    }
    else if (m_InputNodata.size() < nbInputs)
    {
        this->InvokeEvent(itk::NMLogEvent("'InputNodata' values are not defined for all input layers!",
                                          itk::NMLogEvent::NM_LOG_WARN));
    }

    // ======================================================================
    // IMAGE AXES DIMENSION CHECK
    // ======================================================================

    // check, whether all inputs have the same dimension
    // (... and hopefully also the same projection - but
    // we don't check that ...)
    IndexType inputSize[2];
    inputSize[0] = m_InputImages.at(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = m_InputImages.at(0)->GetLargestPossibleRegion().GetSize(1);

    // NMDebugAI(<< "nbInputs = " << nbInputs << std::endl);
    for (unsigned int p=0; p < nbInputs; ++p)
    {
        if((inputSize[0] != m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(0))
           || (inputSize[1] != m_InputImages.at(p)->GetLargestPossibleRegion().GetSize(1)))
          {
              itk::ExceptionObject e(__FILE__, __LINE__);
              e.SetLocation(ITK_LOCATION);
              e.SetDescription("Input regions don't match in size!");
              this->InvokeEvent(itk::NMLogEvent("Input regions differ in size!",
                                                itk::NMLogEvent::NM_LOG_ERROR));
              NMDebugCtx(ctx, << "done!");
              throw e;
          }
    }

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
    ///

    determineProcOrder();

    // set up objects for the first pipline
    typedef typename otb::CombineTwoFilter<TInputImage, TOutputImage> CombFilterType;
    typedef typename otb::RATValExtractor<TOutputImage> ExtractorType;
    typedef typename otb::NMImageReader<TOutputImage> ReaderType;
    typedef typename otb::StreamingRATImageFileWriter<TOutputImage> WriterType;

    typename ReaderType::Pointer iterReader = 0;
    typename CombFilterType::Pointer ctFilter = CombFilterType::New();
    ctFilter->SetReleaseDataFlag(true);
    ctFilter->SetWorkspace(m_Workspace);

    otb::SQLiteTable::Pointer uvTable = otb::SQLiteTable::New();
    uvTable->SetUseSharedCache(false);


    std::string temppath = "";
#ifndef _WIN32
    temppath = std::tmpnam(0);
    int posDiv = temppath.rfind('/');
    temppath = temppath.substr(0, posDiv+1);
#else
    char s[MAX_PATH];
    GetTempPath(MAX_PATH, s);
    temppath = s;
#endif

    if (!m_Workspace.empty())
    {
        if (m_Workspace.at(m_Workspace.size()-1) != '/')
        {
            temppath = m_Workspace + "/";
        }
        else
        {
            temppath = m_Workspace;
        }
    }

    std::stringstream uvTableName;
    uvTableName << temppath << "uv_" << this->getRandomString(10) << ".ldb";
    if (uvTable->CreateTable(uvTableName.str()) == otb::SQLiteTable::ATCREATE_ERROR)
    {
        this->InvokeEvent(itk::NMLogEvent("Failed to create the combinations table!",
                                          itk::NMLogEvent::NM_LOG_ERROR));
        itkExceptionMacro(<< "Combinatorial analysis failed!");
        NMDebugCtx(ctx, << "done!");
        return;
    }
    std::string uvTableTabName = uvTable->GetTableName();
    uvTable->CloseTable();

    int numIter = 1;
    unsigned long long accIdx = static_cast<unsigned long long>(m_vInRAT.at(m_ProcOrder.at(0))->GetNumRows());
    int fstImg = 0;
    int lastImg = this->nextUpperIterationIdx(static_cast<unsigned int>(fstImg), accIdx);
    int pos = fstImg;
    std::vector<long long> nodata;
    std::vector<std::string> names;
    std::vector<std::string> doneNames;

    float progr = 0.0f;
    float chunk;

    while (lastImg < nbRAT && !this->GetAbortGenerateData())
    {
        // ------------------------------------------------------------------------
        //      SET UP THE COMBINE-TWO-FILTER  - THE WORKHORSE OF THE ANALYSIS
        // ------------------------------------------------------------------------

        chunk = (float)(lastImg - fstImg + 1) / (float)nbRAT;
        chunk *= 0.33;

        // do  the combinatorial analysis ...
        std::stringstream msg;
        //NMDebugAI( << ">>>>> Iteration " << numIter << " <<<<<<" << std::endl);
        msg << ">>>>> Iteration " << numIter << " <<<<<<" << std::endl;
        this->InvokeEvent(itk::NMLogEvent(msg.str(), itk::NMLogEvent::NM_LOG_INFO));

        //NMDebugAI( << "  combining imgs #" << fstImg << " to #" << lastImg << std::endl);
        msg.str("");
        msg << "combining imgs #" << fstImg << " to #" << lastImg << std::endl;
        this->InvokeEvent(itk::NMLogEvent(msg.str(), itk::NMLogEvent::NM_LOG_INFO));
        // set up the combination filter

        for (int i=fstImg; i <= lastImg; ++i, ++pos)
        {
            ctFilter->SetInput(pos, m_InputImages.at(m_ProcOrder.at(i)));
            ctFilter->setRAT(pos, m_vInRAT.at(m_ProcOrder.at(i)));

            if (m_InputNodata.size() == 0)
            {
                nodata.push_back(itk::NumericTraits<long long>::NonpositiveMin());
            }
            else if (m_ProcOrder.at(i) < m_InputNodata.size())
            {
                nodata.push_back(static_cast<long long>(m_InputNodata.at(m_ProcOrder.at(i))));
            }
            else
            {
               nodata.push_back(static_cast<long long>(m_InputNodata.at(m_InputNodata.size()-1)));
            }

            if (m_ProcOrder.at(i) < m_ImageNames.size())
            {
                names.push_back(m_ImageNames.at(m_ProcOrder.at(i)));
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
        ctTableNameStr << temppath << "cttab" << numIter << "_" << this->getRandomString(10) << ".ldb";
        ctFilter->SetOutputTableFileName(ctTableNameStr.str());


        typename WriterType::Pointer ctWriter = WriterType::New();
        std::stringstream ctImgNameStr;
        ctImgNameStr << temppath << "ctimg" << numIter << "_"  << this->getRandomString(10) << ".nc:/uv";
        ctWriter->SetFileName(ctImgNameStr.str());
        ctWriter->SetResamplingType("NONE");
        ctWriter->SetInput(ctFilter->GetOutput());
        ctWriter->SetReleaseDataFlag(true);

        msg.str("");
        //NMDebugAI( << "  do combinatorial analysis ..." << std::endl);
        msg << "  do combinatorial analysis ..." << std::endl;
        this->InvokeEvent(itk::NMLogEvent(msg.str(), itk::NMLogEvent::NM_LOG_INFO));
        ctWriter->Update();

        otb::AttributeTable::Pointer tempUvTable = ctFilter->getRAT(0);
        accIdx = static_cast<unsigned long long>(ctFilter->GetNumUniqueCombinations());

        for (int d=fstImg; d <= lastImg; ++d)
        {
            m_InputImages.at(m_ProcOrder.at(d))->ReleaseData();
        }
        ctFilter->GetOutput()->ReleaseData();
        ctFilter = 0;
        ctWriter = 0;

        progr += chunk;
        this->UpdateProgress(progr);

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
        //NMDebugAI( << "  do table magic ..." << std::endl);
        msg.str("");
        msg << "  do table magic ..." << std::endl;
        this->InvokeEvent(itk::NMLogEvent(msg.str(), itk::NMLogEvent::NM_LOG_INFO));
        otb::SQLiteTable::Pointer sqlTemp = static_cast<otb::SQLiteTable*>(tempUvTable.GetPointer());

        // create some shortcut strings for programmer's readability ....
        // most recent table incl. 'uvimg'
        std::string tt = sqlTemp->GetTableName();
        // tab from prev. iter cont. rowidx: -> tt.uvimg = uv.rowidx
        std::string uv = uvTableTabName;//uvTable->GetTableName();

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
                itkExceptionMacro(<< "Combinatorial analysis failed to attach database '"
                << uvTableName.str() << "'!");
                return;
            }

            NMDebugAI(<< "JOINING CURRENT & PREVIOUS RESULTS ... " << std::endl);
            std::stringstream ssuvtmp;
            ssuvtmp << "_uv_tmp_" << numIter;
            std::string uvtmp_ = ssuvtmp.str();

            sql.str("");
            sql << "BEGIN TRANSACTION;";
            sql << "CREATE TEMP TABLE _uv_tmp_ AS "
                <<  "SELECT "
                    << tt << ".rowidx, " << tt << ".UvId, "
                    << uv_doneNamesStr.str() << ", " << tt_newNamesStr.str()
                << " FROM " << tt <<  " LEFT JOIN " << uv
                << " ON " << tt << ".uvimg = "<< uv << ".rowidx; ";
            sql << "DROP TABLE " << tt << "; ";
            sql << "CREATE TABLE " << tt << " AS SELECT * FROM _uv_tmp_;";
            sql << "CREATE TABLE " << uvtmp_ << " AS SELECT * FROM _uv_tmp_;";
            sql << "DROP TABLE _uv_tmp_;";
            sql << "END TRANSACTION;";

            // ", "<< uv <<

            NMDebugAI(<< sql.str() << std::endl);

            if (!sqlTemp->SqlExec(sql.str()))
            {
                NMDebugAI(<< "Failed joining previous with current results!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed to join previous with current results!");
                return;
            }

            if (!sqlTemp->DetachDatabase("uvdb"))
            {
                NMDebugAI(<< "Failed detaching uvdb database!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed to detach database 'uvdb'!");
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
            itkExceptionMacro(<< "Combinatorial analysis failed to create or attach '"
                << uvTableName.str() << "'!");
            return;
        }

        // create a normalised unique value table including results from
        // all previous combination iterations
        sql.str("");
        sql << "BEGIN TRANSACTION;";
        sql << "DROP TABLE IF EXISTS main." << uv << ";";
        //sql << "ALTER TABLE main." << uv << " RENAME TO main." << uv << "_" << numIter << ";";
        sql << "CREATE TABLE main." << uv << " AS "
             << " SELECT UvId as rowidx, " << (numIter == 1 ? tt_newNamesStr.str() : tt_doneColsStr.str())
             << " FROM sqltmp." << tt << ";";
        sql << "CREATE TABLE main." << uv << "_" << numIter << " AS SELECT * from " << uv << ";";
        sql << "END TRANSACTION;";

        NMDebugAI(<< sql.str() << std::endl);

        if (!uvTable->SqlExec(sql.str()))
        {
            NMDebugAI(<< "Failed updating normalised unique value table!\n");
            itkExceptionMacro(<< "Combinatorial analysis failed to update the normalised unique value table!");
            return;
        }

        if (!uvTable->DetachDatabase("sqltmp"))
        {
            {
                NMDebugAI(<< "Detaching database uvdb from sqlTemp failed!\n");
                itkExceptionMacro(<< "Combinatorial analysis failed to detach 'uvdb' from the temp database!");
                return;
            }
        }

        // repopulate the table admin structures
        uvTable->PopulateTableAdmin();

        progr += chunk;
        this->UpdateProgress(progr);

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

        typename ExtractorType::Pointer normFilter = ExtractorType::New();
        normFilter->SetReleaseDataFlag(true);
        normFilter->SetUseTableColumnCache(true);
        normFilter->SetNthInput(0, imgReader->GetOutput());
        normFilter->SetExpression("UvId");
        std::vector<std::string> vColumns;
        vColumns.push_back("UvId");
        normFilter->SetNthAttributeTable(0, tempUvTable, vColumns);

        OutputPixelType maxCombis = itk::NumericTraits<OutputPixelType>::max();
        unsigned int maxUintC = itk::NumericTraits<unsigned int>::max();
        bool bNetCDF = false;
        if (accIdx > maxCombis && accIdx > maxUintC)
        {
            bNetCDF = true;
        }

        // if this is the last iteration, we use the user specified
        // image file name for the final output (if specified)
        // otherwise we just keep using temp filenames!
        std::stringstream normImgNameStr;
        unsigned long long tmpIdx = accIdx;
        if (    this->nextUpperIterationIdx(lastImg+1, tmpIdx) >= nbRAT
            &&  !m_OutputImageFileName.empty()
           )
        {
            if (bNetCDF)
            {
                size_t pos = m_OutputImageFileName.find_last_of('.');
                std::string outNameWOSuf = m_OutputImageFileName.substr(0, pos);
                size_t pos2 = outNameWOSuf.find_last_of("/\\");
                std::string baseName = outNameWOSuf.substr(pos2+1);
                std::string ncOutName = outNameWOSuf + ".nc:/" + baseName;
                normImgNameStr << ncOutName;
            }
            else
            {
                normImgNameStr << m_OutputImageFileName;
            }
        }
        else
        {

            normImgNameStr << temppath << "norm_" << numIter << this->getRandomString(10) << ".kea";//".nc:/uv";//".img";//<< ".kea";
        }

        typename WriterType::Pointer normWriter = WriterType::New();
        normWriter->SetReleaseDataFlag(true);
        normWriter->SetFileName(normImgNameStr.str());
        normWriter->SetResamplingType(bNetCDF ? "NONE" : "NEAREST");
        normWriter->SetInput(normFilter->GetOutput());
        normWriter->SetInputRAT(uvTable);
        NMDebugAI( << "  normalise the image ..." << std::endl);
        normWriter->Update();

        imgReader->GetOutput()->ReleaseData();
        normFilter->GetOutput()->ReleaseData();
        normFilter->ResetPipeline();
        normWriter = 0;
        normFilter = 0;
        imgReader = 0;
        sqlTemp->CloseTable();


        progr += chunk;
        this->UpdateProgress(progr);

        // ------------------------------------------------------------
        //      PREPARE THE NEXT ITERATION STEP
        // ------------------------------------------------------------

        fstImg = lastImg+1;
        lastImg = this->nextUpperIterationIdx(fstImg, accIdx);
        ++numIter;

        nodata.clear();
        names.clear();
        pos = 1;
        if (lastImg < nbRAT)
        {
            iterReader = ReaderType::New();
            iterReader->SetReleaseDataFlag(true);
            iterReader->RATSupportOn();
            iterReader->SetRATType(otb::AttributeTable::ATTABLE_TYPE_SQLITE);
            iterReader->SetFileName(normImgNameStr.str());

            ctFilter = CombFilterType::New();
            ctFilter->SetReleaseDataFlag(true);
            ctFilter->SetInput(0, iterReader->GetOutput());
            ctFilter->setRAT(0, static_cast<otb::AttributeTable*>(uvTable.GetPointer()));

            nodata.push_back(0);
            names.push_back("uvimg");
        }
    }
    uvTable->CloseTable();

    this->UpdateProgress(1.0f);
    NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
unsigned int
UniqueCombinationFilter< TInputImage, TOutputImage >
::nextUpperIterationIdx(unsigned int idx, unsigned long long &accIdx)
{
    unsigned int cnt = idx;
    unsigned int nbRAT = m_vInRAT.size();
    if (idx >= nbRAT)
    {
        return nbRAT;
    }

    //IndexType maxIdx = itk::NumericTraits<IndexType>::max();
    OutputPixelType maxIdx = itk::NumericTraits<OutputPixelType>::max();
    while (   cnt < nbRAT
           && (accIdx <= maxIdx / (m_vInRAT.at(m_ProcOrder.at(cnt))->GetNumRows() > 0
                                    ? m_vInRAT.at(m_ProcOrder.at(cnt))->GetNumRows()
                                    : 1)
              )
          )
    {
        accIdx *= m_vInRAT.at(m_ProcOrder.at(cnt))->GetNumRows();
        ++cnt;
    }

    if (cnt > idx)
    {
        --cnt;
    }

    return cnt;
}

template< class TInputImage, class TOutputImage >
std::string
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRandomString(int len)
{
    if (len < 1)
    {
        return "";
    }

    std::random_device rand_rd;
    std::mt19937 rand_mt(rand_rd());
    std::uniform_int_distribution<int> rand_1_1e6(1, 1e6);
    std::uniform_int_distribution<int> rand_48_57(48, 57);
    std::uniform_int_distribution<int> rand_65_90(65, 90);
    std::uniform_int_distribution<int> rand_97_122(97, 122);

    //std::srand(std::time(0));
    char* nam = new char[len+1];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (rand_1_1e6(rand_mt) % 2 == 0)
            {
                nam[i] = rand_65_90(rand_mt);
            }
            else
            {
                nam[i] = rand_97_122(rand_mt);
            }
        }
        else
        {
            if (rand_1_1e6(rand_mt) % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (rand_1_1e6(rand_mt) % 5 == 0)
            {
                nam[i] = rand_65_90(rand_mt);
            }
            else if (rand_1_1e6(rand_mt) % 3 == 0)
            {
                nam[i] = rand_97_122(rand_mt);
            }
            else
            {
                nam[i] = rand_48_57(rand_mt);
            }
        }
    }
    nam[len] = '\0';
    std::string ret = nam;
    delete[] nam;


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
