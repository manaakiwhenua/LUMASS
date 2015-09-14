/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2014 Landcare Research New Zealand Ltd
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



#ifndef __otbCombineTwoFilter_txx
#define __otbCombineTwoFilter_txx

#include "nmlog.h"
#include "otbCombineTwoFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

// TOKYO CABINET
//#include "tcutil.h"
#include "tchdb.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace otb
{

template< class TInputImage, class TOutputImage >
CombineTwoFilter< TInputImage, TOutputImage >
::CombineTwoFilter()
{
    this->SetNumberOfRequiredInputs(1);
	this->SetNumberOfRequiredOutputs(1);

    m_UniqueComboIdx = 0;
    m_StreamingProc = false;
    m_HDBFileName = "";
    m_HDB = tchdbnew();

    m_ComboTable = AttributeTable::New();
}

template< class TInputImage, class TOutputImage >
CombineTwoFilter< TInputImage, TOutputImage >
::~CombineTwoFilter()
{
    if (m_HDB)
        tchdbdel(m_HDB);
}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream& os, itk::Indent indent) const
{

}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer tab)
{
    m_vRAT.resize(idx+1);
    m_vRAT[idx] = tab;
}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::GenerateData()
{
	NMDebugCtx(ctx, << "...");

    int nbInputImages = this->GetNumberOfIndexedInputs();

    std::vector<TInputImage*> inputImages(nbInputImages, 0);
    for (int i=0; i < nbImages; ++i)
    {
        inputImages[i] = static_cast<TInputImage*>(this->GetInput(i));
    }

    // for now, the size of the input regions must be exactly the same
    InputImageRegionType lprCtrl = inputImages[0]->GetLargestPossibleRegion();

    for (int i=1; i < nbImages; ++i)
    {
        if (	(	inputImages[i]->GetLargestPossibleRegion().GetSize(0)
                 != lprCtrl.GetSize(0))
            ||  (   inputImages[i].GetSize(1)
                 != lprCtrl.GetSize(1))
           )
        {
            itkExceptionMacro(<< "Input imgages' dimensions don't match!");
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }


    // ======================================================================
    // ALLOCATE THE OUTPUT IMAGE
    // ======================================================================
    InputImagePointerType inImg = inputImages[0];
    OutputImagePointerType outImg = this->GetOutput();
    outImg->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
    outImg->SetBufferedRegion(inImg->GetBufferedRegion());
    outImg->SetRequestedRegion(inImg->GetRequestedRegion());
    outImg->Allocate();

    ComboIndexType lprPixNum = lprCtrl.GetNumberOfPixels();
    OutputImageRegionType outRegion = outImg->GetBufferedRegion();

    // ======================================================================
    // Set up the output table (ComboTable)
    // ======================================================================

    if (!m_StreamingProc)
    {
        m_ComboMap.clear();
        m_StreamingProc = true;
        m_UniqueComboIdx = 0;
        m_TotalPixCount = 0;
        m_NodataCount = 0;

        if (m_ComboTable->createTable("", "1") == AttributeTable::ATCREATE_ERROR)
        {
            NMDebugCtx(ctx, << "done!");
            itkExceptionMacro("Failed creating unique combination attribute table!");
            return;
        }
        m_ComboTable->beginTransaction();
        m_ComboTable->AddColumn("rowidx");
        m_ComboTable->AddColumn("count");
        std::stringstream sscolname;
        for (int i=0; i < nbInputImages; ++i)
        {
            sscolname.str();
            sscolname << "L" << i;
            m_ComboTable->AddColumn(sscolname.str().c_str());
        }
        m_ComboTable->endTransaction();

        if (m_HDB != 0)
        {
            tchdbvanish(m_HDB);
            tchdbclose(m_HDB);
            m_HDB = 0;
        }

        m_HDB = tchdbnew();
        m_HDBFileName = std::tmpnam(0);
        if (!tchdbopen(m_HDB, m_HDBFileName.c_str(), HDBOWRITER | HDBOCREAT))
        {
            itkExceptionMacro(<< "Failed creating key-value store!");
            return;
        }
    }

    // ======================================================================
    // Identify unique combinations
    // ======================================================================
    itk::ProgressReporter progress(this, 0, outRegion.GetNumberOfPixels());

    typedef itk::ImageRegionConstIterator<InputImageType> InIteratorType;
    std::vector<InIteratorType> vInIters;
    for (int i=0; i < nbInputImages; ++i)
    {
        InIteratorType ii(inputImages[i], outRegion);
        vInIters.push_back(ii);
        ii.GoToBegin();
    }
    itk::ImageRegionIterator<OutputImageType> outIter(this->GetOutput(), outRegion);
    outIter.GoToBegin();

    bool nodata;
    InputPixelType curVal;
    std::vector<InputPixelType> combo(nbInputImages);
    std::vector<OutputPixelType> comboValue(2);
    while (!outIter.IsAtEnd() && !this->GetAbortGenerateData())
    {
        nodata = false;
        for (int in=0; in < nbInputImages; ++in)
        {
            curVal = vInIters[in].Get();
            if (curVal == m_InputNodata[in])
            {
                nodata = true;
            }
            combo[in] = curVal;
            ++vInIters[in];
        }

        if (!nodata)
        {
            if (tchdbget3(m_HDB,
                          static_cast<void*>(&combo[0]),
                          sizeof(InputPixelType) * nbInputImages,
                          static_cast<void*>(&comboValue[0]),
                          sizeof(OutputPixelType) * 2)
                == -1)
            {
                comboValue[0] = m_UniqueComboIdx;
                comboValue[1] = 1;
                tchdbput(m_HDB,
                              static_cast<void*>(&combo[0]),
                              sizeof(InputPixelType) * nbInputImages,
                              static_cast<void*>(&comboValue[0]),
                              sizeof(OutputPixelType) * 2);
                outIt.Set(m_UniqueComboIdx);
                ++m_UniqueComboIdx;
            }
            else
            {
                comboValue[1] += 1;


                outIt.Set(comboValue[0]);
            }
        }
        else
        {
            for (int i=0; i < nbInputImages; ++i)
            {
                if (i < m_InputNodata.size())
                    combo[i] = m_InputNodata[i];
                else
                    combo[i] = 0;
            }
            comboValue[0] = 0;
            comboValue[1] = ++m_NodataCount;


            outIt.Set(0);
        }

        progress.CompletedPixel();
        ++outIt;
        ++m_TotalPixCount;
    }


    void* nextKey = 0;
    int sizeKey = 0;
    if (m_TotalPixCount == lprCtrl.GetNumberOfPixels())
    {
        std::vector<otb::AttributeTable::ColumnValue> inVals(nbInputImages+2);
        for (int i=0; i < nbInputImages+2; ++i)
        {
            inVals[i].type = AttributeTable::ATTYPE_INT;
        }

        if (!tchdbiterinit(m_HDB))
        {
            int ecode = tchdbecode(m_HDB);
            itkExceptionMacro(<< "ERROR writing unique value table: "
                              << tchdberrmsg(ecode));
            tchdbclose(m_HDB);
            this->m_ComboTable->closeTable();
            //NMDebugCtx(ctx, << "done!");
            return;
        }

        //itk::ProgressReporter writeProgress(this, 0, m_OutIdx);

        m_ComboTable->beginTransaction();
        while(nextKey = tchdbiternext(m_HDB, &sizeKey))
        {
            if (tchdbget3(m_HDB, nextKey, sizeKey,
                          static_cast<void*>(&comboValue),
                          sizeof(typename TOutputImage::PixelType))
                == -1)
            {
                itkWarningMacro(<< "Failed reading Tokyo Cabinet Record!");
                free(nextKey);
                continue;
            }

            inVals[0].ival = comboValue[0];
            inVals[1].ival = comboValue[1];
            for (int i=0; i < nbInputImages; ++i)
            {
                inVals[i+2].ival = static_cast<InputPixelType*>(nextKey)[i];
            }
            m_ComboTable->doBulkSet(inVals);

            free(nextKey);
            //writeProgress.CompletedPixel();
        }
        m_ComboTable->endTransaction();


        // just close the table
        //m_UVTable->closeTable();

    }



	NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
	NMDebugCtx(ctx, << "...");

//    if (m_dropTmpDBs)
//    {
//        mZoneTable->closeTable(true);
//    }
//    else
//    {
//        mZoneTable->closeTable();
//    }
//    mZoneTable = 0;
//    m_NextZoneId = 0;

    //    if (m_HDB)
    //    {
    //        if (m_dropTmpDBs)
    //        {
    //            tchdbvanish(m_HDB);
    //        }
    //        tchdbclose(m_HDB);
    //    }
    m_TotalPixCount = 0;
    m_UniqueComboIdx = 0;
    m_StreamingProc = false;
    m_ComboTable = AttributeTable::New();

	Superclass::ResetPipeline();
	NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbCombineTwoFilter_txx
