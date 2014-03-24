/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2014 Landcare Research New Zealand Ltd
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



#ifndef __otbSumZonesFilter_txx
#define __otbSumZonesFilter_txx

#include <map>
#include "nmlog.h"
#include "otbSumZonesFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkMacro.h"

#define ctx "otb::SumZonesFilter"

namespace otb
{

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::SumZonesFilter()
{
	this->SetNumberOfRequiredInputs(2);
	this->SetNumberOfRequiredOutputs(0);
	//this->SetNumberOfThreads(1);
}

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::~SumZonesFilter()
{
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream& os, itk::Indent indent) const
{

}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetValueImage(const OutputImageType* image)
{
	mValueImage = const_cast<TOutputImage*>(image);
	this->SetNthInput(1, const_cast<TOutputImage*>(image));
}

//template< class TInputImage, class TOutputImage >
//void SumZonesFilter< TInputImage, TOutputImage >
//::SetInput(const InputImageType* image)
//{
//	mZoneImage = const_cast<TInputImage*>(image);
//
//
//	Superclass::ProcessObject::SetInput(0, const_cast<TInputImage*>(image));
//}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneImage(const InputImageType* image)
{
	mZoneImage = const_cast<TInputImage*>(image);
	this->SetNthInput(0, const_cast<TInputImage*>(image));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneTable(AttributeTable::Pointer tab)
 {
	mZoneTable = tab;
 }

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	NMDebugCtx(ctx, << "...");
	if (mZoneImage.IsNull())
	{
		itkExceptionMacro(<< "Zone Image has not been specified!");
		return;
	}

	if (mValueImage.IsNull())
	{
		itkExceptionMacro(<< "Value Image has not been specified!");
		NMDebugCtx(ctx, << "done!");
		return;
	}

	unsigned int numThreads = this->GetNumberOfThreads();

	mThreadValueStore.clear();
	for (int t=0; t < numThreads; ++t)
	{
		mThreadValueStore.push_back(ZoneMapType());
	}

	// for now, the size of the input regions must be exactly the same
	if (	(	mZoneImage->GetLargestPossibleRegion().GetSize(0)
			 !=  mValueImage->GetLargestPossibleRegion().GetSize(0))
		||  (   mZoneImage->GetLargestPossibleRegion().GetSize(1)
			 !=  mValueImage->GetLargestPossibleRegion().GetSize(1))
	   )
	{
		itkExceptionMacro(<< "ZoneImage and ValueImage dimensions don't match!");
		NMDebugCtx(ctx, << "done!");
		return;
	}

	NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, int threadId)
{
	//typename OutputImageType::Pointer output = this->GetOutput();

	if (threadId == 0)
	{
		NMDebugCtx(ctx, << "...");
		unsigned int xsize = outputRegionForThread.GetSize(0);
		unsigned int ysize = outputRegionForThread.GetSize(1);
		NMDebugAI(<< "analysing region of " << xsize << " x " << ysize
				<< " pixels ..." << std::endl);
		//NMDebugAI(<< "number of threads: " << this->GetNumberOfThreads() << std::endl);
	}
	typedef itk::ImageRegionConstIterator<TInputImage> InputIterType;
	InputIterType zoneIt(mZoneImage, outputRegionForThread);

	typedef itk::ImageRegionConstIterator<TOutputImage> OutputIterType;
	OutputIterType valueIt(mValueImage, outputRegionForThread);

	ZoneMapType& vMap = mThreadValueStore[threadId];
	ZoneMapTypeIterator mapIt;

	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	if (threadId == 0)
	{
		NMDebugAI(<< "start summarising ..." << std::endl);
	}

	zoneIt.GoToBegin();
	valueIt.GoToBegin();
	while (!zoneIt.IsAtEnd() && !valueIt.IsAtEnd() && !this->GetAbortGenerateData())
	{
		const ZoneKeyType zone = static_cast<ZoneKeyType>(zoneIt.Get());
		const double val = static_cast<double>(valueIt.Get());

		mapIt = vMap.find(zone);
		if (mapIt == vMap.end())
		{
			vMap[zone] = std::vector<double>(5,0);
		}
		std::vector<double>& params = vMap[zone];

		// min
		params[0] = params[0] < val ? params[0] : val;
		// max
		params[1] = params[1] > val ? params[1] : val;
		// sum_val
		params[2] += val;
		// sum_val^2
		params[3] += (val * val);
		// count
		params[4] += 1;

		++zoneIt;
		++valueIt;

		progress.CompletedPixel();
	}

	if (threadId == 0)
	{
		NMDebugCtx(ctx, << "done!");
	}
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
	NMDebugCtx(ctx, << "...");
	//if (mZoneTable.IsNull())
	{
		mZoneTable = AttributeTable::New();
	}

	//mZoneTable->SetBandNumber(1);
	//mZoneTable->SetImgFileName("ZoneTable");
	mZoneTable->AddColumn("rowidx", AttributeTable::ATTYPE_INT);
	// ToDo:: check for integer zone type earlier
	mZoneTable->AddColumn("zone", AttributeTable::ATTYPE_INT);
	mZoneTable->AddColumn("count", AttributeTable::ATTYPE_INT);
	mZoneTable->AddColumn("min", AttributeTable::ATTYPE_DOUBLE);
	mZoneTable->AddColumn("max", AttributeTable::ATTYPE_DOUBLE);
	mZoneTable->AddColumn("mean", AttributeTable::ATTYPE_DOUBLE);
	mZoneTable->AddColumn("stddev", AttributeTable::ATTYPE_DOUBLE);
	mZoneTable->AddColumn("sum", AttributeTable::ATTYPE_DOUBLE);


	// ToDo:: mmh, is there a better way of doing this?
	//        we need a list of all zones to pull together the
	//        zone maps of individual threads; for small images
	//        this might prove more inefficient than just doing
	//        it on one thread in the first place ... ??
	NMDebugAI(<< "create set of zones ..." << std::endl);
	long numzones = 0;
	std::set<ZoneKeyType> zones;
	//ZoneMapType vMap;
	ZoneMapTypeIterator mapIt;
	for (int t=0; t < this->GetNumberOfThreads(); ++t)
	{
		ZoneMapType& vMap = mThreadValueStore[t];
		mapIt = vMap.begin();
		while(mapIt != vMap.end())
		{
			if ((zones.insert(mapIt->first)).second)
			{
				++numzones;
			}
			++mapIt;
		}
	}

	NMDebugAI(<< "add " << numzones << " rows to table ..."<< std::endl;)
	// add a chunk of rows
	mZoneTable->AddRows(numzones);

	typename std::set<ZoneKeyType>::const_iterator zoneIt =
			zones.begin();
	long rowcounter = 0;
	while(zoneIt != zones.end())
	{
		double min = itk::NumericTraits<double>::max();
		double max = itk::NumericTraits<double>::max() * -1;
		double sum_Zone = 0;
		double sum_Zone2 = 0;
		double mean = 0;
		double sd = 0;
		long count = 0;

		ZoneKeyType zone = *zoneIt;
		for (int t=0; t < this->GetNumberOfThreads(); ++t)
		{
			ZoneMapType& vMap = mThreadValueStore[t];
			mapIt = vMap.find(zone);
			if (mapIt == vMap.end())
			{
				continue;
			}
			std::vector<double>& params = mapIt->second;
			// min
			min = params[0] < min ? params[0] : min;
			// max
			max = params[1] > max ? params[1] : max;
			// sum_val
			sum_Zone += params[2];
			// sum_val^2
			sum_Zone2 += params[3];
			// count
			count += params[4];
		}

		mean = sum_Zone / (double)count;
		sd = ::sqrt( (sum_Zone2 / (double)count) - (mean * mean) );

		mZoneTable->SetValue("rowidx", rowcounter, rowcounter);
		mZoneTable->SetValue("zone", rowcounter, static_cast<long>(zone));
		mZoneTable->SetValue("count", rowcounter, count);
		mZoneTable->SetValue("min", rowcounter, min);
		mZoneTable->SetValue("max", rowcounter, max);
		mZoneTable->SetValue("mean", rowcounter, mean);
		mZoneTable->SetValue("stddev", rowcounter, sd);
		mZoneTable->SetValue("sum", rowcounter, sum_Zone);

		++rowcounter;
		++zoneIt;
	}

	//mZoneTable->Print(std::cout, itk::Indent(2), numzones);

	NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
