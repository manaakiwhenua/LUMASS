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
 * DEMSlopeAspectFilter.txx
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef __otbDEMSlopeAspectFilter_txx
#define __otbDEMSlopeAspectFilter_txx

#include "otbDEMSlopeAspectFilter.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkNeighborhoodIterator.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"


namespace otb {

template <class TInputImage, class TOutputImage>
DEMSlopeAspectFilter<TInputImage, TOutputImage>
::DEMSlopeAspectFilter()
	: m_algo(GRADIENT_HORN), m_unit(GRADIENT_DEGREE),
	  m_xdist(1), m_ydist(1)
{
}

template <class TInputImage, class TOutputImage>
DEMSlopeAspectFilter<TInputImage, TOutputImage>
::~DEMSlopeAspectFilter()
{
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SetGradientAlgorithm(NmGradientAlgorithm algo)
{
	this->m_algo = algo;
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SetGradientUnit(NmGradientUnit unit)
{
	this->m_unit = unit;
}

template <class TInputImage, class TOutputImage>
void
DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
  InputImageSizeType radius;
  radius.Fill(1);
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


template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
    //NMDebug(<< "Enter FlowAcc::GenerateData" << std::endl);
    // Allocate output
    typename OutputImageType::Pointer pOutImg = this->GetOutput();
    typename  InputImageType::ConstPointer pInImg  = this->GetInput();

    // Find the data-set boundary "faces"
    typename InputImageType::SizeType radius;
    radius.Fill(1);

    typename itk::NeighborhoodAlgorithm
            ::ImageBoundaryFacesCalculator<InputImageType>::FaceListType faceList;
    itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
    faceList = bC(pInImg, outputRegionForThread, radius);

    typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>
            ::FaceListType::iterator fit;

    // support progress methods/callbacks
    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());


	// get pixel size in x and y direction
    typename TInputImage::SpacingType spacing = pInImg->GetSignedSpacing();
	m_xdist = spacing[0];
	m_ydist = abs(spacing[1]);

	// get the number of pixels
    //	long numcols = pInImg->GetLargestPossibleRegion().GetSize(0);
    //	long numrows = pInImg->GetLargestPossibleRegion().GetSize(1);
    //	long numpix = numcols * numrows;

    //	NMDebugInd(1, << "cellsize is: " << spacing[0] << ", " << spacing[1] << std::endl);
    //	NMDebugInd(1, << "m_xdist: " << m_xdist << ", m_ydist: " << m_ydist << std::endl);
    //	NMDebugInd(1, << "numcols: " << numcols << ", numrows: " << numrows <<
    //			", numpix: " << numpix << std::endl);

    itk::ZeroFluxNeumannBoundaryCondition<TInputImage> bndCond;
    typedef itk::ConstNeighborhoodIterator<TInputImage> KernelIterType;
    typedef itk::ImageRegionIterator<TOutputImage> RegionIterType;

    KernelIterType inIter; //(radius, pInImg, pOutImg->GetRequestedRegion());
    RegionIterType outIter; //(pOutImg, pOutImg->GetRequestedRegion());

    typedef typename itk::PixelTraits< OutputImagePixelType >::ValueType OutputValueType;
    OutputValueType nodata = itk::NumericTraits< OutputValueType >::ZeroValue();

    for (fit = faceList.begin(); fit != faceList.end(); ++fit)
    {
        inIter = KernelIterType(radius, pInImg, *fit);
        inIter.OverrideBoundaryCondition(&bndCond);
        outIter = RegionIterType(pOutImg, *fit);

        for (inIter.GoToBegin(), outIter.GoToBegin(); !inIter.IsAtEnd(); ++inIter, ++outIter)
        {
            double val;
            NeighborhoodType nh = inIter.GetNeighborhood();
            this->slope(nh, &val);
            val = ::isnan(val) ? nodata : val;
            outIter.Set(val);

            progress.CompletedPixel();
        }
    }

    //NMDebug(<< "Leave FlowAcc::GenerateData" << std::endl);
}

//template <class TInputImage, class TOutputImage>
//OffsetType DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
//void DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::aspect(const NeighborhoodType& nh, double* val)
//{
//    double zx, zy;
//    dZdX(nh, &zx);
//    dZdY(nh, &zy);

//    *val = ::atan2(zy,zx) * 180 / vnl_math::pi;
//    if (*val < 0)
//        *val += 360;
//}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
//InputImagePixelType DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::GetNumUpwardCells(NeighborhoodIteratorType iter)
//{
//
//}


//template <class TInputImage, class TOutputImage>
//void DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
//void DEMSlopeAspectFilter<TInputImage, TOutputImage>
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
