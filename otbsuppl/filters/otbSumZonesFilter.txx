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
	this->SetNumberOfRequiredOutputs(1);
	mStreamingProc = false;
	mZoneTable = 0;
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
::SetValueImage(const InputImageType* image)
{
	mValueImage = const_cast<TInputImage*>(image);
	this->SetNthInput(0, const_cast<TInputImage*>(image));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneImage(const OutputImageType* image)
{
	mZoneImage = const_cast<TOutputImage*>(image);
	this->SetNthInput(1, const_cast<TOutputImage*>(image));
	this->GraftOutput(static_cast<TOutputImage*>(mZoneImage));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneTable(AttributeTable::Pointer tab)
 {
	//mZoneTable = tab;
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

	if (!mStreamingProc)
	{
		NMDebugAI(<< "clearing mThreadValueStore ..." << std::endl);
		mThreadValueStore.clear();
		for (int t=0; t < numThreads; ++t)
		{
			mThreadValueStore.push_back(ZoneMapType());
		}

		NMDebugAI(<< "creating new attribute table ..." << std::endl);
		mZoneTable = AttributeTable::New();
		mZoneTable->AddColumn("rowidx", AttributeTable::ATTYPE_INT);
		// ToDo:: check for integer zone type earlier
		mZoneTable->AddColumn("zone", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("count", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("min", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("max", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("mean", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("stddev", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("sum", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("sum2", AttributeTable::ATTYPE_DOUBLE);

		mStreamingProc = true;
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
	if (threadId == 0)
	{
		NMDebugCtx(ctx, << "...");
		unsigned int xsize = outputRegionForThread.GetSize(0);
		unsigned int ysize = outputRegionForThread.GetSize(1);
		NMDebugAI(<< "analysing region of " << xsize << " x " << ysize
				<< " pixels ..." << std::endl);
		//NMDebugAI(<< "number of threads: " << this->GetNumberOfThreads() << std::endl);
	}
	typedef itk::ImageRegionConstIterator<TOutputImage> InputIterType;
	InputIterType zoneIt(mZoneImage, outputRegionForThread);

	typedef itk::ImageRegionConstIterator<TInputImage> OutputIterType;
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
		NMDebugAI( << "mStreamingProc = " << mStreamingProc << std::endl);
		NMDebugCtx(ctx, << "done!");
	}
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
	NMDebugCtx(ctx, << "...");

	if (mZoneTable.IsNull())
	{
		NMErr(ctx, << "oops, don't have a zone table - shouldn't have happened!");
		itk::ExceptionObject eo;
		eo.SetDescription("no table found! How could that happen?!");
		throw eo;
	}

	// ToDo:: mmh, is there a better way of doing this?
	//        we need a list of all zones to pull together the
	//        zone maps of individual threads; for small images
	//        this might prove more inefficient than just doing
	//        it on one thread in the first place ... ??
	NMDebugAI(<< "create set of zones ..." << std::endl);
	long numzones = mZones.size();
	long newzones = 0;
	ZoneMapTypeIterator mapIt;
	for (int t=0; t < this->GetNumberOfThreads(); ++t)
	{
		ZoneMapType& vMap = mThreadValueStore[t];
		mapIt = vMap.begin();
		while(mapIt != vMap.end())
		{
			// add zone to set if not already present
			if ((mZones.insert(mapIt->first)).second)
			{
				++newzones;
			}
			++mapIt;
		}
	}

	NMDebugAI(<< "added " << newzones << " new zones, which gives total of " << numzones+newzones << " ..."<< std::endl;)
	// add a chunk of rows
	if (newzones > 0)
		mZoneTable->AddRows(newzones);

	typename std::set<ZoneKeyType>::const_iterator zoneIt =
			mZones.begin();
	long rowcounter = 0;

	double min 		 ;
	double max 		 ;
	double sum_Zone  ;
	double sum_Zone2 ;
	double mean      ;
	double sd        ;
	long   count     ;

	long rowid = -1;
	while(zoneIt != mZones.end())
	{
		ZoneKeyType zone = *zoneIt;
		long lz = static_cast<long>(zone);
		if (	mStreamingProc
			&& (rowid = mZoneTable->GetRowIdx("zone", (void*)&lz) >= 0)
		   )
		{
			count     = mZoneTable->GetIntValue("count", rowid);
			min 	  = mZoneTable->GetDblValue("min", rowid);
			max       = mZoneTable->GetDblValue("max", rowid);
			sum_Zone  = mZoneTable->GetDblValue("sum", rowid);
			sum_Zone2 = mZoneTable->GetDblValue("sum2", rowid);
		}
		else
		{
			min 	  = itk::NumericTraits<double>::max();
			max       = itk::NumericTraits<double>::NonpositiveMin();
			sum_Zone  = 0;
			sum_Zone2 = 0;
			count     = 0;
		}
		mean      = 0;
		sd        = 0;

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
		mZoneTable->SetValue("zone", rowcounter, lz);//static_cast<long>(zone));
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

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
	NMDebugCtx(ctx, << "...");
	mStreamingProc = false;
	Superclass::ResetPipeline();
	NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
