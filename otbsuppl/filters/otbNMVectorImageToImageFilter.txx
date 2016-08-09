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

#ifndef OTBNMVECTORIMAGETOIMAGEFILTER_TXX
#define OTBNMVECTORIMAGETOIMAGEFILTER_TXX

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "otbNMVectorImageToImageFilter.h"

namespace otb
{

template < class TInputImage, class TOutputImage>
NMVectorImageToImageFilter< TInputImage, TOutputImage>
::NMVectorImageToImageFilter()
    : m_Band(1)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);
}

template < class TInputImage, class TOutputImage>
void
NMVectorImageToImageFilter< TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
    InputImageType* input = const_cast<InputImageType*>(this->GetInput());
    OutputImageType* output = this->GetOutput();

    // check for same dimension
    if (input->GetImageDimension() != output->GetImageDimension())
    {
        itk::ExceptionObject e(__FILE__, __LINE__);
        e.SetLocation(ITK_LOCATION);
        e.SetDescription("Inupt and ouput dimensions do not match!");
        throw e;
    }


    // check for input image being a vector image
    if (input->GetNumberOfComponentsPerPixel() < 2)
    {
        itk::ExceptionObject e(__FILE__, __LINE__);
        e.SetLocation(ITK_LOCATION);
        e.SetDescription("Inupt image is not a vector image!");
        throw e;
    }

    // turn 1-based band index into 0-based VariableLengthVector index
    // used for accessing pixel components
    int tband = static_cast<int>(m_Band);
    --tband;

    // check whether specified band index (0-based) is valid
    if (    tband >= input->GetNumberOfComponentsPerPixel()
        ||  tband < 0
       )
    {
        itk::ExceptionObject e(__FILE__, __LINE__);
        e.SetLocation(ITK_LOCATION);
        e.SetDescription("Specified (1-based) band index is out of bounds!");
        throw e;
    }

}

template < class TInputImage, class TOutputImage>void
NMVectorImageToImageFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(
        const OutputImageRegionType& outputRegionForThread,
        itk::ThreadIdType threadId)
{
    InputImageType* input = const_cast<InputImageType*>(this->GetInput());
    OutputImageType* output = this->GetOutput();

    //    typedef itk::VectorImageToImageAdaptor<InputPixelType, InputImageDimension> AdaptorType;
    //    typename AdaptorType::Pointer adaptor = AdaptorType::New();
    //    adaptor->SetExtractComponentIndex(m_Band);
    //    adaptor->SetImage(input);

    itk::ImageRegionConstIterator<InputImageType> inIter(input, outputRegionForThread);
    itk::ImageRegionIterator<OutputImageType> outIter(output, outputRegionForThread);

    itk::ProgressReporter progress(
                      this, threadId,
                      outputRegionForThread.GetNumberOfPixels());

    const int bandidx = m_Band-1;
    inIter.GoToBegin();
    outIter.GoToBegin();
    while (!inIter.IsAtEnd() && !this->GetAbortGenerateData())
    {
        outIter.Set(static_cast<OutputPixelType>(inIter.Get()[bandidx]));
        ++inIter;
        ++outIter;
        progress.CompletedPixel();
    }
}


} // namespace otb

#endif // OTBNMVECTORIMAGETOIMAGEFILTER_TXX
