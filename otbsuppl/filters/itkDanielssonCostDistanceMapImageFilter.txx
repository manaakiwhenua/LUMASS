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

  this->SetNumberOfRequiredOutputs( 2 );

  OutputImagePointer distanceMap = OutputImageType::New();
  this->SetNthOutput( 0, distanceMap.GetPointer() );

//  OutputImagePointer voronoiMap = OutputImageType::New();
//  this->SetNthOutput( 1, voronoiMap.GetPointer() );
//
  this->m_OffsetImage = VectorImageType::New();
//  VectorImagePointer distanceVectors = VectorImageType::New();
//  this->SetNthOutput( 1, distanceVectors.GetPointer() );

  m_SquaredDistance     = false;
  m_InputIsBinary       = false;
  m_UseImageSpacing     = false;
//  m_ComputeVoronoi		= false;
  //m_VectorDistance		= false;
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
 *  Return Closest Points Map
 */
//template <class TInputImage,class TOutputImage>
//typename
//DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>::OutputImageType*
//DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
//::GetVoronoiMap(void)
//{
//  return  dynamic_cast< OutputImageType * >(
//    this->ProcessObject::GetOutput(1) );
//}

/**
 *  Return the distance vectors
 */
//template <class TInputImage,class TOutputImage>
//typename DanielssonCostDistanceMapImageFilter<
//  TInputImage,TOutputImage>::VectorImageType *
//DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
//::GetVectorDistanceMap(void)
//{
//  return  dynamic_cast< VectorImageType * >(
//    this->ProcessObject::GetOutput(2) );
//}

//template <class TInputImage,class TOutputImage>
//void
//DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
//::EnlargeOutputRequestedRegion(DataObject* data)
//{
//	Superclass::EnlargeOutputRequestedRegion(data);
//	data->SetRequestedRegionToLargestPossibleRegion();
//}

/**
 *  Prepare data for computation
 */
template <class TInputImage,class TOutputImage>
void 
DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
::PrepareData(void) 
{
	NMDebugCtx(ctx, << "...");

	InputImagePointer inputImage =
			dynamic_cast<const TInputImage *>(ProcessObject::GetInput(0));
	VectorImagePointer offImg = this->m_OffsetImage;
//			dynamic_cast< VectorImageType *>(this->GetOutput(1));

	OutputImagePointer distanceMap = this->GetDistanceMap();

	distanceMap->SetLargestPossibleRegion(
			inputImage->GetLargestPossibleRegion());
	offImg->SetLargestPossibleRegion(
			inputImage->GetLargestPossibleRegion());

	distanceMap->SetRequestedRegion(inputImage->GetRequestedRegion());
	offImg->SetRequestedRegion(inputImage->GetRequestedRegion());

	distanceMap->SetBufferedRegion(inputImage->GetBufferedRegion());
	offImg->SetBufferedRegion(inputImage->GetBufferedRegion());

	distanceMap->Allocate();
	offImg->Allocate();

	typename OutputImageType::RegionType region =
			distanceMap->GetRequestedRegion();

	region.Print(std::cout, itk::Indent(2));

	// find the largest of the image dimensions
	typename TInputImage::SizeType size = region.GetSize();
	unsigned int maxLength = 0;
	for (unsigned int dim = 0; dim < TInputImage::ImageDimension; dim++)
	{
		if (maxLength < size[dim])
		{
			maxLength = size[dim];
		}
	}

	typename OutputImageType::PixelType maxDist =
			std::numeric_limits<typename OutputImageType::PixelType>::max();

	ImageRegionConstIterator<TInputImage> it(inputImage, region);
	ImageRegionIterator<TOutputImage> ot(distanceMap, region);
	ImageRegionIterator<VectorImageType> vt(offImg, region);


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
	while (!ot.IsAtEnd())
	{
		for (unsigned int e=0; e < this->m_Categories.size(); ++e)
		{
			if (it.Get() == this->m_Categories[e])
			{
				bobj = true;
				break;
			}
		}

		if (bobj)
		{
			ot.Set(static_cast<typename OutputImageType::PixelType>(0));
			vt.Set(zerooffVec);
		}
		else if (this->m_Categories.size() == 0 && it.Get() > 0)
		{
			ot.Set(static_cast<typename OutputImageType::PixelType>(0));
			vt.Set(zerooffVec);
		}
		else
		{
			ot.Set(itk::NumericTraits<typename OutputImageType::PixelType>::max());
			vt.Set(maxoffVec);
		}

		bobj = false;
		++it;
		++ot;
		++vt;
	}

	NMDebugCtx(ctx, << "done!");
}

/**
 *  Post processing for computing the Voronoi Map
 */
//template <class TInputImage,class TOutputImage>
//void
//DanielssonCostDistanceMapImageFilter<TInputImage,TOutputImage>
//::ComputeVoronoiMap()
//{
//  itkDebugMacro( << "ComputeVoronoiMap Start");
//  OutputImagePointer    voronoiMap          =  this->GetVoronoiMap();
//  OutputImagePointer    distanceMap         =  this->GetDistanceMap();
//  VectorImagePointer    distanceComponents  =  this->GetVectorDistanceMap();
//
//  typename OutputImageType::RegionType region  = voronoiMap->GetRequestedRegion();
//
//  ImageRegionIteratorWithIndex< OutputImageType >  ot( voronoiMap,          region );
//  ImageRegionIteratorWithIndex< VectorImageType >  ct( distanceComponents,  region );
//  ImageRegionIteratorWithIndex< OutputImageType >  dt( distanceMap,         region );
//
//  typename InputImageType::SpacingType spacing = Self::GetInput()->GetSpacing();
//
//  itkDebugMacro( << "ComputeVoronoiMap Region: " << region);
//  ot.GoToBegin();
//  ct.GoToBegin();
//  dt.GoToBegin();
//  while( ! ot.IsAtEnd() )
//    {
//    IndexType index = ct.GetIndex() + ct.Get();
//    if( region.IsInside( index ) )
//      {
//      ot.Set( voronoiMap->GetPixel( index ) );
//      }
//
//    OffsetType distanceVector = ct.Get();
//    double distance = 0.0;
//    if (m_UseImageSpacing)
//      {
//      for(unsigned int i=0; i<InputImageDimension; i++)
//        {
//        double spacingComponent = static_cast< double >(spacing[i]);
//        distance += distanceVector[i] * distanceVector[i] * spacingComponent * spacingComponent;
//        }
//      }
//    else
//      {
//      for(unsigned int i=0; i<InputImageDimension; i++)
//        {
//        distance += distanceVector[i] * distanceVector[i];
//        }
//      }
//
//    if( m_SquaredDistance )
//      {
//      dt.Set( static_cast<typename OutputImageType::PixelType>( distance ) );
//      }
//    else
//      {
//      dt.Set( static_cast<typename OutputImageType::PixelType>(vcl_sqrt( distance )) );
//      }
//    ++ot;
//    ++ct;
//    ++dt;
//    }
//  itkDebugMacro( << "ComputeVoronoiMap End");
//}

/**
 *  Locally update the distance.
 */
//template <class TInputImage,class TOutputImage>
//void
//DanielssonCostDistanceMapImageFilter<TInputImage, TOutputImage>
//::UpdateLocalDistance(VectorImageType* components,
//                      const IndexType& here,
//                      const OffsetType& offset)
//{
//  IndexType  there            = here + offset;
//  OffsetType offsetValueHere  = components->GetPixel( here  );
//  OffsetType offsetValueThere = components->GetPixel( there ) + offset;
//
//	  unsigned int k=0;
//	  NMDebugAI(<< "index here: ");
//	  for (k=0; k < InputImageDimension; ++k)
//		  NMDebug(<< here[k] << " ");
//	  NMDebug(<< " | offset value: ");
//	  for (k=0; k < InputImageDimension; ++k)
//		  NMDebug(<< offsetValueHere[k] << " ");
//	  NMDebug(<< endl);
//
//
//	  NMDebugAI(<< "index there: ");
//	  for (k=0; k < InputImageDimension; ++k)
//		  NMDebug(<< there[k] << " ");
//	  NMDebug(<< " | offset value: ");
//	  for (k=0; k < InputImageDimension; ++k)
//		  NMDebug(<< offsetValueThere[k] << " ");
//	  NMDebug(<< endl);
//
//
//  typename InputImageType::SpacingType spacing = Self::GetInput()->GetSpacing();
//
//  double norm1 = 0.0;
//  double norm2 = 0.0;
//  for( unsigned int i=0; i<InputImageDimension; i++ )
//    {
//    double v1 = static_cast< double >(  offsetValueHere[ i]  );
//    double v2 = static_cast< double >(  offsetValueThere[i] );
//
//    if (m_UseImageSpacing)
//      {
//      double spacingComponent = static_cast< double >(spacing[i]);
//      v1 *= spacingComponent;
//      v2 *= spacingComponent;
//      }
//
//    norm1 += v1 * v1;
//    norm2 += v2 * v2;
//	    NMDebugAI(<< "#" << i << ": v1: " << v1
//	    		              <<  " v2: " << v2 << endl);
//    }
//
//  NMDebugAI(<< "norm1: " << norm1 << " norm2: " << norm2 << endl);
//  if( norm1 > norm2 )
//    {
//    components->GetPixel( here ) = offsetValueThere;
//    }
//
//  NMDebug(<< endl);
//
//}
template <class TInputImage, class TOutputImage>
void
DanielssonCostDistanceMapImageFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw (itk::InvalidRequestedRegionError)
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
  inputRequestedRegion.PadByRadius( 1 );

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

	unsigned char nidx[3];
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
				noff[0][0] = 1; noff[0][1] = 0; nidx[0] = 0; // idx 5
				noff[1][0] = 0; noff[1][1] = 1; nidx[1] = 0; // idx 7
				noff[2][0] = 1; noff[2][1] = 1; nidx[2] = 1; // idx 8
			}
			// left to right
			else
			{
				noff[0][0] = -1; noff[0][1] = 0; nidx[0] = 0; // idx 3
				noff[1][0] = -1; noff[1][1] = 1; nidx[1] = 0; // idx 6
				noff[2][0] =  0; noff[2][1] = 1; nidx[2] = 1; // idx 7
			}
		}
		// downwards
		else
		{
			// right to left
			if (it.IsReflected(0))
			{
				noff[0][0] = 0; noff[0][1] = -1; nidx[0] = 0; // idx 1
				noff[1][0] = 1; noff[1][1] = -1; nidx[1] = 1; // idx 2
				noff[2][0] = 1; noff[2][1] =  0; nidx[2] = 0; // idx 5
			}
			// left to right
			else
			{
				noff[0][0] = -1; noff[0][1] = -1; nidx[0] = 1; // idx 0
				noff[1][0] =  0; noff[1][1] = -1; nidx[1] = 0; // idx 1
				noff[2][0] = -1; noff[2][1] =  0; nidx[2] = 0; // idx 3
			}
		}

		// calc the actual distance value from
		// sourrounding pixels
		IndexType here = it.GetIndex();
		unsigned char si = 0;
		minDist = itk::NumericTraits<typename VectorImageType::PixelType::ValueType>::max();
		for (unsigned char c=0; c < 3; ++c)
		{
			neighbourVec = offImg->GetPixel(here + noff[c]);
			//neighbour2 = (neighbourVec[0] * neighbourVec[0]) + (neighbourVec[1] * neighbourVec[1]);
			//vecbuf[c] = neighbourVec;
			if (!region.IsInside(here + noff[c]))// || (neighbour < 0))
			{
				tmpDist = itk::NumericTraits<typename VectorImageType::PixelType::ValueType>::max();
			}
			else
			{
				for (unsigned int d=0; d < InputImageDimension; ++d)
					neighbourVec[d] += vnl_math_abs(noff[c][d]);

				vecbuf[c] = neighbourVec;
				tmpDist = (neighbourVec[0] * neighbourVec[0]) + (neighbourVec[1] * neighbourVec[1]);
			}

			if (tmpDist < minDist)
			{
				si = c;
				minDist = tmpDist;
			}
		}

		distVecHere = offImg->GetPixel(here);
		disthere2 = (distVecHere[0] * distVecHere[0]) + (distVecHere[1] * distVecHere[1]);
		if (minDist < disthere2)
		{
			minDist = ::sqrt(minDist);
			it.Set(static_cast<typename OutputImageType::PixelType>(minDist));
			offImg->GetPixel(here) = vecbuf[si];
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
