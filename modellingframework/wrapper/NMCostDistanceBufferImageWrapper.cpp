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
#include "NMMfwException.h"

#include <QObject>
#include <QString>

#include "itkProcessObject.h"
#include "itkTimeProbe.h"
#include "otbImage.h"
#include "otbGDALRATImageIO.h"
#include "itkNMCostDistanceBufferImageFilter.h"
//#include "itkNMCostDistanceBufferImageFilter_ExplicitInst.h"
#include "otbStreamingRATImageFileWriter.h"
#include "otbImageFileWriter.h"
#include "otbImageFileReader.h"
#include "otbImageIOBase.h"

#ifdef BUILD_RASSUPPORT
    #include "otbRasdamanImageReader.h"
#endif



template <class InPixelType, unsigned int Dimension>
class NMCostDistanceBufferImageWrapper_Internal
{
//private:
//	static const std::string ctx;

public:
    typedef otb::Image<InPixelType, 2> InputImgType;
    typedef otb::Image<float, 2> OutputImgType;
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
            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, const QString& name)
    {
        InputImgType* img = dynamic_cast<InputImgType*>(dataObj);
        DistanceFilterType* filter = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
        if (!name.isEmpty())
        {
            filter->SetInput(name.toStdString(), img);
        }
        filter->SetInput(idx, img);
    }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx)
    {
        DistanceFilterType* f = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
        return dynamic_cast<OutputImgType*>(f->GetOutput(idx));
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

        QVariant objVarList = p->getParameter("ObjectValueList");
        if (objVarList.isValid())
        {
            std::vector<double> objValueVec;
            QStringList objValueList = objVarList.toStringList();
            foreach(const QString& vStr, objValueList)
            {
                double val = vStr.toDouble(&bok);
                if (bok)
                {
                    objValueVec.push_back(val);
                }
                else
                {
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setSource(p->parent()->objectName().toStdString());
                    e.setDescription("Invalid ObjectValue!");
                    throw e;

                    NMLogError(<<  "NMCostDistanceBufferImageWrapper" << "Invalid ObjectValue!");
                    return;
                }
            }
            f->SetCategories(objValueVec);
            QString objValueListProvN = QString("nm:ObjectValueList=\"%1\"").arg(objValueList.join(' '));
            p->addRunTimeParaProvN(objValueListProvN);
        }


        QVariant curMaxDistanceVar = p->getParameter("MaxDistance");
        QString maxDistProvVal;
        if (curMaxDistanceVar.isValid())
        {
            if (!curMaxDistanceVar.toString().isEmpty())
            {
                double curMaxDistance = curMaxDistanceVar.toDouble(&bok);
                if (bok)
                {
                    f->SetMaxDistance(curMaxDistance);
                    maxDistProvVal = QString("%1").arg(curMaxDistance);
                }
            }
        }
        QString maxDistProvN = QString("nm:MaxDistance=\"%1\"").arg(maxDistProvVal);
        p->addRunTimeParaProvN(maxDistProvN);

        // -------------------------------------------------------------------
        // set buffer zone indicator

        QVariant curBufferZoneIndicatorVar = p->getParameter("BufferZoneIndicator");
        QString bufferZoneIDProvVal;
        if (curBufferZoneIndicatorVar.isValid())
        {
            if (!curBufferZoneIndicatorVar.toString().isEmpty())
            {
                int curBufferZoneIndicator = curBufferZoneIndicatorVar.toInt(&bok);
                if (bok)
                {
                    f->SetBufferZoneIndicator(curBufferZoneIndicator);
                    bufferZoneIDProvVal = QString("%1").arg(curBufferZoneIndicator);
                }
            }
        }
        QString bufferZoneIDProvN = QString("nm:BufferZoneIndicator=\"%1\"").arg(bufferZoneIDProvVal);
        p->addRunTimeParaProvN(bufferZoneIDProvN);


        // -------------------------------------------------------------------
        // set the other 'singular / fixed' parameters
        // since we know both variables are boolean and that they are assigned
        // default values, we can rely on them being valid in any case

        QVariant curUseImgSpacVar = p->getParameter("UseImageSpacing");
        if (curUseImgSpacVar.isValid())
        {
            if (!curUseImgSpacVar.toString().isEmpty())
            {
                bool useImgSpac = curUseImgSpacVar.toBool();
                f->SetUseImageSpacing(useImgSpac);
                QString useImgSpacProvN = QString("nm:UseImageSpacing=\"%1\"").arg(useImgSpac);
                p->addRunTimeParaProvN(useImgSpacProvN);
            }
        }


        QVariant curCreateBufferVar = p->getParameter("CreateBuffer");
        if (curCreateBufferVar.isValid())
        {
            if (!curCreateBufferVar.toString().isEmpty())
            {
                bool createBuffer = curCreateBufferVar.toBool();
                f->SetCreateBuffer(createBuffer);
                QString createBufferProvN = QString("nm:CreateBuffer=\"%1\"").arg(createBuffer);
                p->addRunTimeParaProvN(createBufferProvN);
            }
        }


        // set the observer for this process object
        NMCostDistanceBufferImageWrapper::DistanceObserverType::Pointer observer =
                NMCostDistanceBufferImageWrapper::DistanceObserverType::New();
        observer->SetCallbackFunction(p,
                &NMCostDistanceBufferImageWrapper::UpdateProgressInfo);
        f->AddObserver(itk::ProgressEvent(), observer);
        //f->AddObserver(itk::StartEvent(), observer);
        //f->AddObserver(itk::EndEvent(), observer);
        f->AddObserver(itk::AbortEvent(), observer);
    }

    static void execute(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, NMProcess* proc)
    {
        NMDebugCtx("CostDistanceInternal", << "...");

        DistanceFilterType* distfilter = dynamic_cast<DistanceFilterType*>(otbFilter.GetPointer());
        NMCostDistanceBufferImageWrapper* p = qobject_cast<NMCostDistanceBufferImageWrapper*>(proc);

        // get the input, cost, and output image filenames
        int pos = p->mParamPos;

        QVariant fileNameVar = p->getParameter("InputImageFileName");
        QString fileName;
        if (fileNameVar.isValid())
        {
            fileName = fileNameVar.toString();
        }

        if (fileName.isEmpty())
        {
            NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
            e.setSource(p->parent()->objectName().toStdString());
            e.setDescription("No input image filename!");
            throw e;

            NMLogError(<< "CostDistanceInternal: Please provide an input image file name!");
            return;
        }

        bool bInRas = false;
        if (!fileName.contains('.'))
            bInRas = true;

        pos = p->mParamPos;

        QVariant costFNVar = p->getParameter("CostImageFileName");
        QString costFN;
        if (costFNVar.isValid())
        {
            costFN = costFNVar.toString();
        }


        bool bCostRas = false;
        if (!costFN.contains('.'))
        {
            bCostRas = true;
        }

        pos = p->mParamPos;

        QVariant outputImageFNVar = p->getParameter("OutputImageFileName");
        QString out;
        if (outputImageFNVar.isValid())
        {
            out = outputImageFNVar.toString();
        }

        if (out.isEmpty())
        {
            NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
            e.setSource(p->parent()->objectName().toStdString());
            e.setDescription("No output image filename!");
            throw e;

            NMLogError(<< "NMCostDistanceBufferImageWrapper: No output image filename!");
            return;
        }

        bool bOutRas = false;
        if (!out.contains('.'))
            bOutRas = true;


#ifdef BUILD_RASSUPPORT
        RasdamanConnector* rcon = 0;
        if (bOutRas || bInRas || bCostRas)
        {
            if (p->mRasConnector == 0)
            {
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(p->parent()->objectName().toStdString());
                e.setDescription("Invalid RasdamanConnector object!");
                throw e;

                NMLogError(<<  "NMCostDistanceBufferImageWrapper" << "RasdamanConnector object!");
                return;
            }
            else
            {
                rcon = const_cast<RasdamanConnector*>(
                        p->mRasConnector->getConnector());
            }
        }


        typename RasReaderType::Pointer rasreader;
        typename RasReaderType::Pointer rascostreader;
        typename RasOutReaderType::Pointer rasoutreader;
#endif


        // instantiate readers and writers
        otb::GDALRATImageIO::Pointer inrio;
        typename ReaderType::Pointer reader;
#ifdef BUILD_RASSUPPORT
        if (bInRas)
        {
            rasreader = RasReaderType::New();
            rasreader->SetRasdamanConnector(rcon);
            rasreader->SetFileName(fileName.toStdString().c_str());
        }
        else
#endif
        {
            reader = ReaderType::New();
            inrio = otb::GDALRATImageIO::New();
            inrio->SetRATSupport(true);
            reader->SetFileName(fileName.toStdString().c_str());
            reader->SetImageIO(dynamic_cast<otb::ImageIOBase*>(inrio.GetPointer()));
        }



        typename TmpOutReaderType::Pointer tmpOutReader;// = TmpOutReaderType::New();
        otb::GDALRATImageIO::Pointer tmpOutGIO;
#ifdef BUILD_RASSUPPORT
        if (bOutRas)
        {
            rasoutreader = RasOutReaderType::New();
            rasoutreader->SetRasdamanConnector(rcon);
        }
        else
#endif
        {
            tmpOutReader = TmpOutReaderType::New();
            tmpOutGIO = otb::GDALRATImageIO::New();
            tmpOutReader->SetImageIO(dynamic_cast<otb::ImageIOBase*>(tmpOutGIO.GetPointer()));
        }


        typedef otb::StreamingRATImageFileWriter<OutputImgType> WriterType;
        typename WriterType::Pointer writer = WriterType::New();
        writer->SetFileName(out.toStdString().c_str());
        otb::GDALRATImageIO::Pointer outgio;
#ifdef BUILD_RASSUPPORT
        if (bOutRas)
        {
            writer->SetRasdamanConnector(rcon);
            writer->SetFileName(out.toStdString().c_str());
        }
        else
#endif
        {
            outgio = otb::GDALRATImageIO::New();
            writer->SetImageIO(dynamic_cast<otb::ImageIOBase*>(outgio.GetPointer()));
        }
        //writer->SetNumberOfDivisionsStrippedStreaming(1);

        // let's get an idea about the size of the image to process
        typename InputImgType::RegionType lpr;
        typename InputImgType::RegionType pr;
        typename InputImgType::SpacingType inputSpacing;
        typename InputImgType::PointType inputOrigin;
#ifdef BUILD_RASSUPPORT
        if (bInRas)
        {
            rasreader->GetOutput()->UpdateOutputInformation();
            lpr          = rasreader->GetOutput()->GetLargestPossibleRegion();
            inputSpacing = rasreader->GetOutput()->GetSignedSpacing();
            inputOrigin  = rasreader->GetOutput()->GetOrigin();
        }
        else
#endif
        {
            reader->GetOutput()->UpdateOutputInformation();
            lpr          = reader->GetOutput()->GetLargestPossibleRegion();
            inputSpacing = reader->GetOutput()->GetSignedSpacing();
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
        writer->SetStreamingMethod("NO_STREAMING");

        typename DistanceFilterType::OutputImageRegionType distOutLPR;
        distOutLPR.SetSize(0,  lpr.GetSize()[0]);
        distOutLPR.SetSize(1,  lpr.GetSize()[1]);
        distOutLPR.SetIndex(0,  lpr.GetIndex()[0]);
        distOutLPR.SetIndex(1,  lpr.GetIndex()[1]);

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
            NMLogError(<< "CostDistanceInternal: Chunk size below minimum (< 2)!");
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
#ifdef BUILD_RASSUPPORT
        if (bInRas)
            distfilter->SetInput(rasreader->GetOutput());
        else
#endif
            distfilter->SetInput(reader->GetOutput());

        typename ReaderType::Pointer costreader;
        if (!costFN.isNull())
        {
#ifdef BUILD_RASSUPPORT
            if (bCostRas)
            {
                rascostreader = RasReaderType::New();
                rascostreader->SetRasdamanConnector(rcon);
                rascostreader->SetFileName(costFN.toStdString().c_str());
                distfilter->SetInput(1, rascostreader->GetOutput());
            }
            else
#endif
            {
                costreader = ReaderType::New();
                costreader->SetFileName(costFN.toStdString().c_str());
                distfilter->SetInput(1, costreader->GetOutput());
            }
        }

        /* ================================================================== */
        /* just check the single-run algorithm */

        p->UpdateProgressInfo(distfilter, itk::StartEvent());
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
                p->UpdateProgressInfo(distfilter, itk::EndEvent());
                return;
            }
        /* ================================================================== */


        distfilter->resetExecCounter();
        distfilter->SetProcessDownward(true);
        // iterate over the regions (chunks of rows)
        // to get the job done
        //QString nfn = QString("%1:_last_").arg(out.toStdString().c_str());
        for (int iter=0; iter <= niter && !p->mbAbortExecution; ++iter)
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
                    tmpOutReader->SetImageIO(
                            dynamic_cast<otb::ImageIOBase*>(tmpOutGIO.GetPointer()));
                    tmpOutReader->GetOutput()->SetRequestedRegion(pr);
                    tmpOutReader->GetOutput()->SetLargestPossibleRegion(distOutLPR);
                    tmpOutReader->Update();

                    tout = tmpOutReader->GetOutput();

                    //writer->SetFileName(out.toStdString().c_str());
                }
#ifdef BUILD_RASSUPPORT
                else
                {
                    rasoutreader->SetFileName(nfn.toStdString().c_str());
                    rasoutreader->GetOutput()->SetRequestedRegion(pr);
                    rasoutreader->Update();

                    tout = rasoutreader->GetOutput();
                    writer->SetFileName(nfn.toStdString().c_str());
                }
#endif
                tout->DisconnectPipeline();
                distfilter->GraftOutput(tout);

                writer->SetUpdateMode(true);
            }

            // set the largest possible and requested regions
            distfilter->GetOutput()->SetRequestedRegion(pr);
            distfilter->GetOutput()->SetLargestPossibleRegion(distOutLPR);
            distfilter->Update();

            OutputImgType::Pointer di = distfilter->GetOutput();
            di->DisconnectPipeline();

            writer->SetInput(0, di);
            //writer->SetInput(0, distfilter->GetOutput());
            writer->SetUpdateRegion(ior);
            //writer->GetOutput()->SetRequestedRegion(pr);
            writer->Update();

            // need to close the datasets explicitly this time,
            // since they're not closed for re-reading in non
            // update mode
            if (iter == 0)
            {
#ifdef BUILD_RASSUPPORT
                if (!bOutRas)
#endif
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
        for (int iter=0; iter <= niter && !p->mbAbortExecution; ++iter)
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
                tmpOutReader->SetImageIO(
                        dynamic_cast<otb::ImageIOBase*>(tmpOutGIO.GetPointer()));
                tmpOutReader->GetOutput()->SetRequestedRegion(pr);
                tmpOutReader->GetOutput()->SetLargestPossibleRegion(distOutLPR);
                tmpOutReader->Update();

                tout = tmpOutReader->GetOutput();
            }
#ifdef BUILD_RASSUPPORT
            else
            {
                QString nfn = QString("%1:_last_").arg(out.toStdString().c_str());
                rasoutreader->SetFileName(nfn.toStdString().c_str());
                rasoutreader->GetOutput()->SetRequestedRegion(pr);
                rasoutreader->Update();

                tout = rasoutreader->GetOutput();
            }
#endif
            tout->DisconnectPipeline();
            distfilter->GraftOutput(tout);

            // set the largest possible and requested regions
            distfilter->GetOutput()->SetRequestedRegion(pr);
            distfilter->GetOutput()->SetLargestPossibleRegion(distOutLPR);
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

        p->UpdateProgressInfo(distfilter, itk::EndEvent());
        NMDebugCtx("CostDistanceInternal", << "done!");
    }

};

template class NMCostDistanceBufferImageWrapper_Internal<unsigned char, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<char, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned short, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<short, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned int, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<int, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned long, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<long, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<float, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<double, 1>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned char, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<char, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned short, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<short, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned int, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<int, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned long, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<long, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<float, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<double, 2>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned char, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<char, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned short, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<short, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned int, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<int, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<unsigned long, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<long, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<float, 3>;
template class NMCostDistanceBufferImageWrapper_Internal<double, 3>;


InstantiateInputTypeObjectWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
SetInputTypeNthInputWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
GetInputTypeOutputWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )
LinkInputTypeInternalParametersWrap( NMCostDistanceBufferImageWrapper, NMCostDistanceBufferImageWrapper_Internal )

NMCostDistanceBufferImageWrapper::NMCostDistanceBufferImageWrapper(QObject* parent)
    : mMemoryMax(512), mUseImageSpacing(true),
      mCreateBuffer(false)
{
    this->setParent(parent);
    this->setObjectName("NMCostDistanceBufferImageWrapper");
    this->mInputComponentType = otb::ImageIOBase::INT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mParamPos = 0;
    this->mbRasMode = false;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 2;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("InputImageFileName"), QStringLiteral("InputImageFileName"));
    mUserProperties.insert(QStringLiteral("CostImageFileName"), QStringLiteral("CostImageFileName"));
    mUserProperties.insert(QStringLiteral("OutputImageFileName"), QStringLiteral("OutputImageFileName"));
    mUserProperties.insert(QStringLiteral("ObjectValueList"), QStringLiteral("ObjectValueList"));
    mUserProperties.insert(QStringLiteral("MaxDistance"), QStringLiteral("MaxDistance"));
    mUserProperties.insert(QStringLiteral("UseImageSpacing"), QStringLiteral("UseImageSpacing"));
    mUserProperties.insert(QStringLiteral("CreateBuffer"), QStringLiteral("CreateBuffer"));
    mUserProperties.insert(QStringLiteral("BufferZoneIndicator"), QStringLiteral("BufferZoneIndicator"));


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

void NMCostDistanceBufferImageWrapper::setRasConnector(NMRasdamanConnectorWrapper* rw)
{
    if (rw != this->mRasConnector)
    {
        this->mRasConnector = rw;
        emit nmChanged();
    }
}

NMRasdamanConnectorWrapper* NMCostDistanceBufferImageWrapper::getRasConnector(void)
{
    return this->mRasConnector;
}

#endif

void
NMCostDistanceBufferImageWrapper::UpdateProgressInfo(itk::Object* obj, const itk::EventObject& event)
{
    // just call base class implementation here
    NMProcess::UpdateProgressInfo(obj, event);
}

void
NMCostDistanceBufferImageWrapper::update()
{
    if (!this->mbIsInitialised)
        return;

    int oldParamPos = this->mParamPos;
    switch(this->mInputComponentType)
    {
    MacroPerType( callInputTypeInternalExecute, NMCostDistanceBufferImageWrapper_Internal )
    default:
        break;
    }

    this->mMTime = QDateTime::currentDateTimeUtc();
    this->mParamPos = oldParamPos + 1;
}

void
NMCostDistanceBufferImageWrapper::linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    // this process component doesn't have any 'buffered' data input but reads its own
    // data in chunks as required, so we don't do anything here; we only want to avoid
    // that the superclass' method is being called;
}


