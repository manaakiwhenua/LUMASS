/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2021 Manaaki Whenua - Landcare Research New Zealand Ltd
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

#ifndef __nmDataBufferFilter_txx
#define __nmDataBufferFilter_txx

#include "nmlog.h"
#include "nmDataBufferFilter.h"
#include <itkDataObject.h>
#include <itkImportImageContainer.h>
#include <itkImageScanlineConstIterator.h>
#include <itkImageScanlineIterator.h>

namespace otb {

template <class TInputImage>
DataBufferFilter<TInputImage>
::DataBufferFilter()
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);
}

template <class TInputImage>
DataBufferFilter<TInputImage>
::~DataBufferFilter()
{
}

template <class TInputImage>
void DataBufferFilter<TInputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
}

template <class TInputImage>
void DataBufferFilter<TInputImage>
::SetNthInput(unsigned int num, itk::DataObject *input)
{
    InputImageType* img = dynamic_cast<InputImageType*>(input);

    if (img != nullptr)
    {
        this->SetInput(0, img);
    }

    this->Modified();
}

//template <class TInputImage> //
//void DataBufferFilter<TInputImage, TOutputImage>
//::GenerateOutputInformation()
//{
//    Superclass::GenerateOutputInformation();
//}
//
//template <class TInputImage> //
//void DataBufferFilter<TInputImage, TOutputImage>
//::GenerateInputRequestedRegion()
//{
//    Superclass::GenerateInputRequestedRegion();
//}

template <class TInputImage>
void DataBufferFilter<TInputImage>
::GenerateData(void)
{
    this->AllocateOutputs();
    InputImageType* in = const_cast<InputImageType*>(this->GetInput());
    InputImageType* out = dynamic_cast<InputImageType*>(this->GetOutput());



    using IteratorType = typename itk::ImageScanlineIterator<InputImageType>;
    using IteratorPixelType = typename IteratorType::InternalPixelType;

    IteratorType inIt(in, in->GetRequestedRegion());
    IteratorType outIt(out, out->GetRequestedRegion());

    while(!inIt.IsAtEnd())
    {
        while(!inIt.IsAtEndOfLine())
        {
            //IteratorPixelType *ival = static_cast<IteratorPixelType>(inIt.Value());
            outIt.Set(inIt.Get());

            ++inIt;
            ++outIt;
        }
        inIt.NextLine();
        outIt.NextLine();
    }
}

}      // end of namespace otb

#endif // end include guard
