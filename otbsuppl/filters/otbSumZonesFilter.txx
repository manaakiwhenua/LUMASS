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

#include "otbSumZonesFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkMacro.h"

namespace otb
{

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::SumZonesFilter()
{
	this->SetNumberOfRequiredInputs(2);
}

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::~SumZonesFilter()
{
	if (mThreadValueStore)
		delete[] mThreadValueStore;
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream& os, itk::Indent indent) const
{

}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetValueImage(const TOutputImage* image) const
{
	mValueImage = image;
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneImage(const TInputImage* image) const
{
	mZoneImage = image;
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
	if (mZoneImage.IsNull())
	{
		itkExceptionMacro(<< "Zone Image has not been specified!");
		return;
	}

	if (mValueImage.IsNull())
	{
		itkExceptionMacro(<< "Value Image has not been specified!");
		return;
	}

	unsigned int numThreads = this->GetNumberOfThreads();
	if (mThreadValueStore)
		delete[] mThreadValueStore;

	mThreadValueStore =
			new std::unordered_map< InputPixelType, std::vector< double> >[numThreads];

	for (int t=0; t < numThreads; ++t)
	{
		mThreadValueStore[t] = std::unordered_map< InputPixelType,
				std::vector<double>,
				Alloc=std::pair< InputPixelType, std::vector<double>(5,0) > >;
	}

	// for now, the size of the input regions must be exactly the same
	if (	mZoneImage->GetLargestPossibleRegion().GetSize(0)
		!=  mValueImage->GetLargestPossibleRegion().GetSize(1))
	{
		itkExceptionMacro(<< "ZoneImage and ValueImage dimensions don't match!");
		return;
	}

}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, int threadId)
{
	typedef itk::ImageRegionConstIterator<TInputImage> InputIterType;
	InputIterType zoneIt(mZoneImage, outputRegionForThread);

	typedef itk::ImageRegionConstIterator<TOutputImage> OutputIterType;
	OutputIterType valueIt(mValueImage, outputRegionForThread);

	std::unordered_map< InputPixelType, std::vector<double> >* vMap = mThreadStore[threadId];

	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	zoneIt.GoToBegin();
	valueIt.GoToBegin();
	while (!zoneIt.IsAtEnd())
	{
		InputPixelType zone = zoneIt.Get();
		double val = valueIt.Get();

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
	}
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
	if (mZoneTable.IsNull())
	{
		mZoneTable = AttributeTable::New();
	}

	mZoneTable->SetBandNumber(1);
	mZoneTable->SetImageFileName("ZoneTable");
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
	long numzones = 0;
	std::set<InputPixelType> zones;
	std::unordered_map< InputPixelType, std::vector<double> >* vMap;
	std::unordered_map< InputPixelType, std::vector<double> >::const_iterator mapIt;
	for (int t=0; t < this->GetNumberOfThreads(); ++t)
	{
		vMap = mThreadStore[t];
		mapIt = vMap->cbegin();
		while(mapIt != vMap->cend())
		{
			if ((zones.insert(mapIt->first)).second)
			{
				++numzones;
			}
			++mapIt;
		}
	}
	// add a chunk of rows
	mZoneTable->AddRows(numzones);

	std::set<InputPixelType>::const_iterator zoneIt =
			zones.cbegin();
	long rowcounter = 0;
	while(zoneIt != zones.cend())
	{
		double min = itk::NumericTraits<double>::max();
		double max = itk::NumericTraits<double>::max() * -1;
		double sum_Zone = 0;
		double sum_Zone2 = 0;
		double mean = 0;
		double sd = 0;
		long count = 0;

		InputPixelType zone = *zoneIt;
		for (int t=0; t < this->GetNumberOfThreads(); ++t)
		{
			vMap = mThreadStore[t];
			try
			{
				std::vector<double>& params = vMap[zone];
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
			catch (const std::out_of_range& e)
			{
				// current zone not identified by thread t
				continue;
			}
		}

		mean = sum_Zone / (double)count;
		sd = ::sqrt( (sum_Zone2 / (double)count) - (mean * mean) );

		mZoneTable->SetValue("rowidx", rowcounter, rowcounter);
		mZoneTable->SetValue("zone", rowcounter, (long)zone);
		mZoneTable->SetValue("count", rowcounter, count);
		mZoneTable->SetValue("min", rowcounter, min);
		mZoneTable->SetValue("max", rowcounter, max);
		mZoneTable->SetValue("mean", rowcounter, mean);
		mZoneTable->SetValue("stddev", rowcounter, sd);
		mZoneTable->SetValue("sum", rowcounter, sum_Zone);

		++rowcounter;
		++zoneIt;
	}
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
