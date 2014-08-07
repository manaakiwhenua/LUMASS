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
	this->mComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mbStatsAvailable = false;

	this->mLayerIcon = QIcon(":image_layer.png");

    this->mOverviewIdx = -1; // (loading original image)
    this->mbUseOverviews = true;

    //vtkInteractorObserver* style = mRenderWindow->GetInteractor()->GetInteractorStyle();
    //this->mVtkConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();

//	this->mVtkConn->Connect(style, vtkCommand::ResetWindowLevelEvent,
//			this, SLOT(windowLevelReset(vtkObject*)));
//	this->mVtkConn->Connect(style, vtkCommand::InteractionEvent,
//			this, SLOT(windowLevelChanged(vtkObject*)));
}

NMImageLayer::~NMImageLayer()
{
	if (this->mReader)
		delete this->mReader;
	if (this->mPipeconn)
		delete this->mPipeconn;

//	if (this->mTableView)
//	{
//		this->mTableView->close();
//		delete this->mTableView;
//	}
}

void
NMImageLayer::computeStats(void)
{
	NMDebugCtx(ctxNMImageLayer, << "...");
    if (true)//this->mbStatsAvailable)
	{
		QtConcurrent::run(this, &NMImageLayer::updateStats);
	}

	NMDebugAI(<< "Image Statistics for '" << this->objectName().toStdString() << "' ... " << std::endl);
	NMDebugAI(<< "min:     " << mImgStats[0] << std::endl);
	NMDebugAI(<< "max:     " << mImgStats[1] << std::endl);
	NMDebugAI(<< "mean:    " << mImgStats[2] << std::endl);
	NMDebugAI(<< "median:  " << mImgStats[3] << std::endl);
	NMDebugAI(<< "std.dev: " << mImgStats[4] << std::endl);

	//vtkSmartPointer<vtkRenderWindow> renwin = vtkSmartPointer<vtkRenderWindow>::New();

	NMDebugCtx(ctxNMImageLayer, << "done!");
}

const double*
NMImageLayer::getStatistics(void)
{
	if (!this->mbStatsAvailable)
        this->updateStats();

	return this->mImgStats;
}

void NMImageLayer::test()
{
	this->updateAttributeTable();

	double nodata;
	switch(this->mComponentType)
	{
		case otb::ImageIOBase::UCHAR:  nodata = 255; 		 break;
		case otb::ImageIOBase::DOUBLE: nodata = -3.40282e38; break;
		default:					   nodata = -2147483647; break;
	}


	///////////////////////////////// GET RAT PROPS INCASE WE HAVE ONE /////////////////////////////////////////
	int ncols=0, numcolors=256;
	QStringList cols;
	QList<int> colindices;
	int idxred = -1, idxblue = -1, idxgreen = -1, idxalpha = -1;
	if (this->mTableModel)
	{
		ncols = this->mTableModel->columnCount(QModelIndex());
		numcolors = this->mTableModel->rowCount(QModelIndex());

		for (int i=0; i < ncols; ++i)
		{
			const QModelIndex ci = mTableModel->index(0, i, QModelIndex());
			if (mTableModel->data(ci, Qt::DisplayRole).type() != QVariant::String)
			{
				QString fieldName = mTableModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
				cols << fieldName;
				colindices << i;

				if (fieldName.compare("red", Qt::CaseInsensitive) == 0)
					idxred = i;
				else if (fieldName.compare("green", Qt::CaseInsensitive) == 0)
					idxgreen = i;
				else if (fieldName.compare("blue", Qt::CaseInsensitive) == 0)
					idxblue = i;
				else if (	fieldName.compare("alpha", Qt::CaseInsensitive) == 0
						 || fieldName.compare("opacity", Qt::CaseInsensitive) == 0
						)
					idxalpha = i;
			}
		}
	}

	////////////////////// GET USER INPUT ////////////////////////////////////////////////

	QString theCol;
	int colidx;
	if (mTableModel)
	{
		theCol= QInputDialog::getItem(0, "Select Value Field", "", cols);
		colidx = colindices.at(cols.indexOf(theCol));
		NMDebugAI(<< "using field #" << colidx << ": " << theCol.toStdString() << std::endl);
	}

	QStringList ramplist;
	ramplist << "Binary" << "Grey" << "Rainbow" << "RedToBlue" << "GreenToBlue";
	if (idxred >= 0 && idxgreen >=0 && idxblue >=0)
		ramplist << "ColourTable";
	QString ramp = QInputDialog::getItem(0, "Select Color Scheme", "", ramplist);

	QString range = QInputDialog::getText(0, "Enter Value Range", "");
	QStringList ranges = range.split(QChar(' '), QString::SkipEmptyParts);
	double lower = 0, upper;
	bool bok;
	if (ranges.size() == 2)
	{
		lower = ranges.at(0).toDouble(&bok);
		upper = ranges.at(1).toDouble(&bok);
		if (!bok)
		{
			lower = 0;
			upper = numcolors-1;
		}
	}
	else if (this->mTableModel && ramp != "ColourTable")
	{
		NMTableCalculator calc(mTableModel, this);

		std::vector<double> stats = calc.calcColumnStats(theCol);
		lower = stats[0];
		upper = stats[1];
		NMDebugAI(<< theCol.toStdString() << " statistics: " << std::endl);
		NMDebugAI(<< "min=" << lower << " max=" << upper << " mean=" << stats[2]
		          << "median=" << stats[3] << "std.dev=" << stats[4] << " numVals=" << stats[5] << std::endl);
	}
	else if (this->mTableModel == 0 && this->mLayerType == NMLayer::NM_IMAGE_LAYER)
	{
		const double* imgstats = this->getStatistics();
		lower = imgstats[0];
		upper = imgstats[1];
		NMDebugAI(<< this->objectName().toStdString() << " statistics: " << std::endl);
		NMDebugAI(<< "min=" << lower << " max=" << upper << " mean=" << imgstats[2]
		          << "std.dev=" << imgstats[3] << " median=" << imgstats[4] << std::endl);
	}
	if (lower == nodata)
		lower = 0;

	NMDebugAI(<< "user range: " << lower << " - " << upper << std::endl);


	////////////////////////////////// PREPARE LOOKUP TABLE ///////////////////////////////////////////

	// since color is addressed by 0-based index, we don't need the +1 here
	double valrange = upper - lower;// + 1;
	double step = valrange / (double)numcolors;
	double tolerance = step * 0.5;

	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->SetNumberOfTableValues(numcolors+2);
	double* clrptr = 0;
	if (mTableModel)
	{

		clrtab->SetTableRange(-1, numcolors);
		//clrtab->IndexedLookupOn();
		//clrtab->SetNanColor(0, 0, 0, 0);
	}
	else if (ramp == "RedToBlue")
	{
		clrtab->SetNumberOfTableValues(numcolors);
		//clrtab->SetTableRange(lower-step, upper+step);
		clrtab->SetTableRange(nodata, upper+0.0000001);
		clrptr = new double[3*numcolors];
	}


	/////////////////////////////////// BUILD THE COLOR TRANSFER FUNCTION ///////////////////////////////////////

	vtkSmartPointer<vtkColorTransferFunction> clrfunc =
			vtkSmartPointer<vtkColorTransferFunction>::New();
	if (ramp == "Binary")
	{
		clrfunc->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		clrfunc->AddRGBPoint(1.0, 0.0, 0.0, 0.0);
	}
	else if (ramp == "Grey")
	{
		clrfunc->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		clrfunc->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
	}
	else if (ramp == "Rainbow")
	{
		//clrfunc->SetColorSpaceToHSV();
		double* rgb = new double[3];
		double* hsv = new double[3];
		hsv[0] = 0.0;
		hsv[1] = 1.0;
		hsv[2] = 1.0;
		vtkMath::HSVToRGB(hsv, rgb);
		clrfunc->AddRGBPoint(0.0, rgb[0], rgb[1], rgb[2]);

		hsv[0] = 0.25;
		vtkMath::HSVToRGB(hsv, rgb);
		clrfunc->AddRGBPoint(0.25, rgb[0], rgb[1], rgb[2]);

		hsv[0] = 0.5;
		vtkMath::HSVToRGB(hsv, rgb);
		clrfunc->AddRGBPoint(0.5, rgb[0], rgb[1], rgb[2]);

		hsv[0] = 0.75;
		vtkMath::HSVToRGB(hsv, rgb);
		clrfunc->AddRGBPoint(0.75, rgb[0], rgb[1], rgb[2]);

		hsv[0] = 1.0;
		vtkMath::HSVToRGB(hsv, rgb);
		clrfunc->AddRGBPoint(1.0, rgb[0], rgb[1], rgb[2]);

		delete[] rgb;
		delete[] hsv;
	}
	else if (ramp == "RedToBlue")
	{
		clrfunc->SetColorSpaceToDiverging();
		if (mTableModel)
		{
			clrfunc->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
			clrfunc->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
		}
		else
		{
			clrfunc->AddRGBPoint(nodata, 0.4,0.4,0.4);
			clrfunc->AddRGBPoint(lower-0.0000001, 0.0,0.0,0.0);
			clrfunc->AddRGBPoint(lower, 1.0, 0.0, 0.0);
			clrfunc->AddRGBPoint(upper, 0.0, 0.0, 1.0);
			clrfunc->AddRGBPoint(upper+0.0000001, 0.0,1.0,0.0);
		}
	}
	else if (ramp == "GreenToBlue")
	{
		clrfunc->SetColorSpaceToDiverging();
		clrfunc->AddRGBPoint(0.0, 0.0, 1.0, 0.0);
		clrfunc->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
	}
	clrfunc->Build();


	////////////////////////////// FILL THE LOOKUP TABLE ///////////////////////////////////////

	// everything below the 'range' is transparent
	if (mTableModel || ramp != "RedToBlue")
		clrtab->SetTableValue(0, 0, 0, 0, 0);

	NMDebugAI(<< "checking colour assignments ..." << std::endl);
	long in=0, nod=0, out=0;
	double minclrpos, maxclrpos;
	for (int i=0; i < numcolors; ++i)
	{
		// ============================ USING COLOUR TABLE (i.e. attributes of Table) =====================
		if (mTableModel && ramp == "ColourTable")
		{
			double fc[4];
			const QModelIndex mired   = mTableModel->index(i, idxred  , QModelIndex());
			const QModelIndex migreen = mTableModel->index(i, idxgreen, QModelIndex());
			const QModelIndex miblue  = mTableModel->index(i, idxblue , QModelIndex());
			const QModelIndex mialpha = mTableModel->index(i, idxalpha, QModelIndex());

			const QVariant vred = mTableModel->data(mired, Qt::DisplayRole);
			const QVariant vgreen = mTableModel->data(migreen, Qt::DisplayRole);
			const QVariant vblue = mTableModel->data(miblue, Qt::DisplayRole);
			const QVariant valpha = mTableModel->data(mialpha, Qt::DisplayRole);
			if (vred.type() == QVariant::Double)
			{
				fc[0] = vred.toDouble(&bok);
				fc[1] = vgreen.toDouble(&bok);
				fc[2] = vblue.toDouble(&bok);
				fc[3] = valpha.toDouble(&bok);
			}
			else
			{
				fc[0] = vred.toDouble(&bok)   / 255.0;
				fc[1] = vgreen.toDouble(&bok) / 255.0;
				fc[2] = vblue.toDouble(&bok)  / 255.0;
				fc[3] = valpha.toDouble(&bok) / 255.0;
			}
			//clrtab->SetTableValue(i, fc);
			clrtab->SetTableValue(i+1, fc);
		}
		// ============================= COLOURING ATTRIBUTE TABLE FIELD ==============================
		else if (mTableModel)
		{
			double fc[3];
			const QModelIndex fidx = mTableModel->index(i, colidx, QModelIndex());
			double fieldVal = mTableModel->data(fidx, Qt::DisplayRole).toDouble(&bok);
			if (bok && fieldVal >= lower && fieldVal <= upper && fieldVal != nodata)
			{
				double clrpos = (double)fieldVal/(double)valrange;
				//NMDebug(<< fieldVal << "=in" << "=" << clrpos << " ");
				clrfunc->GetColor(clrpos, fc);
				//clrtab->SetTableValue(i, fc[0], fc[1], fc[2], 1);
				clrtab->SetTableValue(i+1, fc[0], fc[1], fc[2], 1);

				if (i==0)
				{
					minclrpos = clrpos;
					maxclrpos = clrpos;
				}
				else
				{
					minclrpos = ::min(clrpos, minclrpos);
					maxclrpos = ::max(clrpos, maxclrpos);
				}

				++in;
			}
			else if (fieldVal == nodata)
			{
				clrtab->SetTableValue(i+1, 0, 0, 0, 0);
				++nod;
			}
			else if (fieldVal > upper)
			{
				++out;
				clrtab->SetTableValue(i+1, 1, 1, 1, 1);
			}
			else if (fieldVal < lower)
			{
				++out;
				clrtab->SetTableValue(i+1, 0, 0, 0, 1);
			}
		}
		// ==================== JUST COLOUR THE SCALARS (i.e. image without table) =======================
		else
		{
			if (ramp != "RedToBlue")
			{
				double fc[3];
				double clrpos = (double)i/(double)(numcolors-3);
				clrfunc->GetColor(clrpos, fc);
				clrtab->SetTableValue(i+1, fc[0], fc[1], fc[2]);
			}
			else
			{
				if (i == 0)
				{
					clrfunc->GetTable(
							clrtab->GetTableRange()[0],
							clrtab->GetTableRange()[1],
							numcolors, clrptr);
				}
				double* pos = clrptr + 3*i;
				clrtab->SetTableValue(i, pos[0], pos[1], pos[2], 1);
			}
		}
	}
	// everything above the 'range' is going to be transparent as well
	if (mTableModel || ramp != "RedToBlue")
	{
		clrtab->SetTableValue(numcolors+1, 0, 0, 0, 0);
	}
	else
	{
		delete[] clrptr;
		clrptr = 0;
	}

	//else
	{
		int lidx = clrtab->GetIndex(lower);
		int uidx = clrtab->GetIndex(upper);
		NMDebugAI(<< "clrtab: lower-idx=" << lidx << " upper-idx=" << uidx << std::endl);
	}

	NMDebugAI(<< "colouring: in=" << in << " out=" << out << " nodata=" << nod << std::endl);
	NMDebugAI(<< "minclrpos=" << minclrpos << " maxcolrpos=" << maxclrpos << std::endl);

	this->mImgProp->SetUseLookupTableScalarRange(1);
	//if (mTableModel || ramp != "RedToBlue")
		this->mImgProp->SetLookupTable(clrtab);
	//else
		//this->mImgProp->SetLookupTable(clrfunc);

	double* clrrange = clrtab->GetRange();
	long numclr = clrtab->GetNumberOfColors();
	long numavailclr = clrtab->GetNumberOfAvailableColors();
	NMDebugAI(<< "range: " << clrrange[0] << " to " << clrrange[1]
	          << " num clr: " << numclr << " avail clr: " << numavailclr << std::endl);

	emit visibilityChanged(this);
	emit legendChanged(this);
}

double NMImageLayer::getDefaultNodata()
{
	double nodata;

	switch(this->mComponentType)
	{
		case otb::ImageIOBase::UCHAR:
			nodata = std::numeric_limits<unsigned short>::max();//255;
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

void
NMImageLayer::updateStats(void)
{
	emit layerProcessingStart();

	vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
	cast->SetOutputScalarTypeToDouble();
	cast->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

	vtkSmartPointer<vtkImageHistogramStatistics> stats = vtkSmartPointer<vtkImageHistogramStatistics>::New();
	stats->SetInputConnection(cast->GetOutputPort());
	//stats->GenerateHistogramImageOn();
	//stats->SetHistogramImageScaleToSqrt();
	//stats->SetHistogramImageSize(256,256);
	stats->Update();

	mImgStats[0] = stats->GetMinimum();
	mImgStats[1] = stats->GetMaximum();
	mImgStats[2] = stats->GetMean();
	mImgStats[3] = stats->GetMedian();
	mImgStats[4] = stats->GetStandardDeviation();

    this->mbStatsAvailable = true;

	emit layerProcessingEnd();
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
	unsigned int d=0;
    double spacing[3];
	double origin[3];
	int dims[3];
	double bnd[6];
	double err[3];

    if (bOnLPR)
    {
        for (int d=0; d<3; ++d)
        {
            bnd[d*2] = mBBox[d*2];
            bnd[d*2+1] = mBBox[d*2+1];
            origin[d] = mOrigin[d];
            spacing[d] = mSpacing[d];//::fabs(mSpacing[d]);
            wcoord[d] += spacing[d] / 2.0;
            dims[d] = (mBBox[d*2+1] - mBBox[d*2]) / ::fabs(spacing[d]);
            err[d] = 0.001 * spacing[d];
        }
    }
    else
    {
        vtkImageData* img = vtkImageData::SafeDownCast(
                const_cast<vtkDataSet*>(this->getDataSet()));

        if (img == 0)
            return;

        img->GetSpacing(spacing);
        img->GetOrigin(origin);
        img->GetDimensions(dims);

        // adjust coordinates & calc bnd
        for (d=0; d<3; ++d)
        {
            // account for 'pixel corner coordinate' vs.
            // 'pixel centre coordinate' philosophy of
            // vtk vs itk
            wcoord[d] += spacing[d] / 2.0;

            // account for origin is 'upper left' as for itk
            // vs. 'lower left' as for vtk
            if (d == 1)
            {
                bnd[d*2+1] = origin[d];
                bnd[d*2] = origin[d] + dims[d] * spacing[d];
            }
            else
            {
                bnd[d*2] = origin[d];
                bnd[d*2+1] = origin[d] + dims[d] * spacing[d];
            }

            // set the 'pointing accuracy' to 1 percent of
            // pixel width for each dimension
            err[d] = spacing[d] * 0.001;
        }
    }

	// check, whether the user point is within the
	// image boundary
    if (bImgConstrained)
    {
        if (vtkMath::PointIsWithinBounds(wcoord, bnd, err))
        {
            // calculate the image/pixel coordinates
            for (d = 0; d < 3; ++d)
            {
                pixel[d] = 0;

                if (dims[d] > 0)
                    pixel[d] = ::abs((int)((wcoord[d] - origin[d]) / spacing[d]));
            }
        }
    }
    else
    {
        //spacing[1] = -spacing[1];
        for (d = 0; d < 3; ++d)
        {
            if (dims[d] > 0)
                pixel[d] = (int)((wcoord[d] - origin[d]) / spacing[d]);
        }
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
	//if (this->mRATVec.size() < 1)
	//	return;

	if (this->mTableView != 0)
	{
		return;
		//delete this->mTableView;
		//this->mTableView = 0;
	}

	if (this->mOtbRAT.IsNull())
	{
		if (!this->updateAttributeTable())
			return;
	}

	if (this->mTableModel == 0)
		return;


	this->mTableView = new NMTableView(this->mTableModel, 0);
	this->mTableView->setSelectionModel(this->mSelectionModel);
	this->mTableView->setTitle(tr("Attributes of ") + this->objectName());

	connect(this, SIGNAL(selectabilityChanged(bool)), mTableView, SLOT(setSelectable(bool)));
	connect(mTableView, SIGNAL(notifyLastClickedRow(long)), this, SLOT(forwardLastClickedRowSignal(long)));

	// connect layer signals to tableview slots and model components list
	//this->connect(this, SIGNAL(attributeTableChanged(vtkTable*)),
	//		this->mTableView, SLOT(setTable(vtkTable*)));

	//TODO: connect to signal/slots for table updates

}

int
NMImageLayer::updateAttributeTable()
{
	if (this->mTableModel != 0)
		return 1;

    //disconnectTableSel();

    this->mOtbRAT = this->getRasterAttributeTable(1);
    if (mOtbRAT.IsNull())
	{
		NMWarn(ctxNMImageLayer, << "No attribute table available!");
		return 0;
	}
	NMQtOtbAttributeTableModel* otbModel;
	if (this->mTableModel == 0)
	{
		otbModel = new NMQtOtbAttributeTableModel(this->mOtbRAT);
	}
	else
	{
		otbModel = qobject_cast<NMQtOtbAttributeTableModel*>(this->mTableModel);
		otbModel->setTable(this->mOtbRAT);
		otbModel->setKeyColumn("rowidx");
	}
	otbModel->setKeyColumn("rowidx");

	// in any case, we create a new item selection model
	if (this->mSelectionModel == 0)
	{
		this->mSelectionModel = new NMFastTrackSelectionModel(otbModel, this);
	}
	this->mTableModel = otbModel;

	connectTableSel();
	emit legendChanged(this);

	return 1;
}

//void
//NMImageLayer::updateDataSet(QStringList& slAlteredColumns,
//		QStringList& slDeletedColumns)
//{
//	/* for image layers this means we write back any changes
//	 * made to table by the associated tableview (vtktable)
//	 *
//	 *
//	 */
//
//	NMDebugCtx(ctxNMImageLayer, << "...");
//
//	if (slDeletedColumns.size() == 0 && slAlteredColumns.size() == 0)
//	{
//		NMDebugAI(<< "nothing to save!" << endl);
//		NMDebugCtx(ctxNMImageLayer, << "done!");
//		return;
//	}
//
//
//
//
//
//	// vtkDataArray::ExportToVoidPointer(void* out_ptr);
//
//
//
//	NMDebugCtx(ctxNMImageLayer, << "done!");
//}

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
    }

	// TODO: find a more unambiguous way of determining
	// whether we're loading a rasdaman image or not
#ifdef BUILD_RASSUPPORT
	if (!filename.contains(".") && this->mpRasconn != 0)
	{
		NMRasdamanConnectorWrapper raswrap;
		raswrap.setConnector(this->mpRasconn);
		this->mReader->setRasConnector(&raswrap);
		//->setRasdamanConnector(this->mpRasconn);
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
    //this->mReader->getInternalProc()->ReleaseDataBeforeUpdateFlagOn();



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
	this->mNumDimensions = this->mReader->getOutputNumDimensions();

    // ==> set the desired overview and requested region,
    //     if supported
    this->mapExtentChanged();

	if (this->mNumBands != 1)
	{
		emit layerProcessingEnd();
		NMErr(ctxNMImageLayer, << "we currently only support single band images!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
	}

	// concatenate the pipeline
    this->mPipeconn->setInput(this->mReader->getOutput(0));
    //this->mPipeconn->getVtkImageImport()->ReleaseDataFlagOn();

//    this->mImgInfoChange = vtkSmartPointer<vtkImageChangeInformation>::New();
//    this->mImgInfoChange->SetInputConnection(this->mPipeconn->getVtkImageImport()->GetOutputPort());
//    vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
//    m->SetInputConnection(this->mImgInfoChange->GetOutputPort());



//    int we[6];
//    double origin[3];
//    double spacing[3];

//    vtkImageImport ii;

//    vtkInformation* outInfo = mPipeconn->getVtkImageImport()->GetOutputPortInformation(0);
//    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), we, 6);
//    outInfo->Get(vtkDataObject::ORIGIN(), origin, 3);
//    outInfo->Get(vtkDataObject::SPACING(), spacing, 3);

//    this->mImgInfoChange->SetOutputExtentStart(we[0], we[2], we[4]);
//    this->mImgInfoChange->SetOutputOrigin(origin);
//    this->mImgInfoChange->SetOutputSpacing(spacing);

    //vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
    //vtkSmartPointer<vtkImageSliceMapper> m = vtkSmartPointer<vtkImageSliceMapper>::New();
    vtkSmartPointer<NMVtkOpenGLImageSliceMapper> m = vtkSmartPointer<NMVtkOpenGLImageSliceMapper>::New();
    //vtkSmartPointer<vtkImageMapper> m = vtkSmartPointer<vtkImageMapper>::New();
    m->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

    //m->ResampleToScreenPixelsOn();
    //m->ResampleToScreenPixelsOff();
    //m->SeparateWindowLevelOperationOff();
    m->SetBorder(1);

	// adjust origin
    //double ori[3], spc[3];
    //vtkImageData* img = vtkImageData::SafeDownCast(this->mPipeconn->getVtkImage());
    //img->GetOrigin(ori);
    //img->GetSpacing(spc);

    //vtkSmartPointer<vtkImageActor> a = vtkSmartPointer<vtkImageActor>::New();
    vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
    //vtkSmartPointer<vtkActor2D> a = vtkSmartPointer<vtkActor2D>::New();
    a->SetMapper(m);

    mImgProp = vtkSmartPointer<vtkImageProperty>::New();
    mImgProp->SetInterpolationTypeToNearest();
    a->SetProperty(mImgProp);

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
    this->mActor = a;

    // we're going to update the attribute table, since
	// we'll probably need it
	//if (!this->updateAttributeTable())
	//{
	//	NMWarn(ctxNMImageLayer, << "Couldn't update the attribute table, "
	//			<< "which might lead to trouble later on!");
	//}

	this->mImage = 0;
	this->mFileName = filename;
    this->initiateLegend();

	emit layerProcessingEnd();
	emit layerLoaded();
	NMDebugCtx(ctxNMImageLayer, << "done!");
	return true;
}

void
NMImageLayer::mapExtentChanged(void)
{
    NMDebugCtx(ctxNMImageLayer, << "...");

    // check which overview is currently set
    // check the size of the current overview (mBufferedBox)

    // this makes only sense, if this layer is reader-based rather than
    // data buffer-based, so

    if (    !this->mbUseOverviews
        ||  this->mReader == 0
        ||  !this->mReader->isInitialised()
        ||  this->mReader->getNumberOfOverviews() == 0
       )
    {
        //        // can't do anything without available overviews
        //        NMDebugAI(<< "NMImageLayer::mapExtentChanged(): No reader or overivews."
        //                  << std::endl);
        NMDebugCtx(ctxNMImageLayer, << "done!");
        return;
    }

    // get the bbox and fullsize of this layer
    double h_ext = (mBBox[1] - mBBox[0]);
    double v_ext = (mBBox[3] - mBBox[2]);

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
        double wbr[] = {-1,-1,-1};
        double wtl[] = {-1,-1,-1};

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

    // also check the original resolution


    //NMDebug( << ">>>-------------------" << std::endl);
    this->mReader->setOverviewIdx(ovidx);
    if (this->mRenderer->GetViewProps()->GetNumberOfItems() > 0)
    {

        int cols = ovidx >= 0 ? this->mReader->getOverviewSize(ovidx)[0]: fullcols;
        int rows = ovidx >= 0 ? this->mReader->getOverviewSize(ovidx)[1]: fullrows;

        double uspacing[3];
        this->mReader->getSpacing(uspacing);

        int xo, yo, xe, ye;
        xo = (wminx - mBBox[0]) / uspacing[0];
        yo = (mBBox[3] - wmaxy) / ::fabs(uspacing[1]);
        xe = (wmaxx - mBBox[0]) / uspacing[0];
        ye = (mBBox[3] - wminy) / ::fabs(uspacing[1]);

        // calc update extent
        int uext[6];
        uext[0] = xo > cols-1 ? cols-1 : xo < 0 ? 0 : xo;
        uext[2] = yo > rows-1 ? rows-1 : yo < 0 ? 0 : yo;
        uext[1] = xe > cols-1 ? cols-1 : xe < 0 ? 0 : xe;
        uext[3] = ye > rows-1 ? rows-1 : ye < 0 ? 0 : ye;
        uext[4] = 0;
        uext[5] = 0;

        int wext[6];
        wext[0] = 0;
        wext[1] = cols-1;
        wext[2] = 0;
        wext[3] = rows-1;
        wext[4] = 0;
        wext[5] = 0;

        double uor[3];
        uor[0] = ::max<double>(mBBox[0],(wminx - mBBox[0]) / uspacing[0]);
        uor[1] = ::min<double>(mBBox[3], (mBBox[3] - wmaxy) / ::fabs(uspacing[1]));
        uor[2] = 0;

        itk::ImageRegion<2> reg;
        reg.SetIndex(0, uext[0]);
        reg.SetIndex(1, uext[2]);
        reg.SetSize(0, uext[1] - uext[0] + 1);
        reg.SetSize(1, uext[3] - uext[2] + 1);

        this->mReader->getOutput(0)->setImageRegion(NMItkDataObjectWrapper::NM_LARGESTPOSSIBLE_REGION,
                                                    (void*)&reg);

        this->mReader->getOutput(0)->setImageRegion(NMItkDataObjectWrapper::NM_REQUESTED_REGION,
                                                            (void*)&reg);


//        if (this->mOverviewIdx != ovidx)
//            this->mReader->update();

//        this->mPipeconn->getVtkImageImport()->SetWholeExtent(uext);
//        //this->mPipeconn->getVtkImageImport()->UpdateInformation();
//        this->mPipeconn->getVtkImageImport()->SetUpdateExtent(uext);



//        vtkStreamingDemandDrivenPipeline* sddp = vtkStreamingDemandDrivenPipeline::SafeDownCast(
//                    this->mPipeconn->getVtkImageImport()->GetExecutive());

//        vtkInformation* outInfo = sddp->GetOutputInformation(0);

//        //vtkImageData* img = vtkImageData::SafeDownCast(sddp->GetOutputData(0));
//        //img->SetExtent(uext);
//        //img->SetOrigin(uor);


//        outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), uext, 6);
//        outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uext, 6);
//        outInfo->Set(vtkDataObject::DATA_EXTENT(), uext, 6);
//        outInfo->Set(vtkDataObject::SPACING(), uspacing, 3);
//        outInfo->Set(vtkDataObject::ORIGIN(), uor, 3);

        this->mMapper->UpdateInformation();

//        vtkImageData* img = vtkImageData::SafeDownCast(this->mMapper->GetInputDataObject(0, 0));
//        img->SetExtent(uext);
//        img->SetOrigin(uor);

        NMVtkOpenGLImageSliceMapper* ism = NMVtkOpenGLImageSliceMapper::SafeDownCast(this->mMapper);
        ism->SetDisplayExtent(uext);
        ism->SetDataWholeExtent(uext);

        this->mRenderer->Render();

    }
    this->mOverviewIdx = ovidx;
    NMDebugCtx(ctxNMImageLayer, << "done!");
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
NMImageLayer::setImage(NMItkDataObjectWrapper* imgWrapper)
{
	if (imgWrapper == 0)
		return;

    this->mbUseOverviews = false;
	this->mImage = imgWrapper->getDataObject();
    this->mOtbRAT = imgWrapper->getOTBTab();
	this->mComponentType = imgWrapper->getItkComponentType();
	this->mNumDimensions = imgWrapper->getNumDimensions();
	this->mNumBands = imgWrapper->getNumBands();

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
    if (this->mReader != 0 && this->mReader->isInitialised())
    {

        if (band < 1 || band > this->mReader->getOutputNumBands())
            return 0;

        return this->mReader->getRasterAttributeTable(band);
    }
    else
    {
        return this->mOtbRAT;
    }
}

//void NMImageLayer::fetchRATs(void)
//{
//	// fetch attribute tables, if applicable
//	this->mRATVec.resize(this->mReader->getOutputNumBands());
//	for (int b=0; b < this->mReader->getOutputNumBands(); ++b)
//		this->mRATVec[b] = this->mReader->getRasterAttributeTable(b+1);
//}

NMItkDataObjectWrapper* NMImageLayer::getOutput(unsigned int idx)
{
	return 0;//return this->getImage();
}

NMItkDataObjectWrapper* NMImageLayer::getImage(void)
{
	itk::DataObject* img = this->getITKImage();
	if (img == 0)
		return 0;

	NMItkDataObjectWrapper* imgW = new NMItkDataObjectWrapper(this,
			img, this->mComponentType, this->mNumDimensions,
			this->mNumBands);
	return imgW;
}

itk::DataObject* NMImageLayer::getITKImage(void)
{
	if (mReader->getImageIOBase() == 0)
		return this->mImage;
	else
		return this->mReader->getItkImage();
}

otb::ImageIOBase::IOComponentType NMImageLayer::getITKComponentType(void)
{
	return this->mComponentType;
}

void NMImageLayer::setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg)
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
    const char* fn = this->mFileName.toLatin1().data();
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

//void
//NMImageLayer::windowLevelReset(vtkObject* obj)
//{
//	double window = this->mImgProp->GetColorWindow();
//	double level = this->mImgProp->GetColorLevel();

//	//NMDebugAI(<< "window: " << window << " level: " << level << std::endl);

//}

//void
//NMImageLayer::windowLevelChanged(vtkObject* obj)
//{
//	double window = this->mImgProp->GetColorWindow();
//	double level = this->mImgProp->GetColorLevel();

//	//NMDebugAI(<< "window: " << window << " level: " << level << std::endl);
//}


//void
//NMImageLayer::mapUniqueValues()
//{
//	NMDebugCtx(ctxNMImageLayer, << "...");
//
//	//if (this->mRATVec.size() == 0)
//	if (mOtbRAT.IsNull())
//	{
//		if (!this->updateAttributeTable())
//		{
//			NMDebugAI(<< "trouble getting an attribute table!");
//			return;
//		}
//	}
//
//	// make a list of available attributes
//	//otb::AttributeTable::Pointer tab = this->mRATVec.at(0);
//	int idxField = mOtbRAT->ColumnExists(mLegendValueField.toStdString());
//	if (idxField < 0)
//	{
//		NMErr(ctxNMImageLayer, << "the specified attribute does not exist!");
//		return;
//	}
//
//	// let's find out about the attribute
//	// if we've got doubles, we refuse to map unique values ->
//	// doesn't make sense, does it?
//	bool bNum = true;
//	if (mOtbRAT->GetColumnType(idxField) == otb::AttributeTable::ATTYPE_STRING)
//	{
//		bNum = false;
//	}
//	else if (mOtbRAT->GetColumnType(idxField) != otb::AttributeTable::ATTYPE_INT)
//	{
//		NMDebugAI( << "oh no, not with doubles!" << endl);
//		return;
//	}
//
//	// let's get the statistics if not done already
//	if (!this->mbStatsAvailable)
//	{
//		this->updateStats();
//	}
//	double min = this->mImgStats[0];
//	double max = this->mImgStats[1];
//	bool bMinIncluded = false;
//	bool bMaxIncluded = false;
//
//	// we create a new look-up table and set the number of entries we need
//	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
//	mLookupTable->Allocate(mOtbRAT->GetNumRows()+1);
//	mLookupTable->SetNumberOfTableValues(mOtbRAT->GetNumRows()+1);
//
//	// let's create a new legend info table
//	//this->resetLegendInfo();
//
//
//	// we iterate over the number of tuples in the user specified attribute array
//	// and assign each unique categorical value its own (hopefully unique)
//	// random colour, which is then inserted into the layer's lookup table; we further
//	// specify a default name for each colour and put it together with the
//	// chosen colour into a LengendInfo-Table, which basically holds the legend
//	// category to display; for linking attribute values to table-info and lookup-table
//	// indices, we fill the HashMap mHashValueIndices (s. Header file for further descr.)
//	bool bConvOk;
//	int clrCount = 0, val;
//	std::string fn = mLegendValueField.toStdString();
//	QString sVal;
//	vtkMath::RandomSeed(QTime::currentTime().msec());
//	for (int t=0; t < mOtbRAT->GetNumRows(); ++t)
//	{
//		int pixval = mOtbRAT->GetIntValue("rowidx", t);
//		if (pixval == min)
//		{
//			bMinIncluded = true;
//		}
//		else if (pixval == max)
//		{
//			bMaxIncluded = true;
//		}
//
//		if (bNum)
//		{
//			int val = mOtbRAT->GetIntValue(fn, t);
//			sVal = QString(tr("%1")).arg(val);
//		}
//		else
//		{
//			sVal = QString(mOtbRAT->GetStrValue(fn, t).c_str());
//		}
//
//		QHash<QString, QVector<int> >::iterator it = this->mHashValueIndices.find(sVal);
//		if (it == this->mHashValueIndices.end())
//		{
//			// add the key value pair to the hash map
//			QVector<int> idxVec;
//			idxVec.append(clrCount);
//			this->mHashValueIndices.insert(sVal, idxVec);
//
//			// add a new row to the legend_info table
//			vtkIdType newidx = this->mLegendInfo->InsertNextBlankRow(-9);
//
//			// add the value to the index map
//			double lowup[2];
//			lowup[0] = val;
//			lowup[1] = val;
//
//			vtkDoubleArray* lowupAbstrAr = vtkDoubleArray::SafeDownCast(
//					this->mLegendInfo->GetColumnByName("range"));
//			lowupAbstrAr->SetTuple(newidx, lowup);
//
//			// generate a random triple of uchar values
//			double rgba[4];
//			for (int i=0; i < 3; i++)
//				rgba[i] = vtkMath::Random();
//			rgba[3] = 1;
//
//			// add the color spec to the colour map
//			vtkDoubleArray* rgbaAr = vtkDoubleArray::SafeDownCast(
//					this->mLegendInfo->GetColumnByName("rgba"));
//			rgbaAr->SetTuple(newidx, rgba);
//
//			// add the name (sVal) to the name column of the legendinfo table
//			vtkStringArray* nameAr = vtkStringArray::SafeDownCast(
//					this->mLegendInfo->GetColumnByName("name"));
//			nameAr->SetValue(newidx, sVal.toStdString().c_str());
//
//			// add the color spec to the mapper's color table
//			mLookupTable->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
//			//			NMDebugAI( << clrCount << ": " << sVal.toStdString() << " = " << rgba[0]
//			//					<< " " << rgba[1] << " " << rgba[2] << endl);
//
//			clrCount++;
//		}
//		else
//		{
//			// add the index to the index map
//			int tabInfoIdx = this->mHashValueIndices.find(sVal).value()[0];
//			this->mHashValueIndices.find(sVal).value().append(t);
//
//			// add the colour to the real color table
//			vtkDoubleArray* dblAr = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("rgba"));
//			double tmprgba[4];
//			dblAr->GetTuple(tabInfoIdx, tmprgba);
//
//			mLookupTable->SetTableValue(t, tmprgba[0], tmprgba[1], tmprgba[2]);
//		}
//	}
//
//	if (!bMinIncluded && !bMaxIncluded)
//	{
//		// the nodata value
//		double val = bMinIncluded ? max : (bMaxIncluded ? val : max);
//
//		// add the key value pair to the hash map
//		QVector<int> idxVec;
//		idxVec.append(clrCount);
//		this->mHashValueIndices.insert("nodata", idxVec);
//
//		// add a new row to the legend_info table
//		vtkIdType newidx = this->mLegendInfo->InsertNextBlankRow(-9);
//
//		// add the value to the index map
//		double lowup[2];
//		lowup[0] = val;
//		lowup[1] = val;
//
//		vtkDoubleArray* lowupAbstrAr = vtkDoubleArray::SafeDownCast(
//				this->mLegendInfo->GetColumnByName("range"));
//		lowupAbstrAr->SetTuple(newidx, lowup);
//
//		// generate a random triple of uchar values
//		double rgba[4];
//		for (int i=0; i < 3; i++)
//			rgba[i] = 0;
//		rgba[3] = 0;
//
//		// add the color spec to the colour map
//		vtkDoubleArray* rgbaAr = vtkDoubleArray::SafeDownCast(
//				this->mLegendInfo->GetColumnByName("rgba"));
//		rgbaAr->SetTuple(newidx, rgba);
//
//		// add the name (sVal) to the name column of the legendinfo table
//		vtkStringArray* nameAr = vtkStringArray::SafeDownCast(
//				this->mLegendInfo->GetColumnByName("name"));
//		nameAr->SetValue(newidx, sVal.toStdString().c_str());
//
//		// add the color spec to the mapper's color table
//		mLookupTable->SetTableValue(mOtbRAT->GetNumRows(), rgba[0], rgba[1], rgba[2], rgba[3]);
//		//			NMDebugAI( << clrCount << ": " << sVal.toStdString() << " = " << rgba[0]
//		//					<< " " << rgba[1] << " " << rgba[2] << endl);
//
//	}
//
//	//this->mImgProp->SetLookupTable(mLookupTable);
//	//this->mImgProp->UseLookupTableScalarRangeOn();
//    //
//	//emit visibilityChanged(this);
//	//emit legendChanged(this);
//
//	NMDebugCtx(ctxNMImageLayer, << "done!");
//	return;
//}
