/******************************************************************************
* Created by Alexander Herzig
* Copyright 2019 Landcare Research New Zealand Ltd
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

/*!
 *      This class is a streaming-enabled adapation of itk::RegionOfInterestImageFilter;
 *      Since ThreadedGenerateData in itk::RegionOfInterestImageFilter is
 *      non virtual, we had to inherit from itk::ImageToImageFilter and
 *      copy the parts we wanted to re-use from itk::RegionOfInterestImageFilter;
 *
 *      The adapted class is based itk::RegionOfInterestImageFilter (ITK 4.13), see copyright
 *      info below:
 *
         /*=========================================================================
         *
         *  Copyright Insight Software Consortium
         *
         *  Licensed under the Apache License, Version 2.0 (the "License");
         *  you may not use this file except in compliance with the License.
         *  You may obtain a copy of the License at
         *
         *         http://www.apache.org/licenses/LICENSE-2.0.txt
         *
         *  Unless required by applicable law or agreed to in writing, software
         *  distributed under the License is distributed on an "AS IS" BASIS,
         *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
         *  See the License for the specific language governing permissions and
         *  limitations under the License.
         *
         *=========================================================================
*/

#ifndef __NMSTREAMINGROIIMAGEFILTER_TXX
#define __NMSTREAMINGROIIMAGEFILTER_TXX

#include "nmStreamingROIImageFilter.h"

#include <itkProgressReporter.h>
#include <itkImageAlgorithm.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

#include "nmlog.h"

namespace nm {

template< class TInputImage, class TOutputImage>
StreamingROIImageFilter< TInputImage, TOutputImage>
::StreamingROIImageFilter()
{
    IndexType idx;
    idx.Fill(0);

    SizeType sz;
    sz.Fill(0);

    m_RegionOfInterest.SetIndex(idx);
    m_RegionOfInterest.SetSize(sz);

    PointType origin, length;
    for (int d=0; d < TInputImage::ImageDimension; ++d)
    {
        origin[d] = 0.0;
        length[d] = 0.0;
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  // Do not call the superclass' implementation of this method since
  // this filter allows the input the output to be of different dimensions

  // Get pointers to the input and output
  OutputImagePointer outputPtr = this->GetOutput();
  InputImageConstPointer inputPtr  = this->GetInput();

  if ( !outputPtr || !inputPtr )
    {
    return;
    }

  bool coords = false;

  // expect non-zero region for each dimension, for slicing
  // or dimension collapse, use itk::ExtractImageFilter
  for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
  {
      if (m_RegionOfInterest.GetSize(d) == 0)
      {
          coords = true;
      }
  }

  // Set the output image size to the same value as the region of interest.
  RegionType region;
  IndexType  start;
  IndexType  roistart;
  PointType  outputOrigin;

  start.Fill(0);
  region.SetIndex(start);

  if (coords)
  {
     // ROI defined in coordinates space (= projected coordinates and assoicated units)
     using SizeValueType = typename TInputImage::SizeValueType;
     SpacingType spacing = inputPtr->GetSpacing();

     if (!inputPtr->TransformPhysicalPointToIndex(m_Origin, roistart))
     {
         itkExceptionMacro(<< "Input origin is outside the input image region!");
         return;
     }
     m_RegionOfInterest.SetIndex(roistart);

     for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
     {
        if (m_Length[d] == 0)
        {
            itkExceptionMacro(<< "Invalid input region length (=0) for dimension " << d << "!");
            return;
        }

        region.SetSize(d, static_cast<SizeValueType>((m_Length[d] / spacing[d]) + 0.5));
     }
     m_RegionOfInterest.SetSize(region.GetSize());
  }
  else // ROI defined by image region (= pixel space)
  {
      for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
      {
         roistart[d] = m_RegionOfInterest.GetIndex(d);
      }
      region.SetSize( m_RegionOfInterest.GetSize() );
  }

  // start with copying all input data without modification
  outputPtr->CopyInformation(inputPtr);

  // constrain size of region to largestpossible region of
  // input image
  RegionType lpr = inputPtr->GetLargestPossibleRegion();
  for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
  {
      if (region.GetSize(d) + roistart[d] > lpr.GetSize(d))
      {
          region.SetSize(d, lpr.GetSize(d) - roistart[d]);
          NMProcWarn(<< "Cropped requested input region size ("
                     << d << ": " << region.GetSize(d)
                     << ") to largest possible input region size"
                     << " realtive to the origin ("
                     << d << ": " << static_cast<long>((lpr.GetSize(d) - roistart[d]))
                     << ")!");
      }
  }

  // we determine the origin based on the index to avoid any image shift;
  // the objective is to have the extracted region lying exactly on top
  // of the input image region, i.e. the transform equals the identity matrix
  inputPtr->TransformIndexToPhysicalPoint(roistart, outputOrigin);

  // Adjust output region
  outputPtr->SetLargestPossibleRegion(region);

  // Correct origin of the extracted region.
  outputPtr->SetOrigin(outputOrigin);
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
    //Superclass::GenerateInputRequestedRegion();

    TOutputImage* output = this->GetOutput();
    typename Superclass::OutputImageRegionType requestedRegion;
    if (output != nullptr)
    {
        requestedRegion = output->GetRequestedRegion();
    }

    TInputImage* input = const_cast<TInputImage*>(this->GetInput());
    if (input != nullptr)
    {
        typename Superclass::InputImageRegionType inputRequestedRegion;
        for (int d=0; d < TInputImage::ImageDimension; ++d)
        {
            inputRequestedRegion.SetIndex(d, requestedRegion.GetIndex(d)+m_RegionOfInterest.GetIndex(d));
            inputRequestedRegion.SetSize(d, requestedRegion.GetSize(d));
        }
        input->SetRequestedRegion(inputRequestedRegion);

        // need to force the source object (filter) to update each time
        input->Modified();
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(const RegionType & outputRegionForThread,
                          ThreadIdType threadId)
{
    // Get the input and output pointers
    const TInputImage *inputPtr  = this->GetInput();
    TOutputImage      *outputPtr = this->GetOutput();

    RegionType outputRequestedRegion = outputPtr->GetRequestedRegion();
    RegionType inputRegionForThread = inputPtr->GetRequestedRegion();

    // calc thread index offset for read
    for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
    {
        const IndexValueType readOffset = outputRegionForThread.GetIndex(d) - outputRequestedRegion.GetIndex(d);
        inputRegionForThread.SetIndex(d, inputRegionForThread.GetIndex(d) + readOffset);
        inputRegionForThread.SetSize(d, outputRegionForThread.GetSize(d));
    }

    using InputIteratorType = itk::ImageRegionConstIterator<TInputImage>;
    InputIteratorType inIt = InputIteratorType(inputPtr, inputRegionForThread);

    using OutputIteratorType = itk::ImageRegionIterator<TOutputImage>;
    OutputIteratorType outIt = OutputIteratorType(outputPtr, outputRegionForThread);

    using PixelType = typename TOutputImage::PixelType;

    itk::ProgressReporter progress( this, threadId, outputRegionForThread.GetNumberOfPixels());

    for (inIt.GoToBegin(), outIt.GoToBegin(); !inIt.IsAtEnd(); ++inIt, ++outIt)
    {
        outIt.Set(static_cast<PixelType>(inIt.Get()));
        progress.CompletedPixel();
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::SetROIIndex(std::vector<IndexValueType> idx)
{
    if (idx.size() != TInputImage::ImageDimension )
    {
        NMProcWarn(<< "SetROIIndex: The number of supplied parameters"
                  << " does not match the number of image dimensions!");
        return;
    }

    for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
    {
        m_RegionOfInterest.SetIndex(d, idx[d]);
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::SetROISize(std::vector<IndexValueType> size)
{
    if (size.size() != TInputImage::ImageDimension)
    {
        NMProcWarn(<< "SetROISize: The number of supplied parameters"
                  << " does not match the number of image dimensions!");
        return;
    }

    for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
    {
        m_RegionOfInterest.SetSize(d, size[d]);
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::SetROIOrigin(std::vector<PointValueType> origin)
{
    if (origin.size() != TInputImage::ImageDimension)
    {
        NMProcWarn(<< "SetROIOrigin: The number of supplied parameters"
                  << " does not match the number of image dimensions!");
        return;
    }

    for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
    {
        m_Origin[d] = origin[d];
    }
}

template< class TInputImage, class TOutputImage>
void
StreamingROIImageFilter< TInputImage, TOutputImage>
::SetROILength(std::vector<PointValueType> length)
{
    if (length.size() != TInputImage::ImageDimension)
    {
        NMProcWarn(<< "SetROILength: The number of supplied parameters"
                  << " does not match the number of image dimensions!");
        return;
    }

    for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
    {
        m_Length[d] = length[d];
    }
}



} // end of namespace

#endif // end of include guard
