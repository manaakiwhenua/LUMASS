/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd
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

#ifndef __itkNMCostDistanceBufferImageFilter_txx
#define __itkNMCostDistanceBufferImageFilter_txx

//#define ctx "NMCostDistanceBufferImageFilter"
#include "nmlog.h"

#include <iostream>
#include <limits>

#include "itkNMCostDistanceBufferImageFilter.h"
#include "itkReflectiveImageRegionConstIterator.h"
#include "itkReflectiveImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodIterator.h"

template <class TInputImage,class TOutputImage>
const std::string itk::NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>::ctx = "NMCostDistanceBufferImageFilter";

namespace itk
{

/**
 *    Constructor
 */
template <class TInputImage,class TOutputImage>
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::NMCostDistanceBufferImageFilter()
{
  this->SetNumberOfRequiredOutputs( 1 );

  OutputImagePointer distanceMap = OutputImageType::New();
  this->SetNthOutput( 0, distanceMap.GetPointer() );

  m_rowDist = 0;
  m_colDist = 0;
  m_Categories = 0;
  m_NumCategories = 0;
  m_NumExec = 1;
  m_UpwardCounter = 1;
  m_RowCounter = 1;
  m_BufferZoneIndicator = 0;
  m_CreateBuffer = false;
  m_UseImageSpacing = false;
  m_ProcessDownward = false;
  m_ProcessUpward = false;
  m_MaxDistance = itk::NumericTraits<OutPixelType>::max();
}

template <class TInputImage,class TOutputImage>
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::~NMCostDistanceBufferImageFilter()
{
    if (m_colDist)
    {
        delete[] m_colDist;
        m_colDist = 0;
    }

    if (m_rowDist)
    {
        delete[] m_rowDist;
        m_rowDist = 0;
    }

    if (m_Categories)
        delete[] m_Categories;
    m_NumCategories = 0;
}

template <class TInputImage,class TOutputImage>
void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::SetCategories(std::vector<double> cats)
{
    if (m_Categories)
    {
        delete[] m_Categories;
        m_Categories = 0;
        m_NumCategories = 0;
    }

    if (cats.size() == 0)
        return;

    m_NumCategories = cats.size();
    m_Categories = new double[m_NumCategories];
    for (int i=0; i < cats.size(); ++i)
        m_Categories[i] = cats.at(i);
}

/**
 *  Reset the filter for processing a new image
 *
 */
template <class TInputImage,class TOutputImage>
void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::resetExecCounter(void)
{
  this->m_NumExec = 1;
  this->m_UpwardCounter = 1;
  this->m_RowCounter = 1;
}


/**
 *  Return the distance map Image pointer
 */
template <class TInputImage,class TOutputImage>
typename NMCostDistanceBufferImageFilter<
  TInputImage,TOutputImage>::OutputImageType *
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::GetDistanceMap(void)
{
  return  dynamic_cast< OutputImageType * >(
    this->ProcessObject::GetOutput(0) );
}


template <class TInputImage,class TOutputImage>
void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::GenerateData()
{
    NMDebugCtx(ctx, << "...");

    bool bBiDir = false;
    if (!m_ProcessDownward && !m_ProcessUpward)
        bBiDir = true;

    InputImagePointer inImg = const_cast<InputImageType*>(this->GetInput(0));
    OutputImagePointer distanceMap = this->GetDistanceMap();
    distanceMap->SetLargestPossibleRegion(inImg->GetLargestPossibleRegion());
    distanceMap->SetRequestedRegion(inImg->GetRequestedRegion());

    if (this->m_NumExec == 1)
    {
        //this->InvokeEvent(itk::StartEvent());
        distanceMap->SetBufferedRegion(inImg->GetRequestedRegion());
        distanceMap->Allocate();
    }

    InputImagePointer costImg;
    if (this->GetNumberOfInputs() == 2)
        costImg = const_cast<InputImageType*>(this->GetInput(1));

    typename InputImageType::RegionType region =
            distanceMap->GetRequestedRegion();

    // Support progress methods/callbacks.

    // Each pixel is visited 2^InputImageDimension times, and the number
    // of visits per pixel needs to be computed for progress reporting.
    //unsigned long visitsPerPixel = (1 << InputImageDimension);
    //unsigned long updateVisits, i = 0;
    //updateVisits = region.GetNumberOfPixels() * visitsPerPixel / 10;
    //if (updateVisits < 1)
    //{
    //	updateVisits = 1;
    //}
    //const float updatePeriod = static_cast<float>(updateVisits) * 10.0;

    InPixelType* ibuf = inImg->GetBufferPointer();
    OutPixelType* obuf = distanceMap->GetBufferPointer();
    InPixelType* cbuf = 0;
    if (costImg.IsNotNull())
        cbuf = costImg->GetBufferPointer();

    SpacingType spacing;
    if (this->m_UseImageSpacing)
    {
        spacing = distanceMap->GetSignedSpacing();
    }
    else
    {
        spacing[0] = 1;
        spacing[1] = 1;
    }

    // we square the max distance, since we only compare squared distances
    // in the  calc distance function
    OutPixelType maxDist = static_cast<OutPixelType>(41e9);//itk::NumericTraits<OutPixelType>::max();
    OutPixelType userDist = this->m_MaxDistance;
    if (userDist <= 0)
    {
        userDist = maxDist;
    }

    int ncols = region.GetSize()[0];
    int nrows = region.GetSize()[1];
    int maxrows = distanceMap->GetLargestPossibleRegion().GetSize()[1] * 2;

    double* colDist = 0;
    double* rowDist = 0;
    if (m_NumExec == 1)
    {
        colDist = new double[(ncols+2) * 2];
        rowDist = new double[(ncols+2) * 2];

        for (int v=0; v < ((ncols+2)*2); ++v)
        {
            colDist[v] = -9;
            rowDist[v] = -9;
        }
    }
    else
    {
        colDist = m_colDist;
        rowDist = m_rowDist;
    }

    /** neighbour pixel's relative indices
     *  array order is: diagonal, y-dir, x-dir
     *  0 1 2
     *  3 x 5
     *  6 7 8
     */

    //int odr[3][2];
    int** odr = new int*[3];
    int** odl = new int*[3];
    int** oul = new int*[3];
    int** our = new int*[3];
    for (int i=0; i < 3; ++i)
    {
        odr[i] = new int[2];
        odl[i] = new int[2];
        oul[i] = new int[2];
        our[i] = new int[2];
    }
    odr[0][0] = -1; odr[0][1] = -1; // idx 0
    odr[1][0] =  0; odr[1][1] = -1; // idx 1
    odr[2][0] = -1; odr[2][1] =  0; // idx 3

    //int odl[3][2];
    odl[1][0] = 0; odl[1][1] = -1; // idx 1
    odl[0][0] = 1; odl[0][1] = -1; // idx 2
    odl[2][0] = 1; odl[2][1] =  0; // idx 5

    //int oul[3][2];/home/alex/garage/img/
    oul[2][0] = 1; oul[2][1] = 0; // idx 5
    oul[1][0] = 0; oul[1][1] = 1; // idx 7
    oul[0][0] = 1; oul[0][1] = 1; // idx 8

    //int our[3][2];
    our[2][0] = -1; our[2][1] = 0; // idx 3
    our[0][0] = -1; our[0][1] = 1; // idx 6
    our[1][0] =  0; our[1][1] = 1; // idx 7

    int col, row, bufrow;
    bool bleftright;
    bool binit;
    if (this->m_ProcessDownward || bBiDir)
    {
        //NMDebugAI(<< "downwards ..." << endl);
        row = 1;
        if (bBiDir || this->m_NumExec == 1)
        {
            row = 0;
        }

        bufrow = 1;
        for (; row < nrows; ++row)
        {
            binit = true;
            bleftright = true;
            calcPixelDistance(obuf, ibuf, cbuf, colDist, rowDist, odr, bleftright, binit,
                            row, ncols, //nrows,
                            bufrow,
                            maxDist, userDist, spacing);
            binit = false;
            bleftright = false;
            calcPixelDistance(obuf, ibuf, cbuf, colDist, rowDist, odl, bleftright, binit,
                            row, ncols, //nrows,
                            bufrow,
                            maxDist, userDist, spacing);

            memcpy((void*)colDist, (void*)(colDist+ncols+2), (ncols+2)*sizeof(double));
            memcpy((void*)rowDist, (void*)(rowDist+ncols+2), (ncols+2)*sizeof(double));

            this->UpdateProgress(this->m_RowCounter / (float)maxrows);
            if (this->GetAbortGenerateData())
            {
                goto cleanup;
            }
            ++this->m_RowCounter;
        }
    }

    if (this->m_ProcessUpward || bBiDir)
    {
        //NMDebugAI(<< "and up again ..." << endl);
        row = nrows - 2;
        if (bBiDir || this->m_UpwardCounter == 1)
        {
            row = nrows - 1;
            for (int i=(ncols+2); i < (2*(ncols+2)); ++i)
            {
                colDist[i] = -9;
                rowDist[i] = -9;
            }
            for (int i=1; i < ncols; ++i)
            {
                colDist[i] = maxDist;
                rowDist[i] = maxDist;
            }
            colDist[0] = -9; colDist[ncols+1] = -9;
            rowDist[0] = -9; rowDist[ncols+1] = -9;
        }

        bufrow = 0;
        binit = false;
        for (; row >= 0; --row)
        {
            bleftright = true;
            calcPixelDistance(obuf, ibuf, cbuf, colDist, rowDist, our, bleftright, binit,
                            row, ncols, //nrows,
                            bufrow,
                            maxDist, userDist, spacing);
            bleftright = false;
            calcPixelDistance(obuf, ibuf, cbuf, colDist, rowDist, oul, bleftright, binit,
                            row, ncols, //nrows,
                            bufrow,
                            maxDist, userDist, spacing);

            memcpy((void*)(colDist+ncols+2), (void*)colDist, (ncols+2)*sizeof(double));
            memcpy((void*)(rowDist+ncols+2), (void*)rowDist, (ncols+2)*sizeof(double));

            for (int rr=1; rr < (ncols+1); ++rr)
            {
                colDist[rr] = maxDist;
                rowDist[rr] = maxDist;
            }


            this->UpdateProgress(this->m_RowCounter / (float)maxrows);
            if (this->GetAbortGenerateData())
            {
                goto cleanup;
            }
            ++this->m_RowCounter;
        }
        ++this->m_UpwardCounter;
    }
    ++this->m_NumExec;

    cleanup:
    // clear memory
    for (int i=0; i < 3; ++i)
    {
        delete[] odr[i];
        delete[] odl[i];
        delete[] oul[i];
        delete[] our[i];
    }
    delete[] odr;
    delete[] odl;
    delete[] oul;
    delete[] our;

    // we keep those for the next
    // execution of this filter in
    // sequential processing mode
    // memory gets eventually cleared
    // by destructor
    m_colDist = colDist;
    m_rowDist = rowDist;

    //if (this->m_RowCounter >= maxrows)
        //this->InvokeEvent(itk::EndEvent());

    NMDebugCtx(ctx, << "done!");

} // end GenerateData()


template <class TInputImage,class TOutputImage>
void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Cost-Distance/Buffer-Map " << std::endl;
  os << indent << "Use Image Spacing : " << m_UseImageSpacing << std::endl;
  os << indent << "Maximum Distance  : " << m_MaxDistance << std::endl;
  os << indent << "Input source categories (objects): ";
  for (int c=0; c < this->m_NumCategories; ++c)
      os << this->m_Categories[c] << ", ";
  os << std::endl;

}

} // end namespace itk

#endif
