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
#include "NMImageLayer.h"
#include "NMVectorLayer.h"
#include "NMComponentListItemDelegate.h"
#include "NMProcessComponentItem.h"
#include "otbAttributeTable.h"
#include "NMModelController.h"
#include "NMModelComponent.h"
#include "NMItkDataObjectWrapper.h"
#include "NMGlobalHelper.h"

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
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QImage>
#include <QHeaderView>
#include <QMessageBox>
#include <QColorDialog>
#include <QGraphicsItem>
#include <QFileInfo>
#include <QtConcurrent>

#include "vtkSmartPointer.h"
#include "vtkObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkCamera.h"
#include "QVTKWidget.h"

const std::string ModelComponentList::ctx = "ModelComponentList";

ModelComponentList::ModelComponentList(QWidget *parent)
       :QTreeView(parent)
{
	this->mLayerModel = new NMLayerModel(this);
	this->setModel(this->mLayerModel);

    this->mVTKConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();

	// set the general column count for this control
	this->setHeaderHidden(true);
	this->setRootIsDecorated(false);
	this->setExpandsOnDoubleClick(true);
	this->setUniformRowHeights(false);

	this->setSelectionBehavior(QAbstractItemView::SelectRows);

	this->viewport()->setAcceptDrops(true);
	this->viewport()->installEventFilter(this);

	this->reset();

	// init the popup menu
	this->mMenu = new QMenu(this);

	QAction* actZoom = new QAction(this->mMenu);
	actZoom->setText(tr("Zoom To Layer"));
	QAction* actTable = new QAction(this->mMenu);
	actTable->setText(tr("Open Attribute Table"));
	QAction* actSaveChanges = new QAction(this->mMenu);
	actSaveChanges->setText(tr("Save Changes"));
	QAction* actRemove = new QAction(this->mMenu);
	actRemove->setText(tr("Remove Layer"));

	QAction* actValueStats = new QAction(this->mMenu);
	actValueStats->setText(tr("Value Field Statistics"));

	QAction* actUniqueValues = new QAction(this->mMenu);
	actUniqueValues->setText(tr("Map Unique Values ..."));
	QAction* actSingleSymbol = new QAction(this->mMenu);
	actSingleSymbol->setText(tr("Map Single Symbol"));
	QAction* actClrTab = new QAction(this->mMenu);
	actClrTab->setText(tr("Map Colour Table"));
	QAction* actClrRamp = new QAction(this->mMenu);
	actClrRamp->setText(tr("Map Value Ramp"));

	this->mMenu->addAction(actTable);

	this->mMenu->addSeparator();
	this->mMenu->addAction(actZoom);
	this->mMenu->addSeparator();
	this->mMenu->addAction(actValueStats);
	this->mMenu->addSeparator();
	this->mMenu->addAction(actSingleSymbol);
	this->mMenu->addAction(actClrTab);
	this->mMenu->addAction(actUniqueValues);
	this->mMenu->addAction(actClrRamp);


	this->mMenu->addSeparator();
	this->mMenu->addAction(actSaveChanges);
	this->mMenu->addAction(actRemove);


	this->connect(actZoom, SIGNAL(triggered()), this, SLOT(zoomToLayer()));
	this->connect(actTable, SIGNAL(triggered()), this, SLOT(openAttributeTable()));
	this->connect(actRemove, SIGNAL(triggered()), this, SLOT(removeCurrentLayer()));
	this->connect(actSingleSymbol, SIGNAL(triggered()), this, SLOT(mapSingleSymbol()));
	this->connect(actUniqueValues, SIGNAL(triggered()), this, SLOT(mapUniqueValues()));
	this->connect(actClrTab, SIGNAL(triggered()), this, SLOT(mapColourTable()));
	this->connect(actClrRamp, SIGNAL(triggered()), this, SLOT(mapColourRamp()));
	this->connect(actSaveChanges, SIGNAL(triggered()), this, SLOT(saveLayerChanges()));
	this->connect(actValueStats, SIGNAL(triggered()), this, SLOT(showValueStats()));

#ifdef DEBUG

	QAction* testing = new QAction(this->mMenu);
	testing->setText(tr("Test ..."));
	this->mMenu->addSeparator();
	this->mMenu->addAction(testing);

	this->connect(testing, SIGNAL(triggered()), this, SLOT(test()));

#endif

	// do some stuff we can't do here
	//initView();
	mDelegate = new NMComponentListItemDelegate(this);
	this->setItemDelegate(mDelegate);


	// connect to some of the qvtkWidget events
	//OtbModellerWin* win = qobject_cast<OtbModellerWin*>(this->topLevelWidget());
	//m_vtkConns = vtkSmartPointer<vtkEventQtSlotConnect>::New();
	//QVTKWidget* vtkWidget = this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"));
    //
	//this->m_vtkConns->Connect(vtkWidget->GetRenderWindow()->GetInteractor(),
    //		vtkCommand::MouseWheelForwardEvent,
    //		this, SLOT(zoomChanged(vtkObject*)));
    //this->m_vtkConns->Connect(vtkWidget->GetRenderWindow()->GetInteractor(),
    //		vtkCommand::MouseWheelBackwardEvent,
    //		this, SLOT(zoomChanged(vtkObject*)));

}

ModelComponentList::~ModelComponentList()
{
	delete this->mMenu;
	delete this->mLayerModel;
}

void
ModelComponentList::initView(void)
{
	// we do some init work here we can't do in the constructor
	this->setIconSize(QSize(35,16));
}

void ModelComponentList::openAttributeTable()
{
//	NMDebugCtx(ctx, << "...");

	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	if (l != 0)
		l->showAttributeTable();

//	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::saveLayerChanges()
{
	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	this->setCurrentIndex(QModelIndex());
	if (l != 0)
	{
		NMDebugAI(<< "going to save changes to the data set..." << endl);
		l->writeDataSet();
	}
}

void ModelComponentList::removeCurrentLayer()
{
	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	this->setCurrentIndex(QModelIndex());
	if (l == 0)
		return;

	if (l->hasChanged())
	{
		QMessageBox msgBox;
		QString text = QString(tr("Remove Layer '%1'")).arg(l->objectName());
		msgBox.setText(text);
		msgBox.setInformativeText("Do you want to save your changes before?");
		msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::Yes);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Yes)
		{
			l->writeDataSet();
		}
	}

	this->removeLayer(l);
}

void ModelComponentList::updateLegend(const NMLayer* layer)
{
	//NMDebugAI( << "going to update the legend for " << layer->objectName().toStdString()
		//	<< endl);

	QModelIndex idx = this->mLayerModel->getItemLayerModelIndex(layer->objectName());
	if (this->isExpanded(idx))
	{
		this->collapse(idx);
		this->expand(idx);
	}

	this->update(idx);
}

//void
//ModelComponentList::zoomChanged(vtkObject* obj)
//{
//	this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"))->update();
//	NMDebugAI(<< "zoom" << endl);
//}

NMLayer*
ModelComponentList::getSelectedLayer()
{
	NMLayer* l = 0;

	QModelIndexList il = this->selectedIndexes();
	if (il.size() == 0)
		return l;

	const QModelIndex idx = il.at(0);
	const int stackpos = this->mLayerModel->toLayerStackIndex(idx.row());
	l = this->mLayerModel->getItemLayer(stackpos);

	return l;
}

int ModelComponentList::changeLayerPos(int oldpos, int newpos)
{
	// save the collapsed state of all layers
	QList<bool> collapsed;
	for (unsigned int r=0; r < this->mLayerModel->rowCount(QModelIndex()); ++r)
	{
		if (this->isExpanded(this->mLayerModel->index(r, 0, QModelIndex())))
			collapsed.push_back(false);
		else
			collapsed.push_back(true);
	}

	// swap places in collapsed list
	int oldtree = this->mLayerModel->toTreeModelRow(oldpos);
	int newtree = this->mLayerModel->toTreeModelRow(newpos);
	bool tc = collapsed.takeAt(oldtree);
	collapsed.insert(newtree, tc);

	// now swap layer places
	this->mLayerModel->changeItemLayerPos(oldpos, newpos);

	// reinstate layer's expansion setting
	for (unsigned int r=0; r < this->mLayerModel->rowCount(QModelIndex()); ++r)
	{
		QModelIndex id = this->mLayerModel->index(r, 0, QModelIndex());
		if (collapsed.at(r))
			this->collapse(id);
		else
			this->expand(id);
	}

	// update the map display window
	this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"))->update();
	return oldpos;
}


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
	NMDebugCtx(ctx, << "...");

	disconnect(layer, SIGNAL(visibilityChanged(const NMLayer*)),
				this, SLOT(updateMapWin(const NMLayer*)));
	disconnect(layer, SIGNAL(legendChanged(const NMLayer*)),
			this, SLOT(updateLegend(const NMLayer*)));
	disconnect(this, SIGNAL(selectedLayerChanged(const NMLayer *)),
			layer, SLOT(selectedLayerChanged(const NMLayer *)));

	// remove layer from the model (which updates as well the layer position
	// hold by each layer in the layer stack)
	this->mLayerModel->removeLayer(layer);
	this->reset();
	// update the map display window
	this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"))->update();

	NMDebugCtx(ctx, << "done!");
}


void ModelComponentList::updateMapWin(const NMLayer* layer)
{
	// just do the whole window for now

	// TODO: later we have to get the layer's bbox, transform into the window
	// coordinates and then just update this very region of the renderwidget window

	//NMDebugAI( << "updating map window for " << layer->objectName().toStdString() << endl);

	QVTKWidget* qvtk = this->topLevelWidget()->findChild<QVTKWidget*>(tr("qvtkWidget"));
    qvtk->update();
}


void ModelComponentList::addLayer(NMLayer* layer)
{
	NMDebugCtx(ctx, << "...");

	// TODO: check this
	if (layer == 0 || layer->getRenderer() == 0)
	{
		NMDebugAI(<< "invalid layer!" << endl);
		NMDebugCtx(ctx, << "done!");
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
	connect(this, SIGNAL(selectedLayerChanged(const NMLayer *)),
			layer, SLOT(selectedLayerChanged(const NMLayer *)));

	OtbModellerWin* mwin = qobject_cast<OtbModellerWin*>(this->topLevelWidget());
    connect(layer, SIGNAL(notifyLastClickedRow(NMLayer *, double)),
            mwin, SLOT(updateLayerInfo(NMLayer *, double)));
    connect(mwin, SIGNAL(signalIsIn3DMode(bool)),
            layer, SLOT(setIsIn3DMode(bool)));

	// add the layer to the NMLayerModel
	this->mLayerModel->pileItemLayer(layer);
	int nlayers = this->mLayerModel->getItemLayerCount();
	this->recalcMapBBox();

	// get the camera of the background renderer
    //OtbModellerWin* win = qobject_cast<OtbModellerWin*>(this->topLevelWidget());
    vtkRenderer* bkgRen = const_cast<vtkRenderer*>(mwin->getBkgRenderer());

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
	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::recalcMapBBox(void)
{
	NMDebugCtx(ctx, << "...");

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

	NMDebugCtx(ctx, << "done!");
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
	//NMDebugCtx(ctx, << "...")

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
		//NMDebugCtx(ctx, << "done!")
		return;
	}

	// ---------------------------------------------------------------
	// COLLAPSE AND EXPAND CHILD NODES

	if (this->mLayerModel->rowCount(idx) > 0)
	{
		if (this->isExpanded(idx))
			this->collapse(idx);
		else
			this->expand(idx);
	}
	// ----------------------------------------------------------------
	// EDIT CHILD NODES
	else
	{
		const int toplevelrow = (idx.internalId() / 100) - 1;
		const int level = idx.internalId() % 100;
		const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
		NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

		if (level == 1 && idx.row() > 0)
		{
			if ( !(		l->getLegendType() == NMLayer::NM_LEGEND_RAMP
					&&  idx.row() == NM_LEGEND_RAMP_ROW
				  )
			   )
			{
				double rgba[4];
				l->getLegendColour(idx.row(), rgba);

				QString title = QString("Colour for '%1' of %2")
						.arg(l->getLegendName(idx.row()))
						.arg(l->objectName());

				QColor curclr;
				curclr.setRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);
				QColor clr;

                // ToDo: improve! alpha should work with all layers and
                //       legned options!
                if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
                    //                    ||  (   l->getLegendType() == NMLayer::NM_LEGEND_RAMP
                    //                         && l->getTable() != 0
                    //                        )
                    //				   )
				{
					clr = QColorDialog::getColor(curclr, this, title);
				}
				else
				{
					clr = QColorDialog::getColor(curclr, this, title,
							QColorDialog::ShowAlphaChannel);
				}

                //				NMDebugAI(<< "new colour: "
                //						<< clr.redF() << " "
                //						<< clr.greenF() << " "
                //						<< clr.blueF() << " "
                //						<< clr.alphaF() << std::endl);

				rgba[0] = clr.redF();
				rgba[1] = clr.greenF();
				rgba[2] = clr.blueF();
				rgba[3] = clr.alphaF();
				l->setLegendColour(idx.row(), rgba);
			}
			else
			{
				mDelegate->setLastMousePos(event->pos());
				this->edit(idx);
			}
		}
		else if (level == 2)
		{
			NMLayer* l = static_cast<NMLayer*>(idx.data(Qt::UserRole+100).value<void*>());
			QString itemstr = idx.data(Qt::DisplayRole).toString();
			NMDebugAI(<< "trying to edit '" << itemstr.toStdString() << "' of "
					<< l->objectName().toStdString() << std::endl);
			this->edit(idx);
		}
	}

	//NMDebugCtx(ctx, << "done!")
}

//bool ModelComponentList::eventFilter(QObject* obj, QEvent* event)
//{
//
//
//	return false;
//}

void ModelComponentList::mousePressEvent(QMouseEvent *event)
{
    //NMDebugCtx(ctx, << "...");

	int x = event->pos().x();
	int y = event->pos().y();

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
		NMDebugCtx(ctx, << "done!")
		return;
	}

	if (!idx.parent().isValid())
	{
		this->setCurrentIndex(idx);

		if (event->button() == Qt::LeftButton)
		{
			QRect vrect = visualRect(idx);
			int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
			QRect rect1 = QRect(
					header()->sectionViewportPosition(0) + itemIndentation,
					vrect.y(), 16, vrect.height());
			QRect rect2 = QRect(
					header()->sectionViewportPosition(0) + itemIndentation + 17,
					vrect.y(), 16, vrect.height());
			if (rect1.contains(event->pos()))
			{
				this->mLayerModel->setData(idx, QVariant("VIS"), Qt::CheckStateRole);
				this->processSelection(false);
			}
			else if  (rect2.contains(event->pos()))
			{
				this->mLayerModel->setData(idx, QVariant("SEL"), Qt::CheckStateRole);
                this->processSelection(false);
			}
			else
			{
				this->processSelection(true);
			}
			this->dragStartPosition = event->pos();

		}
		else if (event->button() == Qt::RightButton)
		{
            this->processSelection(false);

			this->mMenu->move(event->globalPos());
			this->mMenu->exec();
		}
	}

    //NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::processSelection(bool toggle)
{
	QModelIndex idx = this->currentIndex();

    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

    bool bselect = toggle ? !l->getIsSelected() : l->getIsSelected();
    if (bselect)
	{
		this->selectionModel()->clearSelection();
		this->selectionModel()->select(idx, QItemSelectionModel::Select |
			QItemSelectionModel::Rows);

        l->setIsSelected(true);
		emit selectedLayerChanged(l);
	}
    else
	{
        l->setIsSelected(false);
		this->selectionModel()->select(idx, QItemSelectionModel::Deselect |
			QItemSelectionModel::Rows);
	}
	this->update();
}

void ModelComponentList::mouseMoveEvent(QMouseEvent *event)
{
	//NMDebugCtx(ctx, << "...");

    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;

    QModelIndex idx = this->indexAt(dragStartPosition);
    if (!idx.isValid())
    	return;

    const int tlr = (idx.internalId() / 100) - 1;
    const int sp = this->mLayerModel->toLayerStackIndex(tlr);
    NMLayer* l = this->mLayerModel->getItemLayer(sp);
    if (l == 0)
    	return;

    QString layerName = l->objectName();//idx.data(Qt::DisplayRole).toString();
    //NMLayer* l = (NMLayer*)idx.internalPointer();
    //NMLayer* l = this->getLayer(layerName);

    QIcon layerIcon = l->getLayerIcon();//idx.data(Qt::DecorationRole).value<QIcon>();

    QSize dragImageSize(32,32);
    QImage dragImage(dragImageSize, QImage::Format_ARGB32_Premultiplied);

    QPainter dragPainter(&dragImage);
    dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
    dragPainter.fillRect(dragImage.rect(), Qt::transparent);
    dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    dragPainter.drawPixmap(0,0,32,32, layerIcon.pixmap(dragImageSize));
    dragPainter.end();


    QDrag* drag = new QDrag(this);
    drag->setPixmap(QPixmap::fromImage(dragImage));
    drag->setDragCursor(QPixmap(":move-icon.png"), Qt::CopyAction);


	QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString::fromLatin1("_ModelComponentList_:%1").arg(layerName).toStdString().c_str());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction, Qt::CopyAction);

    //NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::dropEvent(QDropEvent* event)
{
    NMDebugCtx(ctx, << "...");

    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_NMModelScene_")) == 0)
    {
        NMDebugAI(<< "adding layer: " << dropLayer.toStdString() << std::endl);
        NMDataComponent* comp = qobject_cast<NMDataComponent*>(
                    NMModelController::getInstance()->getComponent(dropLayer));
        if (comp == 0)
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }

        QString layerName = dropLayer;
        if (!comp->getDescription().isEmpty())
        {
            layerName = comp->getDescription();
        }

        NMGlobalHelper h;
        vtkRenderWindow* renWin = h.getRenderWindow();
        NMImageLayer* iLayer = new NMImageLayer(renWin, 0, this);
        iLayer->setObjectName(layerName);
        h.getMainWindow()->connectImageLayerProcSignals(iLayer);

        iLayer->setImage(comp->getOutput(0));

        connect(comp, SIGNAL(NMDataComponentChanged()), iLayer, SLOT(updateSourceBuffer()));

    }
    else if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        QModelIndex destidx = this->indexAt(event->pos());
        if (!destidx.isValid() || destidx.parent().isValid())
        {
            this->mIndicatorIdx = QModelIndex();

            NMDebugAI(<< "no valid drop pos!" << endl);
            NMDebugCtx(ctx, << "done!");
            return;
        }

        const int stackpos = this->mLayerModel->toLayerStackIndex(destidx.row());
        //NMLayer* dl = (NMLayer*)destidx.internalPointer();
        NMLayer* dl = this->mLayerModel->getItemLayer(stackpos);
        if (dl == 0)
            return;
        int destpos = dl->getLayerPos();
        //NMDebugAI(<< "dest pos: " << destpos << endl);

        QModelIndex srcidx = this->indexAt(this->dragStartPosition);
        const int srcstackpos = this->mLayerModel->toLayerStackIndex(srcidx.row());
        //NMLayer* sl = (NMLayer*)srcidx.internalPointer();
        NMLayer* sl = this->mLayerModel->getItemLayer(srcstackpos);
        if (sl == 0)
            return;
        int srcpos = sl->getLayerPos();
        //NMDebugAI(<< "src pos: " << srcpos << endl);

        this->changeLayerPos(srcpos, destpos);

        // stop drawing the indicator
        this->mIndicatorIdx = QModelIndex();

        //event->acceptProposedAction();
    }
    else if (event->mimeData()->hasUrls())
    {
        QString fileName;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                fileName = url.toLocalFile();
                break;
            }
        }

        if (!fileName.isEmpty())
        {
            //event->acceptProposedAction();

            QFileInfo finfo(fileName);

            NMGlobalHelper h;
            vtkRenderWindow* renWin = h.getRenderWindow();
            NMImageLayer* fLayer = new NMImageLayer(renWin, 0, this);
            fLayer->setObjectName(finfo.baseName());
            h.getMainWindow()->connectImageLayerProcSignals(fLayer);
            fLayer->setFileName(fileName);
            //QtConcurrent::run(fLayer, &NMImageLayer::setFileName, fileName);

        }
    }
    //event->setAccepted(true);
    NMDebugCtx(ctx, << "done!");

}


void ModelComponentList::dragEnterEvent(QDragEnterEvent* event)
{
    NMDebugCtx(ctx, << "...");

    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        if (this->getLayer(dropLayer) != 0)
        {
            event->acceptProposedAction();
        }
    }
    else if (dropSource.startsWith(QString::fromLatin1("_NMModelScene_")))
    {
        NMModelComponent* comp = NMModelController::getInstance()->getComponent(dropLayer);
        if (comp != 0)
        {
            if (comp->getOutput(0) == 0 || comp->getOutput(0)->getDataObject() == 0)
            {
                NMDebugAI(<< "empty data object!" << std::endl);
            }
            else
            {
                NMDebugAI(<< "got data on the hook!" << std::endl);
            }
            event->acceptProposedAction();
        }
    }
    else if (event->mimeData()->hasUrls())
    {
        QString fileName;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                fileName = url.toLocalFile();
                break;
            }
        }

        if (!fileName.isEmpty())
        {
            NMDebugAI(<< "file dragged into layer component list" << std::endl);
            event->acceptProposedAction();
        }
    }

    NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::dragMoveEvent(QDragMoveEvent* event)
{
    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        QModelIndex id = this->indexAt(event->pos());
        if (!id.isValid() || id.parent().isValid())
        {
            this->mIndicatorIdx = QModelIndex();
            return;
        }
        this->mIndicatorIdx = id;
        this->viewport()->update();
        event->acceptProposedAction();
    }
    else
    {
        if (    dropSource.startsWith(QString::fromLatin1("_NMModelScene_"))
            ||  event->mimeData()->hasUrls()
            )
        {
            event->acceptProposedAction();
        }
    }
}

void ModelComponentList::mouseReleaseEvent(QMouseEvent* event)
{

}

void ModelComponentList::paintEvent(QPaintEvent* event)
{
	// do all normal painting first
	QTreeView::paintEvent(event);

	// add an indicator rectangle, if applicable
	if (this->mIndicatorIdx.isValid())
	{
		QPainter painter(this->viewport());

		QString text = this->mLayerModel->data(this->mIndicatorIdx, Qt::DisplayRole).toString();
		QFont font = this->mLayerModel->data(this->mIndicatorIdx, Qt::FontRole).value<QFont>();

		QRect vrect = visualRect(this->mIndicatorIdx);
		int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
		int checkboxwidth = style()->pixelMetric(QStyle::PM_IndicatorWidth);

		painter.setFont(font);
		QFontMetrics fm = painter.fontMetrics();
		int textwidth = fm.width(text);
		int iconwidth = vrect.width() - itemIndentation - textwidth - checkboxwidth;

		QRect rect = QRect(
				header()->sectionViewportPosition(0) + itemIndentation,
				vrect.y(),
				vrect.width(),// - itemIndentation,
				//checkboxwidth + iconwidth + textwidth,
				vrect.height());

		QImage img(rect.size(), QImage::Format_ARGB32_Premultiplied);
		img.fill(0);
	    QPainter dragPainter(&img);

	    QLinearGradient bw(0,0,0,img.rect().height()*0.5);
	    bw.setSpread(QGradient::ReflectSpread);
	    bw.setColorAt(0, QColor(0,0,200));
	    bw.setColorAt(0.8, QColor(255,255,255));
	    bw.setColorAt(1, QColor(255,255,255));
	    dragPainter.fillRect(img.rect(), bw);
	    dragPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);

	    QLinearGradient fade(0,0,0,img.rect().height()*0.5);
	    fade.setSpread(QGradient::ReflectSpread);
	    fade.setColorAt(0, QColor(0,0,0,127));
	    fade.setColorAt(1, QColor(0,0,0,0));
	    dragPainter.fillRect(img.rect(), fade);

		//painter.setPen(Qt::red);
		//painter.drawRect(rect);
		//painter.setCompositionMode(QPainter::CompositionMode_Overlay);
		//painter.fillRect(rect, QBrush(QColor(255,0,0,80)));
		//painter.setOpacity(0.2);

	    painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(QPen(QBrush(QColor(0,0,200,127)), 3));
		painter.drawImage(rect, img);
		painter.drawRoundedRect(rect, 5,5);
	}
}

void ModelComponentList::zoomToLayer()
{
	// get the current layer
    QModelIndex idx = this->currentIndex();

	const int toplevelrow = (idx.internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	//NMLayer* l = (NMLayer*)idx.internalPointer();

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
	const int toplevelrow = (idx.internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

	l->setLegendType(NMLayer::NM_LEGEND_SINGLESYMBOL);
	l->updateMapping();
}

void ModelComponentList::mapColourTable()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	const int toplevelrow = (idx.internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

	if (l->getTable() == 0)
	{
		NMDebugAI(<< "cannot map the colour table, since we don't even have an attribute table!" << std::endl);
		return;
	}

	if (!l->hasColourTable())
	{
		NMDebugAI(<< "cannot map the colour table, since we didn't find any colour attributes!" << std::endl);
		return;
	}

	l->setLegendType(NMLayer::NM_LEGEND_CLRTAB);
	l->updateMapping();


}

void ModelComponentList::mapColourRamp()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	const int toplevelrow = (idx.internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

	l->setLegendType(NMLayer::NM_LEGEND_RAMP);
	l->updateMapping();
}

void ModelComponentList::mapUniqueValues()
{
	// get the current layer
	QModelIndex idx = this->currentIndex();
	const int toplevelrow = (idx.internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

	NMVectorLayer* vL = 0;
	NMImageLayer* iL = 0;
	vtkDataSet* ds = 0;


	QStringList sFields;
	if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
		vL = qobject_cast<NMVectorLayer*>(l);
		// ToDo:: this probably works for any feature type -> check!
		if (vL->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
			ds = const_cast<vtkDataSet*>(vL->getDataSet());

		vtkDataSetAttributes* dsAttr = 0;
		if (ds != 0)
			dsAttr = ds->GetAttributes(vtkDataSet::CELL);
		else
			return;

		int numFields = dsAttr->GetNumberOfArrays();

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
	}
	else if (l->getLayerType() == NMLayer::NM_IMAGE_LAYER)
	{
		iL = qobject_cast<NMImageLayer*>(l);
		otb::AttributeTable::Pointer rat = iL->getRasterAttributeTable(1);
		if (rat.IsNull())
		{
			return;
		}

		for (unsigned int c=0; c < rat->GetNumCols(); ++c)
		{
			if (rat->GetColumnType(c) == otb::AttributeTable::ATTYPE_INT ||
			    rat->GetColumnType(c) == otb::AttributeTable::ATTYPE_STRING)
			{
				sFields.append(QString(rat->GetColumnName(c).c_str()));
			}
		}
	}
	else
	{
		return;
	}


	bool bOk = false;
	QString theField = QInputDialog::getItem(this,
			tr("Map Unique Values"), tr("Choose an attribute to map"),
			sFields, 0, false, &bOk, 0);
	// if the user pressed cancel
	if (!bOk)
		return;

	l->setLegendType(NMLayer::NM_LEGEND_INDEXED);
	l->setLegendClassType(NMLayer::NM_CLASS_UNIQUE);
	l->setLegendValueField(theField);
	l->updateMapping();
}

void ModelComponentList::test()
{
	NMDebugCtx(ctx, << "...");
	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::showValueStats()
{
	NMDebugCtx(ctx, << "...");

	NMLayer* l = this->getSelectedLayer();
	if (l == 0)
		return;

	std::vector<double> stats = l->getValueFieldStatistics();
	if (stats.size() == 0)
	{
		NMErr(ctx, << "failed retrieving value field statistics!");
		NMDebugCtx(ctx, << "done!");
		return;
	}

	NMDebugAI(<< "what we've got: ..." << std::endl);
	for (int r=0; r < stats.size(); ++r)
	{
		NMDebugAI(<< "#" << r << ": " << stats[r] << std::endl);
	}


	// min, max, mean, std. dev, sum
	QString smin  = QString("%1").arg(stats[0], 0, 'f', 4); //smin  = smin .rightJustified(15, ' ');
	QString smax  = QString("%1").arg(stats[1], 0, 'f', 4); //smax  = smax .rightJustified(15, ' ');
	QString smean = QString("%1").arg(stats[2], 0, 'f', 4); //smean = smean.rightJustified(15, ' ');
	QString smedi = QString("%1").arg(stats[3], 0, 'f', 4);
	QString ssdev = QString("%1").arg(stats[4], 0, 'f', 4); //ssdev = ssdev.rightJustified(15, ' ');
	QString ssum  = QString("%1").arg(stats[6], 0, 'f', 4); //ssum  = ssum .rightJustified(15, ' ');
	QString ssample = QString("Sample Size: %1").arg(stats[5]);

	QString strmin  ("Minimum: ");  //strmin  = strmin .rightJustified(10, ' ');
	QString strmax  ("Maximum: ");  //strmax  = strmax .rightJustified(10, ' ');
	QString strmean ("Mean: ");     //strmean = strmean.rightJustified(10, ' ');
	QString strmedi ("Median: ");
	QString strsdev ("Std.Dev.: "); //strsdev = strsdev.rightJustified(10, ' ');
	QString strsum  ("Sum: ");      //strsum  = strsum .rightJustified(10, ' ');


	QString res = QString("%1%2\n%3%4\n%5%6\n%7%8\n%9%10\n%11%12\n%13")
			.arg(strmin).arg(smin)
			.arg(strmax).arg(smax)
			.arg(strmean).arg(smean)
			.arg(strmedi).arg(smedi)
			.arg(strsdev).arg(ssdev)
			.arg(strsum).arg(ssum)
			.arg(ssample);

	QString title = QString(tr("%1")).arg(l->getLegendValueField());

	QMessageBox::information(this, title, res);


	NMDebugCtx(ctx, << "done!");
}
