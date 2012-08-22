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
 * otbImage2DToCubeSliceFilter.txx
 *
 *  Created on: 27/05/2012
 *      Author: alex
 */

#ifndef __otbImage2DToCubeSliceFilter_txx
#define __otbImage2DToCubeSliceFilter_txx

#include "otbImage2DToCubeSliceFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace otb {

template <class TInputImage, class TOutputImage>
Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::Image2DToCubeSliceFilter()
{
	this->m_ZLevel = 0;
	this->m_ZOrigin = 0.0;
	this->m_ZSpacing = 1.0;
//	this->m_FirstZLevel = -1;
}

template <class TInputImage, class TOutputImage>
Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::~Image2DToCubeSliceFilter()
{
}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::GenerateOutputInformation(void)
{
	NMDebugCtx(ctxImage2DToCubeSliceFilter, << "...");

	//Superclass::GenerateOutputInformation();

	/*
	 * sizes, band, resolutions, etc.(?)
	 */

	if (InputImageDimension != 2 || OutputImageDimension != 3)
		return;

	TOutputImage* OutImg = const_cast<TOutputImage*>(this->GetOutput());
	TInputImage* InImg = const_cast<TInputImage*>(this->GetInput());

	if (!InImg)
	{
		NMDebugAI(<< "we're missing valid input here and pull out ..." << std::endl);
		NMDebugCtx(ctxImage2DToCubeSliceFilter, << "done!");
		return;
	}


	NMDebugAI(<< "GOI: largest possible region ..." << std::endl);
	InImg->GetLargestPossibleRegion().Print(std::cout, itk::Indent(2));
	NMDebug(<< std::endl << std::endl);

	NMDebugAI(<< "GOI: requested region ..." << std::endl);
	InImg->GetRequestedRegion().Print(std::cout, itk::Indent(2));
	NMDebug(<< std::endl << std::endl);

	NMDebugAI(<< "GOI: buffered region ..." << std::endl);
	InImg->GetBufferedRegion().Print(std::cout, itk::Indent(2));
	NMDebug(<< std::endl << std::endl);


	// generate info about the largest possible region
	OutputIndexType OutIdx;
	OutputSizeType OutSize;
	OutputRegionType OutRegion;
	OutputSpacingType OutSpacing;
	OutputPointType OutOrigin;

	InputRegionType InRegion = InImg->GetLargestPossibleRegion();
//	InputRegionType InRegion = InImg->GetRequestedRegion();
	InputSizeType InSize = InRegion.GetSize();
	InputIndexType InIdx = InRegion.GetIndex();
	InputSpacingType InSpacing = InImg->GetSpacing();
	InputPointType InOrigin = InImg->GetOrigin();

	OutIdx[0] = InIdx[0];
	OutIdx[1] = InIdx[1];
	OutIdx[2] = this->m_ZLevel;

	OutSize[0] = InSize[0];
	OutSize[1] = InSize[1];
	OutSize[2] = 1;

	OutSpacing[0] = InSpacing[0];
	OutSpacing[1] = InSpacing[0];
	OutSpacing[2] = this->m_ZSpacing;

	OutOrigin[0] = InOrigin[0];
	OutOrigin[1] = InOrigin[1];
	OutOrigin[2] = this->m_ZOrigin;

	OutRegion.SetSize(OutSize);
	OutRegion.SetIndex(OutIdx);

	OutImg->SetLargestPossibleRegion(OutRegion);
//	OutImg->SetRequestedRegion(OutRegion);
//	OutImg->SetRegions(OutRegion);
	OutImg->SetSpacing(OutSpacing);
	OutImg->SetOrigin(OutOrigin);

	NMDebugCtx(ctxImage2DToCubeSliceFilter, << "done!");
}

//template <class TInputImage, class TOutputImage>
//void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
//::GenerateInputRequestedRegion(void)
//{
////	TOutputImage* out = dynamic_cast<TOutputImage*>(this->GetOutput());
//	TInputImage* in = const_cast<TInputImage*>(this->GetInput());
//
//	//typename TOutputImage::RegionType outRR = out->GetRequestedRegion();
//	typename TInputImage::RegionType inRR = in->GetLargestPossibleRegion();
//
////	NMDebugAI(<< "GIRR: 'input' output RR ..." << endl);
////	outRR.Print(cout, itk::Indent(4));
////
////
////	typename TInputImage::SizeType inRRSize;
////	inRRSize[0] = outRR.GetSize()[0];
////	inRRSize[1] = outRR.GetSize()[1];
////
////	typename TInputImage::IndexType inRRIndex;
////	inRRIndex[0] = outRR.GetIndex()[0];
////	inRRIndex[1] = outRR.GetIndex()[1];
////
////	inRR.SetSize(inRRSize);
////	inRR.SetIndex(inRRIndex);
//	in->SetRequestedRegion(inRR);
//}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::EnlargeOutputRequestedRegion(itk::DataObject* output)
{
	Superclass::EnlargeOutputRequestedRegion(output);

	OutputImagePointer outImg = this->GetOutput();
	outImg->SetRequestedRegion(outImg->GetLargestPossibleRegion());

//	// show your hands ...
//	typename TOutputImage::RegionType outRR = out->GetRequestedRegion();
//	typename TInputImage::RegionType inRR = in->GetRequestedRegion();
//
//	// set the output largest possible region
//	typename TOutputImage::RegionType outLPR;// = out->GetLargestPossibleRegion();
//	typename TOutputImage::SizeType outLPRSize;
//	outLPRSize[0] = inRR.GetSize()[0];
//	outLPRSize[1] = inRR.GetSize()[1];
//	outLPRSize[2] = 1;
//
//	typename TOutputImage::IndexType outLPRIndex;
//	outLPRIndex[0] = inRR.GetIndex()[0];
//	outLPRIndex[1] = inRR.GetIndex()[1];
//	outLPRIndex[2] = 0;
//
//	outLPR.SetSize(outLPRSize);
//	outLPR.SetIndex(outLPRIndex);
//	out->SetLargestPossibleRegion(outLPR);
//
//	// set the output requested region
//	typename TOutputImage::RegionType oRR;// = tout->GetRequestedRegion();
//	typename TOutputImage::SizeType oRRSize;
//	oRRSize[0] = inRR.GetSize()[0];
//	oRRSize[1] = inRR.GetSize()[1];
//	oRRSize[2] = 1;
//
//	typename TOutputImage::IndexType oRRIndex;
//	oRRIndex[0] = inRR.GetIndex()[0];
//	oRRIndex[1] = inRR.GetIndex()[1];
//	oRRIndex[2] = this->m_ZLevel;
//
////	typename TOutputImage::PointType oRRPoint;
////	oRRPoint[0] = in->GetOrigin()[0];
////	oRRPoint[1] = in->GetOrigin()[1];
////	oRRPoint[2] = this->m_ZOrigin;
//
//	oRR.SetSize(oRRSize);
//	oRR.SetIndex(oRRIndex);
//	out->SetRequestedRegion(oRR);
//
//	// -------------------------------------------------
//	// some debugging output
////	NMDebugAI(<< "enlarge ORR: out LPR ... " << endl);
////	outLPR.Print(cout, itk::Indent(4));
//
//	NMDebugAI(<< "enlarge ORR: 'input' output RR ... " << endl);
//	outRR.Print(cout, itk::Indent(4));
//
//	NMDebugAI(<< "enlarge ORR: 'input' input RR ... " << endl);
//	inRR.Print(cout, itk::Indent(4));
//
////	NMDebugAI(<< "enlarge ORR: 'input' input LPR ... " << endl);
////	inLPR.Print(cout, itk::Indent(4));
//
//	NMDebugAI(<< "enlarge ORR: 'enlarged' output RR ..." << endl);
//	oRR.Print(cout, itk::Indent(4));
//
//	NMDebugAI(<< "enlarge ORR: 'enlarged' output LPR ..." << endl);
//	outLPR.Print(cout, itk::Indent(4));
//
////	reqRegion.Print(cout, itk::Indent(4));
////
////	typename TOutputImage::RegionType::IndexType idx;
////	idx[0] = inLPR.GetIndex()[0];
////	idx[1] = inLPR.GetIndex()[1];
////	idx[2] = this->m_ZLevel;
////	reqRegion.SetIndex(idx);
////
////	typename TOutputImage::PointType origin;
////	origin[0] = in->GetOrigin()[0];
////	origin[1] = in->GetOrigin()[1];
////	origin[2] = this->m_ZOrigin;
////	out->SetOrigin(origin);
////
////	typename TOutputImage::RegionType::SizeType size;
////	size[0] = inLPR.GetSize()[0];
////	size[1] = inLPR.GetSize()[1];
////	size[2] = 1;
////	reqRegion.SetSize(size);
////
////	out->SetRequestedRegion(reqRegion);
////	reqRegion.Print(cout, itk::Indent(4));
}

template <class TInputImage, class TOutputImage>
void Image2DToCubeSliceFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
		  int threadId )
{
	TInputImage* InImg = const_cast<TInputImage*>(this->GetInput());
	TOutputImage* OutImg = const_cast<TOutputImage*>(this->GetOutput());
//
	InputRegionType InReqRegion = InImg->GetRequestedRegion();
	InputRegionType InBufRegion = InImg->GetBufferedRegion();

//	if (threadId == 1)
//	{
//		NMDebugAI(<< "thread #" << threadId << " ..." << std::endl);
//
//		NMDebugAI(<< "requested region ..." << std::endl);
//		InReqRegion.Print(std::cout, itk::Indent(0));
//		NMDebug(<< std::endl << std::endl);
//
//		NMDebugAI(<< "buffered region ..." << std::endl);
//		InBufRegion.Print(std::cout, itk::Indent(0));
//		NMDebug(<< std::endl << std::endl);
//
//		NMDebugAI(<< "output region ..." << std::endl);
//		outputRegionForThread.Print(std::cout, itk::Indent(0));
//	}

	InputIndexType InIdx;
	InIdx[0] = outputRegionForThread.GetIndex()[0];
	InIdx[1] = outputRegionForThread.GetIndex()[1];

	InputSizeType InSize;
	InSize[0] = outputRegionForThread.GetSize()[0];
	InSize[1] = outputRegionForThread.GetSize()[1];

	InputRegionType InRegion;
	InRegion.SetSize(InSize);
	InRegion.SetIndex(InIdx);

	typedef itk::ImageRegionConstIterator<TInputImage> InIterType;
	typedef itk::ImageRegionIterator<TOutputImage> OutIterType;

	InIterType inIter(this->GetInput(), InRegion);
	OutIterType outIter(this->GetOutput(), outputRegionForThread);

	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

	for (inIter.GoToBegin(), outIter.GoToBegin();
			!inIter.IsAtEnd();
				++inIter, ++outIter)
	{
		outIter.Set(inIter.Get());
		progress.CompletedPixel();
	}
}



} // end of namespace

#endif // end of ifndef template instantiation
