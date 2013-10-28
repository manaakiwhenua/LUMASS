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


#include <QTime>

#include "itkImageRegion.h"
#include "itkPoint.h"

#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"

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

	this->mLayerIcon = QIcon(":image_layer.png");
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

void NMImageLayer::createTableView(void)
{
	//if (this->mRATVec.size() < 1)
	//	return;

	if (this->mTableView != 0)
	{
		delete this->mTableView;
	}

	if (!this->updateAttributeTable())
		return;


	// for debug only
	//this->mAttributeTable->Print(std::cout);


	this->mTableView = new NMTableView(this->mOtbRAT, 0);//this->mAttributeTable, 0);
	this->mTableView->hideAttribute("nm_sel");
	//this->mTableView->setRowKeyColumn("nm_id");
	this->mTableView->setRowKeyColumn("rowidx");
	this->mTableView->hideAttribute("rowidx");
	this->mTableView->setTitle(tr("Attributes of ") + this->objectName());

	// connect layer signals to tableview slots and model components list
	//this->connect(this, SIGNAL(attributeTableChanged(vtkTable*)),
	//		this->mTableView, SLOT(setTable(vtkTable*)));

	//TODO: connect to signal/slots for table updates

}

int NMImageLayer::updateAttributeTable()
{
	// for now we're operating only on the table of the first band
	//if (this->mRATVec.size() < 1)
	//	return 0;
	//otb::AttributeTable::Pointer otbtab = this->mRATVec.at(0);
	//otb::AttributeTable::Pointer otbtab = this->mReader->getRasterAttributeTable(1);
	//if (otbtab.IsNull())
	//	return 0;
    //
	//unsigned int nrows = otbtab->GetNumRows();
	//unsigned int ncols = otbtab->GetNumCols();
    //
	//vtkSmartPointer<vtkTable> tab = vtkSmartPointer<vtkTable>::New();
	//tab->SetNumberOfRows(nrows);
    //
	//// add the nm_id admin field
	//vtkSmartPointer<vtkLongArray> id = vtkSmartPointer<vtkLongArray>::New();
	//id->SetNumberOfComponents(1);
	//id->SetNumberOfTuples(nrows);
	//id->SetName("nm_id");
	//id->FillComponent(0,0);
	//tab->AddColumn(id);
    //
	//// add the nm_sel admin field
	//vtkSmartPointer<vtkUnsignedCharArray> sel =
	//		vtkSmartPointer<vtkUnsignedCharArray>::New();
	//sel->SetNumberOfComponents(1);
	//sel->SetNumberOfTuples(nrows);
	//sel->SetName("nm_sel");
	//sel->FillComponent(0,0);
	//tab->AddColumn(sel);
    //
	//// copy table structure
	//for (unsigned int col = 0; col < ncols; ++col)
	//{
	//	vtkSmartPointer<vtkStringArray> sar;
	//	vtkSmartPointer<vtkDoubleArray> dar;
	//	vtkSmartPointer<vtkIntArray> iar;
	//	switch(otbtab->GetColumnType(col))
	//	{
	//	case otb::AttributeTable::ATTYPE_STRING:
	//		sar = vtkSmartPointer<vtkStringArray>::New();
	//		sar->SetNumberOfComponents(1);
	//		sar->SetNumberOfTuples(nrows);
	//		sar->SetName(otbtab->GetColumnName(col).c_str());
	//		tab->AddColumn(sar);
	//		break;
	//	case otb::AttributeTable::ATTYPE_DOUBLE:
	//		dar = vtkSmartPointer<vtkDoubleArray>::New();
	//		dar->SetNumberOfComponents(1);
	//		dar->SetNumberOfTuples(nrows);
	//		dar->SetName(otbtab->GetColumnName(col).c_str());
	//		dar->FillComponent(0,0);
	//		tab->AddColumn(dar);
	//		break;
	//	case otb::AttributeTable::ATTYPE_INT:
	//		iar = vtkSmartPointer<vtkIntArray>::New();
	//		iar->SetNumberOfComponents(1);
	//		iar->SetNumberOfTuples(nrows);
	//		iar->SetName(otbtab->GetColumnName(col).c_str());
	//		iar->FillComponent(0,0);
	//		tab->AddColumn(iar);
	//		break;
	//	}
	//}
    //
	//// copy table content
	//for (unsigned int row = 0; row < nrows; ++row)
	//{
//	//	tab->InsertNextBlankRow(0);
	//	vtkLongArray* nmid = vtkLongArray::SafeDownCast(
	//			tab->GetColumnByName("nm_id"));
	//	nmid->SetValue(row, row);
    //
	//	vtkUnsignedCharArray* nmsel = vtkUnsignedCharArray::SafeDownCast(
	//			tab->GetColumnByName("nm_sel"));
	//	nmsel->SetValue(row, 0);
    //
	//	for (unsigned int col = 2; col < ncols+2; ++col)
	//	{
	//		vtkStringArray* sa;
	//		vtkDoubleArray* da;
	//		vtkIntArray* ia;
	//		switch(otbtab->GetColumnType(col-2))
	//		{
	//		case otb::AttributeTable::ATTYPE_STRING:
	//			sa = vtkStringArray::SafeDownCast(
	//					tab->GetColumn(col));
	//			sa->SetValue(row, otbtab->GetStrValue(col-2, row).c_str());
//	//			ar->InsertVariantValue(row,
//	//					vtkVariant(otbtab->GetStrValue(col, row).c_str()));
	//			break;
	//		case otb::AttributeTable::ATTYPE_DOUBLE:
	//			da = vtkDoubleArray::SafeDownCast(
	//					tab->GetColumn(col));
	//			da->SetValue(row, otbtab->GetDblValue(col-2, row));
//	//			ar->InsertVariantValue(row,
//	//					vtkVariant(otbtab->GetDblValue(col, row)));
	//			break;
	//		case otb::AttributeTable::ATTYPE_INT:
	//			ia = vtkIntArray::SafeDownCast(
	//					tab->GetColumn(col));
	//			ia->SetValue(row, otbtab->GetIntValue(col-2, row));
//	//			ar->InsertVariantValue(row,
//	//					vtkVariant(otbtab->GetIntValue(col, row)));
	//			break;
	//		}
	//	}
	//}
    //
	//this->mAttributeTable = tab;

	// notify table view class about changes
	//emit attributeTableChanged(this->mOtbRAT);//this->mAttributeTable);

	this->mOtbRAT = this->getRasterAttributeTable(1);

	// we add the "nm_sel" column for home grown selection handling
	int selid = this->mOtbRAT->ColumnExists("nm_sel");
	if (selid < 0)
	{
		this->mOtbRAT->AddColumn("nm_sel", otb::AttributeTable::ATTYPE_INT);
		selid = this->mOtbRAT->ColumnExists("nm_sel");

		for (int r=0; r < this->mOtbRAT->GetNumRows(); ++r)
		{
			this->mOtbRAT->SetValue(selid, r, (long)0);
		}
	}

	emit legendChanged(this);

	return 1;
}

void
NMImageLayer::updateDataSet(QStringList& slAlteredColumns,
		QStringList& slDeletedColumns)
{
	/* for image layers this means we write back any changes
	 * made to table by the associated tableview (vtktable)
	 *
	 *
	 */

	NMDebugCtx(ctxNMImageLayer, << "...");

	if (slDeletedColumns.size() == 0 && slAlteredColumns.size() == 0)
	{
		NMDebugAI(<< "nothing to save!" << endl);
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return;
	}





	// vtkDataArray::ExportToVoidPointer(void* out_ptr);



	NMDebugCtx(ctxNMImageLayer, << "done!");
}

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

	this->mReader->setFileName(filename);
	this->mReader->instantiateObject();
	if (!this->mReader->isInitialised())
	{
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

	// we only fetch a RAT when we really need it
	//this->fetchRATs();

	this->mImage = 0;

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

otb::AttributeTable* NMImageLayer::getRasterAttributeTable(int band)
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
	if (!this->isRasLayer())
	{
		// we do something here for file-based layers
	}


#ifdef BUILD_RASSUPPORT
	otb::RasdamanImageIO::Pointer rio = otb::RasdamanImageIO::New();
	std::vector<double> oids = rio->getOIDs();

	rio->writeRAT(this->mOtbRAT, 1, oids[0]);
#endif
}

int NMImageLayer::mapUniqueValues(QString fieldName)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	//if (this->mRATVec.size() == 0)
	if (mOtbRAT.IsNull())
	{
		NMDebugAI(<< "sorry, don't have an attribute table!");
		return 0;
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

	// we create a new look-up table and set the number of entries we need
	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->Allocate(mOtbRAT->GetNumRows());
	clrtab->SetNumberOfTableValues(mOtbRAT->GetNumRows());

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

	// get the mapper and look whats possible
//	vtkMapper* mapper = vtkMapper::SafeDownCast(this->mMapper);
	//mapper->SetScalarRange(0, clrtab->GetNumberOfColors());
//	mapper->SetLookupTable(clrtab);
//	mapper->UseLookupTableScalarRangeOn();

//	clrtab->SetNumberOfColors(clrCount);
//	clrtab->Build();

	this->mImgProp->SetLookupTable(clrtab);
//	this->mImgProp->UseLookupTableScalarRangeOn();


	emit visibilityChanged(this);
	emit legendChanged(this);

	NMDebugCtx(ctxNMImageLayer, << "done!");
	return 1;
}
