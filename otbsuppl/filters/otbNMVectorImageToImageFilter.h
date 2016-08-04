/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
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

#ifndef OTBNMVECTORIMAGETOIMAGEFILTER_H
#define OTBNMVECTORIMAGETOIMAGEFILTER_H

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "otbImage.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*  \brief Extracts a single band from a multi-band (multi-component) image
 */
template < class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT NMVectorImageToImageFilter :
        public itk::ImageToImageFilter< TInputImage, TOutputImage>
{
public:

    itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
    itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

    typedef TInputImage InputImageType;
    typedef TOutputImage OutputImageType;

    typedef itk::ImageToImageFilter< TInputImage, TOutputImage> Superclass;
    typedef NMVectorImageToImageFilter      Self;
    typedef itk::SmartPointer<Self>         Pointer;
    typedef itk::SmartPointer<const Self>   ConstPointer;

    itkNewMacro(Self)
    itkTypeMacro(NMVectorImageToImageFilter, itk::ImageToImageFilter)

    typedef typename InputImageType::PixelType  InputPixelType;
    typedef typename InputImageType::RegionType InputImageRegionType;
    typedef typename InputImageType::SizeType    InputImageSizeType;

    typedef typename OutputImageType::PixelType OutputPixelType;
    typedef typename OutputImageType::RegionType OutputImageRegionType;
    typedef typename OutputImageType::SizeType   OutputImageSizeType;

    /*! 1-based image band index */
    itkSetMacro(Band, unsigned int)


protected:
    NMVectorImageToImageFilter();
    virtual ~NMVectorImageToImageFilter() {}

    void BeforeThreadedGenerateData();
    void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId );

    unsigned int m_Band;

private:
    NMVectorImageToImageFilter(const Self&); //purposley not implemented
    void operator=(const Self&); //purposely not implemented
};

} // namespace otb


#ifndef ITK_MANUAL_INSTANTIATION
#include "otbNMVectorImageToImageFilter.txx"
#endif

#endif // OTBNMVECTORIMAGETOIMAGEFILTER_H

