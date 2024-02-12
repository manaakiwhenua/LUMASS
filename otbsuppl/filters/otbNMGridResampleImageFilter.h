/******************************************************************************
* Adapted by Alexander Herzig from otb::GridResampleImageFilter (copyright &
* licence s. below)
*
* Copyright 2018 Landcare Research New Zealand Ltd
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
 * Copyright (C) 2005-2017 Centre National d'Etudes Spatiales (CNES)
 *
 * This file is part of Orfeo Toolbox
 *
 *     https://www.orfeo-toolbox.org/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef otbNMGridResampleImageFilter_h
#define otbNMGridResampleImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkMeanImageFunction.h"
#include "itkMedianImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkDefaultConvertPixelTraits.h"

namespace otb
{


template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision = double>
class ITK_EXPORT NMGridResampleImageFilter :
    public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NMGridResampleImageFilter                                         Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>              Superclass;
  typedef itk::SmartPointer<Self>                                         Pointer;
  typedef itk::SmartPointer<const Self>                                   ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NMGridResampleImageFilter, itk::ImageToImageFilter);

  /** Number of dimensions. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Typedef parameters*/
  typedef TInputImage                                                     InputImageType;
  typedef typename InputImageType::RegionType                             InputImageRegionType;
  typedef TOutputImage                                                    OutputImageType;
  typedef typename OutputImageType::RegionType                            OutputImageRegionType;
  typedef typename TOutputImage::PixelType                                OutputPixelType;

  typedef typename InputImageRegionType::IndexValueArrayType                   InputIndexArrayType;



  typedef itk::DefaultConvertPixelTraits<OutputPixelType>                 OutputPixelConvertType;
  typedef typename OutputPixelConvertType::ComponentType                  OutputPixelComponentType;

  /** Interpolator type */
  typedef itk::InterpolateImageFunction<InputImageType,
                                        TInterpolatorPrecision>           InterpolatorType;

  using MeanImgFuncType = itk::MeanImageFunction<InputImageType,
                                    TInterpolatorPrecision> ;
  using MedianImgFuncType = itk::MedianImageFunction<InputImageType,
                                    TInterpolatorPrecision>;
  using MeanImgFuncTypePointer = typename MeanImgFuncType::Pointer;
  using MedianImgFuncTypePointer = typename MedianImgFuncType::Pointer;


  typedef typename InterpolatorType::Pointer                              InterpolatorPointerType;
  typedef typename InterpolatorType::OutputType                           InterpolatorOutputType;
  typedef itk::DefaultConvertPixelTraits< InterpolatorOutputType >        InterpolatorConvertType;
  typedef typename InterpolatorConvertType::ComponentType                 InterpolatorComponentType;

  /** Input pixel continuous index typdef */
  typedef typename itk::ContinuousIndex<TInterpolatorPrecision,InputImageDimension >      ContinuousInputIndexType;

  /** ImageBase typedef */
  typedef itk::ImageBase<OutputImageType::ImageDimension>                 ImageBaseType;
  typedef typename ImageBaseType::SpacingType                             SpacingType;
  typedef typename ImageBaseType::SizeType                                SizeType;
  typedef typename SizeType::SizeValueType                                SizeValueType;
  typedef typename ImageBaseType::PointType                               PointType;
  typedef typename ImageBaseType::IndexType                               IndexType;

  itkSetMacro(OutputStartIndex,IndexType);
  itkGetConstReferenceMacro(OutputStartIndex,IndexType);

  void SetSize(SizeType sz);
  void SetSize(SizeValueType* vsz);

  itkSetMacro(OutputSize,SizeType);
  itkGetConstReferenceMacro(OutputSize,SizeType);

  itkSetMacro(OutputOrigin,PointType);
  itkGetConstReferenceMacro(OutputOrigin,PointType);

  itkSetMacro(OutputSpacing,SpacingType);
  itkGetConstReferenceMacro(OutputSpacing,SpacingType);

  //itkSetMacro(EdgePaddingValue,OutputPixelType);
  void SetEdgePaddingValue(OutputPixelType padVal);
  itkGetConstReferenceMacro(EdgePaddingValue,OutputPixelType);

  //itkSetMacro( DefaultPixelValue, OutputPixelType );
  void SetDefaultPixelValue(OutputPixelType defVal);
  itkGetConstReferenceMacro( DefaultPixelValue, OutputPixelType );

  itkSetMacro(CheckOutputBounds,bool);
  itkGetMacro(CheckOutputBounds,bool);
  itkBooleanMacro(CheckOutputBounds);

  itkSetObjectMacro(Interpolator,InterpolatorType);
  itkGetObjectMacro(Interpolator,InterpolatorType);

  /** We add a new parameter InterpolationMethod
   *  representing the InterpolatorType as string
   *  to facilitate interpolator setting without
   *  having to deal with templates. Possible settings
   *  are:
   *
   *  - 'NearestNeighbour'
   *  - 'Linear'
   *  - 'BSpline0
   *  - 'BSpline1'
   *  - 'BSpline2'
   *  - 'BSpline3'
   *  - 'BSpline4'
   *  - 'BSpline5'
   *  - 'Mean'
   *  - 'Median'
   */
  itkSetMacro( InterpolationMethod, std::string& );
  itkGetMacro( InterpolationMethod, std::string );

  /** Import output parameters from a given image */
  void SetOutputParametersFromImage(const ImageBaseType * image);

  /** Method Compute the Modified Time based on changed to the components. */
  itk::ModifiedTimeType GetMTime(void) const ITK_OVERRIDE;

protected:
  NMGridResampleImageFilter();

  /** Destructor */
  ~NMGridResampleImageFilter() ITK_OVERRIDE {}

  void GenerateOutputInformation() ITK_OVERRIDE;

  void GenerateInputRequestedRegion() ITK_OVERRIDE;

  void BeforeThreadedGenerateData() ITK_OVERRIDE;

  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId) ITK_OVERRIDE;

  void AfterThreadedGenerateData() ITK_OVERRIDE;

  void SetInterpolatorFromMethodString();

  inline void CastPixelWithBoundsChecking( const InterpolatorOutputType& value,
                                                      const InterpolatorComponentType& minComponent,
                                                      const InterpolatorComponentType& maxComponent,
                                                      OutputPixelType& outputValue ) const
  {
    // Method imported from itk::ResampleImageFilter
    const unsigned int nComponents = InterpolatorConvertType::GetNumberOfComponents(value);

    itk::NumericTraits<OutputPixelType>::SetLength( outputValue, nComponents );

    for (unsigned int n=0; n<nComponents; n++)
      {
      InterpolatorComponentType component = InterpolatorConvertType::GetNthComponent( n, value );

      if ( m_CheckOutputBounds && component < minComponent )
        {
        OutputPixelConvertType::SetNthComponent( n, outputValue, static_cast<OutputPixelComponentType>( minComponent ) );
        }
      else if ( m_CheckOutputBounds && component > maxComponent )
        {
        OutputPixelConvertType::SetNthComponent( n, outputValue, static_cast<OutputPixelComponentType>( maxComponent ) );
        }
      else
        {
        OutputPixelConvertType::SetNthComponent(n, outputValue,
                                                static_cast<OutputPixelComponentType>( component ) );
        }
      }
  }


  void PrintSelf(std::ostream& os, itk::Indent indent) const ITK_OVERRIDE;

private:
  NMGridResampleImageFilter(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  IndexType              m_OutputStartIndex;     // output image start index
  SizeType               m_OutputSize;            // output image size
  PointType              m_OutputOrigin;         // output image origin
  SpacingType            m_OutputSpacing;        // output image spacing

  OutputPixelType        m_EdgePaddingValue;     // Default pixel value
  OutputPixelType        m_DefaultPixelValue;

  bool                   m_CheckOutputBounds;    // Shall we check
                                                 // output bounds when
                                                 // casting?
  std::string             m_InterpolationMethod; // string specifying the
                                                 // interpolation method

  InterpolatorPointerType m_Interpolator;        // Interpolator used
                                                 // for resampling

  MedianImgFuncTypePointer m_MedianOperator;
  MeanImgFuncTypePointer m_MeanOperator;

  OutputImageRegionType   m_ReachableOutputRegion; // Internal
                                                   // variable for
                                                   // speed-up. Computed
                                                   // in BeforeThreadedGenerateData

};

} // namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbNMGridResampleImageFilter.txx"
#endif

#endif
