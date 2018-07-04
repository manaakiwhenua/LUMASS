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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

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
#include "NMChartView.h"
#include "NMVtkLookupTable.h"

#include <QTime>
#include <QtCore>
#include <QtConcurrent>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QApplication>

// QSQLiteDriver
#include "nmqsql_sqlite_p.h"
#include "nmqsqlcachedresult_p.h"
//#include "qsql_sqlite.cpp"


#include "itkDataObject.h"
#include "otbImage.h"
#include "otbSQLiteTable.h"
#include "otbRAMTable.h"
#include "otbGDALRATImageIO.h"
#include "otbVectorImage.h"
#include "itkImageRegion.h"
#include "itkPoint.h"
#include "itkExceptionObject.h"
#include "itkImageRegion.h"
#include "itkStatisticsImageFilter.h"
#include "itkCastImageFilter.h"

#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkImageStack.h"
#include "vtkLookupTable.h"
#include "vtkUnstructuredGrid.h"
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
//#include "vtkGenericContourFilter.h"
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
#include "vtkTrivialProducer.h"
#include "vtkImageAccumulate.h"
#include "vtkIdTypeArray.h"
#include "vtkContextView.h"
#include "vtkChartXY.h"
#include "vtkChartHistogram2D.h"
#include "vtkPlot.h"
#include "vtkPlotHistogram2D.h"
#include "vtkFloatArray.h"
#include "vtkContextScene.h"
#include "vtkAxis.h"
#include "vtkPlotBar.h"
#include "vtkRect.h"
#include "vtkBoundingBox.h"
//#include "valgrind/callgrind.h"

template<class PixelType, unsigned int Dimension>
class InternalImageHelper
{
public:
    typedef otb::Image<PixelType, Dimension> ImgType;
    typedef typename ImgType::RegionType RegionType;
    typedef otb::VectorImage<PixelType, Dimension> VecImgType;
    typedef typename VecImgType::RegionType VecRegionType;

    //typedef typename itk::StatisticsImageFilter<ImgType> StatsFilterType;
    //typedef typename itk::StatisticsImageFilter<VecImgType> VecStatsFilterType;

//    static void getWholeImageStatistics(itk::DataObject::Pointer& img, unsigned int numBands,
//                                        std::vector<double>& stats)
//    {
//        if (img.IsNull() || img.GetPointer() == 0) return;

//        if (numBands == 1)
//        {
//            ImgType* theimg = dynamic_cast<ImgType*>(img.GetPointer());
//            if (theimg == 0)
//            {
//                return;
//            }

//            typename StatsFilterType::Pointer pF = StatsFilterType::New();
//            pF->SetInput(theimg);
//            pF->Update();

//            stats.clear();
//            stats.push_back(pF->GetMinimum());
//            stats.push_back(pF->GetMaximum());
//            stats.push_back(pF->GetMean());
//            stats.push_back(-9999);
//            stats.push_back(std::sqrt(pF->GetVariance()));
//            stats.push_back(theimg->GetLargestPossibleRegion().GetNumberOfPixels());

//        }
//        // ToDo: ivestigate what to do in the multi-band case
//        else
//        {
//            return;
//        }
//    }


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

//#define getInternalImgStats( PixelType, wrapName ) \
//{	\
//    if (numDims == 2) \
//        wrapName<PixelType, 2>::getBBox(img, numBands, stats); \
//    else \
//        wrapName<PixelType, 3>::getBBox(img, numBands, stats); \
//}

NMImageLayer::NMImageLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
    : NMLayer(renWin, renderer, parent),
      mHistogramView(0), mbLayerLoaded(false)
{
	this->mLayerType = NMLayer::NM_IMAGE_LAYER;
    this->mReader = 0; //new NMImageReader(this);
	this->mPipeconn = new NMItk2VtkConnector(this);

    //this->mSelPipe = new NMItk2VtkConnector(this);

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
    this->mVtkConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->mVtkConn->Connect(mRenderer, vtkCommand::EndEvent,
                            this, SLOT(updateHistogram(vtkObject*)));

    this->mWTLx = itk::NumericTraits<double>::NonpositiveMin();

//	this->mVtkConn->Connect(style, vtkCommand::ResetWindowLevelEvent,
//			this, SLOT(windowLevelReset(vtkObject*)));
//	this->mVtkConn->Connect(style, vtkCommand::InteractionEvent,
//			this, SLOT(windowLevelChanged(vtkObject*)));
}

NMImageLayer::~NMImageLayer()
{
    if (mHistogramView != 0)
    {
        mHistogramView->close();
        delete mHistogramView;
    }

    if (this->mOtbRAT.IsNotNull())
    {
        otb::SQLiteTable::Pointer st = static_cast<otb::SQLiteTable*>(mOtbRAT.GetPointer());
        st->CloseTable();
    }

    if (mScalarBufferFile != 0)
    {
        fclose(mScalarBufferFile);
    }

	if (this->mReader)
    {
		delete this->mReader;
    }

	if (this->mPipeconn)
    {
		delete this->mPipeconn;
    }
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

    if (mReader == nullptr || mReader->getImageIOBase() == nullptr)
    {

        // calc stats
    //        vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
    //        cast->SetOutputScalarTypeToDouble();
    //        cast->SetInputConnection(conn->getVtkAlgorithmOutput());

        vtkSmartPointer<vtkImageHistogramStatistics> stats =
                vtkSmartPointer<vtkImageHistogramStatistics>::New();
        stats->SetInputConnection(mPipeconn->getVtkAlgorithmOutput());
    //    vtkImageMapper3D* im3d = vtkImageMapper3D::SafeDownCast(mMapper);
    //    stats->SetInputData(im3d->GetInput());

        //unsigned int nthreads = std::max(stats->GetNumberOfThreads() - 2, 1);
        //stats->AutomaticBinningOff();
        //stats->SetNumberOfBins(256);
        stats->AutomaticBinningOn();
        stats->SetMaximumNumberOfBins(256);
        //stats->SetNumberOfThreads(1);
        stats->GenerateHistogramImageOff();
        //stats->SetInputConnection(cast->GetOutputPort());
        stats->Update();

        mHistogram = stats->GetHistogram();

        ret.push_back(stats->GetMinimum());
        ret.push_back(stats->GetMaximum());
        ret.push_back(stats->GetMean());
        ret.push_back(stats->GetMedian());
        ret.push_back(stats->GetStandardDeviation());
        ret.push_back(mPipeconn->getVtkImage()->GetNumberOfPoints());
        ret.push_back(-9999);

    }
    else
    {
        if (!this->mFileName.isEmpty())
        {
            NMImageReader* imgReader = new NMImageReader(0);
            imgReader->setFileName(this->mFileName);

            imgReader->instantiateObject();
            if (!imgReader->isInitialised())
            {
                NMLogWarn(<< ctxNMImageLayer << ": Failed initialising NMImageReader!");
            }
            else
            {
                int ovidx = this->mOverviewIdx;

                // is this layer actually visible?
                bool bIsVisible = true;
                for (int d=0; d < 2; ++d)
                {
                    if (mVisibleRegion[d*2+1] == 0)
                    {
                        bIsVisible = false;
                    }
                }

                const int numOverviews = imgReader->getNumberOfOverviews();
                if ((!bIsVisible || !mbLayerLoaded) && numOverviews > 0)
                {
                    ovidx = numOverviews-1;
                }


                std::vector<std::vector<int> > sizes = this->getOverviewSizes();
                if (ovidx >= 0)
                {
                    if (sizes.size() > 0)
                    {
                        for (int d=0; d < 3; ++d)
                        {
                            mVisibleRegion[d*2] = 0;
                            mVisibleRegion[d*2+1] = d < 2 ? sizes[ovidx][d] : 0;
                        }
                    }
                }
                else
                {
                    if (!bIsVisible)
                    {
                        double h_ext = mBBox[1] - mBBox[0];
                        double v_ext = mBBox[3] - mBBox[2];

                        int fullcols = h_ext / mSpacing[0];
                        int fullrows = v_ext / ::fabs(mSpacing[1]);

                        for (int d=0; d < 3; ++d)
                        {
                            mVisibleRegion[d*2] = 0;
                            mVisibleRegion[d*2+1] = d < 1 ? fullcols :
                                                    d < 2 ? fullrows : 0;
                        }
                    }
                }

                imgReader->setOverviewIdx(ovidx, mVisibleRegion);
                ret = imgReader->getImageStatistics();
            }

            delete imgReader;
        }
    }

    emit layerProcessingEnd();
    return ret;
}

void
NMImageLayer::showHistogram(void)
{
    if (mHistogramView == 0)
    {
        mHistogramView = new NMChartView();
        vtkSmartPointer<vtkContextView> view = mHistogramView->getContextView();

        vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
        view->GetScene()->AddItem(chart);

    }


    mHistogramView->show();
    this->updateHistogram(0);
}

void
NMImageLayer::updateHistogram(vtkObject *)
{
    if (    mHistogramView == 0
         || !mHistogramView->isVisible())
    {
        return;
    }

    // prepare the histogram data
    // histogram data
    std::vector<double> stats = this->getWindowStatistics();
    int nbins = mHistogram->GetNumberOfTuples();
    mHistogram->SetName("Frequency");

    vtkSmartPointer<vtkFloatArray> pixval = vtkSmartPointer<vtkFloatArray>::New();
    pixval->SetNumberOfComponents(1);
    pixval->SetNumberOfTuples(nbins);
    pixval->SetName("Value");

    vtkSmartPointer<vtkTable> tab = vtkSmartPointer<vtkTable>::New();
    tab->AddColumn(pixval);
    tab->AddColumn(mHistogram);

    double range = (stats[1] - stats[0]);
    double min = stats[0];
    double npix = stats[5];

    for (int n=0; n < nbins; ++n)
    {
        tab->SetValue(n, 0, vtkVariant((n+1)*range/(double)nbins));
    }

    vtkChartXY* chart = vtkChartXY::SafeDownCast(mHistogramView->getContextView()->GetScene()->GetItem(0));
    chart->GetAxis(1)->SetRange(stats[0], stats[1]);
    chart->GetAxis(1)->SetTitle("Pixel Value");
    chart->GetAxis(0)->SetTitle("Frequency");

    chart->ClearPlots();

    vtkPlot* plot = 0;
    vtkPlotBar* plotBar = 0;

    plot = chart->AddPlot(vtkChart::BAR);
    plotBar = vtkPlotBar::SafeDownCast(plot);
    plotBar->SetInputData(tab, 0, 1);
    plotBar->SetOrientation(vtkPlotBar::VERTICAL);
    plotBar->SetColor(0.7, 0.7, 0.7);


    mHistogramView->show();
    mHistogramView->getRenderWindow()->Render();
}

std::vector<double>
NMImageLayer::getWholeImageStatistics(void)
{
    std::vector<double> stats;

    if (this->mNumBands > 1)
    {
        stats.resize(7,-9999);
        return stats;
    }


    if (!mbStatsAvailable)
    {
        emit layerProcessingStart();


        if (!this->mFileName.isEmpty())
        {

            NMImageReader* imgReader = new NMImageReader(0);
            imgReader->setFileName(this->mFileName);

            imgReader->instantiateObject();
            if (!imgReader->isInitialised())
            {
                NMLogWarn(<< ctxNMImageLayer << ": Failed initialising NMImageReader!");
                return stats;
            }

            stats = imgReader->getImageStatistics();

            delete imgReader;

        }
        else
        {
            stats = this->getWindowStatistics();
        }


        mImgStats[0] = stats[0];
        mImgStats[1] = stats[1];
        mImgStats[2] = stats[2];
        mImgStats[3] = stats[3];
        mImgStats[4] = stats[4];
        mImgStats[5] = stats[5];


        this->mbStatsAvailable = true;

        emit layerProcessingEnd();
    }

    return stats;
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
    bool bPtInBnds = vtkMath::PointIsWithinBounds(wcoord, mBBox, err);

    if (bImgConstrained && !bPtInBnds)
    {
        for (int i=0; i < 3; ++i) pixel[i] = -1;
        return;
    }

    for (d = 0; d < this->mNumDimensions; ++d)
    {
        if (dims[d] > 0)
        {
            pixel[d] = ((world[d] - origin[d]) / spacing[d])+0.5;
            if (!bImgConstrained)
            {
                if (d == 0 && wcoord[d] < origin[d])
                {
                    pixel[d] -= 1;
                }

                if (d == 1 && wcoord[d] > origin[d])
                {
                    pixel[d] -= 1;
                }
            }
            else if (!bPtInBnds)
            {
                pixel[d] = -1;
            }
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
    // no table, no selection
    otb::SQLiteTable::Pointer sqlTab;
    bool alright = false;
    if (mOtbRAT.IsNotNull())
    {
        sqlTab = static_cast<otb::SQLiteTable*>(mOtbRAT.GetPointer());
        if (sqlTab.IsNotNull())
        {
            sqlTab->SetOpenReadOnly(true);
            if (alright = sqlTab->openConnection())
            {
                alright = sqlTab->PopulateTableAdmin();
            }
        }
    }

    if (!alright)
    {
        sqlTab->CloseTable();
        NMLogError(<< "ImageLayer::selectionChanged(): Failed to open the RAT!");
        return;
    }

    // ==========================================================================
    // prepare selection bounding box, if applicable for this layer

    // mSelBBox
    // 0: minx
    // 1: maxx
    // 2: miny
    // 3: maxy
    // 4: minz
    // 5: maxz

    // invalidate the mSelBBox
    for (int i=0; i < 3; ++i)
    {
        mSelBBox[i*2] = mBBox[i*2+1];
        mSelBBox[i*2+1] = mBBox[i*2];
    }

    if (!mHasSelBox)
    {
        for (int i=0; i < 6; ++i)
        {
            mSelBBox[i] = mBBox[i];
        }
    }


    // =====================================================================================
    // prepare selection processing

    // determine whether the legend value field is of type numeric
    bool bNum = false;
    QVariant::Type valType = this->getColumnType(this->getColumnIndex(mLegendValueField));
    if (valType != QVariant::String)
    {
        bNum = true;
    }

    const long long ncells = this->getNumTableRecords();

    // pre-fill look-up table with non-selection values / colours
    int valcolidx = this->getColumnIndex(mLegendValueField);
    vtkSmartPointer<NMVtkLookupTable> selLut = vtkSmartPointer<NMVtkLookupTable>::New();
    selLut->SetNumberOfTableValues(ncells);
    for (int cell=0; cell < ncells; ++cell)
    {
        if (mUseIdxMap)
        {
            if (bNum)
            {
                selLut->SetMappedTableValue(cell,
                                          sqlTab->GetDblValue(valcolidx, cell),
                                          0,0,0,0);
            }
            else
            {
                selLut->SetMappedTableValue(cell,
                                          cell,
                                          0,0,0,0);
            }
        }
        else
        {
            selLut->SetTableValue(cell, 0,0,0,0);
        }
    }


    // iterate over new selection range and set selection colour for
    // seletec rows
    int selcnt = 0;
    foreach(QItemSelectionRange range, newSel)
    {
        const int top = range.top();
        const int bottom = range.bottom();

        // set colour for selected rows
        for (int row=top; row <= bottom; ++row, ++selcnt)
        {
            if (mUseIdxMap)
            {
                if (bNum)
                {
                    selLut->SetMappedTableValue(row,
                                              sqlTab->GetDblValue(valcolidx, row),
                                              1,0,0,1);
                }
                else
                {
                    selLut->SetMappedTableValue(row,
                                              row,
                                              1,0,0,1);
                }
            }
            else
            {
                selLut->SetTableValue(row, 1,0,0,1);
            }


            if (mHasSelBox)
            {
                double minX = sqlTab->GetIntValue("minX", row) * mSpacing[0] + mOrigin[0];
                double minY = mOrigin[1] + mSpacing[1] + (sqlTab->GetIntValue("maxY", row) * mSpacing[1]);
                double minZ = 0;
                double maxX = sqlTab->GetIntValue("maxX", row) * mSpacing[0] + mOrigin[0] + mSpacing[0];
                double maxY = mOrigin[1] + (sqlTab->GetIntValue("minY", row) * mSpacing[1]);
                double maxZ = 0;

                mSelBBox[0] = minX < mSelBBox[0] ? minX : mSelBBox[0];
                mSelBBox[1] = maxX > mSelBBox[1] ? maxX : mSelBBox[1];
                mSelBBox[2] = minY < mSelBBox[2] ? minY : mSelBBox[2];
                mSelBBox[3] = maxY > mSelBBox[3] ? maxY : mSelBBox[3];
                mSelBBox[4] = minZ < mSelBBox[4] ? minZ : mSelBBox[4];
                mSelBBox[5] = maxZ > mSelBBox[5] ? maxZ : mSelBBox[5];
            }
        }
    }
    sqlTab->CloseTable();

    // set selection look-up table and selection
    // 'layer' visibility
    this->mImgSelSlice->SetVisibility(selcnt);
    if (mUseIdxMap)
    {
        selLut->SetIndexedLookup(0);
        selLut->SetUseIndexMapping(true);
        selLut->SetNumberOfColors(ncells);
    }
    else
    {
        selLut->SetTableRange(0, ncells-1);
    }

    this->mImgSelProperty->SetUseLookupTableScalarRange(1);
    this->mImgSelProperty->SetLookupTable(selLut);
    this->mImgSelMapper->Update();

	// call the base class implementation to do datatype agnostic stuff
    NMLayer::selectionChanged(newSel, oldSel);

    mNumSelRows = selcnt;

	emit visibilityChanged(this);
	emit legendChanged(this);

}

void
NMImageLayer::createImgSelData()
{


}

void NMImageLayer::createTableView(void)
{
    if (    this->mSqlTableView != 0
        ||  this->mTableView != 0
       )
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

    NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mTableModel);
    NMQtOtbAttributeTableModel* ramModel = qobject_cast<NMQtOtbAttributeTableModel*>(mTableModel);
    if (ramModel != 0)
    {
        this->mTableView = new NMTableView(ramModel, 0);
        this->mTableView->setSelectionModel(mSelectionModel);
        QString tabletitle = QString("Attributes of %1").arg(this->objectName());
        this->mTableView->setTitle(tabletitle);
        connect(this, SIGNAL(selectabilityChanged(bool)),
                mTableView, SLOT(setSelectable(bool)));
        connect(mTableView, SIGNAL(notifyLastClickedRow(long long)),
                this, SLOT(forwardLastClickedRowSignal(long long)));
    }
    else if (sqlModel != 0)
    {
        this->mSqlTableView = new NMSqlTableView(sqlModel, 0);
        this->mSqlTableView->setLogger(mLogger);
        this->mSqlTableView->setSelectionModel(this->mSelectionModel);
        this->mSqlTableView->setTitle(sqlModel->tableName());
        this->mSqlTableView->setLayerName(this->objectName());
        connect(this, SIGNAL(selectabilityChanged(bool)),
                mSqlTableView, SLOT(setSelectable(bool)));
        connect(mSqlTableView, SIGNAL(notifyLastClickedRow(long long)),
                this, SLOT(forwardLastClickedRowSignal(long long)));
        connect(mSqlTableView, SIGNAL(columnsInserted(QModelIndex,int,int)),
                this, SLOT(tableColumnsInserted(QModelIndex,int,int)));
    }
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

    NMQtOtbAttributeTableModel* ramModel = 0;
    NMSqlTableModel* sqlModel = 0;
    otb::SQLiteTable::Pointer sqlTable = 0;
    otb::RAMTable::Pointer ramTable = 0;
    if (mOtbRAT->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
    {
        ramTable = static_cast<otb::RAMTable*>(mOtbRAT.GetPointer());
    }
    else
    {
        sqlTable = static_cast<otb::SQLiteTable*>(mOtbRAT.GetPointer());
    }


    if (ramTable.IsNotNull())
    {
        ramModel = new NMQtOtbAttributeTableModel(this->mOtbRAT);
    }
    else
    {
        /* NOTE: somehow, we need to explicitly open a separate connection
         * to the DB. It is we cannot re-use the connection (sqlite3*)
         * comming from the otb::SQLiteTable class ...
         * If we did, on Windows, we couldn't use the QSql classes
         * for the GUI-based DB-stuff ... It works fine on Linux, though.
         *
         * Hence, this workaround. Also note, because of that, we have to
         * close the connection initiated by the otb::SQLiteTable, because
         * it is opened in read/write and would therefore block any
         * changes we make to the DB schema via the NMSqlTableView ...
         *
         */

        mSpatialiteCache = spatialite_alloc_connection();
        int rc = ::sqlite3_open_v2(sqlTable->GetDbFileName().c_str(),
                    &mSqlViewConn,
                    SQLITE_OPEN_URI |
                    SQLITE_OPEN_READWRITE |
                    SQLITE_OPEN_SHAREDCACHE, 0);
        if (rc != SQLITE_OK)
        {
            NMLogError(<< ctxNMImageLayer
                << ": Failed opening SqlTableView connection!"
                << " - rc = " << rc);
            ::sqlite3_close(mSqlViewConn);
            spatialite_cleanup_ex(mSpatialiteCache);
            mSpatialiteCache = 0;
            mSqlViewConn = 0;
            return 0;
        }
        spatialite_init_ex(mSqlViewConn, mSpatialiteCache, 1);

        mQSqlConnectionName = QString("%1_%2").arg(sqlTable->GetTableName().c_str())
                .arg(sqlTable->GetRandomString(5).c_str());
        NMQSQLiteDriver* drv = new NMQSQLiteDriver(mSqlViewConn, 0);
        QSqlDatabase db = QSqlDatabase::addDatabase(drv, mQSqlConnectionName);
        db.setDatabaseName(sqlTable->GetDbFileName().c_str());
		
        NMDebugAI(<< ctxNMImageLayer << ": Created QSql connection to '"
                  << sqlTable->GetTableName() << "'" << std::endl);

        sqlModel = new NMSqlTableModel(this, db);
        sqlModel->setDatabaseName(QString(sqlTable->GetDbFileName().c_str()));
        sqlModel->setTable(QString(sqlTable->GetTableName().c_str()));
        sqlModel->select();

        sqlTable->CloseTable();
    }


	// in any case, we create a new item selection model
    if (    this->mSelectionModel == 0
        &&  (sqlModel != 0 || ramModel != 0)
       )
	{
        if (ramModel != 0)
        {
            this->mSelectionModel = new NMFastTrackSelectionModel(ramModel, this);
            this->mTableModel = ramModel;
        }
        else
        {
            this->mSelectionModel = new NMFastTrackSelectionModel(sqlModel, this);
            this->mTableModel = sqlModel;
        }

	}
    ramModel = 0;
    sqlModel = 0;

    if (mSelectionModel)
    {
        connectTableSel();
    }

    // =========================================
    // check for sel box fields

    // setup list of required RAT fields for selection box support
    QStringList bndCols;
    bndCols << "minX" << "minY" << "maxX" << "maxY";

    // determine whether the layer provides the required fields for
    // supporting a selection box
    mHasSelBox = true;
    foreach(const QString& cn, bndCols)
    {
       if (this->getColumnIndex(cn) < 0)
       {
           mHasSelBox = false;
           break;
       }
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
        NMLogError(<< ctxNMImageLayer << ": invalid filename!");
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

        // also we set the type of RAT we'd like to get from
        // the reader
        this->mReader->setRATType("ATTABLE_TYPE_SQLITE");
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
        NMLogError(<< ctxNMImageLayer << ": couldn't read the image!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
	}
    if (this->mReader->getNumberOfOverviews() == 0)
    {
        emit layerProcessingEnd();
        NMLogWarn(<< ctxNMImageLayer << ": layer '" << this->objectName().toStdString() << "'"
                  << " has no overviews!");
        QMessageBox::StandardButton yesno =
                QMessageBox::question(0, QString::fromLatin1("No overviews found!"),
                                      QString::fromLatin1("Do you want to build image overviews?"));
        emit layerProcessingStart();
        if (yesno == QMessageBox::Yes)
        {
            mReader->buildOverviews("NEAREST");
        }
    }

    // get original image attributes before we muck
    // around with scaling and overviews
    this->mReader->getBBox(this->mBBox);
    this->mReader->getBBox(this->mSelBBox);
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

    this->mImgSelMapper = vtkSmartPointer<NMVtkOpenGLImageSliceMapper>::New();
    this->mImgSelMapper->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());
    this->mImgSelMapper->SetNMLayer(this);
    this->mImgSelMapper->SetBorder(1);

    vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
    a->SetMapper(m);

    this->mImgSelSlice = vtkSmartPointer<vtkImageSlice>::New();
    this->mImgSelSlice->SetMapper(mImgSelMapper);
    this->mImgSelSlice->SetVisibility(0);

    mImgProp = vtkSmartPointer<vtkImageProperty>::New();
    mImgProp->SetInterpolationTypeToNearest();
    a->SetProperty(mImgProp);
    mImgProp->SetLayerNumber(0);

    this->mImgSelProperty = vtkSmartPointer<vtkImageProperty>::New();
    this->mImgSelProperty->SetInterpolationTypeToNearest();
    this->mImgSelSlice->SetProperty(this->mImgSelProperty);
    this->mImgSelProperty->SetLayerNumber(1);

    vtkImageStack* imgStack = vtkImageStack::New();
    imgStack->AddImage(a);
    imgStack->AddImage(mImgSelSlice);
    imgStack->SetActiveLayer(0);

    this->mRenderer->AddViewProp(imgStack);
    imgStack->Delete();

	this->mMapper = m;
    this->mActor = a;

	this->mImage = 0;
	this->mFileName = filename;
    //this->mMapper->Update();

    this->initiateLegend();

    emit layerProcessingEnd();
    emit layerLoaded();
    mbLayerLoaded = true;
    NMDebugCtx(ctxNMImageLayer, << "done!");
	return true;
}

std::vector<std::vector<int> >
NMImageLayer::getOverviewSizes()
{
    std::vector<std::vector<int> > sizes;

    if (!this->mFileName.isEmpty())
    {
        for (int i=0; i < this->mReader->getNumberOfOverviews(); ++i)
        {
            std::vector<int> os;
            os.push_back(this->mReader->getOverviewSize(i)[0]);
            os.push_back(this->mReader->getOverviewSize(i)[1]);
            sizes.push_back(os);
        }
    }


    return sizes;
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

    qreal dpr = qApp->devicePixelRatio();

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

    int* size = ren->GetSize();
    double wminx, wmaxx, wminy, wmaxy, wwidth, wheight;
    bool bHasVisReg = false;
    if (ren)
    {
        double wbr[] = {-1,-1,-1,-1};
        double wtl[] = {-1,-1,-1,-1};

        // top left (note: display origin is bottom left)
        vtkInteractorObserver::ComputeDisplayToWorld(ren, 0,size[1]-1,0, wtl);

        // since the image and image-selection mapper are hooked up to this
        // method, we make sure that we don't do double the work, i.e. if
        // nothing has changed ('cause the other mapper has called this function
        // already), we don't have to reload new data;
        if (wtl[0] == mWTLx)
        {
            return;
        }

        wminx = wtl[0];
        wmaxy = wtl[1];

        // bottom right
        vtkInteractorObserver::ComputeDisplayToWorld(ren, size[0]-1,0,0, wbr);
        wmaxx = wbr[0];
        wminy = wbr[1];

        // only works good if
        wwidth = (wmaxx - wminx);
        wheight = (wmaxy - wminy);

        vtkBoundingBox bbworld(wminx, wmaxx, wminy, wmaxy, 0, 0);
        vtkBoundingBox bblayer(mBBox);

        if (bbworld.Intersects(bblayer) == 0)
        {
            // no point in rendering anything, if the layer
            // is outside the view window!
            for (int v=0; v < 6; ++v)
            {
                mVisibleRegion[v] = 0;
            }
            bHasVisReg = false;
        }
        else
        {
            bHasVisReg = true;



            if (wwidth > 1 && wheight > 1)
            {
                h_res = wwidth / (double)size[0];
                v_res = wheight / (double)size[1];
            }
            else
            {
                h_res = mSpacing[0]*dpr;//fullcols / size[0];
                v_res = ::fabs(mSpacing[1])*dpr;//fullrows / size[1];
            }
        }

            //        NMDebugAI(<< "world: tl: " << wtl[0] << " " << wtl[1]
            //                  << " br: " << wbr[0] << " " << wbr[1] << std::endl);
    }


    int ovidx = -1;
    if (!bHasVisReg)
    {
        ovidx = this->mReader->getNumberOfOverviews()-1;
    }
    else
    {
        double target_res;
        double opt_res;
        if (size[0] > size[1])
        {
            opt_res = mSpacing[0]*dpr;
            target_res = h_res;
        }
        else
        {
            target_res = v_res;
            opt_res = ::fabs(mSpacing[1])*dpr;
        }
        opt_res *= 0.15;
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
    }

    if (this->mRenderer->GetViewProps()->GetNumberOfItems() > 0)
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
        uext[1] = xe > cols-1 ? cols-1 : xe < 0 ? 0 : xe;
        uext[2] = yo > rows-1 ? rows-1 : yo < 0 ? 0 : yo;
        uext[3] = ye > rows-1 ? rows-1 : ye < 0 ? 0 : ye;
        uext[4] = 0;
        uext[5] = 0;

        // visible itk image region
        mVisibleRegion[0] = uext[0];                  // x-origin
        mVisibleRegion[1] = uext[1] - uext[0] + 1;    // x-size
        mVisibleRegion[2] = uext[2];                  // y-origin
        mVisibleRegion[3] = uext[3] - uext[2] + 1;    // y-size
        mVisibleRegion[4] = uext[4];                  // z-origin
        mVisibleRegion[5] = uext[4] == 0 && uext[5] == 0 ? 0 : uext[5] - uext[4] + 1; // z-size

        //        NMDebugAI(<< "ov #" << ovidx << ": " << cols << "x" << rows << std::endl);

        //        NMDebugAI(<< "uext: " << uext[0] << ".." << uext[1] << " "
        //                              << uext[2] << ".." << uext[3] << std::endl);

        //        NMDebugAI(<< "wext: " << wext[0] << "," << wext[2] << " "
        //                              << wext[1] << "x" << wext[3] << std::endl);

        this->mReader->setOverviewIdx(ovidx, mVisibleRegion);

        // update mapper, if actor is visible
        if (this->mActor->GetVisibility())
        {
            this->mMapper->UpdateInformation();
            NMVtkOpenGLImageSliceMapper* ism = NMVtkOpenGLImageSliceMapper::SafeDownCast(this->mMapper);
            ism->SetDisplayExtent(uext);
            ism->SetDataWholeExtent(uext);
            this->mMapper->Update();
        }

        // update the selection mapper if selection
        // actor is visible
        if (this->mImgSelSlice->GetVisibility())
        {
            this->mImgSelMapper->UpdateInformation();
            this->mImgSelMapper->SetDisplayExtent(uext);
            this->mImgSelMapper->SetDataWholeExtent(uext);
            this->mImgSelMapper->Update();
        }
    }
    else
    {
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

    if (inScalars == nullptr)
    {
        NMLogDebug(<< "NMImageLayer::mapRGBImage() - inScalars == nullptr");
        return;
    }
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
    if (inScalars == nullptr)
    {
        NMLogDebug(<< "NMImageLayer::mapRGBImageScalars() - inScalars == nullptr");
        return;
    }
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
            NMLogError(<< ctxNMImageLayer << ": Invalid input pixel type!");
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
            NMLogError(<< ctxNMImageLayer << ": Invalid Data Type!");
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
    QVariant::Type valtype = this->getColumnType(colidx);

    if (    colidx < 0
        ||  valtype == QVariant::String
        ||  this->mTableModel == 0
        ||  this->mLegendType == NMLayer::NM_LEGEND_INDEXED
       )
    {
        // account for the case that the primary RAT index is
        // not contiguous, i.e. max index may be >= number of
        // records-1
        if (mLegendType == NMLayer::NM_LEGEND_INDEXED)
        {
            // only if we're not using an index map do we not
            // have to set the attribute value as scalars
            if (!mUseIdxMap)
            {
                return;
            }
        }
        else
        {
            return;
        }
    }

    if (colidx != mScalarColIdx)
    {
        mScalarColIdx = colidx;
        if (mNumRecords <= 1e6)
        {
            mScalarDoubleMap.clear();
            mScalarLongLongMap.clear();
            updateScalarBuffer();
        }
    }

    vtkDataSetAttributes* dsa = img->GetAttributes(vtkDataSet::POINT);
    vtkDataArray* idxScalars = img->GetPointData()->GetArray(0);
    void* buf = idxScalars->GetVoidPointer(0);
    int numPix = idxScalars->GetNumberOfTuples();

    switch(this->getColumnType(mScalarColIdx))
    {
    case QVariant::Double:
        {
            vtkSmartPointer<vtkDoubleArray> ar = vtkSmartPointer<vtkDoubleArray>::New();
            ar->SetName(mLegendValueField.toStdString().c_str());
            ar->SetNumberOfComponents(1);
            ar->SetNumberOfTuples(numPix);

            double* out = static_cast<double*>(ar->GetVoidPointer(0));
            const double nodata = this->getNodata();

            std::map<long long, double>::iterator it;
            switch(idxScalars->GetDataType())
            {
            vtkTemplateMacro(setDoubleScalars(static_cast<VTK_TT*>(buf),
                                              out, numPix, nodata
                                              )
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
                    }
                }
            }



            dsa->AddArray(ar);
            dsa->SetActiveAttribute(mLegendValueField.toStdString().c_str(),
                                    vtkDataSetAttributes::SCALARS);
        }
        break;

    case QVariant::Int:
    case QVariant::LongLong:
    case QVariant::UInt:
    case QVariant::ULongLong:
        {
            vtkSmartPointer<vtkLongLongArray> ar = vtkSmartPointer<vtkLongLongArray>::New();
            ar->SetName(mLegendValueField.toStdString().c_str());
            ar->SetNumberOfValues(numPix);
            ar->SetNumberOfComponents(1);

            long long* out = static_cast<long long*>(ar->GetVoidPointer(0));
            long long nodata = static_cast<long long>(this->getNodata());

            std::map<long long, long long>::iterator it;
            switch(idxScalars->GetDataType())
            {
            vtkTemplateMacro(
                        setLongScalars(static_cast<VTK_TT*>(buf),
                                       out, numPix, nodata
                                       )
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

    NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mTableModel);
    QString conname("NMImageLayer_updateBuffer");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conname);
        db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_OPEN_URI");
        db.setDatabaseName(sqlModel->getDatabaseName());
        if (!db.open())
        {
            NMDebugAI(<< db.lastError().text().toStdString() << std::endl);
            return;
        }

        QString prepStr = QString("SELECT %1,%2 from %3")
                            .arg(sqlModel->getNMPrimaryKey())
                            .arg(mLegendValueField)
                            .arg(sqlModel->tableName());

        db.transaction();
        QSqlQuery q(db);
        q.setForwardOnly(true);
        if (!q.exec(prepStr))
        {
            NMLogError(<< ctxNMImageLayer << ": Couldn't fetch scalar values from database!");
            return;
        }

        double dnodata = 0;
        long long inodata = 0;

        const int rep = mNumRecords / 20;
        for (int r=0; r < mNumRecords; ++r)
        {
            q.next();

            switch(this->getColumnType(mScalarColIdx))
            {
            case QVariant::Int:
            case QVariant::LongLong:
            case QVariant::UInt:
            case QVariant::ULongLong:

                    mScalarLongLongMap.insert(
                                std::pair<long long, long long>(
                                q.value(0).toLongLong(),
                                q.value(1).toLongLong()));
                    break;

            case QVariant::Double:

                    mScalarDoubleMap.insert(
                                   std::pair<long long, double>(
                                   q.value(0).toLongLong(),
                                   q.value(1).toDouble()));
                   break;

            default:
                break;
            }

            if (r % (rep > 0 ? rep : 1) == 0)
            {
                NMDebug(<< ".");
            }
        }
        q.finish();
        q.clear();
        db.commit();
        db.close();
    }
    QSqlDatabase::removeDatabase(conname);

    NMDebug(<< std::endl);
    NMDebugCtx(ctxNMImageLayer, << "done!");
}

// worker function for scalars setting
template<class T>
void
NMImageLayer::setLongScalars(T* buf, long long *out,
                                     long long numPix,
                                     long long nodata)
{
    //CALLGRIND_START_INSTRUMENTATION;
    if (mNumRecords > 1e6)
    {
        int nthreads = QThread::idealThreadCount();
        long long threadpix = numPix / nthreads;
        long long rest = numPix - (threadpix * nthreads);

        QList<QFuture<void> > flist;
        for (int t=0; t < nthreads; ++t)
        {
            flist << QFuture<void>();
        }

        long long start=0, end=0;
        for (int th=0; th < nthreads; ++th)
        {
            end = start+threadpix-1;

            if (th == nthreads-1)
            {
                end += rest;
            }

            flist[th] = QtConcurrent::run(this, &NMImageLayer::setLongDBScalars<T>,
                                           buf, out, start, end, nodata);

            start = end+1;
        }

        for (int th=0; th < nthreads; ++th)
        {
            flist[th].waitForFinished();
        }
    }
    else
    {
        std::map<long long, long long>::iterator it;
        for (int i=0; i < numPix; ++i)
        {
            it = mScalarLongLongMap.find(buf[i]);
            if (it != mScalarLongLongMap.end())
            {
                out[i] = it->second;
            }
            else
            {
                out[i] = nodata;
            }
        }
    }
}

template<class T>
void
NMImageLayer::setLongDBScalars(T* buf,
                               long long* out,
                               long long start,
                               long long end,
                               long long nodata
                               )
{
    NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mTableModel);

    QString conname = QString("NMImageLayerSLS_%1").arg(start);

    // db scope
    // without db and q going out of scope, we cannot remove
    // db without Qt complaining
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conname);
        db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_OPEN_URI");
        db.setDatabaseName(sqlModel->getDatabaseName());
        if (!db.open())
        {
            NMDebugAI(<< db.lastError().text().toStdString() << std::endl);
            return;
        }

        QString prepStr = QString("SELECT %1 from %2 where %3 = :row")
                            .arg(mLegendValueField)
                            .arg(sqlModel->tableName())
                            .arg(sqlModel->getNMPrimaryKey());

        db.transaction();
        QSqlQuery q(db);
        if (q.prepare(prepStr))
        {
            for (int i=start; i <= end; ++i)
            {
                q.bindValue(0, QVariant::fromValue(buf[i]));
                if (q.exec())
                {
                    if (q.next())
                    {
                        out[i] = q.value(0).toLongLong();
                    }
                    else
                    {
                        out[i] = nodata;
                    }
                }
                else
                {
                    out[i] = nodata;
                }
                q.finish();
            }
        }
        else
        {
            std::string err = q.lastError().text().toStdString();
            NMLogError(<< ctxNMImageLayer << ": " << err);
        }
        q.finish();
        q.clear();
        db.commit();
        db.close();
    }
    QSqlDatabase::removeDatabase(conname);
}

template<class T>
void
NMImageLayer::setDoubleScalars(T* buf, double* out,
                               long long numPix, double nodata)
{
    if (mNumRecords > 1e6)
    {
        int nthreads = QThread::idealThreadCount();
        long long threadpix = numPix / nthreads;
        long long rest = numPix - (threadpix * nthreads);

        QList<QFuture<void> > flist;
        for (int t=0; t < nthreads; ++t)
        {
            flist << QFuture<void>();
        }

        long long start=0, end=0;
        for (int th=0; th < nthreads; ++th)
        {
            end = start+threadpix-1;

            if (th == nthreads-1)
            {
                end += rest;
            }

            flist[th] = QtConcurrent::run(this, &NMImageLayer::setDoubleDBScalars<T>,
                                           buf, out, start, end, nodata);
            start = end+1;
        }

        for (int th=0; th < nthreads; ++th)
        {
            flist[th].waitForFinished();
        }
    }
    else
    {
        std::map<long long, double>::iterator it;
        for (int i=0; i < numPix; ++i)
        {
            it = mScalarDoubleMap.find(buf[i]);
            if (it != mScalarDoubleMap.end())
            {
                out[i] = it->second;
            }
            else
            {
                out[i] = nodata;
            }
        }
    }
}

template<class T>
void
NMImageLayer::setDoubleDBScalars(T* buf,
                                 double* out,
                                 long long start,
                                 long long end,
                                 double nodata
                                 )
{
    NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mTableModel);

    QString conname = QString("NMImageLayerSLS_%1").arg(start);

    // db scope
    // without db and q going out of scope, we cannot remove
    // db without Qt complaining
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conname);
        db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_OPEN_URI");
        db.setDatabaseName(sqlModel->getDatabaseName());
        if (!db.open())
        {
            NMDebugAI(<< db.lastError().text().toStdString() << std::endl);
            return;
        }

        QString prepStr = QString("SELECT %1 from %2 where %3 = :row")
                            .arg(mLegendValueField)
                            .arg(sqlModel->tableName())
                            .arg(sqlModel->getNMPrimaryKey());

        db.transaction();
        QSqlQuery q(db);
        if (q.prepare(prepStr))
        {
            for (int i=start; i <= end; ++i)
            {
                q.bindValue(0, QVariant::fromValue(buf[i]));
                if (q.exec())
                {
                    if (q.next())
                    {
                        out[i] = q.value(0).toDouble();
                    }
                    else
                    {
                        out[i] = nodata;
                    }
                }
                else
                {
                    out[i] = nodata;
                }
                q.finish();
            }
        }
        else
        {
            std::string err = q.lastError().text().toStdString();
            NMLogError(<< ctxNMImageLayer << ": " << err);
        }
        q.finish();
        q.clear();
        db.commit();
        db.close();
    }
    QSqlDatabase::removeDatabase(conname);
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
    bool bOpenRAMTable = false;
    bool bOpenSQLTable = false;
    if (dc != 0 && dc->getOutput(0) != 0 && dc->getOutput(0)->getDataObject() != 0)
    {
        QSize tabSize;
        QPoint tabPos;
        if (this->mTableView)
        {
            tabSize = this->mTableView->size();
            tabPos = this->mTableView->pos();

            this->mTableView->clearSelection();
            this->mTableView->close();
            delete this->mTableView;
            mTableView = 0;

            delete mSelectionModel;
            mSelectionModel = 0;


            delete this->mTableModel;
            mTableModel = 0;

            bOpenRAMTable = true;
        }

        if (this->mSqlTableView)
        {
            tabSize = mSqlTableView->size();
            tabPos = mSqlTableView->pos();

            mSqlTableView->clearSelection();
            mSqlTableView->close();
            delete mSqlTableView;
            mSqlTableView = 0;

            NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mTableModel);
            sqlModel->database().close();
            sqlModel->clear();
            delete sqlModel;
            mTableModel = 0;

            {
                QSqlDatabase db = QSqlDatabase::database(mQSqlConnectionName, false);
                if (db.isValid() && db.isOpen())
                {
                    db.close();
                }
            }
            QSqlDatabase::removeDatabase(mQSqlConnectionName);
            mQSqlConnectionName.clear();

            delete mSelectionModel;
            mSelectionModel = 0;

            ::sqlite3_close(mSqlViewConn);
            spatialite_cleanup_ex(mSpatialiteCache);
            mSpatialiteCache = 0;
            mSqlViewConn = 0;

            bOpenSQLTable = true;
        }

        this->mOtbRAT = dc->getOutput(0)->getOTBTab();
        this->updateAttributeTable();
        if (bOpenRAMTable || bOpenSQLTable)
        {
            this->createTableView();
            if (bOpenRAMTable && mTableView)
            {
                mTableView->show();
                mTableView->resize(tabSize);
                mTableView->move(tabPos);
            }
            else if (bOpenSQLTable && mSqlTableView)
            {
                mSqlTableView->show();
                mSqlTableView->resize(tabSize);
                mSqlTableView->move(tabPos);
            }
        }

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

    if (mTableView != nullptr)
    {
        mTableView->clearSelection();
    }
    if (mSqlTableView != nullptr)
    {
        mSqlTableView->clearSelection();
    }

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
        this->mSelBBox[i] = bbox[i];
    }
    delete bbox;

    this->initiateLegend();
    emit layerLoaded();
}


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

long long
NMImageLayer::getNumTableRecords()
{
    return mNumRecords;
}


otb::AttributeTable::Pointer
NMImageLayer::getRasterAttributeTable(int band)
{
    // it could very well be that this function is called
    // multiple times, so in view of potentially large tables,
    // let's not double any efforts and just return what we've got
    // (should be safe, since we don't really allow for changing
    // tables anyway, so: she'll be right!
    if (mOtbRAT.IsNotNull())
    {
       return mOtbRAT;
    }


    otb::AttributeTable::Pointer tab = 0;
    if (this->mReader != 0 && this->mReader->isInitialised())
    {
        if (band < 1 || band > this->mReader->getOutputNumBands())
            return 0;

        // note: we can have a valid table object with no records; no
        // columns shouldn't occur at all, but doesn't do any harm
        // anyway ...
        tab = this->mReader->getRasterAttributeTable(band);
        if (tab.IsNull())
        {
            tab = 0;
            mOtbRAT = 0;
            mNumRecords = 0;
        }
        else
        {
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
    if (mNumBands == 3)
    {
        imgW->setIsRGBImage(true);
    }
    imgW->setOTBTab(this->mOtbRAT);
	return imgW;
}

vtkImageData *NMImageLayer::getVTKImage(void)
{
    if (mPipeconn->isConnected())
    {
        return mPipeconn->getVtkImage();
    }
}

vtkIdTypeArray* NMImageLayer::getHistogram(void)
{
    if (mHistogram.GetPointer() != 0)
    {
        return mHistogram;
    }

    return 0;
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
        NMLogError(<< ctxNMImageLayer << ": No valid file name set! Abort!")
        NMDebugCtx(ctxNMImageLayer, << "done!");
		return;
	}

    otb::SQLiteTable::Pointer sqlTab = static_cast<otb::SQLiteTable*>(this->mOtbRAT.GetPointer());
    sqlTab->SetOpenReadOnly(true);
    sqlTab->SetUseSharedCache(true);
    if (sqlTab->openConnection())
    {
        sqlTab->PopulateTableAdmin();
    }

	bool berr = false;
    //const char* fn = this->mFileName.toUtf8().constData();
    std::string fn = this->mFileName.toStdString();
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

        if (gio->CanWriteFile(fn.c_str()))
		{
			gio->WriteRAT(this->mOtbRAT, band);
		}
		else
		{
			berr = true;
		}
	}

    sqlTab->CloseTable();

	if (berr)
	{
        NMLogError(<< ctxNMImageLayer << ": Couldn't update the RAT!");
	}

	NMDebugCtx(ctxNMImageLayer, << "done!");
}
