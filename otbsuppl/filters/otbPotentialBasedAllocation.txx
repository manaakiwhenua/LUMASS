 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
#ifndef __otbPotentialBasedAllocation_txx
#define __otbPotentialBasedAllocation_txx

#include <algorithm>
#include "otbImage.h"
#include "otbPotentialBasedAllocation.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"
#include "itkExceptionObject.h"
#include "itkSmartPointer.h"
#include "itkProcessObject.h"


namespace otb
{

template <class TInputImage, class TOutputImage>
PotentialBasedAllocation<TInputImage, TOutputImage>
::PotentialBasedAllocation()
{
	this->SetInPlace(true);
}

template< class TInputImage, class TOutputImage>
void
PotentialBasedAllocation< TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
	// check, whether the number of thresholds matches the number of
	// categories provided to this filter, if not - bail out
	if (m_Categories.size() != m_Thresholds.size())
	{
		itk::ExceptionObject e;
		e.SetLocation("PotentialBasedAllocation::BeforeThreadedGenerateData()");
		e.SetDescription("The number of thresholds doesn't match the number of categories!");
		throw e;
	}

	// check, whether all inputs have got the same dimension, size
	InputImagePointer catImg = const_cast<InputImageType*>(this->GetInput(0));
	typename InputImageType::SizeType catSize = catImg->GetRequestedRegion().GetSize();

	InputImagePointer potImg = const_cast<InputImageType*>(this->GetInput(1));
	typename InputImageType::SizeType potSize = potImg->GetRequestedRegion().GetSize();
	for (int d=0; d < InputImageType::ImageDimension; ++d)
	{
		if (catSize[d] != potSize[d])
		{
			itk::ExceptionObject e;
			e.SetLocation("PotentialBasedAllocation::BeforeThreadedGenerateData()");
			e.SetDescription("Input images differ in size!");
			throw e;
		}
	}
}

template< class TInputImage, class TOutputImage>
void
PotentialBasedAllocation< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       int threadId)
{
	OutputImagePointer cats = dynamic_cast<OutputImageType*>(
			const_cast<itk::DataObject*>(itk::ProcessObject::GetOutput(0)));
	InputImagePointer potImg = const_cast<InputImageType*>(this->GetInput(1));

	typedef typename itk::ImageRegionIterator<InputImageType> IteratorType;
	typedef typename itk::ImageRegionConstIterator<InputImageType> ConstIteratorType;
	IteratorType catsIt(cats, outputRegionForThread);
	ConstIteratorType potsIt(potImg, outputRegionForThread);

	// support progress methods/callbacks
	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	typename OutputImageType::PixelType zero = itk::NumericTraits<typename OutputImageType::PixelType>::Zero;
	while(!catsIt.IsAtEnd() && !this->GetAbortGenerateData())
	{
		for (int i=0; i < m_Categories.size(); ++i)
		{
			if (catsIt.Get() == m_Categories[i])
			{
				if (potsIt.Get() < m_Thresholds[i])
				{
					catsIt.Set(zero);
				}
			}
		}

		++catsIt;
		++potsIt;
		progress.CompletedPixel();
	}
}


/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutput>
void
PotentialBasedAllocation<TInputImage, TOutput>
::PrintSelf(
  std::ostream& os, 
  itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "Categories to allocate and assigned thresholds: " << std::endl;
	if (m_Categories.size() == m_Thresholds.size())
	{
		for (int i=0; i < m_Categories.size(); ++i)
		{

			os << indent << "  #" << i << ": " << (m_Categories[i])
			   << " = " << m_Thresholds[i] << std::endl;
		}
	}
	else
	{
		os << indent << "This filter hasn't been configured properly yet! "
		   << "The number of categories doesn't match the number of thresholds!"
		   << std::endl;
	}
}

} // end namespace otb

//#endif

#endif
