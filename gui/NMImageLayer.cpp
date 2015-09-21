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
 * NMImageLayer.cpp
 *
 *  Created on: 18/01/2012
 *      Author: alex
 */
#include "NMImageLayer.h"
#include "NMQtOtbAttributeTableModel.h"
#include "NMFastTrackSelectionModel.h"
#include "NMTableCalculator.h"
#include "NMItkDataObjectWrapper.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "NMRasdamanConnectorWrapper.h"
#include "NMMacros.h"
#include "NMGlobalHelper.h"
#include "NMSqlTableModel.h"
#include "NMSqlTableView.h"

#include <QTime>
#include <QtCore>
#include <QtConcurrent>
#include <QInputDialog>

#include "itkDataObject.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkImageRegion.h"
#include "itkPoint.h"
#include "itkExceptionObject.h"
#include "itkImageRegion.h"

#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkPointData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkInteractorObserver.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkImageCast.h"
#include "vtkLongArray.h"
#include "vtkIdList.h"
#include "vtkGeometryFilter.h"
#include "vtkExtractCells.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkGenericContourFilter.h"
#include "vtkExtractSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelection.h"
#include "vtkProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkImageMapper.h"
#include "vtkCamera.h"
#include "vtkActor2D.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkImageActor.h"
#include "vtkImageSliceMapper.h"
#include "NMVtkOpenGLImageSliceMapper.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkSetGet.h"

//#include "valgrind/callgrind.h"

template<class PixelType, unsigned int Dimension>
class InternalImageHelper
{
public:
	typedef otb::Image<PixelType, Dimension> ImgType;
	typedef typename ImgType::RegionType RegionType;
	typedef otb::VectorImage<PixelType, Dimension> VecImgType;
	typedef typename VecImgType::RegionType VecRegionType;

    static void getBBox(itk::DataObject::Pointer& img, unsigned int numBands,
			double* bbox)
		{
            if (img.IsNull() || img.GetPointer() == 0 || bbox == 0) return;

            // we set everything to zero here
            for(int i=0; i < 6; ++i)
            {
                bbox[i] = 0;
            }

			if (numBands == 1)
			{
                ImgType* theimg = dynamic_cast<ImgType*>(img.GetPointer());
                if (theimg == 0)
                {
                    return;
                }
                RegionType reg = theimg->GetLargestPossibleRegion();//theimg->GetBufferedRegion();

                // problem here is that we could also be dealing with an
                // image of type 'itk::Image' rather than 'otb::Image',
                // so using the convenient 'GetLowerLeftCorner()' etc.
                // functions would crash with 'itk::Image' and hence crash
                //				bbox[0] = theimg->GetLowerLeftCorner()[0];  // minx
                //				bbox[2] = theimg->GetLowerLeftCorner()[1];  // miny
                //				bbox[1] = theimg->GetUpperRightCorner()[0]; // maxx
                //				bbox[3] = theimg->GetUpperRightCorner()[1]; // maxy
                //				bbox[5] = 0;
                //				bbox[6] = 0;

                //				if (theimg->GetImageDimension() == 3)
                //				{
                //					bbox[5] = theimg->GetOrigin()[2];
                //					bbox[6] = bbox[5] + theimg->GetSpacing()[2] * reg.GetSize()[2];
                //				}

                bbox[0] = theimg->GetOrigin()[0];
                bbox[1] = bbox[0] + (theimg->GetSpacing()[0] * reg.GetSize()[0]);
                bbox[2] = theimg->GetOrigin()[1] + (theimg->GetSpacing()[1] * reg.GetSize()[1]);
                bbox[3] = theimg->GetOrigin()[1];
                bbox[4] = 0;
                bbox[5] = 0;
                if (theimg->GetImageDimension() == 3)
                {
                    bbox[4] = theimg->GetOrigin()[2];
                    bbox[5] = bbox[4] + (theimg->GetSpacing()[2] * reg.GetSize()[2]);
                }


			}
			else if (numBands > 1)
			{
                VecImgType* theimg = dynamic_cast<VecImgType*>(img.GetPointer());
                if (theimg == 0)
                {
                    return;
                }
                VecRegionType reg = theimg->GetLargestPossibleRegion();//theimg->GetBufferedRegion();

                bbox[0] = theimg->GetOrigin()[0];
                bbox[1] = bbox[0] + (theimg->GetSpacing()[0] * reg.GetSize()[0]);
                bbox[2] = theimg->GetOrigin()[1] + (theimg->GetSpacing()[1] * reg.GetSize()[1]);
                bbox[3] = theimg->GetOrigin()[1];
                bbox[4] = 0;
                bbox[5] = 0;
                //				bbox[0] = theimg->GetLowerLeftCorner()[0];
                //				bbox[2] = theimg->GetLowerLeftCorner()[1];
                //				bbox[1] = theimg->GetUpperRightCorner()[0];
                //				bbox[3] = theimg->GetUpperRightCorner()[1];
                //                bbox[4] = 0;
                //                bbox[5] = 0;

				if (theimg->GetImageDimension() == 3)
				{
					bbox[5] = theimg->GetOrigin()[2];
					bbox[6] = bbox[5] + theimg->GetSpacing()[2] * reg.GetSize()[2];
				}
			}
		}
};

/** macro for querying the bbox */
#define getInternalBBox( PixelType, wrapName ) \
{	\
	if (numDims == 2) \
        wrapName<PixelType, 2>::getBBox(img, numBands, bbox); \
	else \
        wrapName<PixelType, 3>::getBBox(img, numBands, bbox); \
}

NMImageLayer::NMImageLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
	: NMLayer(renWin, renderer, parent)
{
	this->mLayerType = NMLayer::NM_IMAGE_LAYER;
    this->mReader = 0; //new NMImageReader(this);
	this->mPipeconn = new NMItk2VtkConnector(this);
	this->mFileName = "";

	this->mNumBands = 0;
	this->mNumDimensions = 0;
    this->mNumRecords = 0;
	this->mComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mbStatsAvailable = false;

	this->mLayerIcon = QIcon(":image_layer.png");

    this->mOverviewIdx = -1; // (loading original image)
    this->mbUseOverviews = true;
    this->mBandMap.clear();
    this->mBandMinMax.clear();
    this->mScalarBand = 1;

    this->mScalarColIdx = -1;
    this->mScalarBufferFile = 0;

    //vtkInteractorObserver* style = mRenderWindow->GetInteractor()->GetInteractorStyle();
    //this->mVtkConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();

//	this->mVtkConn->Connect(style, vtkCommand::ResetWindowLevelEvent,
//			this, SLOT(windowLevelReset(vtkObject*)));
//	this->mVtkConn->Connect(style, vtkCommand::InteractionEvent,
//			this, SLOT(windowLevelChanged(vtkObject*)));
}

NMImageLayer::~NMImageLayer()
{
    if (mScalarBufferFile != 0)
    {
        fclose(mScalarBufferFile);
    }
	if (this->mReader)
		delete this->mReader;
	if (this->mPipeconn)
		delete this->mPipeconn;
}

std::vector<double>
NMImageLayer::getWindowStatistics(void)
{
    std::vector<double> ret;

    if (this->mNumBands > 1)
    {
        for (int i=0; i < 7; ++i)
        {
            ret.push_back(-9999);
        }
        return ret;
    }

    emit layerProcessingStart();

    vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
    cast->SetOutputScalarTypeToDouble();
    cast->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

    vtkSmartPointer<vtkImageHistogramStatistics> stats =
            vtkSmartPointer<vtkImageHistogramStatistics>::New();
    stats->SetInputConnection(cast->GetOutputPort());

    stats->Update();

    ret.push_back(stats->GetMinimum());
    ret.push_back(stats->GetMaximum());
    ret.push_back(stats->GetMean());
    ret.push_back(stats->GetMedian());
    ret.push_back(stats->GetStandardDeviation());
    ret.push_back(mPipeconn->getVtkImage()->GetNumberOfPoints());
    ret.push_back(-9999);

    emit layerProcessingEnd();
    return ret;
}

std::vector<double>
NMImageLayer::getWholeImageStatistics(void)
{
    std::vector<double> ret;

    if (this->mNumBands > 1)
    {
        for (int i=0; i < 7; ++i)
        {
            ret.push_back(-9999);
            return ret;
        }
    }

    NMImageReader* imgReader = 0;
    NMItk2VtkConnector* vtkConn = 0;

    if (!mbStatsAvailable)
    {
        emit layerProcessingStart();

        vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
        cast->SetOutputScalarTypeToDouble();
        if (this->mFileName.isEmpty())
        {
            cast->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());
        }
        else
        {
            imgReader = new NMImageReader(0);
            imgReader->setFileName(this->mFileName);
            imgReader->instantiateObject();
            if (!imgReader->isInitialised())
            {
                NMWarn(ctxNMImageLayer, << "Failed initialising NMImageReader!");
                return ret;
            }

            vtkConn = new NMItk2VtkConnector(0);
            vtkConn->setInput(imgReader->getOutput(0));
            cast->SetInputConnection(vtkConn->getVtkAlgorithmOutput());
        }

        vtkSmartPointer<vtkImageHistogramStatistics> stats =
                vtkSmartPointer<vtkImageHistogramStatistics>::New();
        stats->SetInputConnection(cast->GetOutputPort());
        stats->GenerateHistogramImageOff();

        // we leave two threads to keep the OS and UI usable
        int threadNum = max(1, QThread::idealThreadCount()-2);
        stats->SetNumberOfThreads(threadNum);
        stats->Update();

        mImgStats[0] = stats->GetMinimum();
        mImgStats[1] = stats->GetMaximum();
        mImgStats[2] = stats->GetMean();
        mImgStats[3] = stats->GetMedian();
        mImgStats[4] = stats->GetStandardDeviation();
        mImgStats[5] = vtkConn->getVtkImage()->GetNumberOfPoints();
        this->mbStatsAvailable = true;

        emit layerProcessingEnd();
    }

    ret.push_back(mImgStats[0]);
    ret.push_back(mImgStats[1]);
    ret.push_back(mImgStats[2]);
    ret.push_back(mImgStats[3]);
    ret.push_back(mImgStats[4]);
    ret.push_back(mImgStats[5]);
    ret.push_back(-9999);

    if (imgReader) delete imgReader;
    if (vtkConn) delete vtkConn;

    return ret;
}

void NMImageLayer::test()
{
}

double NMImageLayer::getDefaultNodata()
{
	double nodata;

	switch(this->mComponentType)
	{
		case otb::ImageIOBase::UCHAR:
            nodata = std::numeric_limits<unsigned char>::max();//255;
			break;
		case otb::ImageIOBase::CHAR:
			nodata = -std::numeric_limits<char>::max();
			break;
		case otb::ImageIOBase::USHORT:
			nodata = std::numeric_limits<unsigned short>::max();
			break;
		case otb::ImageIOBase::SHORT:
			nodata = -std::numeric_limits<short>::max();
			break;
		case otb::ImageIOBase::UINT:
			nodata = std::numeric_limits<unsigned int>::max();
			break;
		case otb::ImageIOBase::INT:
			nodata = -std::numeric_limits<int>::max();
			break;
		case otb::ImageIOBase::ULONG:
			nodata = std::numeric_limits<unsigned long>::max();
			break;
		case otb::ImageIOBase::LONG:
			nodata = -std::numeric_limits<long>::max();
			break;
		case otb::ImageIOBase::DOUBLE:
			nodata = -std::numeric_limits<double>::max();
			break;
		case otb::ImageIOBase::FLOAT:
			nodata = -std::numeric_limits<float>::max();
			break;
		default: break;//					   nodata = -2147483647; break;
	}

	return nodata;
}


void NMImageLayer::world2pixel(double world[3], int pixel[3],
    bool bOnLPR, bool bImgConstrained)
{
	// we tweak the world coordinates a little bit to
	// generate the illusion as if we had pixel-centered
	// coordinates in vtk as well
    double wcoord[3];
    for (int i=0; i<3; ++i)
    {
        wcoord[i] = world[i];
    }
    double spacing[3];
	double origin[3];
    int dims[3] = {0,0,0};
    double bnd[6];
	double err[3];

    vtkImageData* img = vtkImageData::SafeDownCast(
                const_cast<vtkDataSet*>(this->getDataSet()));

    if (img == 0)
        return;

    img->GetSpacing(spacing);
    img->GetOrigin(origin);

    // adjust coordinates & calc bnd
    unsigned int d;
    for (d = 0; d < this->mNumDimensions; ++d)
    {
        if (bOnLPR)
        {
            spacing[d] = mSpacing[d];
        }
        dims[d] = (mBBox[d*2+1] - mBBox[d*2]) / ::fabs(spacing[d]);

        // account for 'pixel corner coordinate' vs.
        // 'pixel centre coordinate' philosophy of
        // vtk vs itk
        wcoord[d] += spacing[d] / 2.0;
        // set the 'pointing accuracy' to 1 percent of
        // pixel width for each dimension
        err[d] = spacing[d] * 0.001;
    }

	// check, whether the user point is within the
	// image boundary
    if (bImgConstrained)
    {
        if (vtkMath::PointIsWithinBounds(wcoord, mBBox, err))
        {
            // calculate the image/pixel coordinates
            for (d = 0; d < this->mNumDimensions; ++d)
            {
                pixel[d] = 0;

                if (dims[d] > 0)
                    pixel[d] = ::abs((int)((wcoord[d] - origin[d]) / spacing[d]));
            }
        }
    }
    else
    {
        for (d = 0; d < this->mNumDimensions; ++d)
        {
            if (dims[d] > 0)
                pixel[d] = (int)((wcoord[d] - origin[d]) / spacing[d]);
        }
    }

    // fill up until 3rd dimension with zeros, if applicable
    for (; d < 3; ++d)
    {
        pixel[d] = 0;
    }
}

void
NMImageLayer::selectionChanged(const QItemSelection& newSel,
		const QItemSelection& oldSel)
{

	// call the base class implementation to do datatype agnostic stuff
	NMLayer::selectionChanged(newSel, oldSel);

	emit visibilityChanged(this);
	emit legendChanged(this);
}

void NMImageLayer::createTableView(void)
{
    if (this->mSqlTableView != 0)
	{
		return;
	}

	if (this->mOtbRAT.IsNull())
	{
		if (!this->updateAttributeTable())
			return;
	}

	if (this->mTableModel == 0)
		return;

    this->mSqlTableView = new NMSqlTableView(
                qobject_cast<NMSqlTableModel*>(mTableModel), 0);
    this->mSqlTableView->setSelectionModel(this->mSelectionModel);
    this->mSqlTableView->setTitle(tr("Attributes of ") + this->objectName());

    connect(this, SIGNAL(selectabilityChanged(bool)),
            mSqlTableView, SLOT(setSelectable(bool)));
    connect(mSqlTableView, SIGNAL(notifyLastClickedRow(long)),
            this, SLOT(forwardLastClickedRowSignal(long)));
}

int
NMImageLayer::updateAttributeTable()
{
	if (this->mTableModel != 0)
		return 1;

    this->mOtbRAT = this->getRasterAttributeTable(1);
    if (mOtbRAT.IsNull())
	{
        mOtbRAT = 0;
        //NMDebugAI(<< "No attribute table available!");
		return 0;
	}

    mNumRecords = mOtbRAT->GetNumRows();
    if (mNumRecords == 0)
    {
        mOtbRAT = 0;
        return 0;
    }

    //NMQtOtbAttributeTableModel* otbModel;
    NMSqlTableModel* tabModel = 0;
	if (this->mTableModel == 0)
	{
        //otbModel = new NMQtOtbAttributeTableModel(this->mOtbRAT);
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(QString(mOtbRAT->getDbFileName().c_str()));
        if (!db.open())
        {
            NMErr(ctxNMImageLayer, << "Open database failed!" << endl);
        }
        else
        {
            tabModel = new NMSqlTableModel(this, db);
            tabModel->setTable(QString(mOtbRAT->getTableName().c_str()));
            tabModel->select();
        }
	}
	else
	{
        tabModel = qobject_cast<NMSqlTableModel*>(this->mTableModel);
        //		otbModel = qobject_cast<NMQtOtbAttributeTableModel*>(this->mTableModel);
        //		otbModel->setTable(this->mOtbRAT);
        //		otbModel->setKeyColumn("rowidx");
	}
    //otbModel->setKeyColumn("rowidx");

	// in any case, we create a new item selection model
    if (this->mSelectionModel == 0 && tabModel != 0)
	{
        //this->mSelectionModel = new NMFastTrackSelectionModel(otbModel, this);
        this->mSelectionModel = new NMFastTrackSelectionModel(tabModel, this);
	}
    //this->mTableModel = otbModel;
    this->mTableModel = tabModel;
    tabModel = 0;

    if (mSelectionModel)
    {
        connectTableSel();
    }

    emit legendChanged(this);

	return 1;
}

bool
NMImageLayer::setFileName(QString filename)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	if (filename.isEmpty() || filename.isNull())
	{
		NMErr(ctxNMImageLayer, << "invalid filename!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
	}

    // allocate the reader object, if it hasn't been so
    if (this->mReader == 0)
    {
        this->mReader = new NMImageReader(this);
        // we always set the RGBMode here, 'cause we never
        // display more than 3 bands at a time
        this->mReader->setRGBMode(true);
    }

	// TODO: find a more unambiguous way of determining
	// whether we're loading a rasdaman image or not
#ifdef BUILD_RASSUPPORT
	if (!filename.contains(".") && this->mpRasconn != 0)
	{
		NMRasdamanConnectorWrapper raswrap;
		raswrap.setConnector(this->mpRasconn);
		this->mReader->setRasConnector(&raswrap);
	}

#endif

    emit layerProcessingStart();

    this->mReader->setFileName(filename);
	this->mReader->instantiateObject();
	if (!this->mReader->isInitialised())
	{
		emit layerProcessingEnd();
		NMErr(ctxNMImageLayer, << "couldn't read the image!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
	}
    this->mReader->getInternalProc()->ReleaseDataFlagOn();

    // get original image attributes before we muck
    // around with scaling and overviews
    this->mReader->getBBox(this->mBBox);
    this->mReader->getBBox(this->mBufferedBox);
    this->mReader->getSpacing(this->mSpacing);
    this->mReader->getOrigin(this->mOrigin);

    // let's store some meta data, in case someone needs it
	this->mComponentType = this->mReader->getOutputComponentType();
	this->mNodata = this->getDefaultNodata();
	this->mNumBands = this->mReader->getOutputNumBands();
    this->mTotalNumBands = this->mReader->getInputNumBands();
	this->mNumDimensions = this->mReader->getOutputNumDimensions();

    if (mNumBands == 3)
    {
        mBandMap.clear();
        mBandMap.push_back(1);
        mBandMap.push_back(2);
        mBandMap.push_back(3);
    }
    // ==> set the desired overview and requested region, if supported
    this->mapExtentChanged();

	// concatenate the pipeline
    this->mPipeconn->setInput(this->mReader->getOutput(0));

    vtkSmartPointer<NMVtkOpenGLImageSliceMapper> m = vtkSmartPointer<NMVtkOpenGLImageSliceMapper>::New();
    m->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());
    m->SetNMLayer(this);
    m->SetBorder(1);

    vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
    a->SetMapper(m);

    mImgProp = vtkSmartPointer<vtkImageProperty>::New();
    mImgProp->SetInterpolationTypeToNearest();
    a->SetProperty(mImgProp);

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
    this->mActor = a;

	this->mImage = 0;
	this->mFileName = filename;
    this->mMapper->Update();
    this->initiateLegend();

    emit layerProcessingEnd();
    emit layerLoaded();
    NMDebugCtx(ctxNMImageLayer, << "done!");
	return true;
}

void
NMImageLayer::mapExtentChanged(void)
{
    //NMDebugCtx(ctxNMImageLayer, << "...");

    // this makes only sense, if this layer is reader-based rather than
    // data buffer-based, so

    if (    !this->mbUseOverviews
        ||  this->mReader == 0
        ||  !this->mReader->isInitialised()
        //||  this->mReader->getNumberOfOverviews() == 0
        ||  this->mIsIn3DMode
       )
    {
        return;
    }

    if (this->mRenderWindow->GetInteractor()->GetInteractorStyle()->GetClassName() ==
                "vtkInteractorStyleTrackballCamera")
    {
        return;
    }


    // get the bbox and fullsize of this layer
    double h_ext = mBBox[1] - mBBox[0];
    double v_ext = mBBox[3] - mBBox[2];

    int fullcols = h_ext / mSpacing[0];
    int fullrows = v_ext / ::fabs(mSpacing[1]);

    double h_res;
    double v_res;

    vtkRendererCollection* rencoll = this->mRenderWindow->GetRenderers();
    vtkRenderer* ren = 0;
    if (this->mRenderer->GetViewProps()->GetNumberOfItems() == 0)
    {
        if (rencoll->GetNumberOfItems() > 1)
        {
            rencoll->InitTraversal();
            for (int r=0; r < rencoll->GetNumberOfItems(); ++r)
            {
                ren = rencoll->GetNextItem();
            }
        }
    }
    else
    {
        ren = this->mRenderer;
    }

    int* size;
    double wminx, wmaxx, wminy, wmaxy;
    if (ren)
    {
        double wbr[] = {-1,-1,-1,-1};
        double wtl[] = {-1,-1,-1,-1};

        size = ren->GetSize();

        // top left (note: display origin is bottom left)
        vtkInteractorObserver::ComputeDisplayToWorld(ren, 0,size[1]-1,0, wtl);
        wminx = wtl[0];
        wmaxy = wtl[1];

        //this->world2pixel(wtl, ptl, true, false);

        // bottom right
        vtkInteractorObserver::ComputeDisplayToWorld(ren, size[0]-1,0,0, wbr);
        wmaxx = wbr[0];
        wminy = wbr[1];
        //this->world2pixel(wbr, pbr, true, false);

        // only works good if
        h_res = (wmaxx - wminx) / (double)size[0];
        v_res = (wmaxy - wminy) / (double)size[1];

        //        NMDebugAI(<< "world: tl: " << wtl[0] << " " << wtl[1]
        //                  << " br: " << wbr[0] << " " << wbr[1] << std::endl);
    }
    else
    {
        // full extent and overview according to window size
        ren = this->mRenderWindow->GetRenderers()->GetFirstRenderer();
        size = ren->GetSize();

        h_res = h_ext / (double)size[0];
        v_res = v_ext / (double)size[1];
    }

    int ovidx = -1;
    double target_res;
    double opt_res;
    if (size[0] > size[1])
    {
        opt_res = mSpacing[0];
        target_res = h_res;
    }
    else
    {
        target_res = v_res;
        opt_res = mSpacing[1];
    }
    double diff = ::fabs(opt_res-target_res);


    //        NMDebugAI(<< "screen size: " << size[0] << "x" << size[1] << std::endl);
    //        NMDebugAI(<< "opt_res: " << opt_res << " | target_res: " << target_res << " | diff: "
    //                  << diff << std::endl);

    for (int i=0; i < mReader->getNumberOfOverviews(); ++i)
    {
        double _tres;
        if (size[0] > size[1])
        {
            _tres = h_ext / (double)mReader->getOverviewSize(i)[0];
        }
        else
        {
            _tres = v_ext / (double)mReader->getOverviewSize(i)[1];
        }

        if (::fabs(_tres-target_res) < diff)
        {
            diff = ::fabs(_tres-target_res);
            //            NMDebugAI(<< "_tres: " << _tres << " | target_res: " << target_res << " | diff: "
            //                      << diff << " --> idx = " << i << std::endl);
            ovidx = i;
        }
    }

    if (this->mRenderer->GetViewProps()->GetNumberOfItems() > 0)
    //if (ren)
    {
        // overview properties and visible extent
        int cols = ovidx >= 0 ? this->mReader->getOverviewSize(ovidx)[0]: fullcols;
        int rows = ovidx >= 0 ? this->mReader->getOverviewSize(ovidx)[1]: fullrows;

        double uspacing[3];
        uspacing[0] = h_ext / cols;
        uspacing[1] = v_ext / rows;
        uspacing[2] = 0;

        int xo, yo, xe, ye, zo, ze;
        xo = (wminx - mBBox[0]) / uspacing[0];
        yo = (mBBox[3] - wmaxy) / ::fabs(uspacing[1]);
        xe = (wmaxx - mBBox[0]) / uspacing[0];
        ye = (mBBox[3] - wminy) / ::fabs(uspacing[1]);

        // calc vtk update extent
        int uext[6];
        uext[0] = xo > cols-1 ? cols-1 : xo < 0 ? 0 : xo;
        uext[2] = yo > rows-1 ? rows-1 : yo < 0 ? 0 : yo;
        uext[1] = xe > cols-1 ? cols-1 : xe < 0 ? 0 : xe;
        uext[3] = ye > rows-1 ? rows-1 : ye < 0 ? 0 : ye;
        uext[4] = 0;
        uext[5] = 0;

        // visible itk image region
        int wext[6];
        wext[0] = uext[0];
        wext[1] = uext[1] - uext[0] + 1;
        wext[2] = uext[2];
        wext[3] = uext[3] - uext[2] + 1;
        wext[4] = uext[4];
        wext[5] = uext[4] == 0 && uext[5] == 0 ? 0 : uext[5] - uext[4] + 1;

        //        NMDebugAI(<< "ov #" << ovidx << ": " << cols << "x" << rows << std::endl);

        //        NMDebugAI(<< "uext: " << uext[0] << ".." << uext[1] << " "
        //                              << uext[2] << ".." << uext[3] << std::endl);

        //        NMDebugAI(<< "wext: " << wext[0] << "," << wext[2] << " "
        //                              << wext[1] << "x" << wext[3] << std::endl);

        //        if (this->mNumBands == 3 && this->mBandMap.size() == 3)
        //        {
        //            this->mReader->setBandMap(this->mBandMap);
        //        }
        this->mReader->setOverviewIdx(ovidx, wext);
        this->mMapper->UpdateInformation();

        NMVtkOpenGLImageSliceMapper* ism = NMVtkOpenGLImageSliceMapper::SafeDownCast(this->mMapper);
        ism->SetDisplayExtent(uext);
        ism->SetDataWholeExtent(uext);

        this->mMapper->Update();
    }
    else
    {
        //        if (this->mNumBands == 3 && this->mBandMap.size() == 3)
        //        {
        //            this->mReader->setBandMap(this->mBandMap);
        //        }
        this->mReader->setOverviewIdx(ovidx, 0);
    }

    this->mOverviewIdx = ovidx;
    //NMDebugCtx(ctxNMImageLayer, << "done!");
}

void
NMImageLayer::setBandMap(const std::vector<int> map)
{
    this->mBandMap = map;
    if (this->mReader != 0 && this->mReader->isInitialised())
    {
        this->mReader->setBandMap(mBandMap);
    }
}

void
NMImageLayer::mapRGBImage(void)
{
    // need three bands
    if (this->getNumBands() != 3)
    {
        return;
    }

    vtkImageData* img = vtkImageData::SafeDownCast(const_cast<vtkDataSet*>(this->getDataSet()));
    vtkDataSetAttributes* dsa = img->GetAttributes(vtkDataSet::POINT);
    vtkDataArray* inScalars = img->GetPointData()->GetArray(0);
    int numComp = inScalars->GetNumberOfComponents();

    if (mBandMinMax.size() == 0)
    {
        for (int n=0; n < numComp; ++n)
        {
            double* range = inScalars->GetRange(n);
            mBandMinMax.push_back(range[0]);
            mBandMinMax.push_back(range[1]);
        }
    }

    if (this->mLegendValueField.startsWith(QString("Band #")))
    {
        if (mLastLegendType == NMLayer::NM_LEGEND_RGB)
        {
            // mScalarBand refers to mTotalNumBand
            int idx = mScalarBand % numComp;
            idx = idx == 0 ? 2 : idx - 1;

            double* range = inScalars->GetRange(idx);
            if (range)
            {
                setLower(range[0]);
                setUpper(range[1]);
            }
        }
        setLegendType(NMLayer::NM_LEGEND_RAMP);
        mLastLegendType = NMLayer::NM_LEGEND_RAMP;
    }
    else
    {
        mNumClasses = 3;
        mNumLegendRows = 4;
        setLegendValueField(QString("RGB"));
        setLegendDescrField(QString("Band Number"));
        setLegendType(NMLayer::NM_LEGEND_RGB);
        mImgProp->SetUseLookupTableScalarRange(0);
        mImgProp->SetLookupTable(0);
        this->mLookupTable = 0;
        this->mClrFunc = 0;

        mLastLegendType = NMLayer::NM_LEGEND_RGB;
    }
}

void
NMImageLayer::mapRGBImageScalars(vtkImageData* img)
{
    //NMDebugCtx(ctxNMImageLayer, << "...");
    if (    this->mComponentType == otb::ImageIOBase::UCHAR
        &&  this->mLegendValueField == "RGB"
       )
    {
        // we're done - nothing to do!
        return;
    }

    vtkDataSetAttributes* dsa = img->GetAttributes(vtkDataSet::POINT);
    vtkDataArray* inScalars = img->GetPointData()->GetArray(0);
    void* in = inScalars->GetVoidPointer(0);
    int numComp = inScalars->GetNumberOfComponents();
    int numPix = inScalars->GetNumberOfTuples();

    if (this->mLegendValueField == "RGB")
    {
        // we create an rgb out put array and map the input value component per component ot UCHAR
        vtkSmartPointer<vtkUnsignedCharArray> rgbAr = vtkSmartPointer<vtkUnsignedCharArray>::New();
        rgbAr->SetName("RGBColours");
        rgbAr->SetNumberOfComponents(numComp);
        rgbAr->SetNumberOfTuples(numPix);
        unsigned char* out = static_cast<unsigned char*>(rgbAr->GetVoidPointer(0));

        switch(inScalars->GetDataType())
        {
        vtkTemplateMacro(mapScalarsToRGB(static_cast<VTK_TT*>(in),
                                         out, numPix, numComp, mBandMinMax));
        default:
            NMErr(ctxNMImageLayer, << "Invalid input pixel type!");
            break;
        }

        dsa->AddArray(rgbAr);
        dsa->SetActiveAttribute(rgbAr->GetName(), vtkDataSetAttributes::SCALARS);
        vtkInformation* info = img->GetInformation();
        img->SetNumberOfScalarComponents(numComp, info);
    }
    else if (this->mLegendValueField.startsWith(QString("Band #")))
    {
        vtkSmartPointer<vtkDataArray> da = vtkDataArray::CreateDataArray(inScalars->GetDataType());
        QString name = QString("Band_%1").arg(mScalarBand);
        da->SetName(name.toStdString().c_str());
        da->SetNumberOfComponents(1);
        da->SetNumberOfTuples(numPix);
        void* out = da->GetVoidPointer(0);

        switch(inScalars->GetDataType())
        {
        vtkTemplateMacro(setComponentScalars(static_cast<VTK_TT*>(in),
                                             static_cast<VTK_TT*>(out),
                                             numPix));
        default:
            NMErr(ctxNMImageLayer, << "Invalid Data Type!");
            return;
        }

        dsa->AddArray(da);
        dsa->SetActiveAttribute(da->GetName(), vtkDataSetAttributes::SCALARS);
        vtkInformation* info = img->GetInformation();
        img->SetNumberOfScalarComponents(1, info);

    }

    //NMDebugCtx(ctxNMImageLayer, << "done!");
}

void
NMImageLayer::setScalars(vtkImageData* img)
{
    //NMDebugCtx(ctxNMImageLayer, << "...");


    int colidx = this->getColumnIndex(mLegendValueField);

    if (colidx < 0 || this->mOtbRAT.IsNull())
    {
        //NMWarn(ctxNMImageLayer, << "Table is NULL or Value Field is invalid!");
        //NMDebugCtx(ctxNMImageLayer, << "done!");
        return;
    }

    if (colidx != mScalarColIdx)
    {
        mScalarColIdx = colidx;
        if (mNumRecords <= 1e6)
        {
            updateScalarBuffer();
        }
        else
        {
            if (!mOtbRAT->prepareColumnByIndex(mLegendValueField.toStdString()))
            {
                NMErr(ctxNMImageLayer, << "Failed preparing fast scalar column access!");
                return;
            }
        }

        //        std::vector<std::string> colnames;
        //        colnames.push_back(mOtbRAT->getPrimaryKey());
        //        colnames.push_back(mLegendValueField.toStdString());
        //        std::stringstream where;
        //        where << "where " << mOtbRAT->getPrimaryKey() << " = ?1";
        //        mOtbRAT->prepareBulkGet(colnames, where.str());
    }

    vtkDataSetAttributes* dsa = img->GetAttributes(vtkDataSet::POINT);
    vtkDataArray* idxScalars = img->GetPointData()->GetArray(0);
    void* buf = idxScalars->GetVoidPointer(0);
    int numPix = idxScalars->GetNumberOfTuples();
    int maxidx = mNumRecords;//mOtbRAT->GetNumRows()-1;
    if (mNumRecords > 1e6)
    {
        mOtbRAT->beginTransaction();
    }

    switch(mOtbRAT->GetColumnType(colidx))
    {
    case otb::AttributeTable::ATTYPE_DOUBLE:
        {
            vtkSmartPointer<vtkDoubleArray> ar = vtkSmartPointer<vtkDoubleArray>::New();
            ar->SetName(mLegendValueField.toStdString().c_str());
            ar->SetNumberOfComponents(1);
            ar->SetNumberOfTuples(numPix);

            double* out = static_cast<double*>(ar->GetVoidPointer(0));
            double* tabCol = 0; //static_cast<double*>(mOtbRAT->GetColumnPointer(colidx));
            const double nodata = this->getNodata();

            std::map<long long, double>::iterator it;
            switch(idxScalars->GetDataType())
            {
            vtkTemplateMacro(
                        setDoubleScalars(static_cast<VTK_TT*>(buf),
                                                     out, tabCol, numPix, maxidx,
                                                     nodata)
                        );
            default:
                {
                    for (int i=0; i < numPix; ++i)
                    {
                        const long idx = idxScalars->GetVariantValue(i).ToLong();
                        it = mScalarDoubleMap.find(idx);
                        if (it != mScalarDoubleMap.end())
                        {
                            out[i] = it->second;
                        }
                        else
                        {
                            out[i] = nodata;
                        }
                        //                        if (idx < 0 || idx > maxidx)
                        //                        {
                        //                            out[i] = nodata;
                        //                        }
                        //                        else
                        //                        {
                        //                            fseek(mScalarBufferFile, sizeof(long)*idx, SEEK_SET);
                        //                            fread(out+i, sizeof(double), 1, mScalarBufferFile);
                        //                        }
                    }
                }
            }
            dsa->AddArray(ar);
            dsa->SetActiveAttribute(mLegendValueField.toStdString().c_str(),
                                    vtkDataSetAttributes::SCALARS);
        }
        break;
    case otb::AttributeTable::ATTYPE_INT:
        {
            vtkSmartPointer<vtkLongLongArray> ar = vtkSmartPointer<vtkLongLongArray>::New();
            ar->SetName(mLegendValueField.toStdString().c_str());
            ar->SetNumberOfValues(numPix);
            ar->SetNumberOfComponents(1);

            long long* out = static_cast<long long*>(ar->GetVoidPointer(0));
            long* tabCol = 0;//static_cast<long*>(mOtbRAT->GetColumnPointer(colidx));
            long long nodata = static_cast<long>(this->getNodata());

            std::map<long long, long long>::iterator it;
            switch(idxScalars->GetDataType())
            {
            vtkTemplateMacro(
                        setLongScalars(static_cast<VTK_TT*>(buf),
                                                     out, tabCol, numPix, maxidx,
                                                     nodata)
                        );
            default:
                {
                    for (int i=0; i < numPix; ++i)
                    {
                        const long idx = idxScalars->GetVariantValue(i).ToLong();
                        it = mScalarLongLongMap.find(idx);
                        if (it != mScalarLongLongMap.end())
                        {
                            out[i] = it->second;
                        }
                        else
                        {
                            out[i] = nodata;
                        }
                        //                        if (idx < 0 || idx > maxidx)
                        //                        {
                        //                            out[i] = nodata;
                        //                        }
                        //                        else
                        //                        {
                        //                            fseek(mScalarBufferFile, sizeof(long)*idx, SEEK_SET);
                        //                            fread(out+i, sizeof(long), 1, mScalarBufferFile);
                        //                        }
                    }
                }
            }
            dsa->AddArray(ar);
            dsa->SetActiveAttribute(mLegendValueField.toStdString().c_str(),
                                    vtkDataSetAttributes::SCALARS);
        }
        break;
    default:
        dsa->SetActiveAttribute(0, vtkDataSetAttributes::SCALARS);
    }
    // end read transaction, if we're accessing the table directly
    // rather than using a RAM buffer
    if (mNumRecords > 1e6)
    {
        mOtbRAT->endTransaction();
    }

    mbUpdateScalars = false;
    //NMDebugCtx(ctxNMImageLayer, << "done!");
}

template<class T>
void
NMImageLayer::setComponentScalars(T* in, T* out, int numPix)
{
    for (int i=0; i < numPix; ++i)
    {
        out[i] = in[i+(mScalarBand-1)*numPix];
    }
}

void
NMImageLayer::updateScalarBuffer()
{
    NMDebugCtx(ctxNMImageLayer, << "...");
    //    if (mScalarBufferFile != 0)
    //    {
    //        fclose(mScalarBufferFile);
    //    }

    mScalarDoubleMap.clear();
    mScalarLongLongMap.clear();

    std::vector< std::string > colnames;
    colnames.push_back(mOtbRAT->getPrimaryKey());
    colnames.push_back(mLegendValueField.toStdString());

    std::vector< otb::AttributeTable::ColumnValue > values;
    values.resize(2);

    //    mScalarBufferFile = tmpfile();
    //    rewind(mScalarBufferFile);

    double dnodata = 0;
    long long    inodata = 0;

    mOtbRAT->beginTransaction();
    mOtbRAT->prepareBulkGet(colnames, "");
    const int nrows = mOtbRAT->GetNumRows();
    const int rep = nrows / 20;
    for (int r=0; r < nrows; ++r)
    {
        switch(mOtbRAT->GetColumnType(mScalarColIdx))
        {
        case otb::AttributeTable::ATTYPE_INT:
            if (mOtbRAT->doBulkGet(values))
            {
                mScalarLongLongMap.insert(
                            std::pair<long long, long long>(
                                values[0].ival, values[1].ival));
                //fwrite(&(values[0].ival), sizeof(long), 1, mScalarBufferFile);
            }
            // NOTE: no backup here, since we don't know the rowid value stored
            // in the db; anyway setLongScalars will take care of it
            //            else
            //            {
            //                //fwrite(&inodata, sizeof(long), 1, mScalarBufferFile);
            //            }
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            if (mOtbRAT->doBulkGet(values))
            {
                mScalarDoubleMap.insert(
                            std::pair<long long, double>(
                                values[0].ival, values[1].dval));
                //fwrite(&(values[0].dval), sizeof(double), 1, mScalarBufferFile);
            }
            //            else
            //            {
            //                fwrite(&dnodata, sizeof(double), 1, mScalarBufferFile);
            //            }
            break;
        default:
            break;
        }

        if (r % (rep > 0 ? rep : 1) == 0)
        {
            NMDebug(<< ".");
        }
    }
    mOtbRAT->endTransaction();
    NMDebug(<< std::endl);
    NMDebugCtx(ctxNMImageLayer, << "done!");
}

// worker function for scalars setting
template<class T>
void
NMImageLayer::setLongScalars(T* buf, long long *out, long* tabCol,
                             int numPix, int maxidx, long long nodata)
{
    //CALLGRIND_START_INSTRUMENTATION;
    if (mNumRecords > 1e6)
    {
        for (int i=0; i < numPix; ++i)
        {
              out[i] = mOtbRAT->nextIntValue(static_cast<T*>(buf)[i]);
        }
    }
    else
    {
        std::map<long long, long long>::iterator it;
        for (int i=0; i < numPix; ++i)
        {
            it = mScalarLongLongMap.find(static_cast<T*>(buf)[i]);
            if (it != mScalarLongLongMap.end())
            {
                out[i] = it->second;
            }
            else
            {
                out[i] = nodata;
            }


            // this is really deprecated
            //        if (buf[i] < 0 || buf[i] > maxidx)
            //        {
            //            out[i] = nodata;
            //        }
            //        else
            //        {
            //            fseek(mScalarBufferFile, (sizeof(long))*(buf[i]), SEEK_SET);
            //            fread(out+i, sizeof(long), 1, mScalarBufferFile);
            //        }
        }
    }
    //    CALLGRIND_STOP_INSTRUMENTATION;
    //    CALLGRIND_DUMP_STATS;
}

template<class T>
void
NMImageLayer::setDoubleScalars(T* buf, double* out, double* tabCol,
                               int numPix, int maxidx, double nodata)
{
    //CALLGRIND_START_INSTRUMENTATION;

    if (mNumRecords > 1e6)
    {
        for (int i=0; i < numPix; ++i)
        {
              out[i] = mOtbRAT->nextDoubleValue(static_cast<T*>(buf)[i]);
        }
    }
    else
    {
        std::map<long long, double>::iterator it;
        for (int i=0; i < numPix; ++i)
        {
            it = mScalarDoubleMap.find(static_cast<T*>(buf)[i]);
            if (it != mScalarDoubleMap.end())
            {
                out[i] = it->second;
            }
            else
            {
                out[i] = nodata;
            }

            //        if (buf[i] < 0 || buf[i] > maxidx)
            //        {
            //            out[i] = nodata;
            //        }
            //        else
            //        {
            //            fseek(mScalarBufferFile, (sizeof(long))*(buf[i]), SEEK_SET);
            //            fread(out+i, sizeof(double), 1, mScalarBufferFile);
            //        }
        }
    }
    //    CALLGRIND_STOP_INSTRUMENTATION;
    //    CALLGRIND_DUMP_STATS;
}

template<class T>
void
NMImageLayer::mapScalarsToRGB(T* in, unsigned char* out, int numPix, int numComp,
                              const std::vector<double>& minmax)
{
    const double* mm = &minmax[0];
    // the mapping is according to itk::RescaleIntensityImageFilter<...>
    for (int i=0; i < numPix; ++i)
    {
        // components don't seem to be interleaved but rather one after another
        // for the whole image
        for (int c=0; c < numComp; ++c)
        {
            // ToDo: double check scaling; more options?
            const double cr = (static_cast<double>(in[i+c*numPix]) - mm[c*2]) * (255.0 / (mm[c*2+1] - mm[c*2]));
            //const double cr = 255.0 / static_cast<double>(in[i+c*numPix]) * 255.0 + 0.5;
            out[i+c*numPix] = static_cast<unsigned char>(cr < 0 ? 0 : cr > 255 ? 255 : cr);
        }
    }
}

void
NMImageLayer::updateSourceBuffer(void)
{
    NMDataComponent* dc = qobject_cast<NMDataComponent*>(this->sender());
    if (dc != 0 && dc->getOutput(0) != 0 && dc->getOutput(0)->getDataObject() != 0)
    {
        this->mPipeconn->setInput(dc->getOutput(0));
        this->mMapper->Update();
        this->mRenderer->Render();
        this->updateMapping();
    }
}

void
NMImageLayer::setImage(QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
    if (imgWrapper.data() == 0)
		return;

    this->mbUseOverviews = false;
	this->mImage = imgWrapper->getDataObject();
    this->mOtbRAT = imgWrapper->getOTBTab();
	this->mComponentType = imgWrapper->getItkComponentType();
	this->mNumDimensions = imgWrapper->getNumDimensions();
	this->mNumBands = imgWrapper->getNumBands();

    if (mNumBands > 1)
    {
        // currently don't handle this case
        NMMsg(<< "don't display multi-band image buffers, for now ...!");
        mImage = 0;
        mOtbRAT = 0;
        mRenderer = 0;
        return;
    }

    // concatenate the pipeline
	this->mPipeconn->setInput(imgWrapper);

	vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
	m->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

	vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
	a->SetMapper(m);

	mImgProp = vtkSmartPointer<vtkImageProperty>::New();
	mImgProp->SetInterpolationTypeToNearest();
	a->SetProperty(mImgProp);

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
	this->mActor = a;

    // check bouding box before we to into heres
    unsigned int numDims = imgWrapper->getNumDimensions();
    unsigned int numBands = imgWrapper->getNumBands();
    itk::DataObject::Pointer img = this->mImage;
    double* bbox = new double[6];
    otb::ImageIOBase::IOComponentType type = imgWrapper->getItkComponentType();
    switch(type)
    {
    MacroPerType( getInternalBBox, InternalImageHelper )
    default:
        break;
    }

    for(int i=0; i < 6; ++i)
    {
        this->mBBox[i] = bbox[i];
    }
    delete bbox;

    this->initiateLegend();
    emit layerLoaded();
}

//void NMImageLayer::setITKImage(itk::DataObject* img,
//		otb::ImageIOBase::IOComponentType pixType,
//		unsigned int numDims,
//		unsigned int numBands)
//{
//	this->mImage = img;
//	this->mComponentType = pixType;
//	this->mNumDimensions = numDims;
//	this->mNumBands = numBands;
//
//	// concatenate the pipeline
//	this->mPipeconn->setInput(img, pixType, numDims, numBands);
//
//	vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
//	m->SetInputConnection(this->mPipeconn->getOutputPort());
//
//	vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
//	a->SetMapper(m);
//
//	this->mRenderer->AddViewProp(a);
//
//	this->mMapper = m;
//	this->mActor = a;
//
////	switch (pixType)
////	{
////	case otb::ImageIOBase::UCHAR:
////		getInternalBBox( unsigned char );
////		break;
////	case otb::ImageIOBase::CHAR:
////		getInternalBBox( char );
////		break;
////	case otb::ImageIOBase::USHORT:
////		getInternalBBox( unsigned short );
////		break;
////	case otb::ImageIOBase::SHORT:
////		getInternalBBox( short );
////		break;
////	case otb::ImageIOBase::UINT:
////		getInternalBBox( unsigned int );
////		break;
////	case otb::ImageIOBase::INT:
////		getInternalBBox( int );
////		break;
////	case otb::ImageIOBase::ULONG:
////		getInternalBBox( unsigned long );
////		break;
////	case otb::ImageIOBase::LONG:
////		getInternalBBox( long );
////		break;
////	case otb::ImageIOBase::FLOAT:
////		getInternalBBox( float );
////		break;
////	case otb::ImageIOBase::DOUBLE:
////		getInternalBBox( double );
////		break;
////	default:
////		break;
////	}
//}

void NMImageLayer::getBBox(double bbox[6])
{
    //NMDebugCtx(ctxNMImageLayer, << "...");

	bbox[0] = this->mBBox[0];
	bbox[1] = this->mBBox[1];
	bbox[2] = this->mBBox[2];
	bbox[3] = this->mBBox[3];
	bbox[4] = this->mBBox[4];
	bbox[5] = this->mBBox[5];

    //NMDebugCtx(ctxNMImageLayer, << "done!");
}

const vtkDataSet* NMImageLayer::getDataSet()
{
    return this->mPipeconn->getVtkImage();
}


otb::AttributeTable::Pointer
NMImageLayer::getRasterAttributeTable(int band)
{
    otb::AttributeTable::Pointer tab = 0;
    if (this->mReader != 0 && this->mReader->isInitialised())
    {
        if (band < 1 || band > this->mReader->getOutputNumBands())
            return 0;

        // note: we can have a valid table object with no records; no
        // columns shouldn't occur at all, but doesn't do any harm
        // anyway ...
        tab = this->mReader->getRasterAttributeTable(band);
        mNumRecords = tab->GetNumRows();
        if (    mNumRecords == 0
            ||  tab->GetNumCols() == 0
           )
        {
            tab = 0;
            mOtbRAT = 0;
        }
        else
        {
            mOtbRAT = tab;
        }
    }
    else
    {
        tab = this->mOtbRAT;
    }

    return tab;
}


QSharedPointer<NMItkDataObjectWrapper>
NMImageLayer::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> dw;
    dw.clear();
    return dw;
}

QSharedPointer<NMItkDataObjectWrapper> NMImageLayer::getImage(void)
{
    QSharedPointer<NMItkDataObjectWrapper> dw;
    dw.clear();

	itk::DataObject* img = this->getITKImage();
	if (img == 0)
        return dw;

    QSharedPointer<NMItkDataObjectWrapper> imgW(new NMItkDataObjectWrapper(0,
			img, this->mComponentType, this->mNumDimensions,
            this->mNumBands));
    imgW->setOTBTab(this->mOtbRAT);
	return imgW;
}

itk::DataObject* NMImageLayer::getITKImage(void)
{
    if (mReader == 0 || mReader->getImageIOBase() == 0)
		return this->mImage;
	else
		return this->mReader->getItkImage();
}

otb::ImageIOBase::IOComponentType NMImageLayer::getITKComponentType(void)
{
	return this->mComponentType;
}

void
NMImageLayer::setNthInput(unsigned int idx,
          QSharedPointer<NMItkDataObjectWrapper> inputImg)
{
	this->setImage(inputImg);
}

void
NMImageLayer::writeDataSet(void)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	// call parent first, to deal with the
	// layer's state recording
	NMLayer::writeDataSet();

	if (this->mFileName.isEmpty())
	{
		NMErr(ctxNMImageLayer, << "No valid file name set! Abort!")
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return;
	}

	bool berr = false;
    const char* fn = this->mFileName.toUtf8().constData();
	unsigned int band = this->mOtbRAT->GetBandNumber();

#ifdef BUILD_RASSUPPORT
	if (this->isRasLayer())
	{
		NMDebugAI(<< "writing rasdaman stuff ... !" << std::endl);
		otb::RasdamanImageIO::Pointer rio = otb::RasdamanImageIO::New();
		rio->setRasdamanConnector(this->mpRasconn);

		rio->SetFileName(fn);
		if (rio->CanWriteFile(0) && this->mOtbRAT.IsNotNull())
		{
			std::vector<double> oids = rio->getOIDs();
			rio->writeRAT(this->mOtbRAT.GetPointer(), band, oids[band-1]);
		}
		else
		{
			berr = true;
		}
	}
	else
#endif
	{
		NMDebugAI(<< "writing GDAL stuff ... !" << std::endl);

		otb::GDALRATImageIO::Pointer gio = otb::GDALRATImageIO::New();
		gio->SetFileName(fn);

		if (gio->CanWriteFile(fn))
		{
			gio->WriteRAT(this->mOtbRAT, band);
		}
		else
		{
			berr = true;
		}
	}

	if (berr)
	{
		NMErr(ctxNMImageLayer, << "Bugger! Couldn't update the RAT!");
	}

	NMDebugCtx(ctxNMImageLayer, << "done!");
}
