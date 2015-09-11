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

// TOKYO CABINET
//#include "tcutil.h"
#include "tchdb.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace otb
{

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::SumZonesFilter()
{
    this->SetNumberOfRequiredInputs(1);
	this->SetNumberOfRequiredOutputs(1);

    m_HaveMaxKeyRows = false;
	m_IgnoreNodataValue = true;
	m_NodataValue = itk::NumericTraits<InputPixelType>::NonpositiveMin();

    m_NextZoneId = 0;
	mStreamingProc = false;
    m_ZoneTableFileName = "";
    m_HDBFileName = "";
    m_HDB = tchdbnew();

    mZoneTable = AttributeTable::New();
}

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::~SumZonesFilter()
{
    if (m_HDB)
        tchdbdel(m_HDB);
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

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneTableFileName(const std::string &tableFileName)
{
    if (tableFileName.empty())
    {
        m_dropTmpDBs = true;
    }
    else
    {
        m_dropTmpDBs = false;
    }

    m_ZoneTableFileName = tableFileName;
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetHaveMaxKeyRows(bool maxkeyrows)
{
    m_HaveMaxKeyRows = maxkeyrows;
    //this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetIgnoreNodataValue(bool ignore)
{
    m_IgnoreNodataValue = ignore;
    //this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetNodataValue(InputPixelType nodata)
{
    m_NodataValue = nodata;
    //this->ResetPipeline();
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
        itkDebugMacro(<< "Just summarising the zone imgae itself!");
        NMDebugAI(<< "Just summarising the zone imgae itself..." << std::endl);
	}
    else
    {
        NMDebugAI(<< "Summarise values per zone..." << std::endl);
    }

    NMDebugAI( << "  IgnoreNodataValues = " << m_IgnoreNodataValue << std::endl);
    NMDebugAI( << "  NodataValue        = " << m_NodataValue << std::endl);
    NMDebugAI( << "  HaveMaxKeyRows     = " << m_HaveMaxKeyRows << std::endl);

	unsigned int numThreads = this->GetNumberOfThreads();

    // create the zone table (db)
    NMDebugAI( << "Loading / creating the zone table ..." << std::endl);
    if (mZoneTable->createTable(m_ZoneTableFileName) == otb::AttributeTable::ATCREATE_ERROR)
    {
        itkExceptionMacro(<< "Failed to create the zone table!");
        return;
    }


    if (!mStreamingProc)
    {
        // if we're using a temp data base, we want to know the name for
        // to open the same table again during sequential processing
        if (m_ZoneTableFileName.empty())
        {
            m_dropTmpDBs = true;
        }
        m_ZoneTableFileName = mZoneTable->getDbFileName();
        size_t pos = m_ZoneTableFileName.find_last_of('.');
        if (pos != std::string::npos)
        {
            m_HDBFileName = m_ZoneTableFileName.substr(0,pos);
            m_HDBFileName += ".tchdb";
        }
        NMDebugAI( << "Tokyo cabinet file: '"
                   << m_HDBFileName << "'" << std::endl);


		NMDebugAI(<< "clearing mThreadValueStore ..." << std::endl);
		mThreadValueStore.clear();
		for (int t=0; t < numThreads; ++t)
		{
			mThreadValueStore.push_back(ZoneMapType());
		}

        // open the key-value store
        if (!tchdbopen(m_HDB, m_HDBFileName.c_str(),
                       HDBOWRITER | HDBOCREAT))
        {
            int ecode = tchdbecode(m_HDB);
            itkExceptionMacro( << "ERROR: " << tchdberrmsg(ecode));
            return;
        }
        tchdbvanish(m_HDB);

        NMDebugAI(<< "Adding columns to zone table ..." << std::endl);
        mZoneTable->beginTransaction();
        mZoneTable->AddColumn("zone_id", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("count", AttributeTable::ATTYPE_INT);
		mZoneTable->AddColumn("min", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("max", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("mean", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("stddev", AttributeTable::ATTYPE_DOUBLE);
		mZoneTable->AddColumn("sum", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->endTransaction();

        //        std::vector<std::string> idxCols;
        //        idxCols.push_back(mZoneTable->getPrimaryKey());
        //        idxCols.push_back("zone_id");
        //        mZoneTable->createIndex(idxCols, true);


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
                vMap[zone] = std::vector<double>(6,0);
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
            // id -> take care about that later

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
                vMap[zone] = std::vector<double>(6,0);
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
            // id -> take care about that later

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


    // =============================================================
    // MERGING THREAD DATA
    // =============================================================

    // merging the data gathered in different threads into one
    // key-value store for a RAM independent (but still fast)
    // record of what to put in the proper data base

    NMDebugAI(<< "update set of zones - adding: ");
    //long numzones = mZones.size();
    ZoneKeyType numzones = tchdbrnum(m_HDB);
    ZoneKeyType newzones = 0;
    ZoneKeyType maxKey = 0;
    ZoneMapTypeIterator mapIt;


    /*  keep track of zones ...
     *  0: min
     *  1: max
     *  2: sum_val
     *  3: sum_val^2
     *  4: count
     *  5: zone_id
     */
    int lenTCRec = 6;
    std::vector<double> params(lenTCRec,0);
    params[0] = itk::NumericTraits<double>::max();
    params[1] = itk::NumericTraits<double>::NonpositiveMin();
    params[5] = -1;

    for (int t=0; t < this->GetNumberOfThreads(); ++t)
	{
		ZoneMapType& vMap = mThreadValueStore[t];
		mapIt = vMap.begin();
		while(mapIt != vMap.end())
		{
            // --------------------------------------------------
            // read values for KEY from db
            // --------------------------------------------------
            ZoneKeyType zone = mapIt->first;
            if (tchdbget3(m_HDB,
                          static_cast<void*>(&zone),
                          sizeof(ZoneKeyType),
                          &params[0],
                          sizeof(double)*lenTCRec)
                == -1)
            {
                // .................................
                // KEY not present! -> put it in db
                // .................................
                mapIt->second[5] = m_NextZoneId;
                if (!tchdbput(m_HDB,
                              static_cast<void*>(&zone),
                              sizeof(ZoneKeyType),
                              &mapIt->second[0],
                              sizeof(double)*lenTCRec))
                {
                    itkWarningMacro(<< "Failed setting record for key "
                                    << mapIt->first << " while merging "
                                    << "thread values");
                    continue;
                }
                // keep track of max key and number of new keys
                maxKey = mapIt->first > maxKey ? mapIt->first : maxKey;
                ++newzones;
                ++m_NextZoneId;
            }
            else
            {
                // ..................................
                // update parameters for current KEY
                // ..................................

                // update stats parameter
                // min
                params[0] = params[0] < mapIt->second[0] ?
                            params[0] : mapIt->second[0];
                // max
                params[1] = params[1] > mapIt->second[0] ?
                            params[1] : mapIt->second[1];
                // sum_val
                params[2] += mapIt->second[2];
                // sum_val^2
                params[3] += mapIt->second[3];
                // count
                params[4] += mapIt->second[4];
                // we keep the already asigned zone_id
                //params[5] = mapIt->second[5];

                // ------------------------------------------------
                // put data into key-value store
                if (!tchdbput(m_HDB,
                              static_cast<void*>(&zone),
                              sizeof(ZoneKeyType),
                              &params[0],
                              sizeof(double)*lenTCRec))
                {
                    itkWarningMacro(<< "Failed setting record for key "
                                    << mapIt->first << " while merging "
                                    << "thread values");
                    continue;
                }
            }

            //std::vector<double>& param =

			// add zone to set if not already present
            //			if ((mZones.insert(mapIt->first)).second)
            //			{
            //                maxKey = mapIt->first > maxKey ? mapIt->first : maxKey;
            //                //NMDebug(<< mapIt->first << " ");
            //				++newzones;
            //			}
			++mapIt;
		}
	}
	NMDebug(<< std::endl);
    NMDebugAI(<< "Merged threads ... " << std::endl);
    NMDebugAI(<< "... new zones  = " << newzones << std::endl);
    NMDebugAI(<< "... num zones  = " << numzones+newzones << std::endl);
    NMDebugAI(<< "... maxKey     = " << maxKey << std::endl);
    NMDebugAI(<< "... NextZoneId = " << m_NextZoneId << std::endl);

    // =============================================================
    // BUMPING UP THE NUMBER OF ROWS IN THE DB (if required)
    // =============================================================
    std::vector<double> fillRec(lenTCRec,0);
    fillRec[5] = -1;
    if (this->GetHaveMaxKeyRows())
    {
        if (maxKey+1 > numzones + newzones)
        {
            ZoneKeyType addzones = maxKey+1 - (numzones + newzones);
            NMDebugAI(<< "Adding " << addzones
                      << " recs to match the max key size (+1)" << std::endl);
            for (ZoneKeyType nz = 0; nz < maxKey+1; ++nz)
            {
                tchdbputkeep(m_HDB, static_cast<void*>(&nz),
                             sizeof(ZoneKeyType),
                             static_cast<void*>(&fillRec[0]),
                             sizeof(double)*lenTCRec);
            }
        }
    }

    numzones = tchdbrnum(m_HDB);
    NMDebugAI(<< "Got " << numzones << " zones on record now ..." << std::endl);

    //	double min 		 ;
    //	double max 		 ;
    //	double sum_Zone  ;
    //	double sum_Zone2 ;
    //	double mean      ;
    //	double sd        ;
    //	long   count     ;

    // ==========================================================================
    // prepare prepared update/insert statements for summary table
    // ==========================================================================

    std::vector<std::string> colnames;
    colnames.push_back(mZoneTable->getPrimaryKey()); // 0
    colnames.push_back("zone_id");                   // 1
    colnames.push_back("count");                     // 2
    colnames.push_back("min");                       // 3
    colnames.push_back("max");                       // 4
    colnames.push_back("mean");                      // 5
    colnames.push_back("stddev");                    // 6
    colnames.push_back("sum");                       // 7

    int numIntCols = 3;
    std::vector<otb::AttributeTable::ColumnValue> values;
    for (int i=0; i < numIntCols; ++i)
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
    mZoneTable->prepareBulkSet(colnames, true);

    if (!tchdbiterinit(m_HDB))
    {
        int ecode = tchdbecode(m_HDB);
        itkExceptionMacro(<< "ERROR writing zone table: "
                          << tchdberrmsg(ecode));
        return;
    }

    int sizeKey = 0;
    void* nextKey = 0;
    std::vector<double> zoneRec(lenTCRec,0);

    while(nextKey = tchdbiternext(m_HDB, &sizeKey))
	{
        ZoneKeyType zone = *static_cast<ZoneKeyType*>(nextKey);
        if (tchdbget3(m_HDB, nextKey, sizeKey,
                      static_cast<void*>(&zoneRec[0]),
                      sizeof(double)*lenTCRec)
            == -1)
        {
            itkWarningMacro(<< "Failed reading "
                            << "zone db record for zone " << zone);
            free(nextKey);
            continue;
        }

        // skip if just a fill record (i.e. when HaveMaxKeyRows = true)
        if (zoneRec[5] == -1)
        {
            values[0].ival = zone;
            values[1].ival = -1;
            values[2].ival = 0;
            for (int v=3; v < 8; ++v)
            {
                values[v].dval = 0.0;
            }
        }
        else
        {
            // mean = sum_Zone / (count > 0 ? count : 1);
            values[5].dval = zoneRec[2] / ((double)zoneRec[4] > 0 ?
                                           (double)zoneRec[4] : 1);
            // stddev =
            values[6].dval = values[5].dval != zoneRec[2]
                    ? ::sqrt( (zoneRec[3] / (double)zoneRec[4])
                              - (values[5].dval * values[5].dval) )
                    : 0;


            values[0].ival = zone;          // rowidx
            values[1].ival = zoneRec[5];    // zone_id
            values[2].ival = zoneRec[4];    // count
            values[3].dval = zoneRec[0];    // min;
            values[4].dval = zoneRec[1];    // max;
            //values[5].dval = mean;
            //values[6].dval = sd;
            values[7].dval = zoneRec[2];    // sum_Zone;
        }

        mZoneTable->doBulkSet(values);

        free(nextKey);
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

    if (m_dropTmpDBs)
    {
        mZoneTable->closeTable(true);
    }
    else
    {
        mZoneTable->closeTable();
    }
    mZoneTable = 0;
    m_NextZoneId = 0;

    if (m_HDB)
    {
        if (m_dropTmpDBs)
        {
            tchdbvanish(m_HDB);
        }
        tchdbclose(m_HDB);
    }

    mZones.clear();
	mZoneTable = AttributeTable::New();

	Superclass::ResetPipeline();
	NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
