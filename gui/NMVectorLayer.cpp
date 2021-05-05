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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif
#include "NMGlobalHelper.h"

#include "NMVectorLayer.h"

#include <QHash>
#include <QTime>
#include <QVector>
#include <QFileInfo>
#include <QColor>

#include "vtkIntArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkLookupTable.h"
#include "vtkTriangleFilter.h"

#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkPolygon.h"
#include "vtkMath.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkSmartPointer.h"

#include "vtkQtEditableTableModelAdapter.h"
#include "vtkQtTableModelAdapter.h"

#include "vtkTableToSQLiteWriter.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSQLiteQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLiteDatabase.h"
#include "vtkProperty.h"
#include "vtkExtractCells.h"
#include "vtkGeometryFilter.h"
#include "vtkDataSetMapper.h"

#include "NMPolygonToTriangles.h"


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
	this->mContourColour = QColor(0, 0, 0, 255);

	this->mLayerIcon = QIcon(":vector_layer.png");
    this->mContourOnly = false;
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
#ifdef VTK_OPENGL2
    vtkSmartPointer<vtkPolyDataMapper> m = vtkSmartPointer<vtkPolyDataMapper>::New();
#else
    vtkSmartPointer<vtkOGRLayerMapper> m = vtkSmartPointer<vtkOGRLayerMapper>::New();
#endif



//    vtkSmartPointer<vtkTriangleFilter> tf = vtkSmartPointer<vtkTriangleFilter>::New();
//    tf->SetInputData(this->mContour);
//    m->SetInputConnection(tf->GetOutputPort());
    m->SetInputData(this->mContour);

	vtkSmartPointer<vtkLookupTable> olclrtab = vtkSmartPointer<vtkLookupTable>::New();
	long ncells = this->mContour->GetNumberOfCells();
	olclrtab->Allocate(ncells);
	olclrtab->SetNumberOfTableValues(ncells);

	// since we can save selections with the data set, we better check, whether
	// we've got some
	vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
	vtkDataArray* sel = dsAttr->GetArray("nm_sel");

	for (int c=0; c < ncells; ++c)
	{
		olclrtab->SetTableValue(c,
					mContourColour.redF(),
					mContourColour.greenF(),
					mContourColour.blueF(),
					mContourColour.alphaF());
	}
	//olclrtab->SetTableValue(0, 0, 0, 0, 0);
	//olclrtab->SetTableValue(ncells, 0, 0, 0, 0);
	olclrtab->SetTableRange(0, ncells-1);

	m->SetLookupTable(olclrtab);
	m->SetUseLookupTableScalarRange(1);
	this->mContourMapper = m;

	// create a contour actor
	vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
	a->SetMapper(m);
	//a->GetProperty()->SetOpacity(0.2);
	this->mContourActor = a;
	this->mContourActor->SetVisibility(1);
	this->mContourActor->GetProperty()->SetLineWidth(1);
	this->mRenderer->AddActor(a);
}

void
NMVectorLayer::setContourColour(QColor clr)
{
    if (this->getFeatureType() != NMVectorLayer::NM_POLYGON_FEAT)
        return;

    if (this->mContourActor.GetPointer() == 0)
        return;

    if (clr.isValid())
    {
        this->mContourColour = clr;
    }

    vtkLookupTable* cclrs = vtkLookupTable::SafeDownCast(mContourMapper->GetLookupTable());
    for(int a=0; a < cclrs->GetNumberOfTableValues(); ++a)
    {
        cclrs->SetTableValue(a,
                             mContourColour.redF(),
                             mContourColour.greenF(),
                             mContourColour.blueF(),
                             mContourColour.alphaF());
    }
    this->mContourMapper->Update();
}

void NMVectorLayer::setDataSet(vtkDataSet* dataset)
{
	// set the polydata
	if (!dataset)
	{
		NMDebugCtx(ctxNMVectorLayer, << "data set is NULL!");
		return;
	}

	emit layerProcessingStart();

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
#ifdef VTK_OPENGL2
    vtkSmartPointer<NMVtkOpenGLPolyDataMapper2> m = vtkSmartPointer<NMVtkOpenGLPolyDataMapper2>::New();
    m->SetInputData(pd);
#else
    vtkSmartPointer<vtkOGRLayerMapper> m = vtkSmartPointer<vtkOGRLayerMapper>::New();
    m->SetInputData(pd);
#endif

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
        cont->SetLines(pd->GetPolys());
        this->setContour(cont);
    }

	this->initiateLegend();

	emit layerProcessingEnd();
}

const vtkPolyData* NMVectorLayer::getContour(void)
{
	return this->mContour;
}

#ifdef VTK_OPENGL2
const vtkPolyDataMapper *NMVectorLayer::getContourMapper(void)
#else
const vtkOGRLayerMapper* NMVectorLayer::getContourMapper(void)
#endif
{
	return this->mContourMapper;
}

const vtkActor* NMVectorLayer::getContourActor(void)
{
	return  this->mContourActor;
}

void NMVectorLayer::setVisible(bool visible)
{
    //NMDebugCtx(ctxNMVectorLayer, << "...");

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
            {
				this->mContourActor->SetVisibility(vis);
            }

            if (!mContourOnly)
            {
                this->mActor->SetVisibility(vis);
            }

            this->mIsVisible = visible;
			emit visibilityChanged(this);
		}
	}

    //NMDebugCtx(ctxNMVectorLayer, << "done!");
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


void NMVectorLayer::createTableView(void)
{
	if (this->mTableView != 0)
	{
		return;
		//delete this->mTableView;
		//this->mTableView = 0;
	}

	if (!this->updateAttributeTable())
		return;

	if (this->mTableModel == 0)
	{
        NMLogError(<< ctxNMVectorLayer << ": table model missing!");
		return;
	}

	this->mTableView = new NMTableView(this->mTableModel, 0);
	this->mTableView->setSelectionModel(this->mSelectionModel);

	// hide the 'hole' rows
	if (this->mFeatureType == NMVectorLayer::NM_POLYGON_FEAT)
	{
		vtkUnsignedCharArray* holes = vtkUnsignedCharArray::SafeDownCast(
				this->mAttributeTable->GetColumnByName("nm_hole"));
		QList<int> hiddenrows;
		//NMDebugAI(<< __FUNCTION__ << ": hidden rows ..." << std::endl);
		for (int row=0; row < this->mAttributeTable->GetNumberOfRows(); ++row)
		{
			if (holes->GetValue(row))
			{
				//NMDebug(<< row << " ");
				hiddenrows << row;
			}
		}
		//NMDebug(<< " --> sum=" << hiddenrows.size() << std::endl);
		this->mTableView->hideSource(hiddenrows);
		this->mTableView->hideAttribute("nm_hole");
	}
	this->mTableView->hideAttribute("nm_sel");
	this->mTableView->setTitle(tr("Attributes of ") + this->objectName());

	connect(this, SIGNAL(selectabilityChanged(bool)), mTableView, SLOT(setSelectable(bool)));
    connect(mTableView, SIGNAL(notifyLastClickedRow(long long)), this, SLOT(forwardLastClickedRowSignal(long long)));
}

int NMVectorLayer::updateAttributeTable(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	if (mTableModel != 0)
	{
		NMDebugAI(<< "we've got a table already!" << std::endl);
		NMDebugCtx(ctxNMVectorLayer, << "done!");
		return 1;
	}

	vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
	if (dsa == 0 || dsa->GetNumberOfArrays() == 0)
		return 0;

	disconnectTableSel();

	vtkSmartPointer<vtkTable> rawtab = vtkSmartPointer<vtkTable>::New();
	rawtab->SetRowData(dsa);

	this->mAttributeTable = rawtab;
	vtkQtEditableTableModelAdapter* tabModel;
	if (this->mTableModel == 0)
	{
		tabModel = new vtkQtEditableTableModelAdapter(mAttributeTable);
	}
	else
	{
		tabModel = qobject_cast<vtkQtEditableTableModelAdapter*>(this->mTableModel);
		tabModel->setTable(this->mAttributeTable);
	}
	tabModel->SetKeyColumnName("nm_id");

	// in any case, we create a new item selection model
	if (this->mSelectionModel == 0)
	{
		this->mSelectionModel = new NMFastTrackSelectionModel(tabModel, this);
	}
	this->mTableModel = tabModel;

	connectTableSel();
	emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
	return 1;
}

void NMVectorLayer::writeDataSet(void)
{
	NMDebugCtx(ctxNMVectorLayer, << "...");

	QFileInfo finfo(this->mFileName);
	if (!finfo.isWritable())
	{
        NMLogError(<< ctxNMVectorLayer << ": can't write to '" << this->mFileName.toStdString() <<
				"' - check permissions!" << std::endl);
		return;
	}

	// save the layer's data set under the current name
	vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<
			vtkPolyDataWriter>::New();
	writer->SetFileName(this->mFileName.toStdString().c_str());
    writer->SetInputData(this->mDataSet);
    //writer->SetFileTypeToBinary();
    writer->SetFileTypeToASCII();
	writer->Update();

	this->mHasChanged = false;
	//emit legendChanged(this);

	NMDebugCtx(ctxNMVectorLayer, << "done!");
}

void
NMVectorLayer::setContoursVisible(bool vis)
{
    if (this->mContourActor.GetPointer() != 0)
    {
        if (vis && this->isVisible())
        {
            this->mContourActor->SetVisibility(true);
        }
        else
        {
            this->mContourActor->SetVisibility(false);
        }
    }
}

void
NMVectorLayer::setFeaturesVisible(bool vis)
{
    if (this->mActor.GetPointer() != 0)
    {
        if (vis && this->isVisible())
        {
            this->mActor->SetVisibility(true);
        }
        else
        {
            this->mActor->SetVisibility(false);
        }
    }
    mContourOnly = !vis;
}

void
NMVectorLayer::selectionChanged(const QItemSelection& newSel,
		const QItemSelection& oldSel)
{
	//const int numranges = newSel.size();
	//const int nrows = this->mAttributeTable->GetNumberOfRows();

	// create new selections
#ifdef VTK_OPENGL2
    mSelectionMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
#else
    mSelectionMapper = vtkSmartPointer<vtkOGRLayerMapper>::New();
#endif
	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	vtkSmartPointer<vtkIdList> selCellIds = vtkSmartPointer<vtkIdList>::New();
	vtkSmartPointer<vtkLongArray> scalars = vtkSmartPointer<vtkLongArray>::New();

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
	selCellIds->SetNumberOfIds(selcnt);
	scalars->SetNumberOfTuples(selcnt);
	clrtab->SetNumberOfTableValues(selcnt);

    //this->printSelRanges(newSel, "incoming update selection");

	int clrcnt = 0;
	foreach(const QItemSelectionRange& range, newSel)
	{
		const int top = range.top();
		const int bottom = range.bottom();
		for (int row=top; row<=bottom; ++row)
		{
			scalars->SetValue(clrcnt, clrcnt);
			selCellIds->SetId(clrcnt, row);
            //clrtab->SetTableValue(clrcnt, 1, 0, 0);
            clrtab->SetTableValue(clrcnt, mClrSelection.redF(),
                                          mClrSelection.greenF(),
                                          mClrSelection.blueF(),
                                          mClrSelection.alphaF());
			++clrcnt;
		}
	}

	//NMDebugAI(<< "we should have " << selCellIds->GetNumberOfIds() << " extracted cells" << std::endl);

	if (this->mCellSelection.GetPointer() != 0 && mSelectionActor.GetPointer() != 0)
	{
        //NMDebugAI(<< "removed old selection" << std::endl);
		this->mRenderer->RemoveActor(mSelectionActor);
	}

	vtkSmartPointer<vtkExtractCells> extractor = vtkSmartPointer<vtkExtractCells>::New();
    extractor->SetInputData(mDataSet);
	extractor->SetCellList(selCellIds);

	vtkSmartPointer<vtkGeometryFilter> geoFilter = vtkSmartPointer<vtkGeometryFilter>::New();
	geoFilter->SetInputConnection(extractor->GetOutputPort());
	geoFilter->Update();

	mCellSelection = vtkSmartPointer<vtkPolyData>::New();

	mCellSelection->SetPoints(geoFilter->GetOutput()->GetPoints());
	mCellSelection->SetLines(geoFilter->GetOutput()->GetPolys());
	mCellSelection->GetCellData()->SetScalars(scalars);

    //	NMDebugAI(<< "we've got " << mCellSelection->GetNumberOfCells()
    //			<< " cells in selection" << std::endl);

    mSelectionMapper->SetInputData(mCellSelection);
	mSelectionMapper->SetLookupTable(clrtab);

	vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
	a->SetMapper(mSelectionMapper);

	mSelectionActor = a;
	mRenderer->AddActor(a);

	// call the base class implementation to do datatype agnostic stuff
	NMLayer::selectionChanged(newSel, oldSel);

	emit visibilityChanged(this);
	emit legendChanged(this);
}

void
NMVectorLayer::updateSelectionColor()
{
    const QItemSelection& curSel = this->mSelectionModel->selection();
    vtkSmartPointer<vtkLookupTable> clrtab = vtkLookupTable::SafeDownCast(
                                                this->mSelectionMapper->GetLookupTable());

    int clrcnt = 0;
    foreach(const QItemSelectionRange& range, curSel)
    {
        const int top = range.top();
        const int bottom = range.bottom();
        for (int row=top; row<=bottom; ++row)
        {
            clrtab->SetTableValue(clrcnt, mClrSelection.redF(),
                                          mClrSelection.greenF(),
                                          mClrSelection.blueF(),
                                          mClrSelection.alphaF());
            ++clrcnt;
        }
    }

    this->mSelectionMapper->Update();
}
