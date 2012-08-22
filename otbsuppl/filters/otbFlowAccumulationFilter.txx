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
 * FlowAccumulationFilter.txx
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef __otbFlowAccumulationFilter_txx
#define __otbFlowAccumulationFilter_txx

#include "otbFlowAccumulationFilter.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodIterator.h"

//#include "itkZeroFluxNeumannBoundaryCondition.h"

class vnl_math;

namespace otb {

template <class TInputImage, class TOutputImage>
FlowAccumulationFilter<TInputImage, TOutputImage>
::FlowAccumulationFilter()
	: m_algo(GRADIENT_HORN), m_unit(GRADIENT_DEGREE),
	  m_xdist(1), m_ydist(1)
{
}

template <class TInputImage, class TOutputImage>
FlowAccumulationFilter<TInputImage, TOutputImage>
::~FlowAccumulationFilter()
{
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::SetGradientAlgorithm(NmGradientAlgorithm algo)
{
	this->m_algo = algo;
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::SetGradientUnit(NmGradientUnit unit)
{
	this->m_unit = unit;
}


template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::GenerateData(void)
{
	NMDebug(<< "Enter FlowAcc::GenerateData" << std::endl);

	// get a handle of the in- and output data
	InputImagePointer pInImg =  const_cast<TInputImage* >(this->GetInput());
	OutputImagePointer pOutImg = this->GetOutput();
	pOutImg->SetBufferedRegion( pOutImg->GetRequestedRegion() );
	pOutImg->Allocate();

	// get pixel size in x and y direction
	typename TInputImage::SpacingType spacing = pInImg->GetSpacing();
	m_xdist = spacing[0];
	m_ydist = abs(spacing[1]);

	// get the number of pixels
	long numcols = pInImg->GetLargestPossibleRegion().GetSize(0);
	long numrows = pInImg->GetLargestPossibleRegion().GetSize(1);
	long numpix = numcols * numrows;

	NMDebugInd(1, << "cellsize is: " << spacing[0] << ", " << spacing[1] << std::endl);
	NMDebugInd(1, << "m_xdist: " << m_xdist << ", m_ydist: " << m_ydist << std::endl);
	NMDebugInd(1, << "numcols: " << numcols << ", numrows: " << numrows <<
			", numpix: " << numpix << std::endl);

//	typedef itk::ZeroFluxNeumannBoundaryCondition<TInputImage> BndCondType;
	typedef itk::NeighborhoodIterator<TInputImage> KernelIterType;
	typedef itk::ImageRegionIterator<TOutputImage> RegionIterType;

	typename KernelIterType::RadiusType radius;
	radius.Fill(1);
	KernelIterType inIter(radius, pInImg, pOutImg->GetRequestedRegion());
	RegionIterType outIter(pOutImg, pOutImg->GetRequestedRegion());

	long counter = 0;
	for (inIter.GoToBegin(), outIter.GoToBegin(); !inIter.IsAtEnd(); ++inIter, ++outIter)
	{
		double val;
		const NeighborhoodType nh = inIter.GetNeighborhood();
		this->slope(nh, &val);
		outIter.Set(val);

		this->SetProgress(++counter/(float)numpix);
	}

	NMDebug(<< "Leave FlowAcc::GenerateData" << std::endl);
}

//template <class TInputImage, class TOutputImage>
//OffsetType FlowAccumulationFilter<TInputImage, TOutputImage>
//::GetNextUpwardCell(NeighborhoodIteratorType iter, double xdist, double ydist, double digdist)
//{
//	OffsetType o1 = {{-1,-1}};
//	OffsetType o2 = {{0,-1}};
//	OffsetType o3 = {{1,-1}};
//	OffsetType o4 = {{-1,0}};
//	OffsetType o5 = {{0,0}};
//	OffsetType o6 = {{1,0}};
//	OffsetType o7 = {{-1,1}};
//	OffsetType o8 = {{0,1}};
//	OffsetType o9 = {{1,1}};
//
//
//
//
//}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::dZdX(const NeighborhoodType& nh, double* val)
{
	switch (m_algo)
	{
	case GRADIENT_ZEVEN:
		*val = (-nh[3] + nh[5]) / (m_xdist + m_ydist);
		break;
	case GRADIENT_HORN:
	default:
		*val = ((nh[2] + 2*nh[5] + nh[8]) - (nh[0] + 2*nh[3] + nh[6])) / (4*(m_xdist + m_ydist));
		break;
	}
}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::dZdY(const NeighborhoodType& nh, double* val)
{
	switch (m_algo)
	{
	case GRADIENT_ZEVEN:
		*val = (nh[1] - nh[7]) / (m_xdist + m_ydist);
		break;
	case GRADIENT_HORN:
	default:
		*val = ((nh[8] + 2*nh[7] + nh[6]) - (nh[2] + 2*nh[1] + nh[0])) / (4*(m_xdist + m_ydist));
		break;
	}
}

//template <class TInputImage, class TOutputImage>
//void FlowAccumulationFilter<TInputImage, TOutputImage>
//::aspect(const NeighborhoodType& nh, double* val)
//{
//	double zx, zy;
//	dZdX(nh, &zx);
//	dZdY(nh, &zy);
//
//	*val = ::atan2(zy,zx) * 180 / vnl_math::pi;
//	if (*val < 0)
//		*val += 360;
//}

template <class TInputImage, class TOutputImage>
void FlowAccumulationFilter<TInputImage, TOutputImage>
::slope(const NeighborhoodType& nh, double* val)
{
	double zx, zy;
	dZdX(nh, &zx);
	dZdY(nh, &zy);

	switch (m_unit)
	{
	case GRADIENT_ASPECT:
		if (m_algo == GRADIENT_HORN)
			*val = atan2(zy,zx) - vnl_math::pi / 2.0;
		else
			*val = atan2(zx,zy) - vnl_math::pi;

		*val *= 180 / vnl_math::pi;
		if (*val < 0)
			*val += 360;
		break;
	case GRADIENT_PERCENT:
		*val = sqrt(zx * zx + zy * zy) * 100;
		break;
	case GRADIENT_DEGREE:
	default:
		*val = atan(sqrt(zx * zx + zy * zy)) * 180 / vnl_math::pi;
		break;
	}
}

//template <class TInputImage, class TOutputImage>
//InputImagePixelType FlowAccumulationFilter<TInputImage, TOutputImage>
//::GetNumUpwardCells(NeighborhoodIteratorType iter)
//{
//
//}


//template <class TInputImage, class TOutputImage>
//void FlowAccumulationFilter<TInputImage, TOutputImage>
//::GenerateOutputInformation()
//{
//	NMDebug(<< "Enter FlowAcc::GenerateOutputInfo" << endl);
//
//
//
//	NMDebug(<< "\twe don't do anything here though!" << endl);
//	NMDebug(<< "Leave FlowAcc::GenerateOutputInfo" << endl);
//}

//template <class TInputImage, class TOutputImage>
//void FlowAccumulationFilter<TInputImage, TOutputImage>
//::GenerateInputRequestedRegion()
//{
//	NMDebug(<< "Enter FlowAcc::GenerateInputReqReg" << endl);
//
//	Superclass::GenerateInputRequestedRegion();
//
//	// pad the input region by one pixel
////	typename TInputImage::SizeType radius;
////	radius.Fill(1);
//
//	InputImagePointer pInImg = const_cast<TInputImage* >(this->GetInput());
////	OutputImagePointer pOutImg = const_cast<TOutputImage* >(this->GetOutput());
////	typename TInputImage::RegionType inRegion;
////	inRegion = pInImg->GetRequestedRegion();
////	inRegion.PadByRadius(radius);
////	NMDebugInd(1, << "new Region-Index: " << inRegion.GetIndex() <<
////			" | new Region-Size: " << inRegion.GetSize() << endl);
//
////	inRegion.Crop(pInImg->GetLargestPossibleRegion());
//	pInImg->SetRequestedRegion(pInImg->GetLargestPossibleRegion());
//
//
//	NMDebug(<< "\twe don't do anything here though!" << endl);
//	NMDebug(<< "Leave FlowAcc::GenerateInputReqReg" << endl);
//}


} // end namespace

#endif
