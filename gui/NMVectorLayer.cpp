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
 * NMVectorLayer.cpp
 *
 *  Created on: 11/03/2011
 *      Author: alex
 */

#include "NMTableView.h"

#include <QHash>
#include <QTime>
#include <QVector>
#include <QFileInfo>

#include <NMVectorLayer.h>
#include "vtkIntArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkLookupTable.h"
#include "vtkOGRLayerMapper.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkPolygon.h"
#include "vtkMath.h"
#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkSmartPointer.h"

#include "vtkTableToSQLiteWriter.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSQLiteQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLiteDatabase.h"
#include "vtkProperty.h"


NMVectorLayer::NMVectorLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer,
		QObject* parent)
	: NMLayer(renWin, renderer, parent)
{
	this->mLayerType = NMLayer::NM_VECTOR_LAYER;
	this->mFeatureType = NMVectorLayer::NM_UNKNOWN_FEATURE;
	this->mContour = 0;
	this->mContourMapper = 0;
	this->mContourActor = 0;

	this->mLayerIcon = QIcon(":vector_layer.png");
}

NMVectorLayer::~NMVectorLayer()
{
	NMDebugCtx(ctxNMVectorLayer, << "...");
	NMDebugAI(<< "removing layer: " << this->objectName().toStdString() << std::endl);

	if (this->mRenderer != 0)
	{
		this->removeFromMap();
	}

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

NMVectorLayer::NMFeatureType NMVectorLayer::getFeatureType(void)
{
	return this->mFeatureType;
}

void NMVectorLayer::removeFromMap(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	// the actor is going to be removed by the superclass NMLayer
//	if (this->mActor != 0)
//		if (this->mRenderer != 0)
//			this->mRenderer->RemoveActor(this->mActor);

	if (this->mContourActor != 0)
		if (this->mRenderer != 0)
			this->mRenderer->RemoveActor(this->mContourActor);

//	this->setVisible(false);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

void NMVectorLayer::setContour(vtkPolyData* contour)
{
	if (!contour)
	{
		NMDebugCtx(ctxNMLayer, << "contour is NULL!");
		return;
	}

	this->mContour = contour;

	// create a default black colour for the outline
	vtkSmartPointer<vtkLongArray> contClr = vtkSmartPointer<vtkLongArray>::New();
	contClr->Allocate(this->mContour->GetNumberOfCells());
	contClr->SetNumberOfValues(this->mContour->GetNumberOfCells());
	for (int i=0; i < this->mContour->GetNumberOfCells(); ++i)
			contClr->SetValue(i, i);
	this->mContour->GetCellData()->SetScalars(contClr);

	// create a contour mapper
	vtkSmartPointer<vtkOGRLayerMapper> m = vtkSmartPointer<vtkOGRLayerMapper>::New();
	m->SetInput(this->mContour);

	vtkSmartPointer<vtkLookupTable> olclrtab = vtkSmartPointer<vtkLookupTable>::New();
	olclrtab->Allocate(this->mContour->GetNumberOfCells());
	olclrtab->SetNumberOfTableValues(this->mContour->GetNumberOfCells());

	// since we can save selections with the data set, we better check, whether
	// we've got some
	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* sel = dsAttr->GetArray("nm_sel");

	for (int c=0; c < contour->GetNumberOfCells(); ++c)
	{
		if (sel->GetTuple1(c) == 0)
			olclrtab->SetTableValue(c, 0,0,0,1);
		else
			olclrtab->SetTableValue(c, 1,0,0,1);
	}

	m->SetLookupTable(olclrtab);
	m->SetScalarRange(0, contour->GetNumberOfCells());
	this->mContourMapper = m;

	// create a contour actor
	vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
	a->SetMapper(m);
	this->mContourActor = a;
	this->mContourActor->SetVisibility(1);
	this->mContourActor->GetProperty()->SetLineWidth(1);
	this->mRenderer->AddActor(a);
}

void NMVectorLayer::setDataSet(vtkDataSet* dataset)
{
	// set the polydata
	if (!dataset)
	{
		NMDebugCtx(ctxNMVectorLayer, << "data set is NULL!");
		return;
	}

	vtkPolyData* pd = vtkPolyData::SafeDownCast(dataset);
	this->mDataSet = pd;

	// set the feature type
	if (pd->GetVerts()->GetNumberOfCells())
		this->mFeatureType = NMVectorLayer::NM_POINT_FEAT;
	else if (pd->GetLines()->GetNumberOfCells())
		this->mFeatureType = NMVectorLayer::NM_POLYLINE_FEAT;
	else if (pd->GetPolys()->GetNumberOfCells())
		this->mFeatureType = NMVectorLayer::NM_POLYGON_FEAT;

	// set the bounding box
	pd->GetBounds(this->mBBox);

	// create and set the mapper
	vtkSmartPointer<vtkOGRLayerMapper> m = vtkSmartPointer<vtkOGRLayerMapper>::New();
	m->SetInput(pd);
	this->mMapper = m;

	// create and set the actor
	vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
	a->SetMapper(m);
	this->mActor = a;
	this->mActor->SetVisibility(0);
	this->mRenderer->AddActor(a);

	// create contours, if we've got polygons
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
	{
		//NMDebugAI( << "NMVectorLayer '" << this->objectName().toStdString() <<
		//		"' contains polygons!" << endl);
		vtkSmartPointer<vtkPolyData> cont = vtkSmartPointer<vtkPolyData>::New();
		cont->SetPoints(pd->GetPoints());
//		cont->SetPolys(pd->GetPolys());
		cont->SetLines(pd->GetPolys());
		cont->BuildCells();
		cont->BuildLinks();
		this->setContour(cont);
	}

	// initially, we show all polys in the same colour
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		this->mapSingleSymbol();
}

const vtkPolyData* NMVectorLayer::getContour(void)
{
	return this->mContour;
}

const vtkOGRLayerMapper* NMVectorLayer::getContourMapper(void)
{
	return this->mContourMapper;
}

const vtkActor* NMVectorLayer::getContourActor(void)
{
	return  this->mContourActor;
}

void NMVectorLayer::setVisible(bool visible)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	if (this->mDataSet == 0)
		return;

	if (this->mIsVisible != visible)
	{
		int vis = visible ? 1 : 0;

		if (this->mActor != 0)
		{
			// now handle all additional parts, that have to
			// change visibility
			if (this->mContourActor != 0)
				this->mContourActor->SetVisibility(vis);

			this->mActor->SetVisibility(vis);
			this->mIsVisible = visible;
			emit visibilityChanged(this);
		}
	}

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

/*double NMVectorLayer::getArea()
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	NMDebugAI( << "call base class ..." << endl);
	this->mTotalArea = NMLayer::getArea();
	NMDebugAI( << "base class' area is " << this->mTotalArea / 10000
			<< " ha" << endl);

	// check for polyline or polygon
	vtkPolyData* pd = vtkPolyData::SafeDownCast(this->mDataSet);
	vtkIdType numPolys = pd->GetNumberOfPolys();
	if (numPolys < 1)
	{
		NMDebugAI( << "oh oh, no polys!" << endl);
		return this->mTotalArea;
	}

	// calculate area in x-y plane for bounding box ;
	NMDebugAI( << "calc the plane in x-y" << endl);
	if (numPolys > 0)
	{
		// calculate area for polygons
		double area = 0;
		for (long c=0; c < numPolys; c++)
		{
			vtkIdType cid = c;
			vtkCell* cell = this->mDataSet->GetCell(cid);
			if (cell == 0)
			{
				NMDebugInd(2, << "error getting cell " << c << endl);
				return -1;
			}
			vtkPolygon* poly = vtkPolygon::SafeDownCast(cell);

			area += poly->ComputeArea();
		}
		this->mTotalArea = area;
	}

	NMDebugAI( << "layer's area: " << this->mTotalArea << endl);

	NMDebugCtx(ctxNMVectorLayer, << "done!");

	return this->mTotalArea;
}*/

long NMVectorLayer::getNumberOfFeatures(void)
{
	return this->mDataSet->GetNumberOfCells();
}

int NMVectorLayer::mapUniqueValues(QString fieldName)
{
	if (this->mFeatureType != NMVectorLayer::NM_POLYGON_FEAT)
	{
		return 0;
	}

	// make a list of available attributes
	vtkDataSet* ds = const_cast<vtkDataSet*>(this->getDataSet());
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
	vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
			dsAttr->GetArray("nm_hole"));
	vtkLongArray* nmid = vtkLongArray::SafeDownCast(
			dsAttr->GetArray("nm_id"));

	// let's find out about the attribute
	// if we've got doubles, we refuse to map unique values ->
	// doesn't make sense, does it?
	vtkAbstractArray* anAr = dsAttr->GetAbstractArray(fieldName.toStdString().c_str());
	int type = anAr->GetDataType();
	if (type == VTK_DOUBLE)
	{
		NMDebugAI( << "oh no, not with doubles!" << endl);
		return 0;
	}

	bool bNum = true;
	if (type == VTK_STRING)
		bNum = false;

	// we create a new look-up table and set the number of entries we need
	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->Allocate(anAr->GetNumberOfTuples());
	clrtab->SetNumberOfColors(anAr->GetNumberOfTuples());

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
	QString sVal;
	vtkMath::RandomSeed(QTime::currentTime().msec());
	double rgba[4];
	for (int t=0; t < anAr->GetNumberOfTuples(); ++t)
	{
		if (hole->GetValue(t))
		{
			clrtab->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
			continue;
		}

		if (bNum)
		{
			int val = anAr->GetVariantValue(t).ToInt(&bConvOk);
			sVal = QString(tr("%1")).arg(val);
		}
		else
		{
			sVal = anAr->GetVariantValue(t).ToString().c_str();
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
			//double tmprgba[4];
			dblAr->GetTuple(tabInfoIdx, rgba);

			clrtab->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
		}
	}

	// get the mapper and look whats possible
	vtkMapper* mapper = vtkMapper::SafeDownCast(this->mMapper);
	mapper->SetLookupTable(clrtab);

	emit visibilityChanged(this);
	emit legendChanged(this);

	return 1;
}

void NMVectorLayer::mapSingleSymbol()
{
	if (this->mFeatureType != NMVectorLayer::NM_POLYGON_FEAT)
		return;

	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
	vtkLongArray* nmids = vtkLongArray::SafeDownCast(dsAttr->GetArray("nm_id"));
	long ncells = nmids->GetNumberOfTuples();

	vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
			dsAttr->GetArray("nm_hole"));

	// we create a new look-up table and set the number of entries we need
	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->Allocate(ncells);
	clrtab->SetNumberOfTableValues(ncells);

	// let's create a new legend info table
	this->resetLegendInfo();

	// chose a random colour
	QString sVal;
	vtkMath::RandomSeed(QTime::currentTime().msec());

	double rgba[4];
	for (int i=0; i < 3; i++)
		rgba[i] = vtkMath::Random();
	rgba[3] = 1;

	// cell index vector for has map
	QVector<int> idxVec;

	// fill colour look-up table for mapper
	for (long l=0; l < ncells; ++l)
	{
		// add the new cell index to the hash map
		idxVec.append(l);

		clrtab->SetTableValue(l, rgba[0], rgba[1], rgba[2]);
	}

	// fill hash map
	vtkIdType newidx = this->mLegendInfo->InsertNextBlankRow(-9);
	this->mHashValueIndices.insert(tr(""), idxVec);


	// fill mLegendInfoTable with corresponding infor
	double lowup[2];
	lowup[0] = -9;
	lowup[1] = -9;

	vtkDoubleArray* lowupAbstrAr = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("range"));
	lowupAbstrAr->SetTuple(newidx, lowup);

	vtkStringArray* nameAr = vtkStringArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("name"));
	nameAr->SetValue(newidx, "");

	vtkDoubleArray* clrs = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("rgba"));
	clrs->SetTuple(newidx, rgba);

	// get the mapper and look whats possible
	vtkMapper* mapper = vtkMapper::SafeDownCast(this->mMapper);
//	mapper->SetScalarRange(0, clrtab->GetNumberOfColors());
	mapper->SetLookupTable(clrtab);

	emit visibilityChanged(this);
	emit legendChanged(this);
}

void NMVectorLayer::createTableView(void)
{
	if (this->mTableView != 0)
	{
		delete this->mTableView;
	}

	this->updateAttributeTable();
	if (this->mAttributeTable.GetPointer() == 0 ||
			this->mAttributeTable->GetNumberOfColumns() == 0 ||
			this->mAttributeTable->GetNumberOfRows() == 0)
		return;

	this->mTableView = new NMTableView(this->mAttributeTable, 0);
	this->mTableView->hideAttribute("nm_sel");
	this->mTableView->setRowKeyColumn("nm_id");
	this->mTableView->hideAttribute("nm_id");

	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		this->mTableView->hideAttribute("nm_hole");
	this->mTableView->setTitle(tr("Attributes of ") + this->objectName());

	// connect tableview signals to layer slots
	this->connect(this->mTableView,
			SIGNAL(tableDataChanged(QStringList&, QStringList&)), this,
			SLOT(emitAttributeTableChanged(QStringList&, QStringList&)));
	this->connect(this->mTableView, SIGNAL(selectionChanged()), this,
			SLOT(updateSelectionData()));

	// connect layer signals to tableview slots
	this->connect(this, SIGNAL(attributeTableChanged(vtkTable*)),
			this->mTableView, SLOT(setTable(vtkTable*)));
//	this->connect(this, SIGNAL(layerSelectionChanged(NMLayer*)),
//			this->mTableView, SLOT(updateSelection()));

}

int NMVectorLayer::updateAttributeTable(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
	if (dsa == 0 || dsa->GetNumberOfArrays() == 0)
		return 0;

	vtkSmartPointer<vtkTable> rawtab = vtkSmartPointer<vtkTable>::New();
	rawtab->SetRowData(dsa);

	// create an sqlite mem version of the table
	vtkSmartPointer<vtkSQLiteDatabase> sdb = vtkSQLiteDatabase::SafeDownCast(
			vtkSQLDatabase::CreateFromURL(
					"sqlite://:memory:"));
	sdb->Open("", vtkSQLiteDatabase::USE_EXISTING_OR_CREATE);
	vtkSmartPointer<vtkTableToSQLiteWriter> writer =
			vtkSmartPointer<vtkTableToSQLiteWriter>::New();
	writer->SetDatabase(sdb);
	writer->SetInput(rawtab);
	writer->SetTableName("memtable");
	writer->Update();

	// do the query
	vtkSQLiteQuery* sq = vtkSQLiteQuery::SafeDownCast(sdb->GetQueryInstance());

	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		sq->SetQuery("select * from memtable where nm_hole = 0");
	else
		sq->SetQuery("select * from memtable");

	// filter to a new table
	vtkSmartPointer<vtkRowQueryToTable> rowtotab =
			vtkSmartPointer<vtkRowQueryToTable>::New();
	rowtotab->SetQuery(sq);
	rowtotab->Update();

	this->mAttributeTable = rowtotab->GetOutput();
	emit attributeTableChanged(this->mAttributeTable);
	emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
	return 1;
}

const vtkTable* NMVectorLayer::getTable(void)
{
	if (this->mAttributeTable.GetPointer() == 0)
		this->updateAttributeTable();

	return this->mAttributeTable;
}

void NMVectorLayer::updateDataSet(QStringList& slAlteredColumns,
		QStringList& slDeletedColumns)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	if (slDeletedColumns.size() == 0 && slAlteredColumns.size() == 0)
	{
		NMDebugAI(<< "nothing to save!" << endl);
		NMDebugCtx(ctxNMVectorLayer, << "done!");
		return;
	}

	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);

	vtkUnsignedCharArray* holeAr;
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		holeAr = vtkUnsignedCharArray::SafeDownCast(dsAttr->GetArray("nm_hole"));


	// delete all deleted columns
	for (int d=0; d < slDeletedColumns.size(); ++d)
		dsAttr->RemoveArray(slDeletedColumns.at(d).toStdString().c_str());

	// add / update columns
	for (int a=0; a < slAlteredColumns.size(); ++a)
	{
		vtkAbstractArray* aa = this->mAttributeTable->GetColumnByName(
				slAlteredColumns.at(a).toStdString().c_str());

		vtkSmartPointer<vtkAbstractArray> na =
				this->mTableView->createVTKArray(aa->GetDataType());
		na->SetName(aa->GetName());
		na->SetNumberOfComponents(1);
		na->Allocate(holeAr->GetNumberOfTuples());

		int aaCnt = 0;
		for (int r=0; r < holeAr->GetNumberOfTuples(); ++r)
		{
			if (holeAr->GetValue(r))
			{
				na->InsertVariantValue(r, vtkVariant(0));
			}
			else
			{
				na->InsertVariantValue(r, vtkVariant(aa->GetVariantValue(aaCnt)));
				++aaCnt;
			}
		}

		dsAttr->AddArray(na);
	}

	slDeletedColumns.clear();
	slAlteredColumns.clear();

	emit dataSetChanged(this);
	emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

void NMVectorLayer::writeDataSet(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	QFileInfo finfo(this->mFileName);
	if (!finfo.isWritable())
	{
		NMErr(ctxNMVectorLayer, << "can't write to '" << this->mFileName.toStdString() <<
				"' - check permissions!" << endl);
		return;
	}

	// save the layer's data set under the current name
	vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<
			vtkPolyDataWriter>::New();
	writer->SetFileName(this->mFileName.toStdString().c_str());
	writer->SetInput(this->mDataSet);
	writer->SetFileTypeToBinary();
	writer->Update();

	this->mHasChanged = false;
	emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

void NMVectorLayer::updateSelectionData(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);

	vtkUnsignedCharArray* holeAr;
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		holeAr = vtkUnsignedCharArray::SafeDownCast(dsAttr->GetArray("nm_hole"));

	vtkDataArray* dsSel = dsAttr->GetArray("nm_sel");

	vtkTable* tab = const_cast<vtkTable*>(this->mTableView->getTable());

	vtkDataArray* tabSel = vtkDataArray::SafeDownCast(
					tab->GetColumnByName("nm_sel"));

	vtkLookupTable* clrTab = vtkLookupTable::SafeDownCast(
			this->mContourMapper->GetLookupTable());

	int valCellCnt = 0;
	for (int r = 0; r < holeAr->GetNumberOfTuples(); ++r)
	{
		if (holeAr->GetValue(r) == 1)
		{
			clrTab->SetTableValue(r, 0,0,0,1);
			continue;
		}

		dsSel->SetTuple1(r, tabSel->GetTuple1(valCellCnt));

		if (tabSel->GetTuple1(valCellCnt) != 0)
			clrTab->SetTableValue(r, 1,0,0,1);
		else
			clrTab->SetTableValue(r, 0,0,0,1);

		++valCellCnt;
	}

	emit visibilityChanged(this);
	emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

void NMVectorLayer::updateLayerSelection(QList<long> lstCellId,
		QList<long> lstNMId, NMLayerSelectionType seltype)
{
	// update the table, if present
	if (this->mTableView != 0)
	{
		vtkDataArray* tabSel = vtkDataArray::SafeDownCast(
				this->mAttributeTable->GetColumnByName("nm_sel"));

		switch (seltype)
		{
		case NM_SEL_NEW:
			this->mTableView->clearSelection();
			foreach(const int &nmid, lstNMId)
				tabSel->SetTuple1(nmid-1, 1);
			break;
		case NM_SEL_ADD:
			foreach(const int &nmid, lstNMId)
				tabSel->SetTuple1(nmid-1, 1);
			break;
		case NM_SEL_REMOVE:
			foreach(const int &nmid, lstNMId)
				tabSel->SetTuple1(nmid-1, 0);
			break;
		case NM_SEL_CLEAR:
			this->mTableView->clearSelection();
			break;
		}

		emit attributeTableChanged(this->mAttributeTable);
	}

	// update the data set and selection
	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);

	vtkUnsignedCharArray* holeAr;
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
		holeAr = vtkUnsignedCharArray::SafeDownCast(dsAttr->GetArray("nm_hole"));
	vtkDataArray* dsSel = dsAttr->GetArray("nm_sel");

	vtkLookupTable* clrTab = vtkLookupTable::SafeDownCast(
			this->mContourMapper->GetLookupTable());

	switch(seltype)
	{
	case NM_SEL_NEW:
		for (int r = 0; r < holeAr->GetNumberOfTuples(); ++r)
		{
			clrTab->SetTableValue(r, 0,0,0,1);
			dsSel->SetTuple1(r,0);
		}

		foreach(const int &cid, lstCellId)
		{
			dsSel->SetTuple1(cid,1);
			clrTab->SetTableValue(cid, 1,0,0,1);
		}
		break;
	case NM_SEL_ADD:
		foreach(const int &cid, lstCellId)
		{
			dsSel->SetTuple1(cid,1);
			clrTab->SetTableValue(cid, 1,0,0,1);
		}
		break;
	case NM_SEL_REMOVE:
		foreach(const int &cid, lstCellId)
		{
			dsSel->SetTuple1(cid,0);
			clrTab->SetTableValue(cid, 0,0,0,1);
		}
		break;
	case NM_SEL_CLEAR:
		for (int r = 0; r < holeAr->GetNumberOfTuples(); ++r)
		{
			clrTab->SetTableValue(r, 0,0,0,1);
			dsSel->SetTuple1(r,0);
		}
		break;
	}

	emit dataSetChanged(this);
	emit legendChanged(this);
	emit visibilityChanged(this);
}



