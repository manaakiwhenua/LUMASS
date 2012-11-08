/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkDanielssonCostDistanceMapImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2009-04-06 00:19:17 $
  Version:   $Revision: 1.39 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkDanielssonCostDistanceMapImageFilter_txx
#define __itkDanielssonCostDistanceMapImageFilter_txx

#define ctx "DanielssonCostDistanceMapImageFilter"
#include "nmlog.h"

#include <iostream>
#include <limits>

#include "itkDanielssonCostDistanceMapImageFilter.h"
#include "itkReflectiveImageRegionConstIterator.h"
#include "itkReflectiveImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodIterator.h"

namespace itk
{

/**
 *    Constructor
 */
template <class TInputImage,class TOutputImage>
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::DanielssonCostDistanceMapImageFilter()
{

  this->SetNumberOfRequiredOutputs( 1 );

  OutputImagePointer distanceMap = OutputImageType::New();
  this->SetNthOutput( 0, distanceMap.GetPointer() );

  this->m_OffsetImage = VectorImageType::New();

  m_SquaredDistance     = false;
  m_InputIsBinary       = false;
  m_UseImageSpacing     = false;
}

/**
 *  Return the distance map Image pointer
 */
template <class TInputImage,class TOutputImage>
typename DanielssonCostDistanceMapImageFilter<
  TInputImage,TOutputImage>::OutputImageType * 
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::GetDistanceMap(void)
{
  return  dynamic_cast< OutputImageType * >(
    this->ProcessObject::GetOutput(0) );
}

/**
 *  Prepare data for computation
 */
template <class TInputImage,class TOutputImage>
void 
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::PrepareData(void) 
{
	NMDebugCtx(ctx, << "...");

	InputImagePointer inputImage = dynamic_cast< TInputImage *>(this->ProcessObject::GetInput(0));

	OutputImagePointer distanceMap = dynamic_cast< TOutputImage*>(this->GetOutput());
	distanceMap->CopyInformation(inputImage);
	distanceMap->SetBufferedRegion(distanceMap->GetRequestedRegion());
	distanceMap->Allocate();

	VectorImagePointer offImg = this->m_OffsetImage;
	offImg->CopyInformation(inputImage);
	//offImg->SetLargestPossibleRegion(distanceMap->GetLargestPossibleRegion());
	offImg->SetRequestedRegion(distanceMap->GetRequestedRegion());
	offImg->SetBufferedRegion(distanceMap->GetRequestedRegion());
	offImg->Allocate();

	typename InputImageType::RegionType inrr =
			inputImage->GetRequestedRegion();
	NMDebugAI(<< "input requested region ...");
	inrr.Print(std::cout, itk::Indent(2));
	NMDebugAI(<< "input buffered region ...");
	inputImage->GetBufferedRegion().Print(std::cout, itk::Indent(2));

	typename OutputImageType::RegionType outrr =
			distanceMap->GetRequestedRegion();
	NMDebugAI(<< "output requested region ...");
	inrr.Print(std::cout, itk::Indent(2));
	NMDebugAI(<< "output buffered region ...");
	distanceMap->GetBufferedRegion().Print(std::cout, itk::Indent(2));


	typename VectorImageType::RegionType offrr =
			offImg->GetRequestedRegion();
	NMDebugAI(<< "offest image requested region ...");
	offrr.Print(std::cout, itk::Indent(2));
	NMDebugAI(<< "offest image buffered region ...");
	offImg->GetBufferedRegion().Print(std::cout, itk::Indent(2));

	typename OutputImageType::PixelType maxDist =
			std::numeric_limits<typename OutputImageType::PixelType>::max();

	ImageRegionConstIteratorWithIndex<TInputImage> it(inputImage, inrr);
	ImageRegionIteratorWithIndex<TOutputImage> ot(distanceMap, outrr);
	ImageRegionIteratorWithIndex<VectorImageType> vt(offImg, offrr);

	it.GoToBegin();
	ot.GoToBegin();
	vt.GoToBegin();

	typename VectorImageType::PixelType maxoffVec;
	maxoffVec.Fill(itk::NumericTraits<typename VectorImageType::PixelType::ValueType>::max());
	typename VectorImageType::PixelType zerooffVec;
	zerooffVec.Fill(0.0);

	// looking for those objects
	NMDebugAI( << "looking for categories: ");
	for (unsigned int e=0; e < this->m_Categories.size(); ++e)
	{
		NMDebug(<< this->m_Categories[e] << " ");
	}
	NMDebug(<< endl);

	typename InputImageType::PixelType val;
	bool bobj = false;
	unsigned int npix = inrr.GetNumberOfPixels();
	unsigned int pixcnt = 0;
	while (!ot.IsAtEnd())
	{
		IndexType idx = it.GetIndex();
		for (unsigned int e=0; e < this->m_Categories.size(); ++e)
		{
			if (static_cast<double>(it.Get()) == this->m_Categories[e])
			{
				bobj = true;
				break;
			}
		}

		if (bobj || (this->m_Categories.size() == 0 && it.Get() > 0))
		{
			//distanceMap->SetPixel(idx, static_cast<typename OutputImageType::PixelType>(0));
			ot.Set(static_cast<typename OutputImageType::PixelType>(0));
			vt.Set(zerooffVec);
			//offImg->SetPixel(idx, zerooffVec);
		}
		else
		{
			//distanceMap->SetPixel(idx, itk::NumericTraits<typename OutputImageType::PixelType>::max());
			ot.Set(itk::NumericTraits<typename OutputImageType::PixelType>::max());
			vt.Set(maxoffVec);
			//offImg->SetPixel(idx, maxoffVec);
		}

		bobj = false;
		++it;
		++ot;
		++vt;
		++pixcnt;
	}

	NMDebugCtx(ctx, << "done!");
}

//template <class TInputImage, class TOutputImage>
//void
//DanielssonCostDistanceMapImageFilter<TInputImage, TOutputImage>
//::GenerateInputRequestedRegion() throw(itk::InvalidRequestedRegionError)
//{
//	Superclass::GenerateInputRequestedRegion();
//
//	OutputImageType* out = dynamic_cast< TOutputImage* >(this->GetOutput());
//	InputImageType* in = dynamic_cast< TInputImage* >(this->ProcessObject::GetInput(0));
//
//	if (out != 0)
//		in->SetRequestedRegion(out->GetRequestedRegion());
//
//}
//
//
template <class TInputImage, class TOutputImage>
void
DanielssonCostDistanceMapImageFilter<TInputImage, TOutputImage>
::EnlargeOutputRequestedRegion(DataObject* output) // throw (itk::InvalidRequestedRegionError)
{
	Superclass::EnlargeOutputRequestedRegion(output);

	NMDebugCtx(ctx, << "...");
	// call the superclass' implementation of this method
	//Superclass::EnlargeOutputRequestedRegion(output);

	// get pointers to the input and output
	OutputImagePointer outputImage = dynamic_cast< TOutputImage *>(output);

	if (outputImage.IsNull())
	{
		return;
	}

	//inputImage->SetRequestedRegion(inputImage->GetLargestPossibleRegion());

	RegionType lpr = outputImage->GetLargestPossibleRegion();
	RegionType rr = outputImage->GetRequestedRegion();

	NMDebugAI( << "the 'original' output requested region ..." << endl);
	rr.Print(std::cout, itk::Indent(2));

	// make sure that the requested region covers the whole width of the largest possible
	// region
	rr.SetSize(0, lpr.GetSize()[0]);
	rr.SetIndex(0, lpr.GetIndex()[0]);

	NMDebugAI( << "the 'enlarged' output requested region ..." << endl);
	rr.Print(std::cout, itk::Indent(2));

	outputImage->SetRequestedRegion(rr);


	//// crop the input requested region at the input's largest possible region
	//if (inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()))
	//{
	//	inputPtr->SetRequestedRegion(inputRequestedRegion);
	//	return;
	//}
	//else
	//{
	//	// Couldn't crop the region (requested region is outside the largest
	//	// possible region).  Throw an exception.
    //
	//	// store what we tried to request (prior to trying to crop)
	//	inputPtr->SetRequestedRegion(inputRequestedRegion);
    //
	//	// build an exception
	//	itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
	//	e.SetLocation(ITK_LOCATION);
	//	e.SetDescription(
	//			"Requested region is (at least partially) outside the largest possible region.");
	//	e.SetDataObject(inputPtr);
	//	throw e;
	//}

	NMDebugCtx(ctx, << "done!");
}


/**
 *  Compute Distance and Voronoi maps
 */
template <class TInputImage,class TOutputImage>
void 
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::GenerateData() 
{
	NMDebugCtx(ctx, << "...");

	this->PrepareData();
	// Specify images and regions.

	OutputImagePointer distanceMap = this->GetDistanceMap();
	VectorImagePointer offImg = this->m_OffsetImage;//dynamic_cast<VectorImageType*>(this->GetOutput(1));

	typename InputImageType::RegionType region =
			distanceMap->GetRequestedRegion();


	// Instantiate reflective iterator
	ReflectiveImageRegionIterator<OutputImageType> it(distanceMap,
			region);
	typename OutputImageType::OffsetType ooffset;
	for (unsigned int dim = 0; dim < OutputImageType::ImageDimension; dim++)
	{
		if (region.GetSize()[dim] > 1)
		{
			ooffset[dim] = 0;
		}
		else
		{
			ooffset[dim] = 0;
		}
	}
	it.SetBeginOffset(ooffset);
	it.SetEndOffset(ooffset);

	// Support progress methods/callbacks.

	// Each pixel is visited 2^InputImageDimension times, and the number
	// of visits per pixel needs to be computed for progress reporting.
	unsigned long visitsPerPixel = (1 << InputImageDimension);
	unsigned long updateVisits, i = 0;
	updateVisits = region.GetNumberOfPixels() * visitsPerPixel / 10;
	if (updateVisits < 1)
	{
		updateVisits = 1;
	}
	const float updatePeriod = static_cast<float>(updateVisits) * 10.0;

	std::vector<OffsetType> noff;
	noff.resize(3);
	OffsetType o1={{0,0}}, o2={{0,0}}, o3={{0,0}};
	noff[0] = o1; noff[1] = o2; noff[2] = o3;

	typename VectorImageType::PixelType::ValueType minDist =
			itk::NumericTraits<typename VectorImageType::PixelType::ValueType>::max();
	typename VectorImageType::PixelType::ValueType neighbour;
	typename VectorImageType::PixelType::ValueType disthere2;
	typename VectorImageType::PixelType neighbourVec;
	typename VectorImageType::PixelType vecbuf[3];
	typename VectorImageType::PixelType::ValueType tmpDist;
	typename VectorImageType::PixelType distVecHere;
	typename OutputImageType::PixelType cost;
	float prog =0;

	// Process image.
	NMDebugAI(<< "computing distances from objects ..." << endl);
	it.GoToBegin();
	while (!it.IsAtEnd())
	{
		if (it.Get() == 0)
		{
			++it;
			continue;
		}

		// get the cost for the central pixel
		// ToDo:  to be replaced with cost value from further input image
		cost = 1.0;

		// define neighbouring input pixel indices of x
		/*  index map of neighbourhood pixels
		 *  0 1 2
		 *  3 x 5
		 *  6 7 8
		 */

		// upwards
		if (it.IsReflected(1))
		{
			// right to left
			if (it.IsReflected(0))
			{
				noff[0][0] = 1; noff[0][1] = 0; // idx 5
				noff[1][0] = 0; noff[1][1] = 1; // idx 7
				noff[2][0] = 1; noff[2][1] = 1; // idx 8
			}
			// left to right
			else
			{
				noff[0][0] = -1; noff[0][1] = 0; // idx 3
				noff[1][0] = -1; noff[1][1] = 1; // idx 6
				noff[2][0] =  0; noff[2][1] = 1; // idx 7
			}
		}
		// downwards
		else
		{
			// right to left
			if (it.IsReflected(0))
			{
				noff[0][0] = 0; noff[0][1] = -1; // idx 1
				noff[1][0] = 1; noff[1][1] = -1; // idx 2
				noff[2][0] = 1; noff[2][1] =  0; // idx 5
			}
			// left to right
			else
			{
				noff[0][0] = -1; noff[0][1] = -1; // idx 0
				noff[1][0] =  0; noff[1][1] = -1; // idx 1
				noff[2][0] = -1; noff[2][1] =  0; // idx 3
			}
		}

		// calc the actual distance value from
		// sourrounding pixels
		IndexType here = it.GetIndex();
		unsigned char si = 9;
		unsigned char bNeighbour = 0;
		minDist = itk::NumericTraits<typename VectorImageType::PixelType::ValueType>::max();
		for (unsigned char c=0; c < 3; ++c)
		{
			if (region.IsInside(here + noff[c]))
			{
				neighbourVec = offImg->GetPixel(here + noff[c]);
				for (unsigned int d=0; d < InputImageDimension; ++d)
					neighbourVec[d] += vnl_math_abs(noff[c][d]);

				vecbuf[c] = neighbourVec;
				tmpDist = (neighbourVec[0] * neighbourVec[0]) + (neighbourVec[1] * neighbourVec[1]);
				bNeighbour = 1;


				if (tmpDist < minDist)
				{
					si = c;
					minDist = tmpDist;
				}
			}
		}

		if (bNeighbour)
		{
			distVecHere = offImg->GetPixel(here);
			disthere2 = (distVecHere[0] * distVecHere[0]) + (distVecHere[1] * distVecHere[1]);
			if (minDist < disthere2)
			{
				minDist = ::sqrt(minDist);
				it.Set(static_cast<typename OutputImageType::PixelType>(minDist));
				offImg->GetPixel(here) = vecbuf[si];
			}
		}

		// DEBUG
		//NMDebug(<< here[0] << "," << here[1] << ": ")
		//for (int i=0; i<3; ++i)
		//{   if (region.IsInside(here + noff[i]))
		//	{
		//		NMDebug(<< distanceMap->GetPixel(here + noff[i]) << " ");
		//	}
		//	else
		//	{
		//		NMDebug(<< "nil" << " ");
		//	}
		//}
		//NMDebug(<< endl);

		++it;
	}

	NMDebugCtx(ctx, << "done!");

} // end GenerateData()

/**
 *  Print Self
 */
template <class TInputImage,class TOutputImage>
void 
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  
  os << indent << "Danielsson Distance: " << std::endl;
  os << indent << "Input Is Binary   : " << m_InputIsBinary << std::endl;
  os << indent << "Use Image Spacing : " << m_UseImageSpacing << std::endl;
  os << indent << "Squared Distance  : " << m_SquaredDistance << std::endl;

}

} // end namespace itk

#endif
