 /*****************************i*************************************************
 * Created by Alexander Herzig st
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
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkProcessLUPotentials.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.16 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbProcessLUPotentials_txx
#define __otbProcessLUPotentials_txx

#include <algorithm>
#include "otbImage.h"
#include "otbProcessLUPotentials.h"
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
ProcessLUPotentials<TInputImage, TOutputImage>
::ProcessLUPotentials()
{
	this->SetNumberOfRequiredOutputs(2);

	// output #1 = the max potential map
	this->SetNthOutput(1, this->MakeOutput(1));

	// output #0 = the category map
	this->SetNthOutput(0, this->MakeOutput(0));

	m_MaskIdx = -1;

}

template <class TInputImage, class TOutputImage>
itk::DataObject::Pointer
ProcessLUPotentials<TInputImage, TOutputImage>
::MakeOutput(unsigned int idx)
 {
		switch(idx)
		{
		case 0:	// categorical map
			return dynamic_cast<itk::DataObject*>(
					OutputImageType::New().GetPointer());
			break;
		case 1: // max potential list
			return dynamic_cast<itk::DataObject*>(
					InputImageType::New().GetPointer());
			break;
		default:
			break;
		}

		return 0;
 }


template< class TInputImage, class TOutputImage>
void
ProcessLUPotentials< TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{

	int numInputs;
	if (this->GetNumberOfInputs() > this->m_Categories.size())
	{
		numInputs = this->GetNumberOfInputs() - 1;
		this->m_MaskIdx = this->m_Categories.size();
	}

	// check, whether all inputs have got the same dimension, size
	InputImagePointer refImg = const_cast<InputImageType*>(this->GetInput(0));
	typename InputImageType::SizeType refSize = refImg->GetRequestedRegion().GetSize();

	m_Inputs.push_back(refImg);
	for (int i=1; i < numInputs; ++i)
	{
		InputImagePointer img = const_cast<InputImageType*>(this->GetInput(i));
		m_Inputs.push_back(img);
		typename InputImageType::SizeType size = img->GetRequestedRegion().GetSize();
		for (int d=0; d < InputImageType::ImageDimension; ++d)
		{
			if (size[d] != refSize[d])
			{
				itk::ExceptionObject e;
				e.SetLocation("ProcessLUPotentials::BeforeThreadedGenerateData()");
				e.SetDescription("Input images differ in size!");
				throw e;
			}
		}
	}
}

template< class TInputImage, class TOutputImage>
void
ProcessLUPotentials< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       int threadId)
{
	OutputImagePointer cats = dynamic_cast<OutputImageType*>(
			const_cast<itk::DataObject*>(itk::ProcessObject::GetOutput(0)));
	InputImagePointer pots = dynamic_cast<InputImageType*>(
			const_cast<itk::DataObject*>(itk::ProcessObject::GetOutput(1)));

	typedef typename itk::ImageRegionIterator<InputImageType> InputIteratorType;
	typedef typename itk::ImageRegionIterator<OutputImageType> OutputIteratorType;
	typedef typename itk::ImageRegionConstIterator<InputImageType> InputConstIteratorType;
	OutputIteratorType catsIt(cats, outputRegionForThread);
	InputIteratorType potsIt(pots, outputRegionForThread);

	// create an iterator for each input map
	int numInputs;
	InputImagePointer maskImg;
	InputIteratorType maskIt;
	if (this->m_MaskIdx != -1)
	{
		numInputs = this->GetNumberOfInputs() - 1;
		maskImg = m_Inputs.at(this->m_MaskIdx);
		maskIt = InputIteratorType(maskImg, outputRegionForThread);
	}
	else
	{
		numInputs = this->GetNumberOfInputs();
	}


	std::vector<InputConstIteratorType> inputIts;
	inputIts.resize(numInputs);
	for (int i=0; i < numInputs; ++i)
	{
		inputIts[i] = InputConstIteratorType(m_Inputs.at(i), outputRegionForThread);
	}

	// support progress methods/callbacks
	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	typename InputImageType::PixelType maxVal = itk::NumericTraits<typename InputImageType::PixelType>::min();
	while(!inputIts.at(0).IsAtEnd() && !this->GetAbortGenerateData())
	{
		InputPixelType max = 0;
		int maxIdx=0;
		for (int i=0; i < numInputs; ++i)
		{
			if (inputIts[i].Get() > max)
			{
				max = inputIts[i].Get();
				maxIdx = i;
			}
		}

		//
		if (m_MaskIdx != -1)
		{
			if (maskIt.Get() == 0)
			{
				potsIt.Set(max);
				if (max > 0)
					catsIt.Set(static_cast<OutputPixelType>(m_Categories.at(maxIdx)));
				else
					catsIt.Set(itk::NumericTraits<OutputPixelType>::Zero);
			}
			else
			{
				potsIt.Set(itk::NumericTraits<InputPixelType>::Zero);
				//catsIt.Set(itk::NumericTraits<OutputPixelType>::Zero);
				catsIt.Set(maskIt.Get());
			}
			++maskIt;
		}
		else
		{
			potsIt.Set(max);
			if (max > 0)
				catsIt.Set(static_cast<OutputPixelType>(m_Categories.at(maxIdx)));
			else
				catsIt.Set(itk::NumericTraits<OutputPixelType>::Zero);
		}

		progress.CompletedPixel();

		++catsIt;
		++potsIt;
		for (int i=0; i < numInputs; ++i)
			++inputIts[i];
	}
}


/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutput>
void
ProcessLUPotentials<TInputImage, TOutput>
::PrintSelf(
  std::ostream& os, 
  itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "Number of input layers: " << this->GetNumberOfInputs() << std::endl;
	os << indent << "Categories in order of input: " << std::endl;
	for (int i=0; i < m_Categories.size(); ++i)
	{

		os << indent << "  #" << i << ": " << (m_Categories[i]) << std::endl;
	}
}

} // end namespace otb

//#endif

#endif
