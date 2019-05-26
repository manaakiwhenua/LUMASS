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
 *      This class is o streaming-enabled adaptation of itk::RegionOfInterestImageFilter;
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



#ifndef __NMSTREAMINGROIIMAGEFILTER_H
#define __NMSTREAMINGROIIMAGEFILTER_H

#include <itkImageToImageFilter.h>

#include "otbsupplfilters_export.h"

namespace nm
{
    /*!  \class StreamingROIImageFilter
     *   This filter is a streaming-enabled adaptation of
     *   itk::RegionOfInterestImageFilter (ITK 4.13)
     *
     *   The user can specify the region of interest (ROI)
     *   either by psssing pixel offsets (i.e. start index)
     *   and number of pixels (size in pixels)
     *   for each image dimension, or the origin (upper left
     *   pixel centre coordinates) and length (in coordinate
     *   units) along each image dimension; if coordinates
     *   and pixel measures are provided, pixel measures
     *   preceed.
     */

template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT StreamingROIImageFilter :
        public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
    using Self          = StreamingROIImageFilter;
    using Superclass    = itk::ImageToImageFilter<TInputImage, TOutputImage>;
    using Pointer       = itk::SmartPointer<Self>;
    using ConstPointer  = itk::SmartPointer<const Self>;

    using InputImageConstPointer = typename TInputImage::ConstPointer;
    using OutputImagePointer     = typename TOutputImage::Pointer;

    using PointType     = typename TInputImage::PointType;
    using IndexType     = typename TInputImage::IndexType;
    using SizeType      = typename TInputImage::SizeType;
    using RegionType    = typename TInputImage::RegionType;
    using SpacingType   = typename TInputImage::SpacingType;

    using IndexValueType = typename TInputImage::IndexValueType;
    using PointValueType = typename TInputImage::PointValueType;

    using ThreadIdType  = itk::ThreadIdType;

    itkNewMacro(Self)
    itkTypeMacro(nm::StreamingROIImageFilter, itk::ImageToImageFilter)

    itkSetMacro(RegionOfInterest, RegionType)
    itkGetConstMacro(RegionOfInterest, RegionType)

    itkSetMacro(Origin, PointType)
    itkGetConstMacro(Origin, PointType)

    itkSetMacro(Length, PointType)
    itkGetConstMacro(Length, PointType)

    /*! array of image region offsets pointing at the
     *  start of the region to be extracted
     */
    void SetROIIndex(std::vector<IndexValueType> idx);

    /*! array with ROI size (number of pixels)
     *  for each image dimension
     */
    void SetROISize(std::vector<IndexValueType> size);

    /*! array with ROI origin coordinates (pixel centre!)
     *  for each image dimension
     */
    void SetROIOrigin(std::vector<PointValueType> origin);

    /*! array of ROI (real world) length (in coordinate unints)
     *  for each image dimension
     */
    void SetROILength(std::vector<PointValueType> length);


protected:
    StreamingROIImageFilter();
    virtual ~StreamingROIImageFilter() override {}

    virtual void GenerateOutputInformation() override;
    virtual void GenerateInputRequestedRegion() override;
    virtual void ThreadedGenerateData(const RegionType & outputRegionForThread,
                                      ThreadIdType threadId) override;

    RegionType m_RegionOfInterest;
    PointType m_Origin;
    PointType m_Length;

private:
    ITK_DISALLOW_COPY_AND_ASSIGN(StreamingROIImageFilter);


};  // end of class
}   // end of namespace

#ifndef ITK_MANUAL_INSTANTIATION
#include "nmStreamingROIImageFilter.txx"
#endif

#endif // include guard
