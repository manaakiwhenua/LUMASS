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

#include <NMImageLayer.h>

#include "itkImageRegion.h"
#include "itkPoint.h"

#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"

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
	this->mComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
}

NMImageLayer::~NMImageLayer()
{

}

void NMImageLayer::createTableView(void)
{
	if (this->mRATVec.size() == 0)
		return;

	if (this->mTableView != 0)
	{
		delete this->mTableView;
	}

	this->updateAttributeTable();

	// for debug only
	this->mAttributeTable->Print(std::cout);

	this->mTableView = new NMTableView(this->mAttributeTable, 0);
	this->mTableView->hideAttribute("nm_sel");
	this->mTableView->setRowKeyColumn("nm_id");
	this->mTableView->hideAttribute("nm_id");
	this->mTableView->setTitle(tr("Attributes of ") + this->objectName());

	// connect layer signals to tableview slots and model components list
	this->connect(this, SIGNAL(attributeTableChanged(vtkTable*)),
			this->mTableView, SLOT(setTable(vtkTable*)));

	//TODO: connect to signal/slots for table updates

}

void NMImageLayer::updateAttributeTable()
{
	// for now we're operating only on the table of the first band
	otb::AttributeTable* otbtab = this->mRATVec.at(0);
	unsigned int nrows = otbtab->GetNumRows();
	unsigned int ncols = otbtab->GetNumCols();

	vtkSmartPointer<vtkTable> tab = vtkSmartPointer<vtkTable>::New();
	tab->SetNumberOfRows(nrows);
//	tab->Initialize();

	// add the nm_id admin field
	vtkSmartPointer<vtkLongArray> id = vtkSmartPointer<vtkLongArray>::New();
	id->SetNumberOfComponents(1);
	id->SetNumberOfTuples(nrows);
	id->SetName("nm_id");
	id->FillComponent(0,0);
	tab->AddColumn(id);

	// add the nm_sel admin field
	vtkSmartPointer<vtkUnsignedCharArray> sel =
			vtkSmartPointer<vtkUnsignedCharArray>::New();
	sel->SetNumberOfComponents(1);
	sel->SetNumberOfTuples(nrows);
	sel->SetName("nm_sel");
	sel->FillComponent(0,0);
	tab->AddColumn(sel);

	// copy table structure
	for (unsigned int col = 0; col < ncols; ++col)
	{
		vtkSmartPointer<vtkStringArray> sar;
		vtkSmartPointer<vtkDoubleArray> dar;
		vtkSmartPointer<vtkIntArray> iar;
		switch(otbtab->GetColumnType(col))
		{
		case otb::AttributeTable::ATTYPE_STRING:
			sar = vtkSmartPointer<vtkStringArray>::New();
			sar->SetNumberOfComponents(1);
			sar->SetNumberOfTuples(nrows);
			sar->SetName(otbtab->GetColumnName(col).c_str());
			tab->AddColumn(sar);
			break;
		case otb::AttributeTable::ATTYPE_DOUBLE:
			dar = vtkSmartPointer<vtkDoubleArray>::New();
			dar->SetNumberOfComponents(1);
			dar->SetNumberOfTuples(nrows);
			dar->SetName(otbtab->GetColumnName(col).c_str());
			dar->FillComponent(0,0);
			tab->AddColumn(dar);
			break;
		case otb::AttributeTable::ATTYPE_INT:
			iar = vtkSmartPointer<vtkIntArray>::New();
			iar->SetNumberOfComponents(1);
			iar->SetNumberOfTuples(nrows);
			iar->SetName(otbtab->GetColumnName(col).c_str());
			iar->FillComponent(0,0);
			tab->AddColumn(iar);
			break;
		}
	}

	// copy table content
	for (unsigned int row = 0; row < nrows; ++row)
	{
//		tab->InsertNextBlankRow(0);
		vtkLongArray* nmid = vtkLongArray::SafeDownCast(
				tab->GetColumnByName("nm_id"));
		nmid->SetValue(row, row);

		vtkUnsignedCharArray* nmsel = vtkUnsignedCharArray::SafeDownCast(
				tab->GetColumnByName("nm_sel"));
		nmsel->SetValue(row, 0);

		for (unsigned int col = 2; col < ncols+2; ++col)
		{
			vtkStringArray* sa;
			vtkDoubleArray* da;
			vtkIntArray* ia;
			switch(otbtab->GetColumnType(col-2))
			{
			case otb::AttributeTable::ATTYPE_STRING:
				sa = vtkStringArray::SafeDownCast(
						tab->GetColumn(col));
				sa->SetValue(row, otbtab->GetStrValue(col-2, row).c_str());
//				ar->InsertVariantValue(row,
//						vtkVariant(otbtab->GetStrValue(col, row).c_str()));
				break;
			case otb::AttributeTable::ATTYPE_DOUBLE:
				da = vtkDoubleArray::SafeDownCast(
						tab->GetColumn(col));
				da->SetValue(row, otbtab->GetDblValue(col-2, row));
//				ar->InsertVariantValue(row,
//						vtkVariant(otbtab->GetDblValue(col, row)));
				break;
			case otb::AttributeTable::ATTYPE_INT:
				ia = vtkIntArray::SafeDownCast(
						tab->GetColumn(col));
				ia->SetValue(row, otbtab->GetIntValue(col-2, row));
//				ar->InsertVariantValue(row,
//						vtkVariant(otbtab->GetIntValue(col, row)));
				break;
			}
		}
	}

	this->mAttributeTable = tab;

	// notify table view class about changes
	emit attributeTableChanged(this->mAttributeTable);
	emit legendChanged(this);

}

void NMImageLayer::setFileName(QString filename)
{
	NMDebugCtx(ctxNMImageLayer, << "...");

	if (filename.isEmpty() || filename.isNull())
	{
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return;
	}

	// TODO: find a more unambiguous way of determining
	// whether we're loading a rasdaman image or not
#ifdef BUILD_RASSUPPORT
	if (!filename.contains(".") && this->mpRasconn != 0)
		this->mReader->setRasdamanConnector(this->mpRasconn);
#endif

	this->mReader->setFileName(filename);
	this->mReader->instantiateObject();
	if (!this->mReader->isInitialised())
	{
		NMErr(ctxNMImageLayer, << "couldn't read the image!");
		NMDebugCtx(ctxNMImageLayer, << "done!");
		return;
	}

	// let's store some meta data, in case someone needs it
	this->mComponentType = this->mReader->getOutputComponentType();
	this->mNumBands = this->mReader->getOutputNumBands();
	this->mNumDimensions = this->mReader->getOutputNumDimensions();

	// concatenate the pipeline
	this->mPipeconn->setInput(this->mReader->getOutput());

	vtkSmartPointer<vtkImageResliceMapper> m = vtkSmartPointer<vtkImageResliceMapper>::New();
	m->SetInputConnection(this->mPipeconn->getVtkAlgorithmOutput());

	vtkSmartPointer<vtkImageSlice> a = vtkSmartPointer<vtkImageSlice>::New();
	a->SetMapper(m);

	// experimental
	vtkSmartPointer<vtkImageProperty> ip =
			vtkSmartPointer<vtkImageProperty>::New();
//	ip->SetColorWindow(2000);
//	ip->SetColorLevel(1000);
//	ip->SetAmbient(0.0);
//	ip->SetDiffuse(1.0);
//	ip->SetOpacity(1.0);
//	ip->SetInterpolationTypeToLinear();

	vtkSmartPointer<vtkLookupTable> lt = vtkSmartPointer<vtkLookupTable>::New();
	lt->SetRampToSQRT();
	lt->SetRange(0, 255);
	lt->SetNumberOfTableValues(42);
	lt->Build();
	ip->SetLookupTable(lt);

	a->SetProperty(ip);

	// experimental end

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
	this->mActor = a;

	// set the bounding box
	this->mReader->getBBox(this->mBBox);

	// fetch RATs
	this->fetchRATs();

	this->mImage = 0;

	NMDebugCtx(ctxNMImageLayer, << "done!");
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

	this->mRenderer->AddViewProp(a);

	this->mMapper = m;
	this->mActor = a;
}

//void NMImageLayer::setITKImage(itk::DataObject* img,
//		itk::ImageIOBase::IOComponentType pixType,
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
////	case itk::ImageIOBase::UCHAR:
////		getInternalBBox( unsigned char );
////		break;
////	case itk::ImageIOBase::CHAR:
////		getInternalBBox( char );
////		break;
////	case itk::ImageIOBase::USHORT:
////		getInternalBBox( unsigned short );
////		break;
////	case itk::ImageIOBase::SHORT:
////		getInternalBBox( short );
////		break;
////	case itk::ImageIOBase::UINT:
////		getInternalBBox( unsigned int );
////		break;
////	case itk::ImageIOBase::INT:
////		getInternalBBox( int );
////		break;
////	case itk::ImageIOBase::ULONG:
////		getInternalBBox( unsigned long );
////		break;
////	case itk::ImageIOBase::LONG:
////		getInternalBBox( long );
////		break;
////	case itk::ImageIOBase::FLOAT:
////		getInternalBBox( float );
////		break;
////	case itk::ImageIOBase::DOUBLE:
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
	if (band < 1 || this->mRATVec.size() < band)
		return 0;

	return this->mRATVec.at(band-1);
}

void NMImageLayer::fetchRATs(void)
{
	// fetch attribute tables, if applicable
	this->mRATVec.resize(this->mReader->getOutputNumBands());
	for (int b=0; b < this->mReader->getOutputNumBands(); ++b)
		this->mRATVec[b] = this->mReader->getRasterAttributeTable(b+1);
}

NMItkDataObjectWrapper* NMImageLayer::getOutput(void)
{
	return this->getImage();
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

itk::ImageIOBase::IOComponentType NMImageLayer::getITKComponentType(void)
{
	return this->mComponentType;
}

void NMImageLayer::setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg)
{
	this->setImage(inputImg);
}


