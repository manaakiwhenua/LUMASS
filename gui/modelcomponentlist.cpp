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
#include <limits>

#include "modelcomponentlist.h"
#include "otbmodellerwin.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMainWindow>
#include <QListWidgetItem>
#include <QSharedPointer>
#include <QAbstractItemView>
#include <QTreeWidgetItem>
#include <QTreeView>
#include <QMenu>
#include <QAction>
#include <QItemSelection>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QInputDialog>

#include "vtkDataSetAttributes.h"


#include "QVTKWidget.h"

ModelComponentList::ModelComponentList(QWidget *parent)
       :QTreeView(parent)
{
	this->mLayerModel = new NMLayerModel(this);
	this->setModel(this->mLayerModel);

	// set the general column count for this control
	this->setHeaderHidden(true);
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setExpandsOnDoubleClick(true);
	this->setUniformRowHeights(true);
	this->reset();

	// init the popup menu
	this->mMenu = new QMenu(this);

	QAction* actZoom = new QAction(this->mMenu);
	actZoom->setText(tr("Zoom To Layer"));
	QAction* actTable = new QAction(this->mMenu);
	actTable->setText(tr("Open Attribute Table"));
	QAction* actRemove = new QAction(this->mMenu);
	actRemove->setText(tr("Remove Layer"));
	QAction* actUniqueValues = new QAction(this->mMenu);
	actUniqueValues->setText(tr("Unique Values Map ..."));
	QAction* actSingleSymbol = new QAction(this->mMenu);
	actSingleSymbol->setText(tr("Single Symbol Map"));
	QAction* actSaveChanges = new QAction(this->mMenu);
	actSaveChanges->setText(tr("Save Changes"));

	this->mMenu->addAction(actTable);

	this->mMenu->addSeparator();
	this->mMenu->addAction(actZoom);
	this->mMenu->addAction(actSingleSymbol);
	this->mMenu->addAction(actUniqueValues);

	this->mMenu->addSeparator();
	this->mMenu->addAction(actSaveChanges);
	this->mMenu->addAction(actRemove);


	this->connect(actZoom, SIGNAL(triggered()), this, SLOT(zoomToLayer()));
	this->connect(actTable, SIGNAL(triggered()), this, SLOT(openAttributeTable()));
	this->connect(actRemove, SIGNAL(triggered()), this, SLOT(removeCurrentLayer()));
	this->connect(actSingleSymbol, SIGNAL(triggered()), this, SLOT(mapSingleSymbol()));
	this->connect(actUniqueValues, SIGNAL(triggered()), this, SLOT(mapUniqueValues()));
	this->connect(actSaveChanges, SIGNAL(triggered()), this, SLOT(saveLayerChanges()));

}

ModelComponentList::~ModelComponentList()
{
	delete this->mMenu;
	delete this->mLayerModel;
}

void ModelComponentList::openAttributeTable()
{
//	NMDebugCtx(ctxModelComponentList, << "...");

	NMLayer* l = (NMLayer*)this->currentIndex().internalPointer();
	l->showAttributeTable();

//	NMDebugCtx(ctxModelComponentList, << "done!");
}

void ModelComponentList::saveLayerChanges()
{
	NMLayer* l = (NMLayer*)this->currentIndex().internalPointer();
	this->setCurrentIndex(QModelIndex());
	if (l != 0)
		l->writeDataSet();
}

void ModelComponentList::removeCurrentLayer()
{
	NMLayer* l = (NMLayer*)this->currentIndex().internalPointer();
	this->setCurrentIndex(QModelIndex());
	if (l != 0)
		this->removeLayer(l);
}

void ModelComponentList::updateLegend(const NMLayer* layer)
{
	NMDebugAI( << "going to update the legend for " << layer->objectName().toStdString()
			<< endl);

	QModelIndex idx = this->mLayerModel->getItemLayerModelIndex(layer->objectName());
	if (this->isExpanded(idx))
	{
		this->collapse(idx);
		this->expand(idx);
	}

	this->update(idx);
}

NMLayer* ModelComponentList::getSelectedLayer()
{
	NMLayer* l = 0;

	QModelIndexList il = this->selectedIndexes();
	if (il.size() == 0)
		return l;

	const QModelIndex idx = il.at(0);
	l = (NMLayer*)idx.internalPointer();

	return l;
}


/*int ModelComponentList::changeLayerPos(QString layerName, int newpos)
{
	int ret = -1;

	// paramter meaning
	// layerName: name of the layer to be moved
	// newpos: 0-based index of the tree position with the uppermost layer
	// 			having an index of 0 and the bottom layer of (number of layers)-1


	// NOTE NOTE NOTE NOTE NOTE
	// the position returned by the layer itself refers to its renderer position
	// within the renderer stack of the rendering window; since we've got a
	// background renderer for various reasons, the position the layer returns
	// is 1-based, whereas newpos and ModelComponentList::getLayer(int) refer
	// to a 0-based index!!!!
	//
	// newpos = newpos + 1 (in the layer's world)

	// that's why we are introducing a new var
	int oldpos = -9;
	int oldRenPos = -9;
	int newRenPos = this->toLayerStackIndex(newpos) + 1;

	NMDebugAI( << "initial situation: " << endl);
	NMDebugAI( << "newpos: " << newpos << " oldpos: " << oldpos << " oldRenPos: " << oldRenPos <<
			" newRenPos: " << newRenPos << endl);

	// get hold of the layer
	NMLayer* l = this->getLayer(layerName);
	if (l == 0)
		return ret;

	// check whether newpos lies within  the bounds
	int nlayers = this->getLayerCount();
	if (newpos < 0 || newpos >= nlayers)
		return ret;

	// check, whether we've got a new position at all
	oldRenPos = l->getLayerPos();
	oldpos = this->toTreeModelRow(oldRenPos) - 1;
	if (oldpos == newpos)
		return oldpos;

	// alright, let's move the layer
	QMap<int, QSharedPointer<NMLayer> >::iterator it = this->mLayers.begin();
	int itpos;

	// new postion is higher up in the stack
	if (newRenPos > oldRenPos)
	{
		// newpos is further up in the stack -> move other layers down
		for (; it != this->mLayers.end(); it++)
		{
			itpos = it.value()->getLayerPos();
			if (itpos > oldRenPos && itpos <= newRenPos)
				it.value()->setLayerPos(itpos - 1);
		}
	}
	// new position is further down in the stack
	else
	{
		// move other layers up
		for (; it != this->mLayers.end(); it++)
		{
			itpos = it.value()->getLayerPos();
			if (itpos < oldRenPos && itpos >= newRenPos)
				it.value()->setLayerPos(itpos + 1);
		}
	}

	// 1-based position in renderer stack
	l->setLayerPos(newRenPos);
	ret = newpos;

	QVTKWidget* qvtk = this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"));
	qvtk->update();

	// 0-based  position in tree widget
	QTreeWidgetItem* item = this->takeTopLevelItem(oldpos);
	this->insertTopLevelItem(newpos, item);

	// need to check consistency

	return ret;
}*/

bool ModelComponentList::removeLayer(QString layerName)
{
	NMLayer* l = this->mLayerModel->getItemLayer(layerName);
	if (l == 0)
		return false;

	this->removeLayer(l);
	return  true;
}

void ModelComponentList::removeLayer(NMLayer* layer)
{
	NMDebugCtx(ctxModelComponentList, << "...");

	// remove layer from the model (which updates as well the layer position
	// hold by each layer in the layer stack)
	this->mLayerModel->removeLayer(layer);
	this->reset();
	// update the map display window
	this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"))->update();

	NMDebugCtx(ctxModelComponentList, << "done!");
}


void ModelComponentList::updateMapWin(const NMLayer* layer)
{
	// just do the whole window for now

	// TODO: later we have to get the layer's bbox, transform into the window
	// coordinates and then just update this very region of the renderwidget window

	NMDebugAI( << "updating map window for " << layer->objectName().toStdString() << endl);

	QVTKWidget* qvtk = this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"));
    qvtk->update();
}


void ModelComponentList::addLayer(NMLayer* layer)
{
	NMDebugCtx(ctxModelComponentList, << "...");

	// TODO: check this
	if (layer == 0 || layer->getRenderer() == 0)
	{
		NMDebugAI(<< "invalid layer!" << endl);
		NMDebugCtx(ctxModelComponentList, << "done!");
		return;
	}

	// connect layer with updateMap routine
	// (i.e. whenever the visibility of the layer has changed it emits
	// the signal "visibilityChanged(const NMLayer*)" which is then being processed
	// by the slot "updateMapWin(const NMLayer*)"
	connect(layer, SIGNAL(visibilityChanged(const NMLayer*)),
				this, SLOT(updateMapWin(const NMLayer*)));
	connect(layer, SIGNAL(legendChanged(const NMLayer*)),
			this, SLOT(updateLegend(const NMLayer*)));

	// add the layer to the NMLayerModel
	this->mLayerModel->pileItemLayer(layer);
	int nlayers = this->mLayerModel->getItemLayerCount();
	this->recalcMapBBox();

	// get the camera of the background renderer
    OtbModellerWin* win = qobject_cast<OtbModellerWin*>(this->topLevelWidget());
	vtkRenderer* bkgRen = const_cast<vtkRenderer*>(win->getBkgRenderer());

	// adjust the camera to the map bounding box if it is the first layer we're adding
	if (nlayers == 1)
		bkgRen->ResetCamera(this->mFullMapExt);

	// use the same camera for the newly added layer as for the initial layer
	vtkCamera* cam0 = bkgRen->GetActiveCamera();
	vtkRenderer* ren = const_cast<vtkRenderer*>(layer->getRenderer());
	ren->SetActiveCamera(cam0);

	// adjust the number of layers per this render window and add the renderer
	// to the render window
	// add the renderer to the render window and update the widget
	QVTKWidget* qvtk = this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"));

	// we keep one layer more than required (and keep in mind that we've got the
	// the background layer as well
	qvtk->GetRenderWindow()->SetNumberOfLayers(nlayers+2);
	qvtk->GetRenderWindow()->AddRenderer(ren);
	qvtk->update();

	this->reset();
	NMDebugCtx(ctxModelComponentList, << "done!");
}

void ModelComponentList::recalcMapBBox(void)
{
	NMDebugCtx(ctxModelComponentList, << "...");

	int i;
	double* mb = this->mFullMapExt;

	// reset the actual full map bounding box
	for (i=0; i < 4; i +=2)
		mb[i] = std::numeric_limits<double>::max();

	for (i=1; i < 4; i +=2)
		mb[i] = std::numeric_limits<double>::max() * -1;

	mb[4] = 0; mb[5] = 0;

	int nlayers = this->mLayerModel->getItemLayerCount();
	NMLayer* l;
	for (i=0; i < nlayers; i++)
	{
		l = this->mLayerModel->getItemLayer(i);
		const double* box = l->getBBox();
		this->unionMapBBox(box);
		NMDebugAI( << "processing " << l->objectName().toStdString() <<
						"'s box ..." << endl);
	}

	NMDebugCtx(ctxModelComponentList, << "done!");
}


void ModelComponentList::unionMapBBox(const double* box)
{
	double* mb = this->mFullMapExt;
	int i;
	for (i=0; i < 6; i += 2)
	{
		if (box[i] < mb[i])
			mb[i] = box[i];
	}

	for (i=1; i < 6; i += 2)
	{
		if (box[i] > mb[i])
			mb[i] = box[i];
	}
}

const double* ModelComponentList::getMapBBox(void)
{
	return this->mFullMapExt;
}

void ModelComponentList::mouseDoubleClickEvent(QMouseEvent* event)
{
	NMDebugCtx(ctxModelComponentList, << "...")

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
		NMDebugCtx(ctxModelComponentList, << "done!")
		return;
	}

	if (!idx.parent().isValid())
	{
		NMLayer* l = (NMLayer*)idx.internalPointer();
		int nleg = l->getLegendItemCount();
		NMDebugAI(<< l->objectName().toStdString() << " has " << nleg << " items .." << endl);

		if (this->isExpanded(idx))
			this->collapse(idx);
		else
			this->expand(idx);
	}

	NMDebugCtx(ctxModelComponentList, << "done!")
}

void ModelComponentList::mousePressEvent(QMouseEvent *event)
{
    NMDebugCtx(ctxModelComponentList, << "...");

	int x = event->pos().x();
	int y = event->pos().y();

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
		NMDebugCtx(ctxModelComponentList, << "done!")
		return;
	}

	if (!idx.parent().isValid())
	{
		this->setCurrentIndex(idx);
		if (event->button() == Qt::LeftButton)
		{
			this->dragStartPosition = event->pos();
			int col = idx.column();
			int row = idx.row();

			this->processSelection(true);
		}
		else if (event->button() == Qt::RightButton)
		{
			this->processSelection(false);

			this->mMenu->move(event->globalPos());
			this->mMenu->exec();
		}
	}

    NMDebugCtx(ctxModelComponentList, << "done!");
}

void ModelComponentList::processSelection(bool toggle)
{
	QModelIndex idx = this->currentIndex();
	QModelIndexList il = this->selectedIndexes();
	QModelIndexList::Iterator it = il.begin();
	bool bselect = toggle ? false : true;
	for (; it != il.end(); ++it)
	{
		if ((*it).row() == idx.row())
			bselect = toggle ? true : false;
	}

	if (bselect)
	{
		this->selectionModel()->clearSelection();
		this->selectionModel()->select(idx, QItemSelectionModel::Select |
			QItemSelectionModel::Rows);
	}
	else
	{
		this->selectionModel()->select(idx, QItemSelectionModel::Deselect |
			QItemSelectionModel::Rows);
	}
	this->update();
}

void ModelComponentList::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;

    QModelIndex idx = this->indexAt(dragStartPosition);
    QString layerName = idx.data(Qt::DisplayRole).toString();

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
    mimeData->setText(layerName.toAscii());
    drag->setMimeData(mimeData);
    Qt::DropAction dropAction =
                drag->exec(Qt::CopyAction, Qt::CopyAction);

}

void ModelComponentList::zoomToLayer()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	NMLayer* l = (NMLayer*)idx.internalPointer();

	// get the camera of the background renderer
    OtbModellerWin* win = qobject_cast<OtbModellerWin*>(this->topLevelWidget());
	vtkRenderer* bkgRen = const_cast<vtkRenderer*>(win->getBkgRenderer());
	bkgRen->ResetCamera(const_cast<double*>(l->getBBox()));
	win->findChild<QVTKWidget*>(tr("qvtkWidget"))->update();
}

void ModelComponentList::mapSingleSymbol()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	NMLayer* l = (NMLayer*)idx.internalPointer();
	if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
		vl->mapSingleSymbol();
	}
}

void ModelComponentList::mapUniqueValues()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	NMLayer* l = (NMLayer*)idx.internalPointer();
	if (l->getLayerType() != NMLayer::NM_VECTOR_LAYER)
		return;

	NMVectorLayer* vL = qobject_cast<NMVectorLayer*>(l);

	// make a list of available attributes
	vtkDataSet* ds = const_cast<vtkDataSet*>(vL->getDataSet());
	vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);

	int numFields = dsAttr->GetNumberOfArrays();

	QStringList sFields;
	for (int f=0; f < numFields; f++)
	{
		vtkAbstractArray* aa = dsAttr->GetAbstractArray(f);
		if (strcmp(aa->GetName(), "nm_hole") == 0 ||
			strcmp(aa->GetName(), "nm_sel") == 0)
			continue;

		if (aa->GetDataType() == VTK_DOUBLE ||
			aa->GetDataType() == VTK_FLOAT)
			continue;

		sFields.append(QString(dsAttr->GetArrayName(f)));
	}

	bool bOk = false;
	QString theField = QInputDialog::getItem(this,
			tr("Map Unique Values"), tr("Choose an attribute to map"),
			sFields, 0, false, &bOk, 0);
	// if the user pressed cancel
	if (!bOk)
		return;

	vL->mapUniqueValues(theField);
}
