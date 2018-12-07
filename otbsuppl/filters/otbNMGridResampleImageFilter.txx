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

#ifndef otbNMGridResampleImageFilter_txx
#define otbNMGridResampleImageFilter_txx

#include "otbNMGridResampleImageFilter.h"

#include "otbStreamingTraits.h"
#include "otbImage.h"

#include "itkNumericTraits.h"
#include "itkProgressReporter.h"
#include "itkImageScanlineIterator.h"
#include "itkContinuousIndex.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"

namespace otb
{

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::NMGridResampleImageFilter()
    : m_OutputStartIndex(),
      m_OutputSize(),
      m_OutputOrigin(),
      m_OutputSpacing(),
      m_EdgePaddingValue(),
      m_CheckOutputBounds(true),
      m_Interpolator(),
      m_ReachableOutputRegion(),
      m_InterpolationMethod("NearestNeighbour")
{
    // Set linear interpolator as default
    //m_Interpolator = dynamic_cast<InterpolatorType *>(DefaultInterpolatorType::New().GetPointer());

    // Initialize EdgePaddingValue
    m_EdgePaddingValue = itk::NumericTraits<OutputPixelType>::ZeroValue(m_EdgePaddingValue);
    m_DefaultPixelValue = m_EdgePaddingValue;

    // Initialize origin and spacing
    m_OutputOrigin.Fill(0.);
    m_OutputSpacing.Fill(1.);
    m_OutputStartIndex.Fill(0);
    m_OutputSize.Fill(0);
}


/** Import output parameters from a given image */
template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetOutputParametersFromImage(const ImageBaseType * image)
{
    this->SetOutputOrigin ( image->GetOrigin() );
    this->SetOutputSpacing ( image->GetSpacing() );
    this->SetOutputStartIndex ( image->GetLargestPossibleRegion().GetIndex() );
    this->SetOutputSize ( image->GetLargestPossibleRegion().GetSize() );
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetEdgePaddingValue(OutputPixelType padVal)
{
    m_EdgePaddingValue = padVal;
    m_DefaultPixelValue = padVal;
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetDefaultPixelValue(OutputPixelType defVal)
{
    m_EdgePaddingValue = defVal;
    m_DefaultPixelValue = defVal;
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetSize(SizeType sz)
{
    SetOutputSize(sz);
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetSize(SizeValueType* vsz)
{
    SizeType sz;
    for (int d=0; d < ImageDimension; ++d)
    {
        sz[d] = vsz[d];
    }
    SetOutputSize(sz);
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::GenerateOutputInformation()
{
    // call the superclass' implementation of this method
    Superclass::GenerateOutputInformation();

    // get pointers to the input and output
    OutputImageType *outputPtr = this->GetOutput();
    if ( !outputPtr )
    {
        return;
    }

    // Fill output image data
    typename TOutputImage::RegionType outputLargestPossibleRegion;
    outputLargestPossibleRegion.SetSize(m_OutputSize);
    outputLargestPossibleRegion.SetIndex(m_OutputStartIndex);

    outputPtr->SetLargestPossibleRegion(outputLargestPossibleRegion);
    //outputPtr->SetSignedSpacing(m_OutputSpacing);
    outputPtr->SetSpacing(m_OutputSpacing);
    outputPtr->SetOrigin(m_OutputOrigin);

    // need to set this sometime, so why not now ...
    SetInterpolatorFromMethodString();
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::GenerateInputRequestedRegion()
{
    Superclass::GenerateInputRequestedRegion();

    OutputImageType* outimg = this->GetOutput();
    if (outimg == ITK_NULLPTR)
    {
        return;
    }

    OutputImageRegionType outputRegion = outimg->GetRequestedRegion();

    SizeType outSize = outputRegion.GetSize();
    IndexType outIdxTL =  outputRegion.GetIndex();
    //SpacingType outSpacing = outimg->GetSpacing();
    IndexType outIdxBR;
    for (int d=0; d < InputImageDimension; ++d)
    {
        outIdxBR[d] = outSize[d] > 0 ? outIdxTL[d] + outSize[d] - 1 : outIdxTL[d];
    }

    PointType outTL, outBR;
    outimg->TransformIndexToPhysicalPoint(outIdxTL, outTL);
    outimg->TransformIndexToPhysicalPoint(outIdxBR, outBR);

    InputImageType* img = const_cast<InputImageType*>(this->GetInput());
    if (img == ITK_NULLPTR)
    {
        itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
        e.SetLocation(ITK_LOCATION);
        e.SetDescription("Invalid Input Image!");
        e.SetDataObject(img);
        throw e;
    }

    IndexType idxTL, idxBR;
    ContinuousInputIndexType idxCTL, idxCBR;

    bool insideTL = true;
    bool insideBR = true;
    if (!img->TransformPhysicalPointToContinuousIndex(outTL, idxCTL))
    {
        insideTL = false;
    }

    if (!img->TransformPhysicalPointToContinuousIndex(outBR, idxCBR))
    {
        insideBR = false;
    }

    for (int d=0; d < InputImageDimension; ++d)
    {
        idxTL[d] = vcl_floor(idxCTL[d]);
        idxBR[d] = vcl_ceil(idxCBR[d]);
    }

    InputImageRegionType inImgLPR = img->GetLargestPossibleRegion();
    SizeType inLPRSize = img->GetLargestPossibleRegion().GetSize();

    // if the output requested region is empty,
    // so shall be the input requested region!
    InputImageRegionType inRegion;
    //bool bAllZero = false;
    //    for (int d=0; d < InputImageDimension; ++d)
    //    {
    //        if (outSize[d] == 0)
    //        {
    //            bAllZero = true;
    //        }
    //    }

    //    if (bAllZero)
    //    {
    //        idxTL.Fill(0);
    //        idxBR.Fill(0);

    //        SizeType emptysize;
    //        emptysize.Fill(0);
    //        inRegion.SetSize(emptysize);
    //        img->SetBufferedRegion(inRegion);
    //        img->SetRequestedRegion(inRegion);
    //        return;
    //    }

    // push the index back into the LPR of the input image
    // for the 'dimensions' going over board

    // we assume that the order of indices is correct,
    // i.e. TL is actually top left and BR is actually
    // bottom right

    if(!insideTL)
    {
        for (int d=0; d < InputImageDimension; ++d)
        {
            idxTL[d] = idxTL[d] < 0 ? 0 : idxTL[d];
        }
    }

    if (!insideBR)
    {
        for (int d=0; d < InputImageDimension; ++d)
        {
            idxBR[d] = inLPRSize[d]-1;
        }
    }

    // calculate the size of the input requested region
    for (int d=0; d < InputImageDimension; ++d)
    {
        typename SizeType::SizeValueType sz = idxBR[d] - idxTL[d] + 1;
        inRegion.SetSize(d, sz);
    }
    inRegion.SetIndex(idxTL);


    // pad input requested region for interpolation
    if (    this->m_InterpolationMethod == "NearestNeighbour"
         || this->m_InterpolationMethod == "Linear"
       )
    {
        inRegion.PadByRadius(static_cast<unsigned int>(1));
    }
    else
    {
        inRegion.PadByRadius(static_cast<unsigned int>(2));
    }

    // make sure, we don't go over the largest possible
    // region of the input
    for (int d=0; d < InputImageDimension; ++d)
    {
        typename IndexType::IndexValueType idx;
        typename SizeType::SizeValueType size;
        idx = inRegion.GetIndex(d);
        size = inRegion.GetSize(d);
        if (idx < 0)
        {
            idx = 0;
            --size;
        }
        size = idx + size > inImgLPR.GetSize(d)-1 ? inImgLPR.GetSize(d)-1 - idx + 1 : size;

        inRegion.SetIndex(d, idx);
        inRegion.SetSize(d, size);
    }

    img->SetRequestedRegion(inRegion);
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::SetInterpolatorFromMethodString()
{
    typedef typename itk::BSplineInterpolateImageFunction<InputImageType, TInterpolatorPrecision> SplineType;
    typedef typename itk::NearestNeighborInterpolateImageFunction<InputImageType, TInterpolatorPrecision> NearestType;
    typedef typename itk::LinearInterpolateImageFunction<InputImageType, TInterpolatorPrecision> LinearType;

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
}



template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::BeforeThreadedGenerateData()
{
    this->SetNumberOfThreads(1);

    if ( m_Interpolator.IsNull() )
    {
        itkExceptionMacro(<< "Interpolator not set");
    }

    // Connect input image to interpolator
    m_Interpolator->SetInputImage( this->GetInput() );

    unsigned int nComponents
            = itk::DefaultConvertPixelTraits<OutputPixelType>::GetNumberOfComponents(
                m_EdgePaddingValue );

    if (nComponents == 0)
    {

        // Build a default value of the correct number of components
        OutputPixelComponentType zeroComponent
                = itk::NumericTraits<OutputPixelComponentType>::ZeroValue( zeroComponent );

        nComponents = this->GetInput()->GetNumberOfComponentsPerPixel();

        itk::NumericTraits<OutputPixelType>::SetLength(m_EdgePaddingValue, nComponents );
        for (unsigned int n=0; n<nComponents; n++)
        {
            OutputPixelConvertType::SetNthComponent( n, m_EdgePaddingValue,
                                                     zeroComponent );
        }
    }

    // Compute ReachableOutputRegion
    // InputImage buffered region corresponds to a region of the output
    // image. Computing it beforehand allows saving IsInsideBuffer
    // calls in the interpolation loop

    // Compute the padding due to the interpolator


    IndexType inUL = this->GetInput()->GetBufferedRegion().GetIndex();
    IndexType inLR = this->GetInput()->GetBufferedRegion().GetIndex() + this->GetInput()->GetBufferedRegion().GetSize();
    inLR[0]-=1;
    inLR[1]-=1;

    // We should take interpolation radius into account here, but this
    // does not match the IsInsideBuffer method
    // unsigned int interpolatorRadius =
    // StreamingTraits<typename Superclass::InputImageType>::CalculateNeededRadiusForInterpolator(this->GetInterpolator());
    // inUL[0]+=interpolatorRadius;
    // inUL[1]+=interpolatorRadius;
    // inLR[0]-=interpolatorRadius;
    // inLR[1]-=interpolatorRadius;

    PointType inULp, inLRp;
    this->GetInput()->TransformIndexToPhysicalPoint(inUL,inULp);
    this->GetInput()->TransformIndexToPhysicalPoint(inLR,inLRp);

    inULp-=0.5*this->GetInput()->GetSpacing();
    inLRp+=0.5*this->GetInput()->GetSpacing();

    ContinuousInputIndexType outUL;
    ContinuousInputIndexType outLR;
    this->GetOutput()->TransformPhysicalPointToContinuousIndex(inULp,outUL);
    this->GetOutput()->TransformPhysicalPointToContinuousIndex(inLRp,outLR);

    IndexType outputIndex;
    // This needs to take into account negative spacing
    outputIndex[0] = vcl_ceil(std::min(outUL[0],outLR[0]));
    outputIndex[1] = vcl_ceil(std::min(outUL[1],outLR[1]));

    SizeType outputSize;
    outputSize[0] = vcl_floor(std::max(outUL[0],outLR[0])) - outputIndex[0] + 1;
    outputSize[1] = vcl_floor(std::max(outUL[1],outLR[1])) - outputIndex[1] + 1;

    m_ReachableOutputRegion.SetIndex(outputIndex);
    m_ReachableOutputRegion.SetSize(outputSize);
}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId)
{
    // Get the output pointers
    OutputImageType *outputPtr = this->GetOutput();

    // Get this input pointers
    const InputImageType *inputPtr = this->GetInput();
    double bounds[InputImageDimension][2];
    for (int d=0; d < InputImageDimension; ++d)
    {
        bounds[d][0] = inputPtr->GetLargestPossibleRegion().GetIndex()[d];
        bounds[d][1] = inputPtr->GetLargestPossibleRegion().GetIndex()[d] + inputPtr->GetLargestPossibleRegion().GetSize()[d]-1;
    }

    // Min/max values of the output pixel type AND these values
    // represented as the output type of the interpolator
    const OutputPixelComponentType minValue =  itk::NumericTraits< OutputPixelComponentType >::NonpositiveMin();
    const OutputPixelComponentType maxValue =  itk::NumericTraits< OutputPixelComponentType >::max();

    const InterpolatorComponentType minOutputValue = static_cast< InterpolatorComponentType >( minValue );
    const InterpolatorComponentType maxOutputValue = static_cast< InterpolatorComponentType >( maxValue );

    // Iterator on the output region for current thread
    OutputImageRegionType regionToCompute = outputRegionForThread;
    bool cropSucceed = regionToCompute.Crop(m_ReachableOutputRegion);

    // Fill thread buffer
    itk::ImageScanlineIterator<OutputImageType> outItFull(outputPtr,outputRegionForThread);
    outItFull.GoToBegin();
    while(!outItFull.IsAtEnd())
    {
        while(!outItFull.IsAtEndOfLine())
        {
            outItFull.Set(m_EdgePaddingValue);
            ++outItFull;
        }
        outItFull.NextLine();
    }

    if(!cropSucceed)
        return;

    itk::ImageScanlineIterator<OutputImageType> outIt(outputPtr, regionToCompute);

    // Support for progress methods/callbacks
    itk::ProgressReporter progress( this,
                                    threadId,
                                    regionToCompute.GetSize()[1]);

    // Temporary variables for loop
    PointType outPoint;
    ContinuousInputIndexType inCIndex;
    InterpolatorOutputType interpolatorValue;
    OutputPixelType outputValue;

    // TODO: assert outputPtr->GetSignedSpacing() != 0 here
    assert(outputPtr->GetSpacing()[0]!=0&&"Null spacing will cause division by zero.");
    const double delta = outputPtr->GetSpacing()[0]/inputPtr->GetSpacing()[0];
    double cidx[InputImageDimension];

    // Iterate through the output region
    outIt.GoToBegin();

    while(!outIt.IsAtEnd())
    {
        // Map output index to input continuous index
        outputPtr->TransformIndexToPhysicalPoint(outIt.GetIndex(),outPoint);
        inputPtr->TransformPhysicalPointToContinuousIndex(outPoint,inCIndex);

        while(!outIt.IsAtEndOfLine())
        {
            interpolatorValue = m_Interpolator->EvaluateAtContinuousIndex(inCIndex);

            // Cast and check bounds
            this->CastPixelWithBoundsChecking(interpolatorValue,minOutputValue,maxOutputValue,outputValue);

            // Set output value
            outIt.Set(outputValue);


            // move one pixel forward
            ++outIt;

            // Update input position
            inCIndex[0]+=delta;
        }

        // Report progress
        progress.CompletedPixel();

        // Move to next line
        outIt.NextLine();
    }

}

template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::AfterThreadedGenerateData()
{
    // Disconnect input image from the interpolator
    m_Interpolator->SetInputImage(ITK_NULLPTR);
}


template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
itk::ModifiedTimeType
NMGridResampleImageFilter< TInputImage, TOutputImage, TInterpolatorPrecision >
::GetMTime(void) const
{
    itk::ModifiedTimeType latestTime = itk::Object::GetMTime();

    if ( m_Interpolator )
    {
        if ( latestTime < m_Interpolator->GetMTime() )
        {
            latestTime = m_Interpolator->GetMTime();
        }
    }

    return latestTime;
}


template <typename TInputImage, typename TOutputImage,
          typename TInterpolatorPrecision>
void
NMGridResampleImageFilter<TInputImage, TOutputImage, TInterpolatorPrecision>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);

    os << indent << "EdgePaddingValue: "
       << static_cast< typename itk::NumericTraits< OutputPixelType >::PrintType >
          ( m_EdgePaddingValue )
       << std::endl;
    os << indent << "OutputStartIndex: " << m_OutputStartIndex << std::endl;
    os << indent << "OutputSize: " << m_OutputSize << std::endl;
    os << indent << "OutputOrigin: " << m_OutputOrigin << std::endl;
    os << indent << "OutputSpacing: " << m_OutputSpacing << std::endl;
    os << indent << "Interpolator: " << m_Interpolator.GetPointer() << std::endl;
    os << indent << "CheckOutputBounds: " << ( m_CheckOutputBounds ? "On" : "Off" )
       << std::endl;
}



} // namespace otb


#endif
