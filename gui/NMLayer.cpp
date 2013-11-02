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
 * NMLayer.cpp
 *
 *  Created on: 10/03/2011
 *      Author: alex
 */

#include <NMLayer.h>

#include <QTime>

#include "vtkMapper.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkDataSetAttributes.h"


NMLayer::NMLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
	: QObject(parent), mSelectionModel(0), mTableModel(0), mTableView(0),
	  mDataSet(0), mActor(0), mMapper(0),
	  mIsVisible(false), mIsSelectable(false), mHasChanged(false),
	  mLayerType(NM_UNKNOWN_LAYER)
{
	if (renWin == 0)
	{
		NMErr(ctxNMLayer, << "invalid render window specified!");
		return;
	}
	this->mRenderWindow = renWin;

	if (renderer == 0)
		this->mRenderer = vtkSmartPointer<vtkRenderer>::New();
	else
		this->mRenderer = renderer;

	//this->mRenderer->SetUseDepthPeeling(1);
	//this->mRenderer->SetMaximumNumberOfPeels(100);
	//this->mRenderer->SetOcclusionRatio(0.1);

	// make a legendinfo table for this layer
	this->resetLegendInfo();

	this->mFileName.clear();
	this->mLayerPos = this->mRenderer->GetLayer();

	// set initial bounding box between 0 and 1
	// for each dimension
	for (int i=0; i < 6; i += 2)
		this->mBBox[i] = 0;

	for (int i=1; i < 6; i += 2)
		this->mBBox[i] = 1;

	this->mTotalArea = -1;

	// connect the layer's own dataSetChanged signal to the layer's
	// own updateAttributeTable, to reflect changes to the data set
	// by other processing objects (e.g. NMMosra)
//	this->connect(this, SIGNAL(dataSetChanged(const NMLayer*)),
//			this, SLOT(updateAttributeTable()));
//	this->connect(this, SIGNAL(attributeTableChanged(const NMLayer*)),
//			this, SLOT(updateDataSet()));
}

NMLayer::~NMLayer()
{
	NMDebugCtx(ctxNMLayer, << " - " << this->objectName().toStdString());

	if (this->mRenderer != 0)
	{

		this->removeFromMap();
	}

	if (this->mTableView != 0)
	{
		this->mTableView->close();
		delete this->mTableView;
	}

	NMDebugCtx(ctxNMLayer, << "done!");
}

void NMLayer::emitDataSetChanged()
{
	NMDebugCtx(ctxNMLayer, << "...");

	this->updateAttributeTable();
	this->mHasChanged = true;
	emit dataSetChanged(this);

	NMDebugCtx(ctxNMLayer, << "done!");
}

void NMLayer::emitAttributeTableChanged(
		QStringList& slAlteredColumns,
		QStringList& slDeletedColumns)
{
	NMDebugCtx(ctxNMLayer, << "...");

	this->updateDataSet(slAlteredColumns, slDeletedColumns);
	this->updateAttributeTable();
	this->mHasChanged = true;
//	emit attributeTableChanged(this->mAttributeTable);

	NMDebugCtx(ctxNMLayer, << "done!");
}

void NMLayer::updateLayerSelection(QList<long> lstCellId,
		QList<long> lstNMId, NMLayerSelectionType seltype)
{

}

void NMLayer::resetLegendInfo(void)
{
	// clear the HashMap
	this->mHashValueIndices.clear();

	// create new one
	this->mLegendInfo = vtkSmartPointer<vtkTable>::New();

	vtkSmartPointer<vtkDoubleArray> rgba = vtkSmartPointer<vtkDoubleArray>::New();
	rgba->SetName("rgba");
	rgba->SetNumberOfComponents(4);
	mLegendInfo->AddColumn(rgba);

	vtkSmartPointer<vtkDoubleArray> range = vtkSmartPointer<vtkDoubleArray>::New();
	range->SetName("range");
	range->SetNumberOfComponents(2);
	mLegendInfo->AddColumn(range);

	vtkSmartPointer<vtkStringArray> name = vtkSmartPointer<vtkStringArray>::New();
	name->SetName("name");
	name->SetNumberOfComponents(1);
	mLegendInfo->AddColumn(name);

}

bool  NMLayer::getLegendColour(int legendRow, double* rgba)
{
//	NMDebugCtx(ctxNMLayer, << "...");

	// check whether row is valid or not
	if (legendRow < 0 || legendRow >= this->mLegendInfo->GetNumberOfRows())
		return false;

	vtkDoubleArray* clrs = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("rgba"));
	clrs->GetTuple(legendRow, rgba);

//	NMDebugCtx(ctxNMLayer, << "done!");
	return true;
}

int NMLayer::getLegendItemCount(void)
{
	if (this->mLegendInfo.GetPointer() == 0)
		return 0;
	return this->mLegendInfo->GetNumberOfRows();
}

QString  NMLayer::getLegendName(int legendRow)
{
	QString name;

	// check whether row is valid or not
	if (legendRow < 0 || legendRow >= this->mLegendInfo->GetNumberOfRows())
		return tr("");

	vtkStringArray* names = vtkStringArray::SafeDownCast(this->mLegendInfo->GetColumnByName("name"));
	name = names->GetValue(legendRow).c_str();

	return name;

}


double NMLayer::getArea(void)
{
	if (this->mTotalArea != -1)
		return this->mTotalArea;

	if (this->mDataSet.GetPointer() == 0)
	{
		this->mTotalArea = -1;
		return this->mTotalArea;
	}

	this->mTotalArea = (this->mBBox[1] - this->mBBox[0]) *
					   (this->mBBox[3] - this->mBBox[2]);

	return this->mTotalArea;
}

int NMLayer::setLayerPos(int pos)
{
	if (pos < 0)
		return -1;

	int oldpos = this->mRenderer->GetLayer()-1;
	// account for the background renderer -> so we add one
	// to the desired layer position
	this->mRenderer->SetLayer(pos+1);

	return oldpos;
}

int NMLayer::getLayerPos()
{
	// we account for the presence of the background renderer, so
	// substract that one from the actual layer position
	return this->mRenderer->GetLayer()-1;
}

void NMLayer::removeFromMap(void)
{
	if (this->mActor != 0)
	{
		this->mActor->SetVisibility(0);
		if (this->mRenderer != 0)
		{
			this->mRenderer->RemoveActor(this->mActor);
			if (this->mRenderWindow != 0)
				this->mRenderWindow->RemoveRenderer(this->mRenderer);
		}
	}
}

void NMLayer::setDataSet(vtkDataSet* dataset)
{
	if (!dataset)
	{
		NMErr(ctxNMLayer, << "dataset is NULL!");
		return;
	}

	this->mDataSet = dataset;
	this->mDataSet->GetBounds(this->mBBox);
	this->mTotalArea = -1;
}

const vtkRenderer* NMLayer::getRenderer(void)
{
	return this->mRenderer;
}

//const vtkDataSet* NMLayer::getDataSet(void)
//{
//	return this->mDataSet;
//}

const vtkProp3D* NMLayer::getActor(void)
{
	return this->mActor;
}

const vtkAbstractMapper* NMLayer::getMapper(void)
{
	return this->mMapper;
}

NMLayer::NMLayerType NMLayer::getLayerType(void)
{
	return this->mLayerType;
}

void NMLayer::setBBox(double bbox[6])
{
	if (bbox[0] > bbox[1] ||
		bbox[2] > bbox[3] ||
		bbox[4] > bbox[5])
	{
		NMErr(ctxNMLayer, <<
				"invalid bounding box!");
		return;
	}

	for (int i=0; i < 6; i++)
		this->mBBox[i] = bbox[i];
}

const double* NMLayer::getBBox(void)
{
	return this->mBBox;
}

void NMLayer::getBBox(double bbox[6])
{
	if (!bbox)
		return;

	for (int i=0; i < 6; i++)
		bbox[i] = this->mBBox[i];
}

void NMLayer::setVisible(bool visible)
{
	///if (this->mDataSet == 0)
	if (this->mRenderer.GetPointer() == 0)
		return;

	if (this->mIsVisible != visible)
	{
		if (this->mActor != 0)
			this->mActor->SetVisibility(visible);
		this->mIsVisible = visible;
		emit visibilityChanged(this);
	}
}

bool NMLayer::isVisible(void)
{
	return this->mIsVisible;
}

void NMLayer::setSelectable(bool selectable)
{
	if (this->mDataSet == 0)
		return;

	if (this->mIsSelectable != selectable)
	{
		this->mIsSelectable = selectable;
		emit selectabilityChanged(this);
	}
}

bool NMLayer::isSelectable(void)
{
	return this->mIsSelectable;
}

bool NMLayer::isVectorLayer(void)
{
	if (this->mLayerType == NMLayer::NM_VECTOR_LAYER)
		return true;

	return false;
}

bool NMLayer::isImageLayer(void)
{
	if (this->mLayerType == NMLayer::NM_IMAGE_LAYER)
		return true;

	return false;
}

void NMLayer::showAttributeTable(void)
{
	if (this->mTableView == 0)
		this->createTableView();

	if (this->mTableView != 0)
		this->mTableView->show();
}

void NMLayer::createTableView(void)
{
	// implemented in subclasses
}

int NMLayer::updateAttributeTable(void)
{
	// attribute table from changes in data set
	// subclasses to implement
	return 0;
}

void NMLayer::updateDataSet(QStringList& slAlteredColumns,
		QStringList& slDeletedColumns)
{
	// update data set from changes in attribute table
	// subclasses to implement
}


const vtkTable* NMLayer::getTable(void)
{
	// subclass to implement
	return 0;
}

void NMLayer::writeDataSet()
{
	// subclass  to implement
}

double NMLayer::getLegendItemUpperValue(int legendRow)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return -9;
	}

	vtkDoubleArray* range = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("range"));

	double r[2];
	range->GetTuple(legendRow, r);

	return r[1];
}


double NMLayer::getLegendItemLowerValue(int legendRow)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return -9;
	}

	vtkDoubleArray* range = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("range"));

	double r[2];
	range->GetTuple(legendRow, r);

	return r[0];
}

bool NMLayer::getLegendItemRange(int legendRow, double* range)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return false;
	}

	vtkDoubleArray* rar = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("range"));

	rar->GetTuple(legendRow, range);

	return true;
}

//void NMLayer::updateSelection(void)
//{
//	// subclass to implement
//}

void NMLayer::updateSelectionData(void)
{
	// subclass to implement
}

