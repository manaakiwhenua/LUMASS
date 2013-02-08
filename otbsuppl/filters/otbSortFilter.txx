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
/*
 * SortFilter.txx
 *
 *  Created on: 04/02/2013
 *      Author: alex
 */

#ifndef __otbSortFilter_txx
#define __otbSortFilter_txx

#include "nmlog.h"
#include "otbSortFilter.h"
#include "otbStreamingRATImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkExceptionObject.h"
#include "itkDataObject.h"
#include "itkImageSource.h"

namespace otb {

template <class TInputImage, class TOutputImage>
SortFilter<TInputImage, TOutputImage>
::SortFilter()
 	 : m_SortAscending(false)
{
}

template <class TInputImage, class TOutputImage>
SortFilter<TInputImage, TOutputImage>
::~SortFilter()
{
}

template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "SortFilter - sorting " << this->GetNumberOfInputs()
		<< " layers." << std::endl;
}

template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::GenerateData(void)
{
	this->AllocateOutputs();

	this->BeforeThreadedGenerateData();

	ThreadStruct str;
	str.Filter = this;

	this->GetMultiThreader()->SetNumberOfThreads(this->GetNumberOfThreads());
	this->GetMultiThreader()->SetSingleMethod(this->CalledFromThreader, &str);
	this->GetMultiThreader()->SingleMethodExecute();

	this->AfterThreadedGenerateData();
}



template <class TInputImage, class TOutputImage>
ITK_THREAD_RETURN_TYPE
SortFilter<TInputImage, TOutputImage>
::CalledFromThreader( void *arg )
{
	ThreadStruct *str;
	int total, threadId, threadCount;

	threadId = ((itk::MultiThreader::ThreadInfoStruct *)(arg))->ThreadID;
	threadCount = ((itk::MultiThreader::ThreadInfoStruct *)(arg))->NumberOfThreads;

	str = (ThreadStruct *)(((itk::MultiThreader::ThreadInfoStruct *)(arg))->UserData);

	long offset = 0, length;
	length = str->Filter->SplitArray(threadId, threadCount, &offset);

	if (length > 0)
	{
		str->Filter->ProcessThreaded(threadId, offset,
				length);
	}

	return ITK_THREAD_RETURN_VALUE;
}

template <class TInputImage, class TOutputImage>
long SortFilter<TInputImage, TOutputImage>
::SplitArray(long threadId, long numSplits, long* offset)
{
	long len = 0;
	if (threadId >= mArrayOffsets.size())
	{
		//mArrayOffsets.resize(threadId+1);
		//mChunkSize.resize(threadId+1);
		return len;
	}

	len = this->mChunkSize[threadId];
	*offset = this->mArrayOffsets[threadId];

	return len;
}


template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::AllocateOutputs(void)
{
	NMDebugCtx(ctxSortFilter, << "...");

	// first let's set the number of outputs we're producing
	this->SetNumberOfOutputs(this->GetNumberOfInputs() + 1);

	// we get the first input image and use its props as stencil for
	// outputs, except for the index layer;
	InputImagePointer input = const_cast<InputImageType*>(this->GetInput(0));

	for (int i=0; i < this->GetNumberOfInputs(); ++i)
	{
		OutputImagePointer output = OutputImageType::New();
		output->CopyInformation(input);
		output->SetRequestedRegion(input->GetRequestedRegion());
		output->SetBufferedRegion(input->GetBufferedRegion());
		output->Allocate();
		this->SetNthOutput(i, dynamic_cast<itk::DataObject*>(output.GetPointer()));
	}

	IndexImagePointer indexImg = IndexImageType::New();
	indexImg->CopyInformation(input);
	indexImg->SetRequestedRegion(input->GetRequestedRegion());
	indexImg->SetBufferedRegion(input->GetBufferedRegion());
	indexImg->Allocate();
	this->SetNthOutput(this->GetNumberOfOutputs()-1,
			dynamic_cast<itk::DataObject*>(indexImg.GetPointer()));

	NMDebugCtx(ctxSortFilter, << "done!");
}


template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData(void)
{
	NMDebugCtx(ctxSortFilter, << "...");

	OutputImagePointer out = dynamic_cast<OutputImageType*>(this->GetOutput());
	OutputImageRegionType orr = out->GetBufferedRegion();
	long numpix = orr.GetNumberOfPixels();
	long nbands = out->GetNumberOfComponentsPerPixel();
	if (nbands > 1)
	{
		itk::ExceptionObject e;
		e.SetLocation("SortFilter< >::BeforeThreadedGenerateData()");
		e.SetDescription("SortFilter doesn't support vector (i.e. multi-band) images!");
		throw e;
	}

	long pixsize = sizeof(OutputImagePixelType);
	long arraysize = pixsize * numpix;
//	long numchunks = 1;// = arraysize / m_MaxMem;
//	long lenchunk = numpix; //= numpix / numchunks;
//	long rest = 0;
//	if (arraysize > m_MaxMem)
//	{
//		numchunks = arraysize / m_MaxMem;
//		lenchunk = numpix / numchunks;
//		rest = numpix - (lenchunk * numchunks);
//		if (rest > 0)
//			numchunks++;
//	}

	int numThreads = this->GetNumberOfThreads();
	long numchunks = numThreads;
	long lenchunk = numpix / numchunks;
	if (numpix % numThreads)
		lenchunk = numpix / (numchunks-1);
	long rest = numpix - ((numchunks-1) * lenchunk);
	if (rest == 0)
		numchunks -= 1;

	long offset = 0;
	long chunklen = lenchunk;
	for (int s=0; s < numchunks; ++s)
	{
		mArrayOffsets.push_back(offset);
		mChunkSize.push_back(chunklen);
		offset += chunklen;
		if (s == numchunks - 2 && rest > 0)
		{
			chunklen = rest;
		}
	}

	//this->SetNumberOfThreads(numchunks);

	if (rest > 0)
	{
		NMDebugAI(<< "numpix = lenchunk * numchunks-1 + rest" << endl);
		NMDebugAI(<< numpix << " = " << lenchunk << " * " << numchunks
				<< " - 1 " << " + " << rest << endl);
	}
	else
	{
		NMDebugAI(<< "numpix = lenchunk * numchunks" << endl);
		NMDebugAI(<< numpix << " = " << lenchunk << " * " << numchunks
				 << endl);
	}
	NMDebugAI(<< "pixsize: " << pixsize << " bytes" << endl);
	NMDebugAI(<< "arraysize: " << arraysize << " bytes" << endl);

	NMDebugCtx(ctxSortFilter, << "done!");
}

template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::ProcessThreaded(long threadId, long offset, long length)
{

	NMDebug(<< threadId << ": offset = " << offset << " length = " << length << endl);

	// create a vector of array pointers, pointing at the very chunk of the
	// input and index image data, this thread is responsible for
	std::vector<InputImagePixelType*> inputArrays;
	IndexImagePixelType* idxArray;
	for (int i=0; i < this->GetNumberOfInputs(); ++i)
	{
		InputImagePointer img = const_cast<InputImageType*>(this->GetInput(i));
		InputImagePixelType* buf =
			static_cast<InputImagePixelType*>(img->GetBufferPointer());
		inputArrays.push_back(buf);
	}

	IndexImagePointer idxImg = this->GetIndexImage();
	IndexImagePixelType* idxBuf =
			static_cast<IndexImagePixelType*>(idxImg->GetBufferPointer());
	for (long p=offset; p < offset+length; ++p)
	{
		idxBuf[p] = p;
	}

	if (m_SortAscending)
		this->SortRegionAscending(inputArrays, idxBuf, offset, offset+length-1);
	else
		this->SortRegionDescending(inputArrays, idxBuf, offset, offset+length-1);
}

template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::SortRegionDescending(std::vector<InputImagePixelType*> inArrs,
		IndexImagePixelType* idxBuf, long left, long right)
{
	InputImagePixelType val, mval;
	IndexImagePixelType idx;
	InputImagePixelType vals[inArrs.size()];
	InputImagePixelType* buf = inArrs[0];
	int numarrs = inArrs.size();

	long le = left;
	long ri = right;
	long mi = le + ((ri + 1 - le) / (int)2);
	mval = buf[mi];

	do
	{
		while (buf[le] > mval) ++le;
		while (buf[ri] < mval) --ri;
		if (le <= ri)
		{
			for (int i=0; i < numarrs; ++i)
				vals[i] = inArrs[i][le];
			idx = idxBuf[le];

			for (int i=0; i < numarrs; ++i)
				inArrs[i][le] = inArrs[i][ri];
			idxBuf[le] = idxBuf[ri];

			for (int i=0; i < numarrs; ++i)
				inArrs[i][ri] = vals[i];
			idxBuf[ri] = idx;

			++le;
			--ri;
		}
	} while (le <= ri);
	if (left < ri) this->SortRegionDescending(inArrs, idxBuf, left, ri);
	if (right > le) this->SortRegionDescending(inArrs, idxBuf, le, right);
}

template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::SortRegionAscending(std::vector<InputImagePixelType*> inArrs,
		IndexImagePixelType* idxBuf, long left, long right)
{
	InputImagePixelType val, mval;
	IndexImagePixelType idx;
	InputImagePixelType vals[inArrs.size()];
	InputImagePixelType* buf = inArrs[0];
	int numarrs = inArrs.size();

	long le = left;
	long ri = right;
	long mi = le + ((ri + 1 - le) / (int)2);
	mval = buf[mi];

	do
	{
		while (buf[le] < mval) ++le;
		while (buf[ri] > mval) --ri;
		if (le <= ri)
		{
			for (int i=0; i < numarrs; ++i)
				vals[i] = inArrs[i][le];
			idx = idxBuf[le];

			for (int i=0; i < numarrs; ++i)
				inArrs[i][le] = inArrs[i][ri];
			idxBuf[le] = idxBuf[ri];

			for (int i=0; i < numarrs; ++i)
				inArrs[i][ri] = vals[i];
			idxBuf[ri] = idx;

			++le;
			--ri;
		}
	} while (le <= ri);
	if (left < ri) this->SortRegionAscending(inArrs, idxBuf, left, ri);
	if (right > le) this->SortRegionAscending(inArrs, idxBuf, le, right);
}


template <class TInputImage, class TOutputImage>
void SortFilter<TInputImage, TOutputImage>
::AfterThreadedGenerateData(void)
{
	// create a vector of array pointers, pointing at the very chunk of the
	// input and index image data, this thread is responsible for
	std::vector<InputImagePixelType*> inputArrays;
	for (int i=0; i < this->GetNumberOfInputs(); ++i)
	{
		InputImagePointer img = const_cast<InputImageType*>(this->GetInput(i));
		InputImagePixelType* buf =
			static_cast<InputImagePixelType*>(img->GetBufferPointer());
		inputArrays.push_back(buf);
	}

	std::vector<OutputImagePixelType*> outputArrays;
	OutputImagePointer out;
	for (int i=0; i < this->GetNumberOfInputs(); ++i)
	{
		//InputImagePointer img = const_cast<InputImageType*>(this->GetInput(i));
		//this->GraftNthOutput(i, img);
		out = const_cast<OutputImageType*>(this->GetOutput(i));
		OutputImagePixelType* obuf =
				static_cast<OutputImagePixelType*>(out->GetBufferPointer());
		outputArrays.push_back(obuf);
	}

	IndexImagePointer idxIn = this->GetIndexImage();
	IndexImagePointer idxOut = IndexImageType::New();
	idxOut->CopyInformation(idxIn);
	idxOut->SetRequestedRegion(idxIn->GetRequestedRegion());
	idxOut->SetBufferedRegion(idxIn->GetBufferedRegion());
	idxOut->Allocate();

	IndexImagePixelType* idxBufIn = idxIn->GetBufferPointer();
	IndexImagePixelType* idxBufOut = idxOut->GetBufferPointer();

	int numchunks = mArrayOffsets.size();
	long numpix = out->GetBufferedRegion().GetNumberOfPixels();

	std::vector<long> offcnts;
	for (int r=0; r < numchunks; ++r)
		offcnts.push_back(0);

	// do the merging of the pre-sorted pieces
	OutputImagePixelType val, tmp;
	long pixcnt = 0;
	if (m_SortAscending)
	{
		while(pixcnt < numpix)
		{
			val = itk::NumericTraits<OutputImagePixelType>::max();
			int valIdx = -1;
			long offset = -1;
			for (int c=0; c < numchunks; ++c)
			{
				if (offcnts[c] < mChunkSize[c])
				{
					tmp = inputArrays[0][mArrayOffsets[c] + offcnts[c]];
					if (tmp <= val)
					{
						val = tmp;
						valIdx = c;
						offset = mArrayOffsets[c] + offcnts[c];
					}
				}
			}
			++offcnts[valIdx];

			for (int i=0; i < this->GetNumberOfInputs(); ++i)
			{
				outputArrays[i][pixcnt] = inputArrays[i][offset];
			}
			idxBufOut[pixcnt] = idxBufIn[offset];
			++pixcnt;
		}
	}
	else
	{
		while(pixcnt < numpix)
		{
			val = itk::NumericTraits<OutputImagePixelType>::min();
			int valIdx = -1;
			long offset = -1;
			for (int c=0; c < numchunks; ++c)
			{
				if (offcnts[c] < mChunkSize[c])
				{
					tmp = inputArrays[0][mArrayOffsets[c] + offcnts[c]];
					if (tmp >= val)
					{
						val = tmp;
						valIdx = c;
						offset = mArrayOffsets[c] + offcnts[c];
					}
				}
			}
			++offcnts[valIdx];

			for (int i=0; i < this->GetNumberOfInputs(); ++i)
			{
					outputArrays[i][pixcnt] = inputArrays[i][offset];
			}
			idxBufOut[pixcnt] = idxBufIn[offset];
			++pixcnt;
		}
	}

	//graft the overriden index image on the 'last' output slot
	this->GraftNthOutput(this->GetNumberOfInputs(), idxOut);
}

} // end namespace

#endif
