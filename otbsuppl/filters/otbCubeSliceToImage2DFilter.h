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
 * otbCubeSliceToImage2DFilter.h
 *
 * Created on: 2020-12-07
 *     Author: alex
 *
 */

 /*! NOTE:
  *  This Filter extracts a 2D slice from an
  *  image (hyper) cube. The output image will
  *  be a 2D (x,y) imge; the output image type
  *  need to be setup to be a 2D image!
  */


#ifndef CubeSliceToImage2DFilter_H
#define CubeSliceToImage2DFilter_H

#include <string>
#include <iostream>
#include "itkImageToImageFilter.h"
#include "nmlog.h"
#define ctxCubeSliceToImage2DFilter "CubeSliceToImage2DFilter"

#include "nmotbsupplfilters_export.h"

namespace otb {

template <class TInputImage, class TOutputImage>
class NMOTBSUPPLFILTERS_EXPORT CubeSliceToImage2DFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  using Self                =  CubeSliceToImage2DFilter;
  using Superclass          =  itk::ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer             =  itk::SmartPointer<Self>;
  using ConstPointer        =  itk::SmartPointer<const Self>;

  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

  itkNewMacro(Self)
  itkTypeMacro(CubeSliceToImage2DFilter, itk::ImageToImageFilter)

  using InputImageType      = TInputImage;
  using InputImagePointer   = typename InputImageType::Pointer;
  using InputRegionType     = typename InputImageType::RegionType;
  using InputImagePixelType = typename InputImageType::PixelType;
  using InputsizeType       = typename InputImageType::SizeType;
  using InputIndexType      = typename InputImageType::IndexType;
  using InputSpacingType    = typename InputImageType::SpacingType;
  using InputPointType      = typename InputImageType::PointType;
  using InputDirectionType  = typename InputImageType::DirectionType;

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

  void SetInputOrigin(std::vector<double>& vec) {m_InputOrigin = vec;}
  std::vector<double> GetInputOrigin(std::vector<double>& vec) {return m_InputOrigin;}

  void SetInputSize(std::vector<long long>& vec) {m_InputSize = vec;}
  std::vector<long long> GetInputSize(std::vector<long long>& vec) {return m_InputSize;}

  void SetInputIndex(std::vector<long long>& vec) {m_InputIndex = vec;}
  std::vector<long long> GetInputIndex(std::vector<long long>& vec) {return m_InputIndex;}

  void GenerateOutputInformation(void);
  void GenerateInputRequestedRegion(void);


protected:
  CubeSliceToImage2DFilter();
  ~CubeSliceToImage2DFilter();

  void
  ThreadedGenerateData(
          const OutputRegionType& outputRegionForThread,
          itk::ThreadIdType threadId );


  std::vector<int> m_DimMapping;

  std::vector<long long> m_InputSize;
  std::vector<long long> m_InputIndex;
  std::vector<double> m_InputOrigin;

  int m_CollapsedDimIndex;
  std::vector<int> m_In2OutIdxMap;



}; // end of class definition

} // end of namespace otb

//#include "otbCubeSliceToImage2DFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbCubeSliceToImage2DFilter.txx"
#endif

#endif // ifndef header

