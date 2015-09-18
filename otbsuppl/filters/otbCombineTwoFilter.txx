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

namespace otb
{

template< class TInputImage, class TOutputImage >
CombineTwoFilter< TInputImage, TOutputImage >
::CombineTwoFilter()
{
    this->SetNumberOfRequiredInputs(1);
	this->SetNumberOfRequiredOutputs(1);

    m_NumUniqueCombinations = 0;
    m_StreamingProc = false;

    m_ComboTable = AttributeTable::New();
}

template< class TInputImage, class TOutputImage >
CombineTwoFilter< TInputImage, TOutputImage >
::~CombineTwoFilter()
{
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

    m_vHyperSpaceDomains.resize(idx+1);
    m_vHyperSpaceDomains[idx] = tab->GetNumRows();
}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::SetInputNodata(const std::vector<long long>& inNodata)
{
    m_InputNodata.clear();
    for (int i=0; i < inNodata.size(); ++i)
    {
        m_InputNodata.push_back(inNodata.at(i));
    }
}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::SetImgNames(const std::vector<std::string>& imgNames)
{
    m_ImgNames.clear();
    for (int i=0; i < imgNames.size(); ++i)
    {
        m_ImgNames.push_back(imgNames.at(i));
    }
}

template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::GenerateData()
{
	NMDebugCtx(ctx, << "...");

    int nbInputImages = this->GetNumberOfIndexedInputs();

    std::vector<TInputImage*> inputImages;
    for (int i=0; i < nbInputImages; ++i)
    {
        inputImages.push_back(const_cast<TInputImage*>(this->GetInput(i)));
    }

    // for now, the size of the input regions must be exactly the same
    InputImageRegionType lprCtrl = inputImages[0]->GetLargestPossibleRegion();

    for (int i=1; i < nbInputImages; ++i)
    {
        if (	(	inputImages[i]->GetLargestPossibleRegion().GetSize(0)
                 != lprCtrl.GetSize(0))
            ||  (   inputImages[i]->GetLargestPossibleRegion().GetSize(1)
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
        m_NumUniqueCombinations = 1;
        m_TotalPixCount = 0;
        m_NodataCount = 0;

        m_sComboTracker.clear();
        m_vHyperStrides.clear();
        m_vHyperStrides.resize(m_vHyperSpaceDomains.size());
        m_vHyperStrides[0] = 1;
        for (int s=1; s < m_vHyperSpaceDomains.size(); ++s)
        {
            m_vHyperStrides[s] = m_vHyperStrides[s-1] * m_vHyperSpaceDomains[s-1];
        }

        m_vColnames.clear();
        if (m_ComboTable->createTable("", "1") == AttributeTable::ATCREATE_ERROR)
        {
            NMDebugCtx(ctx, << "done!");
            itkExceptionMacro("Failed creating unique combination attribute table!");
            return;
        }
        m_vColnames.push_back(m_ComboTable->getPrimaryKey());

        m_ComboTable->beginTransaction();
        std::stringstream sscolname;
        for (int i=0; i < nbInputImages; ++i)
        {
            sscolname.str("");
            sscolname << "L" << i;
            m_ComboTable->AddColumn(sscolname.str(), AttributeTable::ATTYPE_INT);
            m_vColnames.push_back(sscolname.str());
        }
        m_ComboTable->endTransaction();

        // nodata values as required
        for (int n = m_InputNodata.size(); n < nbInputImages; ++n)
        {
            m_InputNodata.push_back(m_ComboTable->GetIntNodata());
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
    ComboIndexType curVal;
    std::vector<AttributeTable::ColumnValue> setVals(nbInputImages+1);
    setVals[0].type = AttributeTable::ATTYPE_INT;
    setVals[0].ival = 0;

    for (int i=1; i < nbInputImages; ++i)
    {
        setVals[i].type = AttributeTable::ATTYPE_INT;
        setVals[i].ival = m_InputNodata[i-1];
    }
    setVals[0].ival = 0;

    m_ComboTable->prepareBulkSet(m_vColnames);
    //m_ComboTable->beginTransaction();

//    if (!m_StreamingProc)
//    {
//        m_ComboTable->doBulkSet(setVals);
//    }

    ComboMapTypeIterator ctIter;
    while (!outIter.IsAtEnd() && !this->GetAbortGenerateData())
    {
        nodata = false;
        for (int in=0, va=1; in < nbInputImages; ++in, ++va)
        {
            setVals[va].ival = static_cast<ComboIndexType>(vInIters[in].Get());
            if (setVals[va].ival == m_InputNodata[in])
            {
                nodata = true;
            }

            if (in == 0)
            {
                curVal = setVals[va].ival;
            }
            else
            {
                curVal += setVals[va].ival * m_vHyperStrides[in];
            }
            ++vInIters[in];
        }

        if (nodata)
        {
            outIter.Set(static_cast<OutputPixelType>(0));
        }
        else
        {
            ctIter = m_ComboMap.find(curVal);
            if (ctIter != m_ComboMap.end())
            {
                outIter.Set(static_cast<OutputPixelType>(ctIter->second));
            }
            else
            {
                m_ComboMap[m_NumUniqueCombinations] = curVal;
                outIter.Set(static_cast<OutputPixelType>(m_NumUniqueCombinations));
                ++m_NumUniqueCombinations;
                //setVals[0].ival = curVal;

                //m_ComboTable->doBulkSet(setVals);
            }
        }

        progress.CompletedPixel();
        ++outIter;
        ++m_TotalPixCount;
    }
    //m_ComboTable->endTransaction();

    // set property max ComboIdx


    NMDebugCtx(ctx, << "done!");
}


template< class TInputImage, class TOutputImage >
void CombineTwoFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
	NMDebugCtx(ctx, << "...");

    m_ComboMap.clear();
    m_TotalPixCount = 0;
    m_NumUniqueCombinations = 0;
    m_StreamingProc = false;

    m_ComboTable = 0;
    m_ComboTable = AttributeTable::New();

    m_vHyperSpaceDomains.clear();
    m_sComboTracker.clear();

    Superclass::ResetPipeline();
    NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbCombineTwoFilter_txx
