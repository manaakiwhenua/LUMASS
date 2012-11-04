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
#include "itkImageRegionConstIteratorWithIndex.h"
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

//  OutputImagePointer voronoiMap = OutputImageType::New();
//  this->SetNthOutput( 1, voronoiMap.GetPointer() );
//
//  VectorImagePointer distanceVectors = VectorImageType::New();
//  this->SetNthOutput( 2, distanceVectors.GetPointer() );

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

	OutputImagePointer distanceMap = this->GetDistanceMap();

	distanceMap->SetLargestPossibleRegion(
			inputImage->GetLargestPossibleRegion());

	distanceMap->SetRequestedRegion(inputImage->GetRequestedRegion());

	distanceMap->SetBufferedRegion(inputImage->GetBufferedRegion());

	distanceMap->Allocate();

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

	ImageRegionConstIteratorIndex<TInputImage> it(inputImage, region);
	ImageRegionIteratorIndex<TOutputImage> ot(distanceMap, region);

	it.GoToBegin();
	ot.GoToBegin();

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
			ot.Set(static_cast<typename OutputImageType::PixelType>(0));
		else
			ot.Set(maxDist);

		bobj = false;
		++it;
		++ot;
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
	typename InputImageType::RegionType region =
			distanceMap->GetRequestedRegion();

	// Instantiate reflective iterator
	ReflectiveImageRegionConstIterator<VectorImageType> it(distanceMap,
			region);
	typename OutputImageType::OffsetType ooffset;
	for (unsigned int dim = 0; dim < OutputImageType::ImageDimension; dim++)
	{
		if (region.GetSize()[dim] > 1)
		{
			ooffset[dim] = 1;
		}
		else
		{
			ooffset[dim] = 0;
		}
	}
	it.SetBeginOffset(ooffset);
	it.SetEndOffset(ooffset);

	it.GoToBegin();

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

	// Process image.
	NMDebugAI(<< "computing distances from objects ...")
	while (!it.IsAtEnd())
	{

		if (!(i % updateVisits))
		{
			this->UpdateProgress((float) i / updatePeriod);
		}

		IndexType here = it.GetIndex();
		for (unsigned int dim = 0; dim < VectorImageType::ImageDimension; dim++)
		{
			if (region.GetSize()[dim] <= 1)
			{
				continue;
			}
			if (it.IsReflected(dim))
			{
				offset[dim]++;
				UpdateLocalDistance(distanceComponents, here, offset);
				offset[dim] = 0;
			}
			else
			{
				offset[dim]--;
				UpdateLocalDistance(distanceComponents, here, offset);
				offset[dim] = 0;
			}
		}
		++it;
		++i;
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
