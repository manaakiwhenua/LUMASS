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
 * otbImage2DToCubeSliceFilter.txx
 *
 *  Created on: 27/05/2012
 *      Author: alex
 */

#ifndef __otbImage2DToCubeSliceFilter_txx
#define __otbImage2DToCubeSliceFilter_txx

#include "otbImage2DToCubeSliceFilter.h"
#include "itkImageRegionIterator.h"
//#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkProgressReporter.h"

namespace otb {

template <class TInputImage, class TOutputImage>
Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::Image2DToCubeSliceFilter()
{
    //this->SetNumberOfThreads(1);
}

template <class TInputImage, class TOutputImage>
Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::~Image2DToCubeSliceFilter()
{
}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::GenerateOutputInformation(void)
{
    NMDebugCtx(ctxImage2DToCubeSliceFilter, << "...");

//    Superclass::GenerateOutputInformation();

    /*
     * sizes, band, resolutions, etc.(?)
     */

    if (    m_OutputSize.size() != OutputImageType::ImageDimension
         || m_OutputIndex.size() != OutputImageType::ImageDimension
         || m_OutputSpacing.size() != OutputImageType::ImageDimension
       )
    {
        itkExceptionMacro(<< "The output parameter's dimension doesn't match the "
                             "number of dimension of the output image!");
    }

    TOutputImage* OutImg = const_cast<TOutputImage*>(this->GetOutput());

    OutputRegionType outRegion;
    OutputSpacingType     outSpacing;
    OutputPointType       outOrigin;
    OutputDirectionType   outDirection;
    outDirection.Fill(0);
    for (int d=0; d < OutputImageType::ImageDimension; ++d)
    {
        outRegion.SetSize(d, m_OutputSize[d]);
        outRegion.SetIndex(d, m_OutputIndex[d]);
        outSpacing[d] = m_OutputSpacing[d];
        outOrigin[d]  = m_OutputOrigin[d];
        outDirection[d][d] = d == 1 ? -1 : 1;
    }

    OutImg->SetLargestPossibleRegion(outRegion);
    OutImg->SetSpacing(outSpacing);
    OutImg->SetDirection(outDirection);
    OutImg->SetOrigin(outOrigin);

    NMDebugCtx(ctxImage2DToCubeSliceFilter, << "done!");
}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion(void)
{
    TOutputImage* out = dynamic_cast<TOutputImage*>(this->GetOutput());
    TInputImage* in = const_cast<TInputImage*>(this->GetInput());

    OutputRegionType outRegion = out->GetRequestedRegion();
    InputRegionType  inRegion;
    for (int d=0; d < InputImageType::ImageDimension; ++d)
    {
        inRegion.SetSize(d, outRegion.GetSize(m_DimMapping[d]-1));
        inRegion.SetIndex(d, outRegion.GetIndex(m_DimMapping[d]-1));
    }

    in->SetRequestedRegion(inRegion);
}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputRegionType &outputRegionForThread,
          itk::ThreadIdType threadId )
{
    InputRegionType inRegion;
    for (int d=0; d < InputImageType::ImageDimension; ++d)
    {
        inRegion.SetSize(d, outputRegionForThread.GetSize(m_DimMapping[d]-1));
        inRegion.SetIndex(d, outputRegionForThread.GetIndex(m_DimMapping[d]-1));
    }

    using InIterType  = itk::ImageRegionConstIterator<TInputImage>;
    using OutIterType = itk::ImageRegionIterator<TOutputImage>;

    InIterType inIter(this->GetInput(), inRegion);
    OutIterType outIter(this->GetOutput(), outputRegionForThread);

    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

    for (inIter.GoToBegin(), outIter.GoToBegin();
            !inIter.IsAtEnd();
                ++inIter, ++outIter)
    {
        outIter.Set(inIter.Get());
        progress.CompletedPixel();
    }
}



} // end of namespace

#endif // end of ifndef template instantiation
