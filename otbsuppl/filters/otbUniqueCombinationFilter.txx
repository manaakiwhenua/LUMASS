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

#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

//// TOKYO CABINET
////#include "tcutil.h"
//#include "tchdb.h"
//#include <stdlib.h>
//#include <stdbool.h>
//#include <stdint.h>

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
//::BeforeThreadedGenerateData()
::GenerateData()
{
    //NMDebugCtx(ctx, << "...");
    // ======================================================================
    // IMAGE AXES DIMENSION CHECK
    // ======================================================================

    // check, whether all inputs have the same dimension
    // (... and hopefully also the same projection - but
    // we don't check that ...)
    unsigned long long inputSize[2];
    inputSize[0] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(1);

    unsigned int nbInputs = this->GetNumberOfIndexedInputs();//this->GetNumberOfValidRequiredInputs();
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
    outImg->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
    outImg->SetBufferedRegion(inImg->GetBufferedRegion());
    outImg->SetRequestedRegion(inImg->GetRequestedRegion());
    outImg->Allocate();

    long long lprPixNum = inImg->GetLargestPossibleRegion().GetNumberOfPixels();
    typename TOutputImage::RegionType outRegion = outImg->GetBufferedRegion();

    // ======================================================================
    // PREPARE OUPUT TABLE AND KEY-VALUE STORE
    // ======================================================================
    //    int nbThreads = 1;//this->GetNumberOfThreads();
    //    for (int t=0; t < nbThreads; ++t)
    //    {
    //        TCHDB* hdb = tchdbnew();
    //        std::string hdbName = std::tmpnam(0);
    //        if (!tchdbopen(hdb, hdbName.c_str(), HDBOWRITER | HDBOCREAT))
    //        {
    //            m_threadHDB.push_back(hdb);
    //        }
    //    }

    if (!m_StreamingProc)
    {
        this->m_StreamingProc = true;

        m_TotalStreamedPix = 0;


        // -------------------------------------------------------------
        // PREPARE UNIQUE VALUE (i.e. COMBINATION) TABLE
        // -------------------------------------------------------------

        // note the "1" is to indicate the band this table is for
        if (this->m_UVTable->CreateTable(m_UVTableName, "1") == otb::SQLiteTable::ATCREATE_ERROR)
        {
            itkExceptionMacro(<< "Failed creating output table!");
            return;
        }

        // ad columns (rowidx, L0, L1, L2, ... L<nbInputs-1>)
        m_UVTable->BeginTransaction();
        std::vector<std::string> vColNames;
        vColNames.push_back(m_UVTable->GetPrimaryKey());
        std::stringstream colname;
        for (int l=0; l < nbInputs; ++l)
        {
            colname.str("");
            colname << "L" << l;
            m_UVTable->AddColumn(colname.str(),
                                 AttributeTable::ATTYPE_INT);
            vColNames.push_back(colname.str());
        }
        m_UVTable->EndTransaction();
        m_UVTable->PrepareBulkSet(vColNames, true);


        // reserve first row for nodata values
        std::vector<AttributeTable::ColumnValue> nodatVals;
        AttributeTable::ColumnValue ndv;
        ndv.type = AttributeTable::ATTYPE_INT;
        ndv.ival = m_OutIdx;
        nodatVals.push_back(ndv);
        ++m_OutIdx;

        // make sure, we've got as many nodata values
        // as inputs
        m_InputNodata.resize(nbInputs);

        for (int nd=1; nd < vColNames.size(); ++nd)
        {
            AttributeTable::ColumnValue ndv;
            ndv.type = AttributeTable::ATTYPE_INT;
            if (nd-1 < m_InputNodata.size())
            {
                ndv.ival = m_InputNodata.at(nd-1);
            }
            nodatVals.push_back(ndv);
        }
        m_UVTable->DoBulkSet(nodatVals);


        // -------------------------------------------------------------
        // PREPARE KEY-VALUE STORE KEEPING TRACK OF UNIQUE COMBINATIONS
        // -------------------------------------------------------------

        //        if (m_tcHDB != 0)
        //        {
        //            tchdbvanish(m_tcHDB);
        //            tchdbclose(m_tcHDB);
        //            m_tcHDB = 0;
        //        }

        //        m_tcHDB = tchdbnew();
        //        tchdbtune(m_tcHDB, 200000000, 8, 10, HDBTLARGE | HDBTDEFLATE);
        //        tchdbsetcache(m_tcHDB, 700000);
        //        tchdbsetxmsiz(m_tcHDB, 536870912);

        //        std::string hdbName = std::tmpnam(0);
        //        if (!tchdbopen(m_tcHDB, hdbName.c_str(), HDBOWRITER | HDBOCREAT))
        //        {
        //            itkExceptionMacro(<< "Failed creating main key-value store!");
        //            return;
        //        }
    }

    // ======================================================================
    // SETTING UP REGION ITERATION
    // ======================================================================
    //}

    //template< class TInputImage, class TOutputImage >
    //void
    //UniqueCombinationFilter< TInputImage, TOutputImage >
    //::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
    //                       itk::ThreadIdType threadId )
    //{

    typedef itk::ImageRegionConstIterator<TInputImage> ImageRegionConstIteratorType;
    typedef itk::ImageRegionIterator<TOutputImage> ImageRegionIteratorType;

    std::vector<ImageRegionConstIteratorType> inputIterators(nbInputs);
    for (unsigned int in=0; in < nbInputs; ++in)
    {
        inputIterators[in] = ImageRegionConstIteratorType(this->GetInput(in),
                                        outRegion);
        inputIterators[in].GoToBegin();
    }
    ImageRegionIteratorType outIt(this->GetOutput(0),
                         outRegion);
    outIt.GoToBegin();



    // ======================================================================
    // IDENTIFY UNIQUE COMBINATIONS
    // ======================================================================
    itk::ProgressReporter progress(this, 0,
                                   outRegion.GetNumberOfPixels());

    OutputPixelType readIdx = 0;
    InputPixelType curVal;
    bool nodata = false;
    std::vector<InputPixelType> combo(nbInputs, 0);
    while (!outIt.IsAtEnd() && !this->GetAbortGenerateData())
    {
        nodata = false;
        for (unsigned int in=0; in < nbInputs; ++in)
        {
            curVal = inputIterators[in].Get();
            if (curVal == m_InputNodata[in])
            {
                nodata = true;
            }
            combo[in] = curVal;
            ++inputIterators[in];
        }

//        if (!nodata)
//        {
//            if (tchdbget3(m_tcHDB,
//                          static_cast<void*>(&combo[0]),
//                          sizeof(InputPixelType) * nbInputs,
//                          static_cast<void*>(&readIdx),
//                          sizeof(OutputPixelType))
//                == -1)
//            {
//                tchdbputkeep(m_tcHDB,
//                              static_cast<void*>(&combo[0]),
//                              sizeof(typename TInputImage::PixelType) * nbInputs,
//                              static_cast<void*>(&m_OutIdx),
//                              sizeof(typename TOutputImage::PixelType));
//                outIt.Set(m_OutIdx);//outIt.Set(outIdx);
//                ++m_OutIdx;
//            }
//            else
//            {
//                outIt.Set(readIdx);
//            }
//        }
//        else
        {
            outIt.Set(0);
        }

        progress.CompletedPixel();
        ++outIt;
        ++m_TotalStreamedPix;
    }

    //NMDebugAI(<< m_OutIdx << " unique combinations found so far ... " << std::endl);

    //}

    //template< class TInputImage, class TOutputImage >
    //void
    //UniqueCombinationFilter< TInputImage, TOutputImage >
    //::AfterThreadedGenerateData()
    //{

    // ======================================================================
    // Creating the output unique value table
    // ======================================================================
//    void* nextKey = 0;
//    int sizeKey = 0;
//    if (m_TotalStreamedPix == lprPixNum)
//    {
//        std::vector<otb::AttributeTable::ColumnValue> inVals(nbInputs+1);
//        for (int i=0; i < nbInputs+1; ++i)
//        {
//            inVals[i].type = AttributeTable::ATTYPE_INT;
//        }

//        if (!tchdbiterinit(m_tcHDB))
//        {
//            int ecode = tchdbecode(m_tcHDB);
//            itkExceptionMacro(<< "ERROR writing unique value table: "
//                              << tchdberrmsg(ecode));
//            tchdbclose(m_tcHDB);
//            this->m_UVTable->closeTable();
//            //NMDebugCtx(ctx, << "done!");
//            return;
//        }

//        itk::ProgressReporter writeProgress(this, 0, m_OutIdx);

//        m_UVTable->beginTransaction();
//        while(nextKey = tchdbiternext(m_tcHDB, &sizeKey))
//        {
//            if (tchdbget3(m_tcHDB, nextKey, sizeKey,
//                          static_cast<void*>(&readIdx),
//                          sizeof(typename TOutputImage::PixelType))
//                == -1)
//            {
//                itkWarningMacro(<< "Failed reading Tokyo Cabinet Record!");
//                free(nextKey);
//                continue;
//            }

//            inVals[0].ival = static_cast<long long>(readIdx);
//            for (int i=0; i < nbInputs; ++i)
//            {
//                inVals[i+1].ival = static_cast<typename TInputImage::PixelType*>(nextKey)[i];
//            }
//            m_UVTable->doBulkSet(inVals);

//            free(nextKey);
//            writeProgress.CompletedPixel();
//        }
//        m_UVTable->endTransaction();


//        // just close the table
//        //m_UVTable->closeTable();

//    }

    // set the output RAT table
    //    if (m_vOutRAT.size())
    //    {
    //        m_vOutRAT[0] = m_UVTable;
    //    }
    //    else
    //    {
    //        m_vOutRAT.push_back(m_UVTable);
    //    }


    // clean up the hdbc
    //    for (int d=0; d < m_threadHDB.size(); ++d)
    //    {
    //        if (m_threadHDB[d] != 0)
    //        {
    //            tchdbvanish(hdb);
    //            tchdbclose(m_threadHDB[d]);
    //            m_threadHDB[d] = 0;
    //        }
    //    }
    //    m_threadHDB.clear();

    //NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
    NMDebugCtx(ctx, << "...");

    m_StreamingProc = false;

    // check whether there are any open
    // dbs which need closing
    //    for (int d=0; d < m_threadHDB.size(); ++d)
    //    {
    //        if (m_threadHDB[d] != 0)
    //        {
    //            tchdbvanish(hdb);
    //            tchdbclose(m_threadHDB[d]);
    //            m_threadHDB[d] = 0;
    //        }
    //    }
    //    m_threadHDB.clear();

//    if (m_tcHDB != 0)
//    {
//        tchdbvanish(m_tcHDB);
//        tchdbclose(m_tcHDB);
//        m_tcHDB = 0;
//    }

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
