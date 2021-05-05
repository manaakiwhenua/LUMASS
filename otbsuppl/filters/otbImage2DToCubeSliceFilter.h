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
 * otbImage2DToCubeSliceFilter.h
 *
 * Created on: 27/05/2012
 *     Author: alex
 *
 */

#ifndef IMAGE2DTOCUBESLICEFILTER_H
#define IMAGE2DTOCUBESLICEFILTER_H

#include <string>
#include <iostream>
#include "itkImageToImageFilter.h"
#include "nmlog.h"
#define ctxImage2DToCubeSliceFilter "Image2DToCubeSliceFilter"

#include "otbsupplfilters_export.h"

namespace otb {

template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT Image2DToCubeSliceFilter
	: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  using Self                =  Image2DToCubeSliceFilter;
  using Superclass          =  itk::ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer             =  itk::SmartPointer<Self>;
  using ConstPointer        =  itk::SmartPointer<const Self>;

  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

  itkNewMacro(Self)
  itkTypeMacro(Image2DToCubeSliceFilter, itk::ImageToImageFilter)

  using InputImageType      = TInputImage;
  using InputImagePointer   = typename InputImageType::Pointer;
  using InputRegionType     = typename InputImageType::RegionType;
  using InputImagePixelType = typename InputImageType::PixelType;
  using InputsizeType       = typename InputImageType::SizeType;
  using InputIndexType      = typename InputImageType::IndexType;
  using InputSpacingType    = typename InputImageType::SpacingType;
  using InputPointType      = typename InputImageType::PointType;

  using OutputImageType      = TOutputImage;
  using OutputImagePointer   = typename OutputImageType::Pointer;
  using OutputRegionType     = typename OutputImageType::RegionType;
  using OutputImagePixelType = typename OutputImageType::PixelType;
  using OutputsizeType       = typename OutputImageType::SizeType;
  using OutputIndexType      = typename OutputImageType::IndexType;
  using OutputSpacingType    = typename OutputImageType::SpacingType;
  using OutputPointType      = typename OutputImageType::PointType;
  using OutputDirectionType  = typename OutputImageType::DirectionType;


  void SetDimMapping(std::vector<int>& vec) {m_DimMapping = vec;}
  std::vector<int> GetDimMapping(std::vector<int>& vec) {return m_DimMapping;}

  void SetOutputOrigin(std::vector<double>& vec) {m_OutputOrigin = vec;}
  std::vector<double> GetOutputOrigin(std::vector<double>& vec) {return m_OutputOrigin;}

  void SetOutputSpacing(std::vector<double>& vec) {m_OutputSpacing = vec;}
  std::vector<double> GetOutputSpacing(std::vector<double>& vec) {return m_OutputSpacing;}

  void SetOutputSize(std::vector<long long>& vec) {m_OutputSize = vec;}
  std::vector<long long> GetOutputSize(std::vector<long long>& vec) {return m_OutputSize;}

  void SetOutputIndex(std::vector<long long>& vec) {m_OutputIndex = vec;}
  std::vector<long long> GetOutputIndex(std::vector<long long>& vec) {return m_OutputIndex;}

  void GenerateOutputInformation(void);
  void GenerateInputRequestedRegion(void);


protected:
  Image2DToCubeSliceFilter();
  ~Image2DToCubeSliceFilter();

  void
  ThreadedGenerateData(
          const OutputRegionType& outputRegionForThread,
          itk::ThreadIdType threadId );


  std::vector<int> m_DimMapping;

  std::vector<long long> m_OutputSize;
  std::vector<long long> m_OutputIndex;
  std::vector<double> m_OutputSpacing;
  std::vector<double> m_OutputOrigin;



}; // end of class definition

} // end of namespace otb

//#include "otbImage2DToCubeSliceFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbImage2DToCubeSliceFilter.txx"
#endif

#endif // ifndef header

