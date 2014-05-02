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
 * NMModelViewWidget.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QString>
#include <QGraphicsItem>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>
#include <QDockWidget>
#include <QWidget>
#include <QSplitter>

#include "otbmodellerwin.h"
#include "NMModelViewWidget.h"
#include "NMProcess.h"
#include "NMProcessFactory.h"
#include "NMModelComponentFactory.h"
#include "NMModelSerialiser.h"
#include "NMDataComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMConditionalIterComponent.h"
#include "NMComponentEditor.h"


NMModelViewWidget::NMModelViewWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f), mbControllerIsBusy(false), mScaleFactor(1.075)
{
	ctx = "NMModelViewWidget";
	this->setAcceptDrops(true);
	this->mLastItem = 0;
	
#ifdef BUILD_RASSUPPORT
        this->mRasConn = 0;
#endif


    /* ====================================================================== */
    /* MODEL CONTROLLER SETUP */
    /* ====================================================================== */
    mModelRunThread = new QThread(this);
    connect(this, SIGNAL(widgetIsExiting()), mModelRunThread, SLOT(quit()));

    mModelRunThread->start();

 	mModelController = NMModelController::getInstance();
 	mModelController->moveToThread(mModelRunThread);

 	connect(this, SIGNAL(requestModelExecution(const QString &)),
 			mModelController, SLOT(executeModel(const QString &)));
 	connect(this, SIGNAL(requestModelReset(const QString &)),
 			mModelController, SLOT(resetComponent(const QString &)));
 	connect(this, SIGNAL(requestModelAbortion()),
 			mModelController, SLOT(abortModel()), Qt::DirectConnection);
 	connect(mModelController, SIGNAL(signalIsControllerBusy(bool)),
 			this, SLOT(reportIsModelControllerBusy(bool)));

 	mRootComponent = new NMSequentialIterComponent();
	mRootComponent->setObjectName("root");
	mRootComponent->setDescription(
			"Top level model component managed by the model view widget");
	this->mModelController->addComponent(
			qobject_cast<NMModelComponent*>(mRootComponent));
	connect(mRootComponent, SIGNAL(NMModelComponentChanged()), this, SLOT(compProcChanged()));


    /* ====================================================================== */
    /* GET THE RASDAMAN CONNECTOR FROM THE MAIN WINDOW */
	/* ====================================================================== */
    OtbModellerWin* mainWin = this->getMainWindow();

#ifdef BUILD_RASSUPPORT	
	this->mRasConn = new NMRasdamanConnectorWrapper(this);
	if (   mainWin != 0
		&& mainWin->getRasdamanConnector() != 0)
	{
		this->mRasConn->setConnector(mainWin->getRasdamanConnector());
		NMDebugAI(<< ctx << ": successfully set the rasdaman connector!"<< std::endl);
	}
	else
	{
		NMDebugAI(<< ctx << ": failed setting the rasdaman connector!"<< std::endl);
	}
#endif	

    /* ====================================================================== */
    /* MODEL SCENE SETUP */
	/* ====================================================================== */
	mModelScene = new NMModelScene(this);
    //mModelScene->setSceneRect(-5000,-5000,8000,8000);
	mModelScene->setItemIndexMethod(QGraphicsScene::NoIndex);
	connect(this, SIGNAL(linkToolToggled(bool)), mModelScene,
			SLOT(toggleLinkToolButton(bool)));
	connect(this, SIGNAL(selToolToggled(bool)), mModelScene,
			SLOT(toggleSelToolButton(bool)));
	connect(this, SIGNAL(moveToolToggled(bool)), mModelScene,
			SLOT(toggleMoveToolButton(bool)));

	connect(mModelScene, SIGNAL(itemRightBtnClicked(QGraphicsSceneMouseEvent *,
				QGraphicsItem *)), this, SLOT(callItemContextMenu(QGraphicsSceneMouseEvent *,
				QGraphicsItem *)));
	connect(mModelScene, SIGNAL(linkItemCreated(NMComponentLinkItem*)),
			this, SLOT(linkProcessComponents(NMComponentLinkItem*)));
	connect(mModelScene, SIGNAL(processItemCreated(NMProcessComponentItem*,
			const QString &, QPointF)),
			this, SLOT(createProcessComponent(NMProcessComponentItem*,
			const QString &, QPointF)));
	connect(mModelScene, SIGNAL(rootComponentDblClicked()),
			this, SLOT(editRootComponent()));
	connect(mModelScene, SIGNAL(procAggregateCompDblClicked(const QString &)),
			this, SLOT(callEditComponentDialog(const QString &)));
    connect(mModelScene, SIGNAL(zoom(int)), this, SLOT(zoom(int)));
    connect(mModelScene, SIGNAL(itemLeftClicked(const QString &)), this,
            SLOT(updateTreeEditor(const QString &)));

	/* ====================================================================== */
    /* MODEL VIEW SETUP */
	/* ====================================================================== */
	mModelView = new QGraphicsView(mModelScene, this);
	mModelView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //mModelView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	mModelView->setCacheMode(QGraphicsView::CacheBackground);
	mModelView->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
	mModelView->setDragMode(QGraphicsView::ScrollHandDrag);
	mModelView->setRenderHint(QPainter::Antialiasing, true);
	mModelView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    mModelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mModelView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mModelView->viewport()->installEventFilter(this);

    mTreeCompEditor = 0;

    /* ====================================================================== */
    /* WIDGET BUTTON AND CONTEXT MENU SETUP */
	/* ====================================================================== */
    //	QPushButton* btnCancel = new QPushButton();
    //	btnCancel->setText(tr("Stop Execution"));
    //	btnCancel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    //	QPushButton* btnExec = new QPushButton();
    //	btnExec->setText(tr("Execute"));
    //	btnExec->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    //	QHBoxLayout* boxLayout = new QHBoxLayout();
    //	boxLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    //	boxLayout->addWidget(btnCancel);
    //	boxLayout->addWidget(btnExec);


	QVBoxLayout* gridLayout = new QVBoxLayout();
	gridLayout->addWidget(mModelView);//, 0, 0);
    //gridLayout->addLayout(boxLayout);//, 1, 0);

	this->setLayout(gridLayout);

	// connect buttons
    //	connect(btnExec, SIGNAL(clicked()), this, SLOT(executeModel()));
    //	connect(btnCancel, SIGNAL(clicked()), this, SIGNAL(requestModelAbortion()));

	this->initItemContextMenu();

	qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
}

NMModelViewWidget::~NMModelViewWidget()
{
	if (mModelRunThread != 0)
	{
		if (this->mbControllerIsBusy)
		{
			emit requestModelAbortion();
			this->thread()->wait(10000);
		}
		emit widgetIsExiting();
		mModelRunThread->wait();
	}
}

void
NMModelViewWidget::reportIsModelControllerBusy(bool busy)
{
	this->mbControllerIsBusy = busy;
}

void
NMModelViewWidget::createSequentialIterComponent()
{
	this->createAggregateComponent("NMSequentialIterComponent");
}

void
NMModelViewWidget::createConditionalIterComponent()
{
	this->createAggregateComponent("NMConditionalIterComponent");
}

void NMModelViewWidget::createAggregateComponent(const QString& compType)
{
	NMDebugCtx(ctx, << "...");

	// get the selected items
	QList<QGraphicsItem*> selItems = this->mModelScene->selectedItems();
	if (selItems.count() < 2)
	{
		NMDebugAI(<< "grouping one single component does not make sense!" << endl);
		return;
	}

	QListIterator<QGraphicsItem*> it(selItems);
	QList<NMModelComponent*> selComps;

	// get the host component of the selected items
	NMDebugAI(<< "selected items ..." << std::endl);
	NMIterableComponent* host = 0;
	unsigned int compCounter = 0;
	while(it.hasNext())
	{
		NMProcessComponentItem* procItem = 0;
		NMAggregateComponentItem* aggrItem = 0;
		QGraphicsItem* item = it.next();

		procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

		NMModelComponent* comp = 0;
		QString itemTitle;
		if (procItem != 0)
		{
			itemTitle = procItem->getTitle();
			comp = this->mModelController->getComponent(itemTitle);
		}
		else if (aggrItem != 0)
		{
			itemTitle = aggrItem->getTitle();
			comp = this->mModelController->getComponent(itemTitle);
		}

		if (comp != 0)
		{
			if (compCounter == 0)
				host = comp->getHostComponent();
			selComps.push_back(comp);
		}
		else
		{
			NMDebugAI(<< "oops! The model controller couldn't find the component for '"
					<< itemTitle.toStdString() << "'!" << endl);
			continue;
		}

		// we only consider items sharing the same level
		if (host != comp->getHostComponent())
		{
			NMDebugAI(<< "all components have to be on the same level, i.e. sharing the same host!"
					<< endl);
			//NMDebugAI(<< "ignoring component '" << comp->objectName().toStdString()
			//		<< "', which is not on the grouping level!" << endl);
			return;
		}

		++compCounter;
	}
	if (host == 0)
		host = this->mRootComponent;

	NMDebugAI(<< "identified host of new aggrComp is '" << host->objectName().toStdString() << "'"
			<< std::endl);

	// remove the selected components from the their current host item
	NMDebugAI(<< "removing sel comps from previous host ..." << std::endl);
	QListIterator<NMModelComponent*> itComp(selComps);
	NMModelComponent* tcomp = 0;
	while(itComp.hasNext())
	{
		tcomp = itComp.next();
		host->removeModelComponent(tcomp->objectName());
	}

	// now we create a new ModelComponent, and add the selected components
	NMIterableComponent* aggrComp =
			qobject_cast<NMIterableComponent*>(
					NMModelComponentFactory::instance().createModelComponent(compType));
	aggrComp->setObjectName("AggrComp");

	// set the new aggregated component on time level host_level + 1
	aggrComp->setTimeLevel(host->getTimeLevel() + 1);

	connect(aggrComp, SIGNAL(NMModelComponentChanged()), this, SLOT(compProcChanged()));

	// ToDo: have to check the order in which 'selected items' returns the elements
	// ie is it according to z-order or order of selection?
	tcomp = 0;
	itComp.toFront();
	while(itComp.hasNext())
	{
		tcomp = itComp.next();
		NMDebugAI(<< "adding to aggregate " << tcomp->objectName().toStdString() << std::endl);
		aggrComp->addModelComponent(tcomp);

		if (this->mOpenEditors.contains(tcomp))
			this->mOpenEditors.value(tcomp)->update();
	}

	// now we add the new aggregate component to the identified host
	// component of the model
	QString newAggrItemName = this->mModelController->addComponent(
			qobject_cast<NMModelComponent*>(aggrComp),
			qobject_cast<NMModelComponent*>(host));

	if (this->mOpenEditors.contains(host))
		this->mOpenEditors.value(host)->update();

	// get the host item -> ask the scene it should know
	QGraphicsItem* hostItem = this->mModelScene->getComponentItem(host->objectName());

	// create the new aggregate component item, and add all selected children
	NMAggregateComponentItem* aggrItem = new NMAggregateComponentItem(hostItem);
	aggrItem->setTitle(newAggrItemName);
	it.toFront();
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		NMProcessComponentItem* pItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

		if (pItem != 0)
		{
			aggrItem->addToGroup(pItem);
		}
		else if (aItem != 0)
		{
			aggrItem->addToGroup(aItem);
		}
	}

	// let the individual members of an aggregate component handle their
	// events themselves by default
	aggrItem->setHandlesChildEvents(false);

	// finally add the new group component item itself to the scene
	this->mModelScene->addItem(aggrItem);
	this->mModelScene->invalidate();

	NMDebugCtx(ctx, << "done!");
}

void NMModelViewWidget::initItemContextMenu()
{
	this->mItemContextMenu = new QMenu(this);

	QAction* runComp = new QAction(this->mItemContextMenu);
	runComp->setText(tr("Execute"));
	runComp->setEnabled(false);
	this->mActionMap.insert("Execute", runComp);

	QAction* resetComp = new QAction(this->mItemContextMenu);
	resetComp->setText(tr("Reset"));
	resetComp->setEnabled(false);
	this->mActionMap.insert("Reset", resetComp);

	QAction* groupSeqItems = new QAction(this->mItemContextMenu);
	QString groupSeqItemText = "Create Sequential Group";
	groupSeqItems->setEnabled(false);
	groupSeqItems->setText(groupSeqItemText);
	this->mActionMap.insert(groupSeqItemText, groupSeqItems);

	QAction* groupCondItems = new QAction(this->mItemContextMenu);
	QString groupCondItemText = "Create Conditional Group";
	groupCondItems->setEnabled(false);
	groupCondItems->setText(groupCondItemText);
	this->mActionMap.insert(groupCondItemText, groupCondItems);

	QAction* ungroupItems = new QAction(this->mItemContextMenu);
	QString ungroupItemsText = "Ungroup Components";
	ungroupItems->setEnabled(false);
	ungroupItems->setText(ungroupItemsText);
	this->mActionMap.insert(ungroupItemsText, ungroupItems);

	QAction* delComp = new QAction(this->mItemContextMenu);
	delComp->setText(tr("Delete"));
	this->mActionMap.insert("Delete", delComp);

	QAction* saveComp = new QAction(this->mItemContextMenu);
	saveComp->setText(tr("Save As ..."));
	saveComp->setEnabled(false);
	this->mActionMap.insert("Save As ...", saveComp);

	QAction* loadComp = new QAction(this->mItemContextMenu);
	loadComp->setText(tr("Load ..."));
	this->mActionMap.insert("Load ...", loadComp);

	this->mItemContextMenu->addAction(runComp);
	this->mItemContextMenu->addAction(resetComp);
	this->mItemContextMenu->addSeparator();
	this->mItemContextMenu->addAction(groupSeqItems);
	this->mItemContextMenu->addAction(groupCondItems);
	this->mItemContextMenu->addAction(ungroupItems);
	this->mItemContextMenu->addSeparator();
	this->mItemContextMenu->addAction(delComp);
	this->mItemContextMenu->addSeparator();
	this->mItemContextMenu->addAction(loadComp);
	this->mItemContextMenu->addAction(saveComp);

	connect(runComp, SIGNAL(triggered()), this, SLOT(executeModel()));
	connect(resetComp, SIGNAL(triggered()), this, SLOT(resetModel()));
	connect(delComp, SIGNAL(triggered()), this, SLOT(deleteItem()));
	connect(groupSeqItems, SIGNAL(triggered()), this, SLOT(createSequentialIterComponent()));
	connect(groupCondItems, SIGNAL(triggered()), this, SLOT(createConditionalIterComponent()));
	connect(ungroupItems, SIGNAL(triggered()), this, SLOT(ungroupComponents()));
	connect(saveComp, SIGNAL(triggered()), this, SLOT(saveItems()));
	connect(loadComp, SIGNAL(triggered()), this, SLOT(loadItems()));
}

void NMModelViewWidget::callItemContextMenu(QGraphicsSceneMouseEvent* event,
		QGraphicsItem* item)
{
	bool running = NMModelController::getInstance()->isModelRunning();

	this->mLastScenePos = event->scenePos();
	this->mLastItem = item;

	// Execute && Reset model
	if (((item != 0 && item->type() != NMComponentLinkItem::Type)
		&& !running) || !running)
	{
		this->mActionMap.value("Execute")->setEnabled(true);
		this->mActionMap.value("Reset")->setEnabled(true);
	}
	else
	{
		this->mActionMap.value("Execute")->setEnabled(false);
		this->mActionMap.value("Reset")->setEnabled(false);
	}

	// GROUP
	QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();
	if (selection.count() > 1 && !running)
	{
		int levelIndi = this->shareLevel(selection);
		if (levelIndi >= 0)
		{
			this->mActionMap.value("Create Sequential Group")->setEnabled(true);
			//this->mActionMap.value("Create Conditional Group")->setEnabled(true);
			if (levelIndi > 0)
				this->mActionMap.value("Ungroup Components")->setEnabled(true);
			else
				this->mActionMap.value("Ungroup Components")->setEnabled(false);
		}
		else
		{
			this->mActionMap.value("Create Sequential Group")->setEnabled(false);
			this->mActionMap.value("Create Conditional Group")->setEnabled(false);
			this->mActionMap.value("Ungroup Components")->setEnabled(false);
		}
	}
	else
	{
		this->mActionMap.value("Create Sequential Group")->setEnabled(false);
		this->mActionMap.value("Create Conditional Group")->setEnabled(false);
		this->mActionMap.value("Ungroup Components")->setEnabled(false);
	}

	// DELETE & SAVE AS
	if ((selection.count() > 0 || item != 0) && !running)
		this->mActionMap.value("Delete")->setEnabled(true);
	else
		this->mActionMap.value("Delete")->setEnabled(false);

	// SAVE & LOAD
	if (item != 0 && item->type() != NMComponentLinkItem::Type)
        this->mActionMap.value("Save As ...")->setEnabled(true);
	else
        this->mActionMap.value("Save As ...")->setEnabled(false);

	if (!running)
		this->mActionMap.value("Load ...")->setEnabled(true);
	else
		this->mActionMap.value("Load ...")->setEnabled(false);

	QPoint viewPt = this->mModelView->mapFromScene(this->mLastScenePos);
	this->mItemContextMenu->move(mModelView->mapToGlobal(viewPt));
	this->mItemContextMenu->exec();
}

void
NMModelViewWidget::deleteProcessComponentItem(NMProcessComponentItem* procItem)
{
	NMDebugCtx(ctx, << "...");


	// remove all outgoing links to other components
	QList<NMComponentLinkItem*> outLinks = procItem->getOutputLinks();
	NMDebugAI(<< "found " << outLinks.size() << " outlinks" << std::endl);

	for(unsigned int o=0; o < outLinks.size(); ++o)
	{
		NMComponentLinkItem* link = outLinks[o];
		NMProcessComponentItem* target = link->targetItem();
		target->removeLink(link);
		NMModelComponent* tcomp = mModelController->getComponent(target->getTitle());
		if (tcomp != 0)
		{
			NMIterableComponent* itComp =
					qobject_cast<NMIterableComponent*>(tcomp);
			if (itComp != 0 && itComp->getProcess() != 0)
				itComp->getProcess()->removeInputComponent(procItem->getTitle());
		}

		mModelScene->removeItem(link);
		delete link;
		link = 0;

		if (this->mOpenEditors.contains(tcomp))
			this->mOpenEditors.value(tcomp)->update();
	}

	// remove all incoming links
	QList<NMComponentLinkItem*> inLinks = procItem->getInputLinks();
	NMDebugAI(<< "found " << inLinks.size() << " inlinks" << std::endl);
	for(unsigned int i=0; i < inLinks.size(); ++i)
	{
		NMComponentLinkItem* link = inLinks[i];
		NMProcessComponentItem* source = link->sourceItem();
		source->removeLink(link);
		mModelScene->removeItem(link);
		delete link;
		link = 0;
	}

	// check, whether the component is the last
	// process component in this model component
	NMModelComponent* procComp = mModelController->getComponent(procItem->getTitle());
	if (procComp == 0)
	{
		NMErr(ctx, << "something has gone utterly wrong! The component which is supposed"
				<< " to be deleted, is not controlled by this controller!");
		NMDebugCtx(ctx, << "done!");
		return;
	}
	NMIterableComponent* hostComp = procComp->getHostComponent();

	// finally remove the component itself
	mModelScene->removeItem(procItem);
	if (!mModelController->removeComponent(procItem->getTitle()))
	{
		NMErr(ctx, << "failed to remove component '"
				<< procItem->getTitle().toStdString() << "'!");
	}

	this->deleteEmptyComponent(hostComp);

	NMDebugCtx(ctx, << "done!");
}


void NMModelViewWidget::getSubComps(NMModelComponent* comp, QStringList& subs)
{
	NMIterableComponent* itComp =
			qobject_cast<NMIterableComponent*>(comp);
	if (itComp == 0)
		return;

	NMModelComponent* ic = itComp->getInternalStartComponent();
	while(ic != 0)
	{
		if (!subs.contains(ic->objectName()))
		{
			subs.push_back(ic->objectName());
			NMIterableComponent* itic =
					qobject_cast<NMIterableComponent*>(ic);
			if (itic != 0 && itic->getProcess() == 0)
				this->getSubComps(itic, subs);
			ic = itComp->getNextInternalComponent();
		}
	}
}

void NMModelViewWidget::saveItems(void)
{
	NMDebugCtx(ctx, << "...");
	// we save either all selected items, the item under the mouse pointer
	// or all items of the scene
	QList<QGraphicsItem*> items = this->mModelScene->selectedItems();
	QStringList savecomps;
	if (items.isEmpty())
	{
		if (!this->mModelScene->items().isEmpty())
		{
			items.append(this->mModelScene->items());
			savecomps.push_back("root");
		}
	}

	if (items.count() == 0)
		return;

	// create a list of all components to be saved
	foreach(QGraphicsItem* item, items)
	{
		NMModelComponent* comp = this->componentFromItem(item);
		if (comp != 0)
		{
			if (!savecomps.contains(comp->objectName()))
			{
				savecomps.push_back(comp->objectName());
				NMIterableComponent* itComp =
						qobject_cast<NMIterableComponent*>(comp);
				if (itComp != 0 && itComp->getProcess() == 0)
					this->getSubComps(itComp, savecomps);
			}
		}
	}

	NMDebugAI(<< "save comps '" << savecomps.join(" ").toStdString() << "'" << endl);

	//   keep track of all written links
	QList<NMComponentLinkItem*> writtenLinks;


	QFileDialog dlg(this);
	dlg.setAcceptMode(QFileDialog::AcceptSave);
	dlg.setFileMode(QFileDialog::AnyFile);
	dlg.setWindowTitle(tr("Save Model Component(s)"));
	dlg.setDirectory("~/");
    dlg.setNameFilter("LUMASS Model Component Files (*.lmx *.lmv)");

	QString fileNameString;
	if (dlg.exec())
	{
		fileNameString = dlg.selectedFiles().at(0);
		if (fileNameString.isNull() || fileNameString.isEmpty())
			return;
	}
	else
		return;

	QFileInfo fi(fileNameString);
	QString fnLmx = QString("%1/%2.lmx").arg(fi.absolutePath()).arg(fi.baseName());
	QString fnLmv = QString("%1/%2.lmv").arg(fi.absolutePath()).arg(fi.baseName());

	NMDebugAI(<< "absolutePath: " << fi.absolutePath().toStdString() << endl);
	NMDebugAI(<< "baseName:     " << fi.baseName().toStdString() << endl);
	NMDebugAI(<< "model file:   " << fnLmx.toStdString() << endl);
	NMDebugAI(<< "view file:    " << fnLmv.toStdString() << endl);

	QFile fileLmv(fnLmv);
	if (!fileLmv.open(QIODevice::WriteOnly))
	{
		NMErr(ctx, << "unable to open/create file '" << fnLmv.toStdString()
				<< "'!");
		NMDebugCtx(ctx, << "done!");
		return;
	}
	QDataStream lmv(&fileLmv);
	lmv.setVersion(12);

	int cnt = 0;
	bool append = false;
	NMModelSerialiser xmlS;
	foreach(QString itemName, savecomps)
	{
		if (cnt == 0)
			append = false;
		else
			append = true;
		++cnt;

		QGraphicsItem* item = this->mModelScene->getComponentItem(itemName);
		NMModelComponent* comp;
		NMProcessComponentItem* pi;
		NMAggregateComponentItem* ai;
		NMComponentLinkItem* li;

		// we treat root differently, because it hasn't got an item
		// associated with it
		if (itemName.compare("root") == 0)
		{
			comp = this->mModelController->getComponent("root");
			xmlS.serialiseComponent(comp, fnLmx, 4, false);
			continue;
		}

		switch (item->type())
		{
		case NMProcessComponentItem::Type:
			pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
			comp = this->mModelController->getComponent(pi->getTitle());
			if (comp == 0)
			{
				NMErr(ctx, << "couldn't write '" << pi->getTitle().toStdString() << "' - skip it!");
				return;
			}
			xmlS.serialiseComponent(comp, fnLmx, 4, append);
			lmv << (qint32)NMProcessComponentItem::Type;
			lmv << *pi;

			// save links
			{
				QList<NMComponentLinkItem*> inputLinks = pi->getInputLinks();
				foreach(NMComponentLinkItem* link, inputLinks)
				{
					if (!writtenLinks.contains(link))
					{
						writtenLinks.push_back(link);
						if (savecomps.contains(link->sourceItem()->getTitle()) &&
							savecomps.contains(link->targetItem()->getTitle()))
						{
							lmv << (qint32)NMComponentLinkItem::Type;
							lmv << *link;
						}
					}
				}

				QList<NMComponentLinkItem*> outputLinks = pi->getOutputLinks();
				foreach(NMComponentLinkItem* link, outputLinks)
				{
					if (!writtenLinks.contains(link))
					{
						writtenLinks.push_back(link);
						if (savecomps.contains(link->sourceItem()->getTitle()) &&
							savecomps.contains(link->targetItem()->getTitle()))
						{
							lmv << (qint32)NMComponentLinkItem::Type;
							lmv << *link;
						}
					}
				}
			}
			break;

		case NMAggregateComponentItem::Type:
			ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
			comp = this->mModelController->getComponent(ai->getTitle());
			if (comp == 0)
			{
				NMErr(ctx, << "couldn't write '" << ai->getTitle().toStdString() << "' - skip it!");
				return;
			}
			xmlS.serialiseComponent(comp, fnLmx, 4, append);
			lmv << (qint32)NMAggregateComponentItem::Type;
			lmv << *ai;
			break;

		default:
			break;
		}
	}

	fileLmv.close();

	NMDebugCtx(ctx, << "done!");
}

void NMModelViewWidget::loadItems(void)
{
	QString fileNameString = QFileDialog::getOpenFileName(this,
	     tr("Load LUMASS Model Component"), "~", tr("LUMASS Model Component Files (*.lmx *.lmv)"));
	if (fileNameString.isNull()) return;

	QFileInfo fi(fileNameString);
	QString fnLmx = QString("%1/%2.lmx").arg(fi.absolutePath()).arg(fi.baseName());
	QString fnLmv = QString("%1/%2.lmv").arg(fi.absolutePath()).arg(fi.baseName());

	// read the data model
	QMap<QString, QString> nameRegister;
	NMModelSerialiser xmlS;

#ifdef BUILD_RASSUPPORT	
	nameRegister = xmlS.parseComponent(fnLmx, this->mModelController, *this->mRasConn);
#else
    nameRegister = xmlS.parseComponent(fnLmx, this->mModelController);
#endif

	QFile fileLmv(fnLmv);
	if (!fileLmv.open(QIODevice::ReadOnly))
	{
		NMErr(ctx, << "unable to open file '" << fnLmv.toStdString()
				<< "'!");
		NMDebugCtx(ctx, << "done!");
		return;
	}
	QDataStream lmv(&fileLmv);
	if (lmv.version() < 12)
		return;

	NMDebugAI(<< "reading model view file ..." << endl);
	NMDebugAI(<< "---------------------------" << endl);
	NMDataComponent* dataComp = 0;
	NMIterableComponent* itComp = 0;
	NMProcess* procComp;
	while(!lmv.atEnd())
	{
		qint32 readType;
		lmv >> readType;
		//NMDebugAI(<< "item type is: " << (int)readType << endl);

		NMProcessComponentItem* pi;
		NMAggregateComponentItem* ai;
		NMComponentLinkItem* li;
		switch(readType)
		{
		case (qint32)NMProcessComponentItem::Type:
				{
					pi = new NMProcessComponentItem(0, this->mModelScene);
					lmv >> *pi;
					QString itemTitle = nameRegister.value(pi->getTitle());
					pi->setTitle(itemTitle);

					if (!pi->getIsDataBufferItem())
					{
						// establish across-thread-communication between GUI item and process component
						itComp = qobject_cast<NMIterableComponent*>(
								this->mModelController->getComponent(itemTitle));

						if (itComp != 0 && itComp->getProcess() == 0)
						{
							NMErr(ctx, << "Couldn't find the process component for item '"
									<< itemTitle.toStdString() << "'!");
						}
						pi->setDescription(itComp->getDescription());

						procComp = itComp->getProcess();
						this->connectProcessItem(procComp, pi);
					}
					else
					{
						NMModelComponent* mcomp = this->mModelController->getComponent(itemTitle);
						connect(mcomp, SIGNAL(ComponentDescriptionChanged(const QString &)),
								pi, SLOT(updateDescription(const QString &)));
						pi->setIsDataBufferItem(true);
						pi->setDescription(mcomp->getDescription());

					}
					pi->setFlag(QGraphicsItem::ItemIsMovable, true);
					this->mModelScene->addItem(pi);
				}
				break;

		case (qint32)NMAggregateComponentItem::Type:
				{
					ai = new NMAggregateComponentItem(0);

					QString title;
					QPointF pos;
					QColor color;
					qint32 nkids;
					lmv >> title >> pos >> color >> nkids;

					ai->setTitle(nameRegister.value(title));
					ai->setPos(pos);
					ai->setColor(color);

					for (unsigned v=0; v < nkids; ++v)
					{
						QString dummy;
						lmv >> dummy;
					}

					this->mModelScene->addItem(ai);
				}

				break;

		case (qint32)NMComponentLinkItem::Type:
				{
					int srcIdx, tarIdx;
					QString srcName, tarName;
					lmv >> srcIdx >> srcName >> tarIdx >> tarName;
				}
				break;

		default:
			break;
		}
	}

	// - establish links:
	//   since we don't know the order of components in the file
	//   because we didn't know it as we were saving the file
	//   we have to establish all links at the very end to make
	//   sure that all source/target items have already been
	//   processed and are available!
	// - establish group components
	fileLmv.seek(0);
	lmv.resetStatus();
	while(!lmv.atEnd())
	{
		qint32 readType;
		lmv >> readType;
		NMDebugAI(<< "item type is: " << (int)readType << endl);

		NMProcessComponentItem* pi;
		NMAggregateComponentItem* ai;
		NMComponentLinkItem* li;
		switch (readType)
		{
		case (qint32)NMComponentLinkItem::Type:
				{
					int srcIdx, tarIdx;
					QString srcName, tarName;
					lmv >> srcIdx >> srcName >> tarIdx >> tarName;

					srcName = nameRegister.value(srcName);
					tarName = nameRegister.value(tarName);

					NMProcessComponentItem* si =
							qgraphicsitem_cast<NMProcessComponentItem*>(
								this->mModelScene->getComponentItem(srcName));
					NMProcessComponentItem* ti =
							qgraphicsitem_cast<NMProcessComponentItem*>(
								this->mModelScene->getComponentItem(tarName));
					if (si != 0 && ti != 0)
					{
						li = new NMComponentLinkItem(si, ti, 0);
						li->setZValue(this->mModelScene->getLinkZLevel());
						si->addOutputLink(srcIdx, li);
						ti->addInputLink(tarIdx, li);
						this->mModelScene->addItem(li);
					}
					else
					{
						NMErr(ctx, << "failed linking '"
								<< srcName.toStdString() << "' with '"
								<< tarName.toStdString() << "'!");
					}
				}
				break;
		case (qint32)NMProcessComponentItem::Type:
				pi = new NMProcessComponentItem(0,0);
				lmv >> *pi;
				break;

		case (qint32)NMAggregateComponentItem::Type:
				{
					QString title;
					QPointF pos;
					QColor color;
					qint32 nkids;
					lmv >> title >> pos >> color >> nkids;

					title = nameRegister.value(title);
					ai = qgraphicsitem_cast<NMAggregateComponentItem*>(
							this->mModelScene->getComponentItem(title));

					if (ai == 0 || nkids < 1)
						break;

					NMProcessComponentItem* ipi = 0;
					NMAggregateComponentItem* iai = 0;
					for (unsigned int c=0; c < nkids; ++c)
					{
						QString kname;
						lmv >> kname;

						kname = nameRegister.value(kname);
						ipi = qgraphicsitem_cast<NMProcessComponentItem*>(
								this->mModelScene->getComponentItem(kname));
						iai = qgraphicsitem_cast<NMAggregateComponentItem*>(
								this->mModelScene->getComponentItem(kname));

						if (ipi != 0)
							ai->addToGroup(ipi);
						else if (iai != 0)
							ai->addToGroup(iai);
					}
					ai->setHandlesChildEvents(false);
				}
				break;

		default:
			break;
		}
	}

	this->mModelScene->invalidate();
}

void
NMModelViewWidget::zoomToContent()
{
    // go through the actual content of the scene, and determien the
    // convex hull of all items and set that as the scenes rect

    this->mModelScene->setSceneRect(this->mModelScene->itemsBoundingRect());
    this->mModelView->fitInView(this->mModelScene->sceneRect(),
                                Qt::KeepAspectRatio);
}


int NMModelViewWidget::shareLevel(QList<QGraphicsItem*> list)
{
	// check, whether all items belong to the same host
	bool allLevel = true;
	NMIterableComponent* host = 0;
	NMModelComponent* comp = 0;
	int cnt = 0;
	foreach(QGraphicsItem* item, list)
	{
		NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
		if (pi != 0)
			comp = this->mModelController->getComponent(pi->getTitle());
		else if (ai != 0)
			comp = this->mModelController->getComponent(ai->getTitle());

		if (cnt == 0)
			host = comp->getHostComponent();

		if (host != comp->getHostComponent())
		{
			allLevel = false;
			break;
		}
		++cnt;
	}

	int ret = -1;
	if (allLevel)
	{
		if (host != this->mRootComponent)
			ret = 1;
		else
			ret = 0;
	}

	return ret;
}

void NMModelViewWidget::ungroupComponents()
{
	NMDebugCtx(ctx, << "...");
	QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();
	if (selection.count() == 0)
		return;

	QGraphicsItem* item = selection.at(0);
	NMModelComponent* comp = this->componentFromItem(item);
	if (comp == 0)
	{
		NMDebugCtx(ctx, << "done!");
		return;
	}
	NMIterableComponent* host = comp->getHostComponent();
	if (host == 0)
	{
		NMDebugAI(<< "components haven't got a host!" << endl);
		NMDebugCtx(ctx, << "done!");
		return;
	}

	// get the new host for the components to be ungrouped
	NMIterableComponent* hosthost = host->getHostComponent();
	if (hosthost == 0)
	{
		NMDebugAI(<< "components haven't got a host!" << endl);
		NMDebugCtx(ctx, << "done!");
		return;
	}

	// remove each component from its host and
	// add it to its host's host
	foreach(QGraphicsItem* i, selection)
	{
		NMAggregateComponentItem* parent =
				qgraphicsitem_cast<NMAggregateComponentItem*>(i->parentItem());
		if (parent != 0)
		{
			parent->removeFromGroup(i);
			NMAggregateComponentItem* parentparent =
					qgraphicsitem_cast<NMAggregateComponentItem*>(parent->parentItem());
			if (parentparent != 0)
				parentparent->addToGroup(i);
			else
				this->mModelScene->addItem(i);
		}
		else
			continue;

		NMModelComponent* c = this->componentFromItem(i);
		host->removeModelComponent(c->objectName());
		hosthost->addModelComponent(c);
		c->setTimeLevel(hosthost->getTimeLevel());

		if (this->mOpenEditors.contains(c))
		{
			this->mOpenEditors.value(c)->update();
		}
	}

	this->deleteEmptyComponent(host);

	if (this->mOpenEditors.contains(host))
		this->mOpenEditors.value(host)->update();
	if (this->mOpenEditors.contains(hosthost))
		this->mOpenEditors.value(hosthost)->update();

	NMDebugCtx(ctx, << "done!");
}

NMModelComponent* NMModelViewWidget::componentFromItem(QGraphicsItem* item)
{
	NMModelComponent* ret = 0;
	NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
	NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
	if (pi != 0)
		ret = this->mModelController->getComponent(pi->getTitle());
	else if (ai != 0)
		ret = this->mModelController->getComponent(ai->getTitle());

	return ret;
}

QString NMModelViewWidget::getComponentItemTitle(QGraphicsItem* item)
{
	QString ret;
	NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
	NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

	if (pi != 0)
		ret = pi->getTitle();
	else if (ai != 0)
		ret = ai->getTitle();

	return ret;
}

void NMModelViewWidget::deleteItem()
{
	NMDebugCtx(ctx, << "...");

	NMProcessComponentItem* procItem;
	NMAggregateComponentItem* aggrItem;
	NMComponentLinkItem* linkItem;

    //QList<NMComponentLinkItem*> delLinkList;
	QStringList delList;
	if (this->mModelScene->selectedItems().count())
	{
		foreach(QGraphicsItem* gi, this->mModelScene->selectedItems())
		{
			procItem = qgraphicsitem_cast<NMProcessComponentItem*>(gi);
			aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(gi);
			linkItem = qgraphicsitem_cast<NMComponentLinkItem*>(gi);

			if (procItem != 0)
				delList.push_back(procItem->getTitle());
			else if (aggrItem != 0)
				delList.push_back(aggrItem->getTitle());
            //else if (linkItem != 0)
            //	delLinkList.push_back(linkItem);
		}
	}
	else if (this->mLastItem != 0)
	{
		procItem = qgraphicsitem_cast<NMProcessComponentItem*>(this->mLastItem);
		aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(this->mLastItem);
		linkItem = qgraphicsitem_cast<NMComponentLinkItem*>(this->mLastItem);
		if (linkItem != 0)
        {
			this->deleteLinkComponentItem(linkItem);
        }
		else if (procItem != 0)
        {
            delList.push_back(procItem->getTitle());
            //this->deleteProcessComponentItem(procItem);
        }
		else if (aggrItem != 0)
        {
            delList.push_back(aggrItem->getTitle());
            //this->deleteAggregateComponentItem(aggrItem);
        }
	}


	QStringListIterator sit(delList);
	while(sit.hasNext())
	{
		const QString& name = sit.next();
		QGraphicsItem* item = this->mModelScene->getComponentItem(name);
		if (item == 0 || this->mModelController->getComponent(name) == 0)
		{
			NMDebugAI(<< "can't find any reference to '" << name.toStdString()
					  << "' anymore. Let's move on!" << endl);
			continue;
		}

		NMProcessComponentItem* procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
		//NMComponentLinkItem* linkItem = qgraphicsitem_cast<NMComponentLinkItem*>(item);

		NMModelComponent* pcomp = 0;
		if (procItem != 0)
		{
			// check, whether there are any open edit dialogs for this component
			pcomp = mModelController->getComponent(procItem->getTitle());
			this->removeObjFromOpenEditsList(pcomp);
			this->deleteProcessComponentItem(procItem);
		}
		else if (aggrItem != 0)
		{
			// check, whether there are any open edit dialogs for this component
			pcomp = mModelController->getComponent(aggrItem->getTitle());
			this->removeObjFromOpenEditsList(pcomp);
			this->deleteAggregateComponentItem(aggrItem);
		}
		//else if (linkItem != 0)
		//{
		//	this->deleteLinkComponentItem(linkItem);
		//}
	}

	mModelScene->invalidate();
	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::processProcInputChanged(QList<QStringList> inputs)
{
	NMDebugCtx(ctx, << "...");

	// for now, we're just dealing with the inputs of the first
	// iteration, since we don't support visualising the
	// 2+ iteration inputs as yet
	QStringList srclist;
	if (!inputs.isEmpty())
		srclist = inputs.at(0);

	NMProcess* sender = qobject_cast<NMProcess*>(this->sender());
	if (sender == 0)
		return;

	QString senderName = sender->parent()->objectName();
	QGraphicsItem* gi = this->mModelScene->getComponentItem(senderName);
	if (gi == 0)
		return;
	NMProcessComponentItem* procItem =
			qgraphicsitem_cast<NMProcessComponentItem*>(gi);
	if (procItem == 0)
		return;
	QList<NMComponentLinkItem*> inputLinks = procItem->getInputLinks();


	// strip off any position indices from the source input name
	QStringList list;
	foreach(const QString& src, srclist)
	{
		if (!src.isEmpty())
			list.push_back(src.split(":", QString::SkipEmptyParts).at(0));
	}

	// remove inputs
	for (int a=0; a < inputLinks.size(); ++a)
	{
		NMComponentLinkItem* link = inputLinks.at(a);

		if (!list.contains(procItem->identifyInputLink(a)))
		{
			NMProcessComponentItem* src = link->sourceItem();
			src->removeLink(link);
			procItem->removeLink(link);
			this->mModelScene->removeItem(link);
		}
	}

	// add inputs
	for (int b=0; b < list.size(); ++b)
	{
		if (procItem->getInputLinkIndex(list.at(b)) == -1)
		{
			NMProcessComponentItem* si =
					qgraphicsitem_cast<NMProcessComponentItem*>(
						this->mModelScene->getComponentItem(list.at(b)));
			NMComponentLinkItem* li;
			if (si != 0)
			{
				li = new NMComponentLinkItem(si, procItem, 0);
				li->setZValue(this->mModelScene->getLinkZLevel());
				si->addOutputLink(-1, li);
				procItem->addInputLink(b, li);
				this->mModelScene->addItem(li);
			}
		}
	}

	NMDebugCtx(ctx, << "done!");
}


void
NMModelViewWidget::deleteLinkComponentItem(NMComponentLinkItem* linkItem)
{
	NMProcessComponentItem* targetItem = linkItem->targetItem();
	NMProcessComponentItem* sourceItem = linkItem->sourceItem();
	if (targetItem == 0 || sourceItem == 0)
		return;

	NMModelComponent* targetComp = this->mModelController->getComponent(targetItem->getTitle());
	if (targetComp == 0)
		return;

	NMIterableComponent* itComp =
			qobject_cast<NMIterableComponent*>(targetComp);
	if (itComp == 0)
		return;

	NMProcess* proc = itComp->getProcess();
	if (proc == 0)
		return;

	proc->removeInputComponent(sourceItem->getTitle());

	if (this->mOpenEditors.contains(targetComp))
		this->mOpenEditors.value(targetComp)->update();

	// remove all traces of the link
	targetItem->removeLink(linkItem);
	sourceItem->removeLink(linkItem);
	this->mModelScene->removeItem(linkItem);
}

void
NMModelViewWidget::deleteAggregateComponentItem(NMAggregateComponentItem* aggrItem)
{
	NMDebugCtx(ctx, << "...");

	// object's context, we need further down the track
	QString aggrTitle = aggrItem->getTitle();
	NMModelComponent* comp = mModelController->getComponent(aggrTitle);
	NMModelComponent* hostComp = comp->getHostComponent();

	// iterate over all subcomponents and delete them
	QList<QGraphicsItem*> childItems = aggrItem->childItems();
	QListIterator<QGraphicsItem*> it(childItems);
	NMModelComponent* pcomp = 0;
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		NMProcessComponentItem* procChildItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aggrChildItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

		if (procChildItem != 0)
		{
			NMDebugAI(<< " deleting sub proc comp: "
					<< procChildItem->getTitle().toStdString() << std::endl);
			pcomp = mModelController->getComponent(procChildItem->getTitle());
			this->removeObjFromOpenEditsList(pcomp);
			this->deleteProcessComponentItem(procChildItem);
		}
		else if (aggrChildItem != 0)
		{
			NMDebugAI(<< " deleting sub aggr comp: "
					<< aggrChildItem->getTitle().toStdString() << std::endl);
			pcomp = mModelController->getComponent(aggrChildItem->getTitle());
			this->removeObjFromOpenEditsList(pcomp);
			this->deleteAggregateComponentItem(aggrChildItem);
		}
	}

	// finally remove the component itself
	QString aggrName = aggrItem->getTitle();
	mModelScene->removeItem(aggrItem);
	if (!mModelController->removeComponent(aggrName))
	{
		NMErr(ctx, << "failed to remove component '"
				<< aggrName.toStdString() << "'!");
	}

	this->deleteEmptyComponent(hostComp);

	NMDebugCtx(ctx, << "done!");
}

void NMModelViewWidget::deleteEmptyComponent(NMModelComponent* comp)
{
	NMDebugCtx(ctx, << "...");

	NMIterableComponent* itComp =
			qobject_cast<NMIterableComponent*>(comp);
	if (itComp == 0)
	{
		NMDebugCtx(ctx, << "done!");
		return;
	}

	if (itComp->countComponents() == 0 &&
			itComp->objectName().compare("root") != 0)
	{
		QGraphicsItem* item = mModelScene->getComponentItem(comp->objectName());
		if (item != 0)
			mModelScene->removeItem(item);
		mModelController->removeComponent(comp->objectName());
	}

	NMDebugCtx(ctx, << "done!");
}

void NMModelViewWidget::editRootComponent()
{
	if (this->mRootComponent != 0)
		this->callEditComponentDialog(this->mRootComponent->objectName());
}

void NMModelViewWidget::compProcChanged()
{
//	NMDebugCtx(ctx, << "...");
//	NMModelComponent* comp = qobject_cast<NMModelComponent*>(this->sender());
//	NMProcess* proc = qobject_cast<NMProcess*>(this->sender());
//
//	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::connectProcessItem(NMProcess* proc,
		NMProcessComponentItem* procItem)
{
	connect(proc, SIGNAL(signalInputChanged(QList<QStringList>)),
			this, SLOT(processProcInputChanged(QList<QStringList>)));

	connect(proc, SIGNAL(signalProgress(float)),
			procItem, SLOT(updateProgress(float)));

	connect(proc, SIGNAL(signalExecutionStarted(const QString &)),
			procItem,
			SLOT(reportExecutionStarted(const QString &)));
	connect(proc, SIGNAL(signalExecutionStopped(const QString &)),
			procItem,
			SLOT(reportExecutionStopped(const QString &)));


	connect(proc, SIGNAL(signalExecutionStarted(const QString &)),
			this->mModelController,
			SLOT(reportExecutionStarted(const QString &)));
	connect(proc, SIGNAL(signalExecutionStopped(const QString &)),
			this->mModelController,
			SLOT(reportExecutionStopped(const QString &)));

	connect(this->mModelController, SIGNAL(signalExecutionStopped(const QString &)),
			procItem, SLOT(reportExecutionStopped(const QString &)));

	// connect some host-component signals
	NMModelComponent* comp = qobject_cast<NMModelComponent*>(proc->parent());
	connect(comp, SIGNAL(ComponentDescriptionChanged(const QString &)), procItem,
			SLOT(updateDescription(const QString &)));
}

void
NMModelViewWidget::createProcessComponent(NMProcessComponentItem* procItem,
		const QString& procName, QPointF scenePos)
{
	NMDebugCtx(ctx, << "...");

	QString compName;
	QString tname = procName;
	unsigned int cnt = 1;
	while (this->mModelController->contains(tname))
	{
		tname = QString(tr("%1%2")).arg(procName).arg(cnt);
		++cnt;
	}
	NMDebugAI(<< "finale name of component is '" << tname.toStdString() << "'" << endl);

	NMModelComponent* comp = 0;
	NMDataComponent* dataComp = 0;
	NMIterableComponent* itComp = 0;
	NMProcess* proc = 0;

    if (procName.compare("DataBuffer") == 0)
	{
		NMDebugAI(<< "it's gonna be a DataComponent ... " << endl);
		dataComp = new NMDataComponent();
		dataComp->setObjectName(tname);
		comp = qobject_cast<NMModelComponent*>(dataComp);
		connect(comp, SIGNAL(ComponentDescriptionChanged(const QString &)),
				procItem, SLOT(updateDescription(const QString &)));
		dataComp->setDescription(tname);
	}
    else
    {
        proc = NMProcessFactory::instance().createProcessFromAlias(procName);
    }

	if (proc != 0)// && procName.compare("DataBuffer") != 0)
	{
		NMDebugAI( << "it's gonna be a SequentialIterComponent ..." << endl);
		itComp = new NMSequentialIterComponent();
		itComp->setObjectName(tname);
		itComp->setProcess(proc);
		this->connectProcessItem(proc, procItem);
		comp = qobject_cast<NMModelComponent*>(itComp);
		itComp->setDescription(tname);
	}
	NMDebugAI(<< "and its object name is '" << comp->objectName().toStdString() << "'" << endl);

	if (comp == 0)
	{
		return;
	}

	procItem->setTitle(tname);
	procItem->setDescription(tname);

	// identify the host component, depending on the actual position
    QGraphicsItem* item = this->mModelScene->itemAt(scenePos, this->mModelView->transform());
	NMProcessComponentItem* clickProcItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
	NMAggregateComponentItem* hostItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

	NMIterableComponent* host = 0;
	if (item == 0)
	{
		host = this->mRootComponent;
		hostItem = 0;
	}
	else if (clickProcItem != 0)
	{
		hostItem = qgraphicsitem_cast<NMAggregateComponentItem*>(clickProcItem->parentItem());
		if (hostItem == 0)
		{
			host = this->mRootComponent;
		}
		else
		{
			host = qobject_cast<NMIterableComponent*>(
					this->componentFromItem(hostItem));
		}
	}
	else if (hostItem != 0)
	{
		host = qobject_cast<NMIterableComponent*>(
				this->componentFromItem(hostItem));
	}

	// set time level of the new process to the time level of the host component
	comp->setTimeLevel(host->getTimeLevel());


	if (hostItem == 0)
		this->mModelScene->addItem(procItem);
	else
		hostItem->addToGroup(procItem);
	this->mModelController->addComponent(comp,
			qobject_cast<NMModelComponent*>(host));


    //NMDebugCtx(ctx, << "added " << compName.toStdString() << " to controller!" << std::endl);
	this->mModelScene->invalidate();
	this->mModelView->update();


	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::linkProcessComponents(NMComponentLinkItem* link)
{
	///ToDo: this nedds to be extended to index-based output (ie
	//       source support

	NMDebugCtx(ctx, << "...");

	NMModelComponent* src = this->mModelController->getComponent(
			link->sourceItem()->getTitle());
	NMModelComponent* target = this->mModelController->getComponent(
			link->targetItem()->getTitle());

    if (target != 0)
	{
		QList<QStringList> inpComps = target->getInputs();
		if (inpComps.count() == 0)
		{
			QStringList newlst;
			newlst.push_back(src->objectName());
			inpComps.append(newlst);
		}
		else
		{
			QStringList lst = inpComps.value(0);
			lst.push_back(src->objectName());
			inpComps[0] = lst;
		}
		target->setInputs(inpComps);
	}

	// update any potentially opened editors
	NMEditModelComponentDialog* dlg = 0;
	if (this->mOpenEditors.contains(target))
		dlg = this->mOpenEditors.value(target);

	if (dlg != 0)
		dlg->update();

    if (mTreeCompEditor)
    {
        mTreeCompEditor->setObject(target);
    }


	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::updateTreeEditor(const QString& compName)
{
    if (compName.isEmpty())
        return;

    NMModelComponent* comp = this->mModelController->getComponent(compName);
    if (comp == 0)
    {
        if (mTreeCompEditor)
        {
            mTreeCompEditor->setObject(0);
            mTreeCompEditor->clear();
        }
        return;
    }

    if (mTreeCompEditor == 0)
    {
        OtbModellerWin* otbwin = this->getMainWindow();
        if (otbwin == 0)
        {
            NMErr(ctx, << "Couldn't get hold of main application window!")
            return;
        }

        mTreeCompEditor = const_cast<NMComponentEditor*>(otbwin->getCompEditor());
        connect(this->mModelController, SIGNAL(componentRemoved(const QString &)),
                this, SLOT(updateTreeEditor(const QString &)));
#ifdef BUILD_RASSUPPORT
        mTreeCompEditor->setRasdamanConnectorWrapper(this->mRasConn);
#endif

    }
    mTreeCompEditor->setObject(comp);
    mTreeCompEditor->show();
}

void NMModelViewWidget::callEditComponentDialog(const QString& compName)
{
	if (compName.isEmpty())
		return;

	NMModelComponent* comp = this->mModelController->getComponent(compName);
	if (comp == 0)
	{
		NMErr(ctx, << "component '" << compName.toStdString() << "' couldn't be found!");
		return;
	}

    if (!this->mOpenEditors.contains(comp))
    {
        NMEditModelComponentDialog* dlg = new NMEditModelComponentDialog();
        this->mOpenEditors.insert(comp, dlg);
        connect(dlg, SIGNAL(finishedEditing(QObject*)),
                this, SLOT(removeObjFromOpenEditsList(QObject*)));
#ifdef BUILD_RASSUPPORT
        dlg->setRasdamanConnectorWrapper(this->mRasConn);
#endif
        dlg->setObject(comp);
        dlg->show();
    }
}

void NMModelViewWidget::removeObjFromOpenEditsList(QObject* obj)
{
	NMModelComponent* comp = qobject_cast<NMModelComponent*>(obj);
	NMEditModelComponentDialog* dlg;
	if (this->mOpenEditors.contains(comp))
	{
		dlg = this->mOpenEditors.value(comp);
		this->mOpenEditors.remove(comp);
		delete dlg;
	}

    if (mTreeCompEditor)
    {
        if (!this->mModelController->contains(comp->objectName()))
        {
            mTreeCompEditor->clear();
            mTreeCompEditor->hide();
        }
    }
}

void NMModelViewWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("text/plain"))
		event->acceptProposedAction();
}

void NMModelViewWidget::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat("text/plain"))
		event->acceptProposedAction();
}

void NMModelViewWidget::dropEvent(QDropEvent* event)
{
//	QString droppedText = event->mimeData()->text();
//	NMDebugAI( << "droppedText: " << droppedText.toStdString() << std::endl);
//
//	this->createComponent(droppedText);
//	event->acceptProposedAction();
}

void NMModelViewWidget::executeModel(void)
{
	if (NMModelController::getInstance()->isModelRunning())
	{
		QMessageBox::information(this, "Execute Model Component",
				"There's already a model running at the moment! Please"
				"try again later!");
		return;
	}

	NMModelComponent* comp = 0;
	if (QString("QPushButton").compare(this->sender()->metaObject()->className()) == 0)
		comp = this->mModelController->getComponent("root");
	else if (this->mLastItem != 0)
		comp = this->componentFromItem(this->mLastItem);
	else
		comp = this->mModelController->getComponent("root");

	if (comp == 0)
	{
		NMErr(ctx, << "Failed to perform model execution!");
		return;
	}

	//QString msg = QString(tr("Do you want to execute model component '%1'?"))
	//		.arg(comp->objectName());
	//if (QMessageBox::Ok == QMessageBox::question(this, "Execute Model Component",
	//		msg, QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes))
	{
		emit requestModelExecution(comp->objectName());
	}
}

void
NMModelViewWidget::resetModel(void)
{
	if (NMModelController::getInstance()->isModelRunning())
	{
		QMessageBox::information(this, "Reset Model Component",
				"You cannot reset a model component while a model"
				"is running! Please try again later!");
		return;
	}

	NMModelComponent* comp = 0;
	if (QString("QPushButton").compare(this->sender()->metaObject()->className()) == 0)
		comp = this->mModelController->getComponent("root");
	else if (this->mLastItem != 0)
		comp = this->componentFromItem(this->mLastItem);
	else
		comp = this->mModelController->getComponent("root");

	if (comp == 0)
	{
		NMErr(ctx, << "Failed to perform model reset!");
		return;
	}

	//QString msg = QString(tr("Do you want to reset model component '%1'?")).arg(comp->objectName());
	//if (QMessageBox::Ok == QMessageBox::question(this, "Reset Model Component",
	//		msg, QMessageBox::No | QMessageBox::Yes, QMessageBox::No))
	{
		emit requestModelReset(comp->objectName());
	}
}

void NMModelViewWidget::zoom(int delta)
{
    qreal scaleby = 1;
    if (delta > 0)
        scaleby = mScaleFactor;
    else
        scaleby = 1/mScaleFactor;
    mModelView->scale(scaleby, scaleby);
}

bool
NMModelViewWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::Wheel)
    {
        QWheelEvent* we = static_cast<QWheelEvent*>(e);

        const QPointF pS0 = mModelView->mapToScene(we->pos());

        zoom(we->delta());

        const QPoint pS1 = mModelView->mapFromScene(pS0);
        const QPoint pd = pS1 - we->pos();

        mModelView->horizontalScrollBar()->setValue(pd.x() + mModelView->horizontalScrollBar()->value());
        mModelView->verticalScrollBar()->setValue(pd.y() + mModelView->verticalScrollBar()->value());

        return true;
    }

    return false;
}

OtbModellerWin*
NMModelViewWidget::getMainWindow(void)
{
    OtbModellerWin* mainWin = 0;
    QWidgetList tlw = qApp->topLevelWidgets();
    QWidgetList::ConstIterator it = tlw.constBegin();
    for (; it != tlw.constEnd(); ++it)
    {
        QWidget* w = const_cast<QWidget*>(*it);
        if (w->objectName().compare("OtbModellerWin") == 0)
        {
            mainWin = qobject_cast<OtbModellerWin*>(w);
        }
    }

    return mainWin;
}


