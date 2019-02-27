 /******************************************************************************
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
  Module:    $RCSfile: itkFocalDistanceWeightingFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.16 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbFocalDistanceWeightingFilter_txx
#define __otbFocalDistanceWeightingFilter_txx

#include <algorithm>
#include "otbFocalDistanceWeightingFilter.h"
#include "itkConstShapedNeighborhoodIterator.h"
#include "itkImageRegionIterator.h"
#include "itkConstantBoundaryCondition.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkExceptionObject.h"


namespace otb
{

template <class TInputImage, class TOutputImage>
FocalDistanceWeightingFilter<TInputImage, TOutputImage>
::FocalDistanceWeightingFilter()
{
	m_Radius = 6;
}

template <class TInputImage, class TOutputImage>
void 
FocalDistanceWeightingFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw ()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();
  
  // get pointers to the input and output
  typename Superclass::InputImagePointer inputPtr = 
    const_cast< TInputImage * >( this->GetInput() );
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();
  
  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  typename TInputImage::RegionType::IndexValueArrayType radius;
  radius[0] = m_Radius;
  radius[1] = m_Radius;
  inputRequestedRegion.PadByRadius( radius );

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
    {
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    
    // build an exception
    itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}


template< class TInputImage, class TOutputImage>
void
FocalDistanceWeightingFilter< TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
	// check, whether the number of rows in the weights matrix matches
	// the number of given values to account for during the weighting procedure.
	if (this->m_Values.size() != this->m_Weights.rows())
	{
		itk::ExceptionObject e;
		e.SetDescription("!Exception: The number of rows of the weights matrix\n"
				         "doesn't match the number of neighbourhood values to\n"
				         "account for in the weighting procedure!\n"
				         "Please check your parameter settings!");
		throw e;
	}

	// check, whether the number of provided distances classes (weights) fits
	// with the fiven radius of the circular neighbourhood kernel;
	//         int numclasses = ((radius*radius) / 2.0) + 0.5;  // for odd radii
	//         int numclasses = ((radius*radius) / 2.0) + 1.5;  // for even radii

	int nclasses = 0;
	if (this->m_Radius % 2 == 0)
	{
		nclasses = ((m_Radius*m_Radius) / 2.0) + 1.5;
	}
	else
	{
		nclasses = ((m_Radius*m_Radius) / 2.0) + 0.5;
	}

	if (nclasses != m_Weights.cols())
	{
		itk::ExceptionObject e;
		e.SetDescription("!Exception: The number of columns of the weights matrix\n"
				         "doesn't match the number of distance classes for the given\n"
				         "(circular) neighbourhood (radius).\n"
				         "Please check your parameter settings!");
		throw e;
	}

}

template< class TInputImage, class TOutputImage>
void
FocalDistanceWeightingFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
	unsigned int i;
	itk::ConstantBoundaryCondition<InputImageType> nbc;
	nbc.SetConstant(0);

	typedef typename itk::ConstShapedNeighborhoodIterator<InputImageType> ShapedIteratorType;
	ShapedIteratorType inIt;
	typename ShapedIteratorType::RadiusType itradius;

	int radius = m_Radius;
	itradius.Fill(radius);
	itk::ImageRegionIterator<OutputImageType> outIt;

	// Allocate output
	typename OutputImageType::Pointer output = this->GetOutput();
	typename InputImageType::ConstPointer input = this->GetInput();

	// Find the data-set boundary "faces"
	typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<
			InputImageType>::FaceListType faceList;
	itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
	faceList = bC(input, outputRegionForThread, itradius);

	typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<
			InputImageType>::FaceListType::iterator fit;

	// support progress methods/callbacks
	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	// Process each of the boundary faces.  These are N-d regions which border
	// the edge of the buffer.
	for (fit = faceList.begin(); fit != faceList.end(); ++fit)
	{
		inIt = ShapedIteratorType(itradius, input, *fit);
		outIt = itk::ImageRegionIterator<OutputImageType>(output, *fit);

		// create a list of unique distances
		std::vector<float> distcl;
		std::vector<float>::const_iterator it;
		std::vector<float> distcl_sorted;
		std::vector<unsigned int> neighidx;
		for (int x = radius; x >= -radius; --x)
		{
			for (int y = radius; y >= -radius; --y)
			{
				typename ShapedIteratorType::OffsetType off;

				float dist = ::sqrt((double)(x * x + y * y));
				if (dist <= radius)
				{
					off[0] = static_cast<int>(x);
					off[1] = static_cast<int>(y);
					inIt.ActivateOffset(off);
					distcl.push_back(dist);
					neighidx.push_back(inIt.GetNeighborhoodIndex(off));

					it = std::find(distcl_sorted.begin(), distcl_sorted.end(), dist);
					if ((it == distcl_sorted.end() || distcl_sorted.size() == 0) && dist > 0)
					{
						distcl_sorted.push_back(dist);
					}
				}
			}
		}
		std::sort(distcl_sorted.begin(), distcl_sorted.end());
		std::vector<float>::size_type floatIdx;
		std::vector<unsigned int>::size_type uintIdx;

		inIt.OverrideBoundaryCondition(&nbc);
		inIt.GoToBegin();
		outIt.GoToBegin();
		int cnt=0;
		while (!inIt.IsAtEnd() && !this->GetAbortGenerateData())
		{
			float sum = 0;
			for (size_t i=0; i < neighidx.size(); ++i)
			{
				float v = inIt.GetPixel(neighidx[i]);

				// determine the row of the weights matrix
				int row;
				bool bRow = false;
				for (row=0; row < m_Values.size(); ++row)
				{
					if (v == static_cast<float>(m_Values[row]))
					{
						bRow = true;
						break;
					}
				}
				if (!bRow) continue;

				// determine the column of the weight matrix
				for (int col=0; col < distcl_sorted.size(); ++col)
				{
					if (distcl_sorted[col] == distcl[i])
					{
						sum += (float)m_Weights(row, col);
						break;
					}
				}
			}

			outIt.Set(static_cast<OutputPixelType>(sum));

			++inIt;
			++outIt;
			progress.CompletedPixel();
		}
	}
}

/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutput>
void
FocalDistanceWeightingFilter<TInputImage, TOutput>
::PrintSelf(
  std::ostream& os, 
  itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "Parameters for the focal neighbourhood weighting:" << std::endl;
	os << indent << "Radius of focal neighbourhood: " << m_Radius << std::endl;
	os << indent << "Values of influence in the neigbourhood: " << std::endl;
	for (int i = 0; i < m_Values.size(); ++i)
	{
		os << indent << "  " << (int)m_Values.at(i) << std::endl;
	}

	os << indent << "Weights matrix. The given rows represent the influence of the above given" << std::endl;
	os << indent << "values (one row per value) with increasing distance from the centre." << std::endl;
	for (int r = 0; r < m_Weights.rows(); ++r)
	{
		os << indent << "  ";
		for (int c = 0; c < m_Weights.cols(); ++c)
		{
			os << m_Weights(r,c) << " ";
		}
		os << std::endl;
	}
}

} // end namespace otb

//#endif

#endif
