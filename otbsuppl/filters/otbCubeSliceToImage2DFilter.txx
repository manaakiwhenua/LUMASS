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
/*
 * otbCubeSliceToImage2DFilter.txx
 *
 *  Created on: 2020-12-07
 *      Author: alex
 */

#ifndef __otbCubeSliceToImage2DFilter_txx
#define __otbCubeSliceToImage2DFilter_txx

#include "otbCubeSliceToImage2DFilter.h"
#include "itkImageRegionIterator.h"
//#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
//#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkProgressReporter.h"

namespace otb {

template <class TInputImage, class TOutputImage>
CubeSliceToImage2DFilter<TInputImage, TOutputImage>
::CubeSliceToImage2DFilter()
    : m_CollapsedDimIndex(-1)
{
    //this->SetNumberOfThreads(1);
}

template <class TInputImage, class TOutputImage>
CubeSliceToImage2DFilter<TInputImage, TOutputImage>
::~CubeSliceToImage2DFilter()
{
}

template <class TInputImage, class TOutputImage>
void CubeSliceToImage2DFilter<TInputImage, TOutputImage>
::GenerateOutputInformation(void)
{
    NMDebugCtx(ctxCubeSliceToImage2DFilter, << "...");

    //Superclass::GenerateOutputInformation();

    /*
     * sizes, band, resolutions, etc.(?)
     */

    if (    m_InputSize.size() != InputImageType::ImageDimension
         || m_InputIndex.size() != InputImageType::ImageDimension
       )
    {
        itkExceptionMacro(<< "The input parameters' dimension do not match the "
                             "number of dimension of the input image!");
    }

    InputImageType* InImg = const_cast<TInputImage*>(this->GetInput());
    InputSpacingType inputSpacing = InImg->GetSpacing();
    InputDirectionType inputDirection = InImg->GetDirection();

    TOutputImage* OutImg = const_cast<TOutputImage*>(this->GetOutput());

    OutputRegionType      outRegion;
    OutputSpacingType     outSpacing;
    OutputPointType       outOrigin;
    OutputDirectionType   outDirection;
    outDirection.Fill(0);
    for (int d=0; d < OutputImageType::ImageDimension; ++d)
    {
        outRegion.SetSize(d, m_InputSize[m_DimMapping[d]-1]);
        outRegion.SetIndex(d, m_InputIndex[m_DimMapping[d]-1]);
        outSpacing[d] = inputSpacing[m_DimMapping[d]-1];
        outOrigin[d]  = m_InputOrigin[m_DimMapping[d]-1];
        if (d == 1)
        {
            outDirection[d][d] = -1;
        }
        else
        {
            outDirection[d][d] = 1;
        }
    }

    OutImg->SetLargestPossibleRegion(outRegion);
    OutImg->SetSpacing(outSpacing);
    OutImg->SetDirection(outDirection);
    OutImg->SetOrigin(outOrigin);

    NMDebugCtx(ctxCubeSliceToImage2DFilter, << "done!");
}

template <class TInputImage, class TOutputImage>
void CubeSliceToImage2DFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion(void)
{
    Superclass::GenerateInputRequestedRegion();

    TOutputImage* out = dynamic_cast<TOutputImage*>(this->GetOutput());
    TInputImage* in = const_cast<TInputImage*>(this->GetInput());

    OutputRegionType outRegion = out->GetRequestedRegion();
    InputRegionType  inRegion;

    // identify the index of the input image to be
    // collapsed in the output image
    int sumidx = 0;         // 1-based index
    int maxidx = 0;         // 1-based inex
    m_CollapsedDimIndex = -1;   // 0-based index
    m_In2OutIdxMap.resize(InputImageDimension);
    for (int r=0; r < InputImageDimension; ++r)
    {
        if (r < OutputImageDimension)
        {
            sumidx += m_DimMapping[r];
            m_In2OutIdxMap[m_DimMapping[r]-1] = r;
        }
        maxidx += r+1;
    }
    m_CollapsedDimIndex = maxidx - sumidx - 1;  // -1 to convert to 0-based index
    m_In2OutIdxMap[m_CollapsedDimIndex] = -1;

    // cover the output's two dimensions (x, y)
    for (int d=0; d < InputImageType::ImageDimension; ++d)
    {
        if (d == m_CollapsedDimIndex)
        {
            inRegion.SetSize(m_CollapsedDimIndex, 1);
            inRegion.SetIndex(m_CollapsedDimIndex, m_InputIndex[m_CollapsedDimIndex]);
        }
        else
        {
            inRegion.SetSize(d, outRegion.GetSize(m_In2OutIdxMap[d]));
            inRegion.SetIndex(d, outRegion.GetIndex(m_In2OutIdxMap[d]));
        }
    }

    in->SetRequestedRegion(inRegion);
}

template <class TInputImage, class TOutputImage>
void CubeSliceToImage2DFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputRegionType &outputRegionForThread,
          itk::ThreadIdType threadId )
{
    InputRegionType inRegion;
    // cover the output's two dimensions (x, y)
    for (int d=0; d < InputImageType::ImageDimension; ++d)
    {
        if (d == m_CollapsedDimIndex)
        {
            inRegion.SetSize(d, 1);
            inRegion.SetIndex(d, m_InputIndex[d]);
        }
        else
        {
            inRegion.SetSize(d, outputRegionForThread.GetSize(m_In2OutIdxMap[d]));
            inRegion.SetIndex(d, outputRegionForThread.GetIndex(m_In2OutIdxMap[d]));
        }
    }

    //const long inputNumPix = inRegion.GetNumberOfPixels();
    //const long outputNumPix = outputRegionForThread.GetNumberOfPixels();

    //    if (inputNumPix != outputNumPix)
    //    {
    //        itkExceptionMacro(<< "Input and output region do not have the same size! "
    //                          << "inRegion=" << inputNumPix << " | outRegion=" << outputNumPix);
    //    }

    using InIterType  = itk::ImageRegionConstIterator<TInputImage>;
    using OutIterType = itk::ImageRegionIterator<TOutputImage>;

    InIterType inIter(this->GetInput(), inRegion);
    OutIterType outIter(this->GetOutput(), outputRegionForThread);

    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

    for (inIter.GoToBegin(), outIter.GoToBegin();
            !inIter.IsAtEnd(); //!outIter.IsAtEnd();
                ++inIter, ++outIter)
    {
        outIter.Set(static_cast<OutputImagePixelType>(inIter.Get()));
        progress.CompletedPixel();
    }
}



} // end of namespace

#endif // end of ifndef template instantiation
