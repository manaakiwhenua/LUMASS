/****************************t**************************************************
 * Created by Alexander Herzigd
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
/* NMCostDistanceBufferImageWrapper.cpp
 *
 *  Created on: 5/12/2012
 *      Author: alex
 */

#include <string>
#include <iostream>
#include <vector>

#include "NMMacros.h"
#include "nmlog.h"
#include "NMCostDistanceBufferImageWrapper.h"
#include "NMItkDataObjectWrapper.h"

#include <QObject>
#include <QString>

#include "itkProcessObject.h"
#include "itkTimeProbe.h"
#include "otbImage.h"
#include "otbGDALRATImageIO.h"
#include "itkNMCostDistanceBufferImageFilter.h"
#include "otbStreamingRATImageFileWriter.h"
#include "otbRasdamanImageReader.h"
#include "otbImageFileWriter.h"
#include "otbImageFileReader.h"
#include "itkImageIOBase.h"


template <class InPixelType, unsigned int Dimension>
class NMCostDistanceBufferImageWrapper_Internal
{
//private:
//	static const std::string ctx;

public:
	typedef otb::Image<InPixelType, 2> InputImgType;
	typedef otb::Image<double, 2> OutputImgType;
	typedef itk::NMCostDistanceBufferImageFilter<InputImgType, OutputImgType> DistanceFilterType;

	typedef otb::ImageFileReader<InputImgType> ReaderType;
	typedef otb::ImageFileReader<typename DistanceFilterType::OutputImageType> TmpOutReaderType;

#ifdef BUILD_RASSUPPORT
	typedef otb::RasdamanImageReader<InputImgType> RasReaderType;
	typedef otb::RasdamanImageReader<typename DistanceFilterType::OutputImageType> RasOutReaderType;
#endif

	typedef otb::StreamingRATImageFileWriter<OutputImgType> WriterType;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		typename DistanceFilterType::Pointer f = DistanceFilterType::New();
		otbFilter = f;
	}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
	{
		InputImgType* img = dynamic_cast<InputImgType*>(dataObj);
		DistanceFilterType* filter = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
		filter->SetInput(idx, img);
	}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		DistanceFilterType* f = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutputImgType*>(f->GetOutput());
	}

	static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc, unsigned int step,
			const QMap<QString, NMModelComponent*>& repo)
	{
		typename DistanceFilterType::Pointer f = DistanceFilterType::New();
		otbFilter = f;
		NMCostDistanceBufferImageWrapper* p = dynamic_cast<NMCostDistanceBufferImageWrapper*>(proc);

		// save the current step internally, since we need it again
		// for the execute function to get parameters 'external' to the
		// process object itself
		p->mParamPos = step;

		// -------------------------------------------------------------
		// set the object values
		int ols = step;
		bool bok;

		if (p->mObjectValueList.size())
		{
			if (ols >= p->mObjectValueList.size() || ols < 0)
				ols = 0;


			std::vector<double> vObjValues;
			QStringList strObjValues = p->mObjectValueList.at(ols);
			foreach(const QString& strVal, strObjValues)
			{
				double val = strVal.toDouble(&bok);
				if (bok)
					vObjValues.push_back(val);
			}
			f->SetCategories(vObjValues);
		}

		// -------------------------------------------------------------------
		// set max distances
		ols = step;
		if (ols >= p->mMaxDistance.size() || ols < 0)
			ols = 0;

		double distance;
		if (p->mMaxDistance.size())
		{
			distance = p->mMaxDistance.at(ols).toDouble(&bok);
			if (bok)
				f->SetMaxDistance(distance);
		}

		// -------------------------------------------------------------------
		// set buffer zone indicator
		ols = step;
		if (ols >= p->mBufferZoneIndicator.size() || ols < 0)
			ols = 0;

		int indicator;
		if (p->mBufferZoneIndicator.size())
		{
			indicator = p->mBufferZoneIndicator.at(ols).toInt(&bok);
			if (bok)
				f->SetBufferZoneIndicator(indicator);
		}

		// -------------------------------------------------------------------
		// set the other 'singular / fixed' parameters
		f->SetUseImageSpacing(p->mUseImageSpacing);
		f->SetCreateBuffer(p->mCreateBuffer);
	}

	static void execute(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc)
	{
		NMDebugCtx("CostDistanceInternal", << "...");

		DistanceFilterType* distfilter = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
		NMCostDistanceBufferImageWrapper* p = qobject_cast<NMCostDistanceBufferImageWrapper*>(proc);

		// get the input, cost, and output image filenames
		int pos = p->mParamPos;

		if (pos >= p->mInputImageFileName.size())
			pos = 0;
		QString fileName = p->mInputImageFileName.at(pos);
		if (fileName.isEmpty())
		{
			NMErr(ctx, << "Please provide an input image file name!");
			return;
		}
		bool bInRas = false;
		if (!fileName.contains('.'))
			bInRas = true;

		pos = p->mParamPos;
		QString costFN;
		bool bCostRas = false;
		if (p->mCostImageFileName.size())
		{
			if (pos >= p->mCostImageFileName.size())
				pos = 0;
			costFN = p->mCostImageFileName.at(pos);

			if (!costFN.contains('.'))
				bCostRas = true;
		}

		pos = p->mParamPos;
		QString out;
		if (p->mOutputImageFileName.size())
		{
			if (pos >= p->mOutputImageFileName.size())
				pos = 0;
			out = p->mOutputImageFileName.at(pos);
			if (out.isEmpty())
			{
				NMErr(ctx, << "Please provide an output image file name!");
				return;
			}
		}
		else
		{
			NMErr(ctx, << "Please provide an output image file name!");
			return;
		}
		bool bOutRas = false;
		if (!out.contains('.'))
			bOutRas = true;


		RasdamanConnector* rcon = 0;
		if (bOutRas || bInRas || bCostRas)
		{
			if (p->mRasConnector == 0)
			{
				NMErr(ctx, << "no valid RasdamanConnector object!");
				return;
			}
			else
			{
				rcon = const_cast<RasdamanConnector*>(
						p->mRasConnector->getConnector());
			}
		}


#ifdef BUILD_RASSUPPORT
		typename RasReaderType::Pointer rasreader;
		typename RasReaderType::Pointer rascostreader;
		typename RasOutReaderType::Pointer rasoutreader;
#endif


		// instantiate readers and writers
		otb::GDALRATImageIO::Pointer inrio;
		typename ReaderType::Pointer reader;
		if (bInRas)
		{
#ifdef BUILD_RASSUPPORT
			rasreader = RasReaderType::New();
			rasreader->SetRasdamanConnector(rcon);
			rasreader->SetFileName(fileName.toStdString().c_str());
#endif
		}
		else
		{
			reader = ReaderType::New();
			inrio = otb::GDALRATImageIO::New();
			inrio->SetRATSupport(true);
			reader->SetFileName(fileName.toStdString().c_str());
			reader->SetImageIO(inrio);
		}



		typename TmpOutReaderType::Pointer tmpOutReader;// = TmpOutReaderType::New();
		otb::GDALRATImageIO::Pointer tmpOutGIO;
		if (bOutRas)
		{
#ifdef BUILD_RASSUPPORT
			rasoutreader = RasOutReaderType::New();
			rasoutreader->SetRasdamanConnector(rcon);
#endif
		}
		else
		{
			tmpOutReader = TmpOutReaderType::New();
			tmpOutGIO = otb::GDALRATImageIO::New();
			tmpOutReader->SetImageIO(tmpOutGIO);
		}


		typedef otb::StreamingRATImageFileWriter<OutputImgType> WriterType;
		typename WriterType::Pointer writer = WriterType::New();
		writer->SetFileName(out.toStdString().c_str());
		otb::GDALRATImageIO::Pointer outgio;
		if (bOutRas)
		{
#ifdef BUILD_RASSUPPORT
			writer->SetRasdamanConnector(rcon);
			writer->SetFileName(out.toStdString().c_str());
#endif
			;
		}
		else
		{
			outgio = otb::GDALRATImageIO::New();
			writer->SetImageIO(outgio);
		}
		writer->SetNumberOfDivisionsStrippedStreaming(1);


		// let's get an idea about the size of the image to process
		typename InputImgType::RegionType lpr;
		typename InputImgType::RegionType pr;
		typename InputImgType::SpacingType inputSpacing;
		typename InputImgType::PointType inputOrigin;
		if (bInRas)
		{
			rasreader->GetOutput()->UpdateOutputInformation();
			lpr          = rasreader->GetOutput()->GetLargestPossibleRegion();
			inputSpacing = rasreader->GetOutput()->GetSpacing();
			inputOrigin  = rasreader->GetOutput()->GetOrigin();
		}
		else
		{
			reader->GetOutput()->UpdateOutputInformation();
			lpr          = reader->GetOutput()->GetLargestPossibleRegion();
			inputSpacing = reader->GetOutput()->GetSpacing();
			inputOrigin  = reader->GetOutput()->GetOrigin();
		}
		pr.SetSize(0, lpr.GetSize()[0]);
		pr.SetIndex(0, 0);
		itk::ImageIORegion lprio(2);
		lprio.SetSize(0, lpr.GetSize()[0]);
		lprio.SetSize(1, lpr.GetSize()[1]);
		lprio.SetIndex(0, lpr.GetIndex()[0]);
		lprio.SetIndex(1, lpr.GetIndex()[1]);

		writer->SetForcedLargestPossibleRegion(lprio);

		//typedef itk::ImageIORegionAdaptor<2> IORegAdaptor;
		itk::ImageIORegion ior(2);
		ior.SetSize(0, lpr.GetSize()[0]);
		ior.SetIndex(0, 0);
		int nrows = lpr.GetSize()[1];

		// calc memory cost for image and derive iteration parameters
		int memmax = p->mMemoryMax * 1024 * 1024;
		int rowcost = lpr.GetSize()[0] * sizeof(double) * 5;
		int chunksize = memmax / rowcost;
		bool bRAM = false;
		if (chunksize > nrows)
		{
			bRAM = true;
		}
		if (chunksize < 3)
			chunksize = 3;
	    unsigned long niter = chunksize == 0 ? nrows : nrows / (chunksize-1);
	    unsigned long rest = nrows - (niter * (chunksize-1));
	    while (rest > 0 && rest < 2)
	    {
	    	--chunksize;
	    	niter = nrows / (chunksize-1);
	    	rest = nrows - (niter * (chunksize - 1));
	    }
	    if (chunksize < 2)
	    {
	    	NMErr("CostDistanceInternal", << "Ever thought of calculating this by hand?? It's not even two rows!!");
	    	NMDebugCtx("CostDistanceInternal", << "done!");
	    	return;
	    }

	    long startrow = lpr.GetIndex()[1];
	    unsigned long endrow = startrow + chunksize - 1;
	    unsigned long rowstoread = chunksize;

	    NMDebugAI(<< "Iteration params ..." << endl);
	    NMDebugAI(<< "  chunksize=" << chunksize
	    		  << "  niter=" << niter
	    		  << "  rest=" << rest << endl);

	    //const piepeline connections // and filter settings
		if (bInRas)
			distfilter->SetInput(rasreader->GetOutput());
		else
			distfilter->SetInput(reader->GetOutput());

		typename ReaderType::Pointer costreader;
		if (!costFN.isNull())
		{
			if (bCostRas)
			{
#ifdef BUILD_RASSUPPORT
				rascostreader = RasReaderType::New();
				rascostreader->SetRasdamanConnector(rcon);
				rascostreader->SetFileName(costFN.toStdString().c_str());
				distfilter->SetInput(1, rascostreader->GetOutput());
#endif
			}
			else
			{
				costreader = ReaderType::New();
				costreader->SetFileName(costFN.toStdString().c_str());
				distfilter->SetInput(1, costreader->GetOutput());
			}
		}

		/* ================================================================== */
		/* just check the single-run algorithm */

			if (bRAM)
			{
				writer->SetInput(distfilter->GetOutput());

				itk::TimeProbe chrono;
				chrono.Start();
				writer->Update();
				chrono.Stop();
				NMDebugAI(<< "==> this took " << chrono.GetMean()
						<< " time units " << endl);
				NMDebugCtx("CostDistanceInternal", << "done!");
				return;
			}
		/* ================================================================== */


	    distfilter->resetExecCounter();
	    distfilter->SetProcessDownward(true);
	    // iterate over the regions (chunks of rows)
	    // to get the job done
	    QString nfn = QString("%1:_last_").arg(out.toStdString().c_str());
	    for (int iter=0; iter <= niter; ++iter)
	    {
	    	NMDebugAI(<< "  startrow=" << startrow << "  endrow=" << endrow
	    			<< "  rowstoread=" << rowstoread << "  rest=" << rest << endl);

	    	pr.SetIndex(1, startrow);
	    	ior.SetIndex(1, startrow);
			pr.SetSize(1, rowstoread);
			ior.SetSize(1, rowstoread);

	    	if (iter >= 1)
	    	{
	    		OutputImgType::Pointer tout;
    			if (!bOutRas)
	    		{
	    			tmpOutReader->SetFileName(out.toStdString().c_str());
    				tmpOutReader->SetImageIO(tmpOutGIO);
    				tmpOutReader->GetOutput()->SetRequestedRegion(pr);
    	    		tmpOutReader->Update();

    	    		tout = tmpOutReader->GetOutput();

    	    		//writer->SetFileName(out.toStdString().c_str());
	    		}
    			else
    			{
    				rasoutreader->SetFileName(nfn.toStdString().c_str());
    				rasoutreader->GetOutput()->SetRequestedRegion(pr);
    				rasoutreader->Update();

    				tout = rasoutreader->GetOutput();
    				writer->SetFileName(nfn.toStdString().c_str());
    			}
	    		tout->DisconnectPipeline();
	    		distfilter->GraftOutput(tout);

	    		writer->SetUpdateMode(true);
	    	}

	    	// set the largest possible and requested regions
	    	distfilter->GetOutput()->SetRequestedRegion(pr);
	    	distfilter->Update();

	    	OutputImgType::Pointer di = distfilter->GetOutput();
	    	di->DisconnectPipeline();

	    	writer->SetInput(di);
	    	writer->SetUpdateRegion(ior);
	    	writer->Update();

	   		// need to close the datasets explicitly this time,
	   		// since they're not closed for re-reading in non
	   		// update mode
	   		if (iter == 0)
	   		{	if (!bOutRas)
	   				outgio->CloseDataset();
	   		}

	 		startrow = endrow;
	 		endrow = startrow + chunksize - 1;
	 		endrow = endrow > nrows-1 ? nrows-1 : endrow;
	 		rowstoread = endrow - startrow + 1;
	    }

	    NMDebugAI(<< "!!! GET UP !!!" << endl);

	    // ---------------------------------------------------------------
	    // get up!
	    distfilter->SetProcessDownward(false);
	    distfilter->SetProcessUpward(true);
	    startrow = nrows - chunksize;
	    endrow = nrows - 1;
	    rowstoread = chunksize;
	    for (int iter=0; iter <= niter; ++iter)
	    {
	    	NMDebugAI(<< "startrow=" << startrow << " endrow=" << endrow
	    			<< " rowstoread=" << rowstoread << " rest=" << rest << endl);

	    	pr.SetIndex(1, startrow);
	   		pr.SetSize(1, rowstoread);
	    	ior.SetIndex(1, startrow);
	   		ior.SetSize(1, rowstoread);

    		OutputImgType::Pointer tout;
			if (!bOutRas)
    		{
    			tmpOutReader->SetFileName(out.toStdString().c_str());
				tmpOutReader->SetImageIO(tmpOutGIO);
				tmpOutReader->GetOutput()->SetRequestedRegion(pr);
	    		tmpOutReader->Update();

	    		tout = tmpOutReader->GetOutput();
    		}
			else
			{
				QString nfn = QString("%1:_last_").arg(out.toStdString().c_str());
				rasoutreader->SetFileName(nfn.toStdString().c_str());
				rasoutreader->GetOutput()->SetRequestedRegion(pr);
				rasoutreader->Update();

				tout = rasoutreader->GetOutput();
			}

			tout->DisconnectPipeline();
			distfilter->GraftOutput(tout);

	    	// set the largest possible and requested regions
	    	distfilter->GetOutput()->SetRequestedRegion(pr);
	    	distfilter->Update();

	    	OutputImgType::Pointer di = distfilter->GetOutput();
	    	di->DisconnectPipeline();

	    	writer->SetInput(di);
	    	writer->SetUpdateRegion(ior);
	    	writer->Update();

	        endrow = startrow;
	        startrow = endrow - chunksize + 1;
	        startrow = startrow < 0 ? 0 : startrow;
	        rowstoread = endrow - startrow + 1;
	    }

		NMDebugCtx("CostDistanceInternal", << "done!");
	}

};

//template<class InPixelType, unsigned int Dimension>
//const std::string NMCostDistanceBufferImageWrapper_Internal::ctx = "NMCostDistanceBufferImageWrapper_Internal";

InstantiateInputTypeObjectWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
SetInputTypeNthInputWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
GetInputTypeOutputWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
LinkInputTypeInternalParametersWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )


//const std::string NMCostDistanceBufferImageWrapper::ctx	= "NMCostDistanceBufferImageWrapper";

NMCostDistanceBufferImageWrapper::NMCostDistanceBufferImageWrapper(QObject* parent)
	: mMemoryMax(32), mUseImageSpacing(true),
	  mCreateBuffer(false)
{
	this->setParent(parent);
	this->mInputComponentType = itk::ImageIOBase::INT;
	this->mOutputComponentType = itk::ImageIOBase::DOUBLE;
	this->mParameterHandling = NMProcess::NM_USE_UP;
	this->mParamPos = 0;
	this->mbRasMode = false;
#ifdef BUILD_RASSUPPORT
	this->mRasconn = 0;
	this->mRasConnector = 0;
#endif

}

NMCostDistanceBufferImageWrapper::~NMCostDistanceBufferImageWrapper()
{
}

#ifdef BUILD_RASSUPPORT
void
NMCostDistanceBufferImageWrapper::setRasdamanConnector(RasdamanConnector * rasconn)
{
	this->mRasconn = rasconn;
}
#endif


void
NMCostDistanceBufferImageWrapper::update(void)
{
	if (!this->mbIsInitialised)
		return;

	switch(this->mInputComponentType)
	{
	MacroPerType( callInputTypeInternalExecute, NMCostDistanceBufferImageWrapper_Internal )
	default:
		break;
	}
}

void
NMCostDistanceBufferImageWrapper::linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	// this process component doesn't have any 'buffered' data input but reads its own
	// data in chunks as required, so we don't do anything here; we only want to avoid
	// that the superclass' method is being called;
}

