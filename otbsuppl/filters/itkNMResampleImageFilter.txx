/******************************************************************************
* Adapted by Alexander Herzig
* Copyright 2014 Landcare Research New Zealand Ltd
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
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkResampleImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2009-03-25 21:10:39 $
  Version:   $Revision: 1.66 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkNMResampleImageFilter_txx
#define __itkNMResampleImageFilter_txx

// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.
//#include "itkConfigure.h"

//#ifdef ITK_USE_OPTIMIZED_REGISTRATION_METHODS
//#include "itkOptResampleImageFilter.txx"
//#else

#include "itkNMResampleImageFilter.h"
#include "itkObjectFactory.h"
#include "itkIdentityTransform.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkProgressReporter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageLinearIteratorWithIndex.h"
#include "itkSpecialCoordinatesImage.h"

namespace itk
{

/**
 * Initialize new instance
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
NMResampleImageFilter<TInputImage, TOutputImage,TInterpolatorPrecisionType>
::NMResampleImageFilter()
{
  m_OutputSpacing.Fill(1.0);
  m_OutputOrigin.Fill(0.0);
  m_OutputDirection.SetIdentity();

  m_UseReferenceImage = false;

  m_Size.Fill( 0 );
  m_OutputStartIndex.Fill( 0 );
  
  typedef double CoordRepType;
  m_Transform = IdentityTransform<CoordRepType, ImageDimension>::New();
  m_InterpolationMethod.clear();
  m_Interpolator = LinearInterpolateImageFunction<InputImageType, CoordRepType>::New();
  m_DefaultPixelValue = 0;

  m_UserOrigin = false;
  m_UserSize = false;
  m_UserSpacing = false;
}


/**
 * Print out a description of self
 *
 * \todo Add details about this class
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage, TOutputImage,TInterpolatorPrecisionType>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  
  os << indent << "DefaultPixelValue: "
     << static_cast<typename NumericTraits<PixelType>::PrintType>(m_DefaultPixelValue)
     << std::endl;
  os << indent << "Size: " << m_Size << std::endl;
  os << indent << "OutputStartIndex: " << m_OutputStartIndex << std::endl;
  os << indent << "OutputOrigin: " << m_OutputOrigin << std::endl;
  os << indent << "OutputSpacing: " << m_OutputSpacing << std::endl;
  os << indent << "OutputDirection: " << m_OutputDirection << std::endl;
  os << indent << "Transform: " << m_Transform.GetPointer() << std::endl;
  os << indent << "Interpolator: " << m_Interpolator.GetPointer() << std::endl;
  os << indent << "UseReferenceImage: " << (m_UseReferenceImage ? "On" : "Off") << std::endl;
  return;
}

/**
 * Set the output image spacing.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetOutputSpacing(
  const double* spacing)
{
  SpacingType s(spacing);
  this->SetOutputSpacing( s );
  m_UserSpacing = true;
}


/**
 * Set the output image origin.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetOutputOrigin(
  const double* origin)
{
  OriginPointType p(origin);
  this->SetOutputOrigin( p );
  m_UserOrigin = true;
}

/**
 * Set the output image size.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetSize(
  const unsigned long* size)
{
  SizeType s;
  s.SetSize(size);
  this->SetSize( s );
  m_UserSize = true;
}

/**
 * Set up state of filter before multi-threading.
 * InterpolatorType::SetInputImage is not thread-safe and hence
 * has to be set up before ThreadedGenerateData
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::BeforeThreadedGenerateData()
{
  if( !m_Transform )
    {
    itkExceptionMacro(<< "Transform not set");
    }

  if( !m_Interpolator )
    {
    itkExceptionMacro(<< "Interpolator not set");
    }

  typedef typename itk::BSplineInterpolateImageFunction<InputImageType, CoordRepType> SplineType;
  typedef typename itk::NearestNeighborInterpolateImageFunction<InputImageType, CoordRepType> NearestType;
  typedef typename itk::LinearInterpolateImageFunction<InputImageType, CoordRepType> LinearType;

  // account for InterpolationMethod
  if (!m_InterpolationMethod.empty())
  {
      std::string methods[] = {"NearestNeighbour", "Linear", "BSpline0",
          "BSpline1", "BSpline2", "BSpline3", "BSpline4", "BSpline5"};
      int method = -1;
      for (int k=0; k < 8; ++k)
      {
          if (methods[k] == m_InterpolationMethod)
          {
              method = k;
              break;
          }
      }

      switch (method)
      {
      case 0: //"NearestNeighbour":
          m_Interpolator = NearestType::New();
          break;
      case 1: //"Linear":
          m_Interpolator = LinearType::New();
          break;
      case 2: //"BSpline0":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(0);
              m_Interpolator = spline;
          }
          break;
      case 3: //"BSpline1":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(1);
              m_Interpolator = spline;
          }
          break;
      case 4: //"BSpline2":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(2);
              m_Interpolator = spline;
          }
          break;
      case 5: //"BSpline3":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(3);
              m_Interpolator = spline;
          }
          break;
      case 6: //"BSpline4":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(4);
              m_Interpolator = spline;
          }
          break;
      case 7: //"BSpline5":
          {
              typename SplineType::Pointer spline = SplineType::New();
              spline->SetSplineOrder(5);
              m_Interpolator = spline;
          }
          break;
      default:
          break;
      }
  }

  // Connect input image to interpolator
  m_Interpolator->SetInputImage( this->GetInput() );
}

/**
 * Set up state of filter after multi-threading.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::AfterThreadedGenerateData()
{
  // Disconnect input image from the interpolator
  m_Interpolator->SetInputImage( NULL );
}

/**
 * ThreadedGenerateData
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::ThreadedGenerateData(
  const OutputImageRegionType& outputRegionForThread,
  ThreadIdType threadId)
{
  // Check whether the input or the output is a
  // SpecialCoordinatesImage.  If either are, then we cannot use the
  // fast path since index mapping will definately not be linear.
  typedef SpecialCoordinatesImage<PixelType, ImageDimension> OutputSpecialCoordinatesImageType;
  typedef SpecialCoordinatesImage<InputPixelType, InputImageDimension> InputSpecialCoordinatesImageType;

  // Our friend the SGI needs these declarations to avoid unresolved
  // linker errors.
#ifdef __sgi
  InputSpecialCoordinatesImageType::Pointer foo =
    InputSpecialCoordinatesImageType::New();
  OutputSpecialCoordinatesImageType::Pointer bar =
    OutputSpecialCoordinatesImageType::New();
#endif
  if (dynamic_cast<const InputSpecialCoordinatesImageType *>(this->GetInput())
      || dynamic_cast<const OutputSpecialCoordinatesImageType *>(this->GetOutput()))
    {
    this->NonlinearThreadedGenerateData(outputRegionForThread, threadId);
    return;
    }
  
  // Check whether we can use a fast path for resampling. Fast path
  // can be used if the transformation is linear. Transform respond
  // to the IsLinear() call.
  if( m_Transform->IsLinear() )
    {
    this->LinearThreadedGenerateData(outputRegionForThread, threadId);
    return;
    }

  // Otherwise, we use the normal method where the transform is called
  // for computing the transformation of every point.
  this->NonlinearThreadedGenerateData(outputRegionForThread, threadId);
  
}


template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::NonlinearThreadedGenerateData(
  const OutputImageRegionType& outputRegionForThread,
  ThreadIdType threadId)
{
  // Get the output pointers
  OutputImagePointer      outputPtr = this->GetOutput();

  // Get ths input pointers
  InputImageConstPointer inputPtr=this->GetInput();

  // Create an iterator that will walk the output region for this thread.
  typedef ImageRegionIteratorWithIndex<TOutputImage> OutputIterator;

  OutputIterator outIt(outputPtr, outputRegionForThread);

  // Define a few indices that will be used to translate from an input pixel
  // to an output pixel
  PointType outputPoint;         // Coordinates of current output pixel
  PointType inputPoint;          // Coordinates of current input pixel

  typedef double CoordRepType;
  typedef ContinuousIndex<CoordRepType, ImageDimension> ContinuousIndexType;
  ContinuousIndexType inputIndex;

  // Support for progress methods/callbacks
  ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
        
  typedef typename InterpolatorType::OutputType OutputType;

  // Min/max values of the output pixel type AND these values
  // represented as the output type of the interpolator
  const PixelType minValue =  NumericTraits<PixelType >::NonpositiveMin();
  const PixelType maxValue =  NumericTraits<PixelType >::max();

  const OutputType minOutputValue = static_cast<OutputType>(minValue);
  const OutputType maxOutputValue = static_cast<OutputType>(maxValue);
  
  // Walk the output region
  outIt.GoToBegin();


  // This fix works for images up to approximately 2^25 pixels in
  // any dimension.  If the image is larger than this, this constant
  // needs to be made lower.
  double precisionConstant = 1<<(NumericTraits<double>::digits>>1);

  while ( !outIt.IsAtEnd() )
    {
    // Determine the index of the current output pixel
    outputPtr->TransformIndexToPhysicalPoint( outIt.GetIndex(), outputPoint );

    // Compute corresponding input pixel position
    inputPoint = m_Transform->TransformPoint(outputPoint);
    inputPtr->TransformPhysicalPointToContinuousIndex(inputPoint, inputIndex);

    // The inputIndex is precise to many decimal points, but this precision
    // involves some error in the last bits.  
    // Sometimes, when an index should be inside of the image, the
    // index will be slightly
    // greater than the largest index in the image, like 255.00000000002
    // for a image of size 256.  This can cause an empty row to show up
    // at the bottom of the image.
    // Therefore, the following routine uses a
    // precisionConstant that specifies the number of relevant bits,
    // and the value is truncated to this precision.
    for (int i=0; i < ImageDimension; ++i)
      {
      double roundedInputIndex = vcl_floor(inputIndex[i]);
      double inputIndexFrac = inputIndex[i] - roundedInputIndex;
      double newInputIndexFrac = vcl_floor(precisionConstant * inputIndexFrac)/precisionConstant;
      inputIndex[i] = roundedInputIndex + newInputIndexFrac;
      }
    
    // Evaluate input at right position and copy to the output
    if( m_Interpolator->IsInsideBuffer(inputIndex) )
      {
      PixelType pixval;
      const OutputType value
        = m_Interpolator->EvaluateAtContinuousIndex(inputIndex);

      if( value < minOutputValue )
        {
        pixval = minValue;
        }
      else if( value > maxOutputValue )
        {
        pixval = maxValue;
        }
      else
        {
        pixval = static_cast<PixelType>( value );
        }

      outIt.Set( pixval );
      }
    else
      {
      outIt.Set(m_DefaultPixelValue); // default background value
      }

    progress.CompletedPixel();
    ++outIt;
    }

  return;
}

template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::LinearThreadedGenerateData(
  const OutputImageRegionType& outputRegionForThread,
  ThreadIdType threadId)
{
  // Get the output pointers
  OutputImagePointer      outputPtr = this->GetOutput();

  // Get ths input pointers
  InputImageConstPointer inputPtr=this->GetInput();

  // Create an iterator that will walk the output region for this thread.
  typedef ImageLinearIteratorWithIndex<TOutputImage> OutputIterator;

  OutputIterator outIt(outputPtr, outputRegionForThread);
  outIt.SetDirection( 0 );

  // Define a few indices that will be used to translate from an input pixel
  // to an output pixel
  PointType outputPoint;         // Coordinates of current output pixel
  PointType inputPoint;          // Coordinates of current input pixel
  PointType tmpOutputPoint;
  PointType tmpInputPoint;

  typedef double CoordRepType;
  typedef ContinuousIndex<CoordRepType, ImageDimension> ContinuousIndexType;
  ContinuousIndexType inputIndex;
  ContinuousIndexType tmpInputIndex;
  
  typedef typename PointType::VectorType VectorType;
  VectorType delta;          // delta in input continuous index coordinate frame
  IndexType index;

  // Support for progress methods/callbacks
  ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
        
  typedef typename InterpolatorType::OutputType OutputType;

  // Cache information from the superclass
  PixelType defaultValue = this->GetDefaultPixelValue();

  // Min/max values of the output pixel type AND these values
  // represented as the output type of the interpolator
  const PixelType minValue =  NumericTraits<PixelType >::NonpositiveMin();
  const PixelType maxValue =  NumericTraits<PixelType >::max();

  const OutputType minOutputValue = static_cast<OutputType>(minValue);
  const OutputType maxOutputValue = static_cast<OutputType>(maxValue);
  
  
  // Determine the position of the first pixel in the scanline
  index = outIt.GetIndex();
  outputPtr->TransformIndexToPhysicalPoint( index, outputPoint );
  

  // Compute corresponding input pixel position
  inputPoint = m_Transform->TransformPoint(outputPoint);
  inputPtr->TransformPhysicalPointToContinuousIndex(inputPoint, inputIndex);
  
  // As we walk across a scan line in the output image, we trace
  // an oriented/scaled/translated line in the input image.  Cache
  // the delta along this line in continuous index space of the input
  // image. This allows us to use vector addition to model the
  // transformation.
  //
  // To find delta, we take two pixels adjacent in a scanline 
  // and determine the continuous indices of these pixels when 
  // mapped to the input coordinate frame. We use the difference
  // between these two continuous indices as the delta to apply
  // to an index to trace line in the input image as we move 
  // across the scanline of the output image.
  //
  // We determine delta in this manner so that Images and
  // OrientedImages are both handled properly (with the delta for
  // OrientedImages taking into account the direction cosines).
  // 
  ++index[0];
  outputPtr->TransformIndexToPhysicalPoint( index, tmpOutputPoint );
  tmpInputPoint = m_Transform->TransformPoint( tmpOutputPoint );
  inputPtr->TransformPhysicalPointToContinuousIndex(tmpInputPoint,
                                                    tmpInputIndex);
  delta = tmpInputIndex - inputIndex;


  // This fix works for images up to approximately 2^25 pixels in
  // any dimension.  If the image is larger than this, this constant
  // needs to be made lower.
  double precisionConstant = 1<<(NumericTraits<double>::digits>>1);

  // Delta is precise to many decimal points, but this precision
  // involves some error in the last bits.  This error can accumulate
  // as the delta values are added.
  // Sometimes, when the accumulated delta should be inside of the
  // image, it will be slightly
  // greater than the largest index in the image, like 255.00000000002
  // for a image of size 256.  This can cause an empty column to show up
  // at the right side of the image. If we instead
  // truncate this delta value to some precision, this solves the problem.
  // Therefore, the following routine uses a
  // precisionConstant that specifies the number of relevant bits,
  // and the value is truncated to this precision.
  for (int i=0; i < ImageDimension; ++i)
    {
    double roundedDelta = vcl_floor(delta[i]);
    double deltaFrac = delta[i] - roundedDelta;
    double newDeltaFrac = vcl_floor(precisionConstant * deltaFrac)/precisionConstant;
    delta[i] = roundedDelta + newDeltaFrac;
    }


  while ( !outIt.IsAtEnd() )
    {
    // Determine the continuous index of the first pixel of output
    // scanline when mapped to the input coordinate frame.
    //

    // First get the position of the pixel in the output coordinate frame
    index = outIt.GetIndex();
    outputPtr->TransformIndexToPhysicalPoint( index, outputPoint );

    // Compute corresponding input pixel continuous index, this index
    // will incremented in the scanline loop
    inputPoint = m_Transform->TransformPoint(outputPoint);
    inputPtr->TransformPhysicalPointToContinuousIndex(inputPoint, inputIndex);
    
    // The inputIndex is precise to many decimal points, but this precision
    // involves some error in the last bits.
    // Sometimes, when an index should be inside of the image, the
    // index will be slightly
    // greater than the largest index in the image, like 255.00000000002
    // for a image of size 256.  This can cause an empty row to show up
    // at the bottom of the image.
    // Therefore, the following routine uses a
    // precisionConstant that specifies the number of relevant bits,
    // and the value is truncated to this precision.
    for (int i=0; i < ImageDimension; ++i)
      {
      double roundedInputIndex = vcl_floor(inputIndex[i]);
      double inputIndexFrac = inputIndex[i] - roundedInputIndex;
      double newInputIndexFrac = vcl_floor(precisionConstant * inputIndexFrac)/precisionConstant;
      inputIndex[i] = roundedInputIndex + newInputIndexFrac;
      }


    while( !outIt.IsAtEndOfLine() )
      {
      // Evaluate input at right position and copy to the output
      if( m_Interpolator->IsInsideBuffer(inputIndex) )
        {
        PixelType pixval;
        const OutputType value
          = m_Interpolator->EvaluateAtContinuousIndex(inputIndex);

        if( value < minOutputValue )
          {
          pixval = minValue;
          }
        else if( value > maxOutputValue )
          {
          pixval = maxValue;
          }
        else
          {
          pixval = static_cast<PixelType>( value );
          }

        outIt.Set( pixval );
        }
      else
        {
        outIt.Set(defaultValue); // default background value
        }
      
      progress.CompletedPixel();
      ++outIt;
      inputIndex += delta;
      }
    outIt.NextLine();
    }

  return;
}


/** 
 * Inform pipeline of necessary input image region
 *
 * Determining the actual input region is non-trivial, especially
 * when we cannot assume anything about the transform being used.
 * So we do the easy thing and request the entire input image.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::GenerateInputRequestedRegion()
{
  // call the superclass's implementation of this method
  Superclass::GenerateInputRequestedRegion();

  if ( !this->GetInput() )
    {
    return;
    }

  // get pointers to the input and output
  InputImagePointer  inputPtr  = 
    const_cast< TInputImage *>( this->GetInput() );

  // Request the entire input image
  InputImageRegionType inputRegion;
  inputRegion = inputPtr->GetLargestPossibleRegion();
  inputPtr->SetRequestedRegion(inputRegion);

  return;
}


/** 
 * Set the smart pointer to the reference image that will provide
 * the grid parameters for the output image.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
const typename NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>::OutputImageType *
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::GetReferenceImage() const
{
  Self * surrogate = const_cast< Self * >( this );
  const OutputImageType * referenceImage = 
    static_cast<const OutputImageType *>(surrogate->ProcessObject::GetInput(1));
  return referenceImage;
}


/** 
 * Set the smart pointer to the reference image that will provide
 * the grid parameters for the output image.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetReferenceImage( const TOutputImage *image )
{
  itkDebugMacro("setting input ReferenceImage to " << image);
  if( image != static_cast<const TOutputImage *>(this->ProcessObject::GetInput( 1 )) )
    {
    this->ProcessObject::SetNthInput(1, const_cast< TOutputImage *>( image ) );
    this->Modified();
    m_UseReferenceImage = true;
    }
}

/** Helper method to set the output parameters based on this image */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetOutputParametersFromImage ( const ImageBaseType * image )
{
  this->SetOutputOrigin ( image->GetOrigin() );
  this->SetOutputSpacing ( image->GetSpacing() );
  this->SetOutputDirection ( image->GetDirection() );
  this->SetOutputStartIndex ( image->GetLargestPossibleRegion().GetIndex() );
  this->SetSize ( image->GetLargestPossibleRegion().GetSize() );
}

#if !defined(ITK_LEGACY_REMOVE)
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::SetOutputParametersFromConstImage ( const ImageBaseType * image )
{
  itkGenericLegacyReplaceBodyMacro(itk::NMResampleImageFilter::SetOutputParametersFromConstImage,
                                   3.14, 
                                   itk::NMResampleImageFilter::SetOutputParametersFromImage);
  this->SetOutputParametersFromImage(image);
}
#endif

/** 
 * Inform pipeline of required output region
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
void 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::GenerateOutputInformation()
{
  // call the superclass' implementation of this method
  Superclass::GenerateOutputInformation();

  // get pointers to the input and output
  OutputImagePointer outputPtr = this->GetOutput();
  if ( !outputPtr )
    {
    return;
    }

  const OutputImageType * referenceImage = this->GetReferenceImage();

  // Set the size of the output region
  if( m_UseReferenceImage && referenceImage )
    {
    outputPtr->SetLargestPossibleRegion( referenceImage->GetLargestPossibleRegion() );
    outputPtr->SetOrigin( referenceImage->GetOrigin() );
    outputPtr->SetSpacing( referenceImage->GetSpacing() );
    outputPtr->SetDirection( referenceImage->GetDirection() );
    }
  // Calculate input parameters as needed, or copy from input image
  else
    {   // here, we use user settings as much as possible, however, if we
        // haven't got user settings, we copy from the input image

        InputImageConstPointer inImg = this->GetInput(0);
        SizeType        inSize = inImg->GetLargestPossibleRegion().GetSize();
        OriginPointType inOrigin = inImg->GetOrigin();
        SpacingType     inSpacing = inImg->GetSpacing();

        double ddim[ImageDimension];
        for (int d=0; d < ImageDimension; ++d)
            ddim[d] = (inOrigin[d] + (inSize[d] * inSpacing[d])) - inOrigin[d];

        if (!m_UserSize && m_UserSpacing)
        {
            for (int d=0; d < ImageDimension; ++d)
            {
                m_Size[d] = (ddim[d] / (double)m_OutputSpacing[d]) + 0.5;
            }
            m_UserSize = true;
        }
        else if (m_UserSize && !m_UserSpacing)
        {
            for (int d=0; d < ImageDimension; ++d)
            {
                m_OutputSpacing[d] = (ddim[d]) / (double)m_Size[d];
            }
            m_UserSpacing = true;
        }

        if (!m_UserOrigin)
            m_OutputOrigin = inOrigin;

        if (!m_UserSize)
            m_Size = inSize;

        if (!m_UserSpacing)
            m_OutputSpacing = inSpacing;


        typename TOutputImage::RegionType outputLargestPossibleRegion;
        outputLargestPossibleRegion.SetSize( m_Size );
        outputLargestPossibleRegion.SetIndex( m_OutputStartIndex );
        outputPtr->SetLargestPossibleRegion( outputLargestPossibleRegion );

        outputPtr->SetOrigin( m_OutputOrigin );
        outputPtr->SetSpacing( m_OutputSpacing );
        outputPtr->SetDirection( m_OutputDirection );
    }


}

/** 
 * Verify if any of the components has been modified.
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType>
unsigned long 
NMResampleImageFilter<TInputImage,TOutputImage,TInterpolatorPrecisionType>
::GetMTime( void ) const
{
  unsigned long latestTime = Object::GetMTime(); 

  if( m_Transform )
    {
    if( latestTime < m_Transform->GetMTime() )
      {
      latestTime = m_Transform->GetMTime();
      }
    }

  if( m_Interpolator )
    {
    if( latestTime < m_Interpolator->GetMTime() )
      {
      latestTime = m_Interpolator->GetMTime();
      }
    }

  return latestTime;
}

} // end namespace itk

//#endif

#endif
