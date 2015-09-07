/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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

#ifndef __otbUniqueCombinationFilter_txx
#define __otbUniqueCombinationFilter_txx

#include "nmlog.h"
#include "otbUniqueCombinationFilter.h"

#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

// TOKYO CABINET
//#include "tcutil.h"
#include "tchdb.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace otb
{

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::UniqueCombinationFilter()
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

}

template< class TInputImage, class TOutputImage >
UniqueCombinationFilter< TInputImage, TOutputImage >
::~UniqueCombinationFilter()
{

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::SetNthInput(unsigned int idx, const TInputImage* image)
{
    this->SetInput(idx, const_cast<TInputImage *>( image ));

    // let's make sure, we've got the same size for
    // this vector that we've got for the images
    for (int i=this->m_vRAT.size(); i <= idx; ++i)
    {
        this->m_vRAT.push_back(0);
    }
}



template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer table)
{
    for (int i=this->m_vRAT.size(); i <= idx; ++i)
    {
        this->m_vRAT.push_back(0);
    }

    this->m_vRAT[idx] = table;
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer
UniqueCombinationFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    if (idx < this->m_vRAT.size())
    {
        return this->m_vRAT.at(idx);
    }
    return 0;
}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
    // ======================================================================
    // IMAGE AXES DIMENSION CHECK
    // ======================================================================


    // check, whether all inputs have the same dimension
    // (... and hopefully also the same projection - but
    // we don't check that ...)
    unsigned long long inputSize[2];
    inputSize[0] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(0);
    inputSize[1] = this->GetInput(0)->GetLargestPossibleRegion().GetSize(1);

    unsigned int nbInputs = this->GetNumberOfValidRequiredInputs();
    for (unsigned int p=0; p < nbInputs; ++p)
    {
        if((inputSize[0] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(0))
           || (inputSize[1] != this->GetInput(p)->GetLargestPossibleRegion().GetSize(1)))
          {
          itkExceptionMacro(<< "Input images must have the same dimensions." << std::endl
                            << "image #1 is [" << inputSize[0] << ";" << inputSize[1] << "]" << std::endl
                            << "image #" << p+1 << " is ["
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(0) << ";"
                            << this->GetInput(p)->GetLargestPossibleRegion().GetSize(1) << "]");

          itk::ExceptionObject e(__FILE__, __LINE__);
          e.SetLocation(ITK_LOCATION);
          e.SetDescription("Input regions don't match in size!");
          throw e;
          }
    }

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId )
{

    typedef itk::ImageRegionConstIterator<TInputImage> ImageRegionConstIteratorType;
    typedef itk::ImageRegionIterator<TOutputImage> ImageRegionIteratorType;

    unsigned int nbInputs = this->GetNumberOfValidRequiredInputs();
    if (nbInputs == 0)
    {
        itkExceptionMacro(<< "Well, we do need some input tough!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    std::vector<ImageRegionConstIteratorType> inputIterators(nbInputs);
    for (unsigned int in=0; in < nbInputs; ++in)
    {
        inputIterators[in] = ImageRegionConstIteratorType(this->GetInput(in),
                                        outputRegionForThread);
        inputIterators[in].GoToBegin();
    }
    ImageRegionIteratorType outIt(this->GetOutput(0),
                         outputRegionForThread);
    outIt.GoToBegin();


    itk::ProgressReporter progress(this, threadId,
                                   outputRegionForThread.GetNumberOfPixels());

    while (!outIt.IsAtEnd() && !this->GetAbortGenerateData())
    {
        outIt.Set(static_cast<OutputPixelType>(inputIterators[0].Get()));
        progress.CompletedPixel();
        ++outIt;
        for (unsigned int in=0; in < nbInputs; ++in)
        {
            ++inputIterators[in];
        }
    }

}

template< class TInputImage, class TOutputImage >
void
UniqueCombinationFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{

}

}   // end namespace otb

#endif /* __otbUniqueCombinationFilter_txx */
