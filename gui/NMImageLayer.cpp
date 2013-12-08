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

#include <QTime>
#include <QtCore>

#include "itkImageRegion.h"
#include "itkPoint.h"

#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkRenderWindowInteractor.h"
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

#include "itkDataObject.h"
#include "otbImage.h"
#include "otbVectorImage.h"

template<class PixelType, unsigned int Dimension>
class InternalImageHelper
{
public:
	typedef otb::Image<PixelType, Dimension> ImgType;
	typedef typename ImgType::RegionType RegionType;
	typedef otb::VectorImage<PixelType, Dimension> VecImgType;
	typedef typename VecImgType::RegionType VecRegionType;

	static void getBBox(itk::DataObject* img, unsigned int numBands, double* bbox)
		{
			if (numBands == 1)
			{
				ImgType* theimg = dynamic_cast<ImgType*>(img);
				RegionType reg = theimg->GetBufferedRegion();

				//ImgType::PointType ori = theimg->GetOrigin()

				bbox[0] = theimg->GetLowerLeftCorner()[0];
				bbox[2] = theimg->GetLowerLeftCorner()[1];
				bbox[1] = theimg->GetUpperRightCorner()[0];
				bbox[3] = theimg->GetUpperRightCorner()[1];
				bbox[5] = 0;
				bbox[6] = 0;

				if (theimg->GetImageDimension() == 3)
				{
					bbox[5] = theimg->GetOrigin()[2];
					bbox[6] = bbox[5] + theimg->GetSpacing()[2] * reg.GetSize()[2];
				}
			}
			else if (numBands > 1)
			{
				VecImgType* theimg = (VecImgType*)img;
				VecRegionType reg = theimg->GetBufferedRegion();

				bbox[0] = theimg->GetLowerLeftCorner()[0];
				bbox[2] = theimg->GetLowerLeftCorner()[1];
				bbox[1] = theimg->GetUpperRightCorner()[0];
				bbox[3] = theimg->GetUpperRightCorner()[1];
				bbox[5] = 0;
				bbox[6] = 0;

				if (theimg->GetImageDimension() == 3)
				{
					bbox[5] = theimg->GetOrigin()[2];
					bbox[6] = bbox[5] + theimg->GetSpacing()[2] * reg.GetSize()[2];
				}
			}
		}
};

/** macro for querying the bbox */
#define getInternalBBox( PixelType ) \
{	\
	if (numDims == 2) \
		InternalImageHelper<PixelType, 2>::getBBox(img, numBands, this->mBBox); \
	else \
		InternalImageHelper<PixelType, 3>::getBBox(img, numBands, this->mBBox); \
}

NMImageLayer::NMImageLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
	: NMLayer(renWin, renderer, parent)
{
	this->mLayerType = NMLayer::NM_IMAGE_LAYER;
	this->mReader = new NMImageReader(this);
	this->mPipeconn = new NMItk2VtkConnector(this);
	this->mFileName = "";

	this->mNumBands = 0;
	this->mNumDimensions = 0;
	this->mComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mbStatsAvailable = false;

	this->mLayerIcon = QIcon(":image_layer.png");

	vtkInteractorObserver* style = mRenderWindow->GetInteractor()->GetInteractorStyle();
	this->mVtkConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();

	this->mVtkConn->Connect(style, vtkCommand::ResetWindowLevelEvent,
			this, SLOT(windowLevelReset(vtkObject*)));
	this->mVtkConn->Connect(style, vtkCommand::InteractionEvent,
			this, SLOT(windowLevelChanged(vtkObject*)));

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
	if (!this->mbStatsAvailable)
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

void
NMImageLayer::updateStats(void)
{
	emit layerProcessingStart();

	vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
	cast->SetOutputScalarTypeToFloat();
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

void NMImageLayer::world2pixel(double world[3], int pixel[3])
{
	vtkImageData* img = vtkImageData::SafeDownCast(
			const_cast<vtkDataSet*>(this->getDataSet()));

	if (img == 0)
		return;

	// we tweak the world coordinates a little bit to
	// generate the illusion as if we had pixel-centered
	// coordinates in vtk as well
	unsigned int d=0;
	double spacing[3];
	double origin[3];
	int dims[3];
	double bnd[6];
	double err[3];
	img->GetSpacing(spacing);
	img->GetOrigin(origin);
	img->GetDimensions(dims);

	// adjust coordinates & calc bnd
	for (d=0; d<3; ++d)
	{
		// account for 'pixel corner coordinate' vs.
		// 'pixel centre coordinate' philosophy of
		// vtk vs itk
		world[d] += spacing[d] / 2.0;

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

	// check, whether the user point is within the
	// image boundary
	if (vtkMath::PointIsWithinBounds(world, bnd, err))
	{
		// calculate the image/pixel coordinates
		for (d = 0; d < 3; ++d)
		{
			// init value
			pixel[d] = 0;

			if (dims[d] > 1)
				pixel[d] = ::abs((int)((world[d] - origin[d]) / spacing[d]));

			if (pixel[d] > dims[d]-1)
				pixel[d] = 0;
		}
	}
}

void
NMImageLayer::selectionChanged(const QItemSelection& newSel,
		const QItemSelection& oldSel)
{
	// create new selections
	mSelectionMapper = vtkSmartPointer<vtkOGRLayerMapper>::New();

	int selcnt = 0;
	foreach(const QItemSelectionRange& range, newSel)
	{
		const int top = range.top();
		const int bottom = range.bottom();
		for (int row=top; row<=bottom; ++row)
		{
			++selcnt;
		}
	}
	this->printSelRanges(newSel, "Image Selection");

	vtkSmartPointer<vtkGenericContourFilter> extractor = vtkSmartPointer<vtkGenericContourFilter>::New();
	extractor->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

	int clrcnt = 0;
	foreach(const QItemSelectionRange& range, newSel)
	{
		const int top = range.top();
		const int bottom = range.bottom();
		for (int row=top; row<=bottom; ++row)
		{
			extractor->SetValue(clrcnt, row);
			++clrcnt;
		}
	}
	extractor->Update();

	if (extractor->GetOutput() == 0)
	{
		NMDebugAI(<< "IMAGE SELECTION: mmh, something wrong ... " << std::endl);
		return;
	}
	else
	{
		NMDebugAI(<< "IMAGE SELECTION: promising!" << std::endl);
	}

	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->SetHueRange(0.667, 0.0);

	if (this->mCellSelection.GetPointer() != 0 && mSelectionActor.GetPointer() != 0)
	{
		NMDebugAI(<< "removed old selection" << std::endl);
		this->mRenderer->RemoveActor(mSelectionActor);
	}

	mSelectionMapper = vtkSmartPointer<vtkOGRLayerMapper>::New();
	mSelectionMapper->SetLookupTable(clrtab);
	mSelectionMapper->SetInputConnection(extractor->GetOutputPort());

	if(extractor->GetOutput()->GetPointData() != 0)
	{
		if (extractor->GetOutput()->GetPointData()->GetScalars() != 0)
		{
			mSelectionMapper->SetScalarRange(
					extractor->GetOutput()->GetPointData()->GetScalars()->GetRange());
		}
	}

	vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
	a->SetMapper(mSelectionMapper);

	mSelectionActor = a;
	mRenderer->AddActor(a);

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

	connect(mTableView, SIGNAL(notifyLastClickedRow(long)), this, SLOT(forwardLastClickedRowSignal(long)));

	// connect layer signals to tableview slots and model components list
	//this->connect(this, SIGNAL(attributeTableChanged(vtkTable*)),
	//		this->mTableView, SLOT(setTable(vtkTable*)));

	//TODO: connect to signal/slots for table updates

}

int NMImageLayer::updateAttributeTable()
{
	if (this->mTableModel != 0)
		return 1;

	disconnectTableSel();

	this->mOtbRAT = this->getRasterAttributeTable(1);
	NMQtOtbAttributeTableModel* otbModel;
	if (this->mTableModel == 0)
	{
		otbModel = new NMQtOtbAttributeTableModel(this->mOtbRAT, this);
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

bool NMImageLayer::setFileName(QString filename)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	if (filename.isEmpty() || filename.isNull())
	{
		NMErr(ctxNMImageLayer, << "invalid filename!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
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

	// let's store some meta data, in case someone needs it
	this->mComponentType = this->mReader->getOutputComponentType();
	this->mNumBands = this->mReader->getOutputNumBands();
	this->mNumDimensions = this->mReader->getOutputNumDimensions();

	if (this->mNumBands != 1)
	{
		emit layerProcessingEnd();
		NMErr(ctxNMImageLayer, << "we currently only support single band images!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return false;
	}

	// concatenate the pipeline
	this->mPipeconn->setInput(this->mReader->getOutput(0));

	vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
	m->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());
	m->SetBorder(1);

	// adjust origin
	double ori[3], spc[3];
	vtkImageData* img = vtkImageData::SafeDownCast(this->mPipeconn->getVtkImage());
	img->GetOrigin(ori);
	img->GetSpacing(spc);

	vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
	a->SetMapper(m);

	mImgProp = vtkSmartPointer<vtkImageProperty>::New();
	mImgProp->SetInterpolationTypeToNearest();
	a->SetProperty(mImgProp);

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
	this->mActor = a;

	// set the bounding box
	this->mReader->getBBox(this->mBBox);

	// we're going to update the attribute table, since
	// we'll probably need it
	if (!this->updateAttributeTable())
	{
		NMWarn(ctxNMImageLayer, << "Couldn't update the attibute table, "
				<< "which might lead to trouble later on!");
	}

	this->mImage = 0;

	emit layerProcessingEnd();
	emit layerLoaded();
	NMDebugCtx(ctxNMImageLayer, << "done!");
	return true;
}

void NMImageLayer::setImage(NMItkDataObjectWrapper* imgWrapper)
{
	if (imgWrapper == 0)
		return;

	this->mImage = imgWrapper->getDataObject();
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
	NMDebugCtx(ctxNMImageLayer, << "...");

	bbox[0] = this->mBBox[0];
	bbox[1] = this->mBBox[1];
	bbox[2] = this->mBBox[2];
	bbox[3] = this->mBBox[3];
	bbox[4] = this->mBBox[4];
	bbox[5] = this->mBBox[5];

	NMDebugCtx(ctxNMImageLayer, << "done!");
}

const vtkDataSet* NMImageLayer::getDataSet()
{
	return this->mPipeconn->getVtkImage();
}

otb::AttributeTable::Pointer
NMImageLayer::getRasterAttributeTable(int band)
{
	if (band < 1 || band > this->mReader->getOutputNumBands())
		return 0;

	return this->mReader->getRasterAttributeTable(band);
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
	// call parent first, to deal with the
	// layer's state recording
	NMLayer::writeDataSet();

//	if (!this->isRasLayer())
//	{
//		// we do something here for file-based layers
//	}
//
//
//#ifdef BUILD_RASSUPPORT
//	otb::RasdamanImageIO::Pointer rio = otb::RasdamanImageIO::New();
//	std::vector<double> oids = rio->getOIDs();
//
//	rio->writeRAT(this->mOtbRAT, 1, oids[0]);
//#endif
}

void
NMImageLayer::windowLevelReset(vtkObject* obj)
{
	double window = this->mImgProp->GetColorWindow();
	double level = this->mImgProp->GetColorLevel();

	//NMDebugAI(<< "window: " << window << " level: " << level << std::endl);

}

void
NMImageLayer::windowLevelChanged(vtkObject* obj)
{
	double window = this->mImgProp->GetColorWindow();
	double level = this->mImgProp->GetColorLevel();

	//NMDebugAI(<< "window: " << window << " level: " << level << std::endl);
}


int NMImageLayer::mapUniqueValues(QString fieldName)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	//if (this->mRATVec.size() == 0)
	if (mOtbRAT.IsNull())
	{
		if (!this->updateAttributeTable())
		{
			NMDebugAI(<< "trouble getting an attribute table!");
			return 0;
		}
	}

	// make a list of available attributes
	//otb::AttributeTable::Pointer tab = this->mRATVec.at(0);
	int idxField = mOtbRAT->ColumnExists(fieldName.toStdString());
	if (idxField < 0)
	{
		NMErr(ctxNMImageLayer, << "the specified attribute does not exist!");
		return 0;
	}

	// let's find out about the attribute
	// if we've got doubles, we refuse to map unique values ->
	// doesn't make sense, does it?
	bool bNum = true;
	if (mOtbRAT->GetColumnType(idxField) == otb::AttributeTable::ATTYPE_STRING)
	{
		bNum = false;
	}
	else if (mOtbRAT->GetColumnType(idxField) != otb::AttributeTable::ATTYPE_INT)
	{
		NMDebugAI( << "oh no, not with doubles!" << endl);
		return 0;
	}

	// let's get the statistics if not done already
	if (!this->mbStatsAvailable)
	{
		this->updateStats();
	}
	double min = this->mImgStats[0];
	double max = this->mImgStats[1];
	bool bMinIncluded = false;
	bool bMaxIncluded = false;

	// we create a new look-up table and set the number of entries we need
	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->Allocate(mOtbRAT->GetNumRows()+1);
	clrtab->SetNumberOfTableValues(mOtbRAT->GetNumRows()+1);

	// let's create a new legend info table
	this->resetLegendInfo();


	// we iterate over the number of tuples in the user specified attribute array
	// and assign each unique categorical value its own (hopefully unique)
	// random colour, which is then inserted into the layer's lookup table; we further
	// specify a default name for each colour and put it together with the
	// chosen colour into a LengendInfo-Table, which basically holds the legend
	// category to display; for linking attribute values to table-info and lookup-table
	// indices, we fill the HashMap mHashValueIndices (s. Header file for further descr.)
	bool bConvOk;
	int clrCount = 0, val;
	std::string fn = fieldName.toStdString();
	QString sVal;
	vtkMath::RandomSeed(QTime::currentTime().msec());
	for (int t=0; t < mOtbRAT->GetNumRows(); ++t)
	{
		int pixval = mOtbRAT->GetIntValue("rowidx", t);
		if (pixval == min)
		{
			bMinIncluded = true;
		}
		else if (pixval == max)
		{
			bMaxIncluded = true;
		}

		if (bNum)
		{
			int val = mOtbRAT->GetIntValue(fn, t);
			sVal = QString(tr("%1")).arg(val);
		}
		else
		{
			sVal = QString(mOtbRAT->GetStrValue(fn, t).c_str());
		}

		QHash<QString, QVector<int> >::iterator it = this->mHashValueIndices.find(sVal);
		if (it == this->mHashValueIndices.end())
		{
			// add the key value pair to the hash map
			QVector<int> idxVec;
			idxVec.append(clrCount);
			this->mHashValueIndices.insert(sVal, idxVec);

			// add a new row to the legend_info table
			vtkIdType newidx = this->mLegendInfo->InsertNextBlankRow(-9);

			// add the value to the index map
			double lowup[2];
			lowup[0] = val;
			lowup[1] = val;

			vtkDoubleArray* lowupAbstrAr = vtkDoubleArray::SafeDownCast(
					this->mLegendInfo->GetColumnByName("range"));
			lowupAbstrAr->SetTuple(newidx, lowup);

			// generate a random triple of uchar values
			double rgba[4];
			for (int i=0; i < 3; i++)
				rgba[i] = vtkMath::Random();
			rgba[3] = 1;

			// add the color spec to the colour map
			vtkDoubleArray* rgbaAr = vtkDoubleArray::SafeDownCast(
					this->mLegendInfo->GetColumnByName("rgba"));
			rgbaAr->SetTuple(newidx, rgba);

			// add the name (sVal) to the name column of the legendinfo table
			vtkStringArray* nameAr = vtkStringArray::SafeDownCast(
					this->mLegendInfo->GetColumnByName("name"));
			nameAr->SetValue(newidx, sVal.toStdString().c_str());

			// add the color spec to the mapper's color table
			clrtab->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
			//			NMDebugAI( << clrCount << ": " << sVal.toStdString() << " = " << rgba[0]
			//					<< " " << rgba[1] << " " << rgba[2] << endl);

			clrCount++;
		}
		else
		{
			// add the index to the index map
			int tabInfoIdx = this->mHashValueIndices.find(sVal).value()[0];
			this->mHashValueIndices.find(sVal).value().append(t);

			// add the colour to the real color table
			vtkDoubleArray* dblAr = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("rgba"));
			double tmprgba[4];
			dblAr->GetTuple(tabInfoIdx, tmprgba);

			clrtab->SetTableValue(t, tmprgba[0], tmprgba[1], tmprgba[2]);
		}
	}

	if (!bMinIncluded && !bMaxIncluded)
	{
		// the nodata value
		double val = bMinIncluded ? max : (bMaxIncluded ? val : max);

		// add the key value pair to the hash map
		QVector<int> idxVec;
		idxVec.append(clrCount);
		this->mHashValueIndices.insert("nodata", idxVec);

		// add a new row to the legend_info table
		vtkIdType newidx = this->mLegendInfo->InsertNextBlankRow(-9);

		// add the value to the index map
		double lowup[2];
		lowup[0] = val;
		lowup[1] = val;

		vtkDoubleArray* lowupAbstrAr = vtkDoubleArray::SafeDownCast(
				this->mLegendInfo->GetColumnByName("range"));
		lowupAbstrAr->SetTuple(newidx, lowup);

		// generate a random triple of uchar values
		double rgba[4];
		for (int i=0; i < 3; i++)
			rgba[i] = 0;
		rgba[3] = 0;

		// add the color spec to the colour map
		vtkDoubleArray* rgbaAr = vtkDoubleArray::SafeDownCast(
				this->mLegendInfo->GetColumnByName("rgba"));
		rgbaAr->SetTuple(newidx, rgba);

		// add the name (sVal) to the name column of the legendinfo table
		vtkStringArray* nameAr = vtkStringArray::SafeDownCast(
				this->mLegendInfo->GetColumnByName("name"));
		nameAr->SetValue(newidx, sVal.toStdString().c_str());

		// add the color spec to the mapper's color table
		clrtab->SetTableValue(mOtbRAT->GetNumRows(), rgba[0], rgba[1], rgba[2], rgba[3]);
		//			NMDebugAI( << clrCount << ": " << sVal.toStdString() << " = " << rgba[0]
		//					<< " " << rgba[1] << " " << rgba[2] << endl);

	}

	this->mImgProp->SetLookupTable(clrtab);
	this->mImgProp->UseLookupTableScalarRangeOn();

	emit visibilityChanged(this);
	emit legendChanged(this);

	NMDebugCtx(ctxNMImageLayer, << "done!");
	return 1;
}
