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

#include "nmlog.h"
#include "otbSumZonesFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

//#define ctx "otb::SumZonesFilter"

namespace otb
{

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::SumZonesFilter()
{
    this->SetNumberOfRequiredInputs(1);
	this->SetNumberOfRequiredOutputs(1);

    m_KeyIsRowIdx = true;
    m_HaveMaxKeyRows = false;
	m_IgnoreNodataValue = true;
	m_NodataValue = itk::NumericTraits<InputPixelType>::NonpositiveMin();

	mStreamingProc = false;
    m_ZoneTableFileName = "";
	mZoneTable = AttributeTable::New();
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
    this->SetNthInput(1, const_cast<TInputImage*>(image));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneImage(const OutputImageType* image)
{
	mZoneImage = const_cast<TOutputImage*>(image);
    this->SetNthInput(0, const_cast<TOutputImage*>(image));
	this->GraftOutput(static_cast<TOutputImage*>(mZoneImage));
}

//template< class TInputImage, class TOutputImage >
//void SumZonesFilter< TInputImage, TOutputImage >
//::SetZoneTable(AttributeTable::Pointer tab)
// {
//	//mZoneTable = tab;
// }

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
        itkDebugMacro(<< "Just summarising the zone imgae itself!");
        NMDebugAI(<< "Just summarising the zone imgae itself!" << std::endl);
	}

	unsigned int numThreads = this->GetNumberOfThreads();

    // create the zone table (db)
    mZoneTable->createTable(m_ZoneTableFileName);

	if (!mStreamingProc)
	{
		NMDebugAI(<< "clearing mThreadValueStore ..." << std::endl);
		mThreadValueStore.clear();
		for (int t=0; t < numThreads; ++t)
		{
			mThreadValueStore.push_back(ZoneMapType());
		}

		NMDebugAI(<< "creating new attribute table ..." << std::endl);
		//mZoneTable = AttributeTable::New();

        mZoneTable->beginTransaction();
        mZoneTable->AddColumn("rowidx", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("zone", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("count", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("min", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("max", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("mean", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("stddev", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("sum", AttributeTable::ATTYPE_DOUBLE);
		//mZoneTable->AddColumn("sum2", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->endTransaction();

		mStreamingProc = true;
	}

	// for now, the size of the input regions must be exactly the same
    if (mValueImage.IsNotNull())
    {
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
    }

	NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId)
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

	ZoneMapType& vMap = mThreadValueStore[threadId];
	ZoneMapTypeIterator mapIt;

	itk::ProgressReporter progress(this, threadId,
			outputRegionForThread.GetNumberOfPixels());

	if (threadId == 0)
	{
		NMDebugAI(<< "start summarising ..." << std::endl);
	}

	zoneIt.GoToBegin();

    if (mValueImage.IsNotNull())
    {
        typedef itk::ImageRegionConstIterator<TInputImage> OutputIterType;
        OutputIterType valueIt(mValueImage, outputRegionForThread);

        valueIt.GoToBegin();
        while (!zoneIt.IsAtEnd() && !valueIt.IsAtEnd() && !this->GetAbortGenerateData())
        {
            const ZoneKeyType zone = static_cast<ZoneKeyType>(zoneIt.Get());
            const double val = static_cast<double>(valueIt.Get());
            if (	m_IgnoreNodataValue
                &&	val == static_cast<double>(m_NodataValue)
               )
            {
                ++zoneIt;
                ++valueIt;

                progress.CompletedPixel();
                continue;
            }

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
    }
    else
    {
        while (!zoneIt.IsAtEnd() && !this->GetAbortGenerateData())
        {
            const ZoneKeyType zone = static_cast<ZoneKeyType>(zoneIt.Get());
            double val = static_cast<double>(zone);
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
            progress.CompletedPixel();
        }
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
	NMDebugAI(<< "update set of zones - adding: ");
	long numzones = mZones.size();
	long newzones = 0;
    long maxKey = 0;
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
                maxKey = mapIt->first > maxKey ? mapIt->first : maxKey;
                //NMDebug(<< mapIt->first << " ");
				++newzones;
			}
			++mapIt;
		}
	}
	NMDebug(<< std::endl);
    NMDebugAI(<< "added " << newzones << " new zones, which gives total of "
              << numzones+newzones << " ..."<< std::endl;)

    if (this->GetHaveMaxKeyRows())
    {
        mZoneTable->beginTransaction();
        // add a chunk of rows
        if (newzones > 0)
        {
            if (mZoneTable->GetNumRows() < maxKey+1)
            {
                long numOldRows = mZoneTable->GetNumRows();
                long numNewRows = maxKey+1 - numOldRows;
                mZoneTable->AddRows(numNewRows);
                for (long i = numOldRows; i < maxKey+1; ++i)
                {
                    mZoneTable->SetValue("rowidx", i, i);
                }
            }
        }
        mZoneTable->endTransaction();
    }

	typename std::set<ZoneKeyType>::const_iterator zoneIt =
			mZones.begin();

	double min 		 ;
	double max 		 ;
	double sum_Zone  ;
	double sum_Zone2 ;
	double mean      ;
	double sd        ;
	long   count     ;

    std::vector<std::string> colnames;
    colnames.push_back("rowidx");
    colnames.push_back("zone");
    colnames.push_back("count");
    colnames.push_back("min");
    colnames.push_back("max");
    colnames.push_back("mean");
    colnames.push_back("stddev");
    colnames.push_back("sum");

    std::vector<otb::AttributeTable::ColumnValue> values;
    for (int i=0; i < 3; ++i)
    {
        otb::AttributeTable::ColumnValue v;
        v.type = otb::AttributeTable::ATTYPE_INT;
        values.push_back(v);
    }

    for (int i=0; i < 5; ++i)
    {
        otb::AttributeTable::ColumnValue v;
        v.type = otb::AttributeTable::ATTYPE_DOUBLE;
        values.push_back(v);
    }

	NMDebugAI(<< "updating zone table ..." << std::endl);
    mZoneTable->beginTransaction();
    if (this->GetHaveMaxKeyRows())
    {
        mZoneTable->prepareBulkSet(colnames, false);
    }
    else
    {
        mZoneTable->prepareBulkSet(colnames, true);
    }
    long rowid = mZoneTable->GetNumRows();
	long tr = -1;
	while(zoneIt != mZones.end())
	{
		ZoneKeyType zone = *zoneIt;
		long lz = static_cast<long>(zone);

		min 	  = itk::NumericTraits<double>::max();
		max       = itk::NumericTraits<double>::NonpositiveMin();
		sum_Zone  = 0;
		sum_Zone2 = 0;
		count     = 0;

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

        mean = sum_Zone / ((double)count > 0 ? (double)count : 1);
        sd = mean != sum_Zone
                ? ::sqrt( (sum_Zone2 / (double)count) - (mean * mean) )
                : 0;

        if (    this->GetHaveMaxKeyRows()
            ||  this->GetKeyIsRowIdx()
           )
        {
            values[0].ival = lz;
        }
        else
        {
            values[0].ival = rowid;
        }
        //mZoneTable->SetValue("zone",   lz, lz);
        values[1].ival = lz;
        //mZoneTable->SetValue("count",  lz, count);
        values[2].ival = count;
        //mZoneTable->SetValue("min",    lz, min);
        values[3].dval = min;
        //mZoneTable->SetValue("max",    lz, max);
        values[4].dval = max;
        //mZoneTable->SetValue("mean",   lz, mean);
        values[5].dval = mean;
        //mZoneTable->SetValue("stddev", lz, sd);
        values[6].dval = sd;
        //mZoneTable->SetValue("sum",    lz, sum_Zone);
        values[7].dval = sum_Zone;

        if (this->GetHaveMaxKeyRows())
        {
            mZoneTable->doBulkSet(values, lz);
        }
        else
        {
            mZoneTable->doBulkSet(values);
        }

		++rowid;
		++zoneIt;
	}
    mZoneTable->endTransaction();

	this->GraftOutput(static_cast<TOutputImage*>(mZoneImage));

	NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
	NMDebugCtx(ctx, << "...");
	mStreamingProc = false;
	mZoneTable = AttributeTable::New();

	Superclass::ResetPipeline();
	NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
