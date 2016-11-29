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
#include <QtDebug>
#include <QPropertyAnimation>
#include <QDomDocument>
#include <QGraphicsProxyWidget>

#include "lumassmainwin.h"
#include "NMModelViewWidget.h"
#include "NMProcess.h"
#include "NMProcessFactory.h"
#include "NMModelComponentFactory.h"
#include "NMModelSerialiser.h"
#include "NMDataComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMConditionalIterComponent.h"
#include "NMComponentEditor.h"
#include "NMGlobalHelper.h"
#include "NMParameterTable.h"

const std::string NMModelViewWidget::ctx = "NMModelViewWidget";

NMModelViewWidget::NMModelViewWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f), mbControllerIsBusy(false), mScaleFactor(1.075)
{
	this->setAcceptDrops(true);
	this->mLastItem = 0;
	
#ifdef BUILD_RASSUPPORT
        this->mRasConn = 0;
#endif


    /* ====================================================================== */
    /* MODEL CONTROLLER SETUP */
    /* ====================================================================== */
    LUMASSMainWin* mainWin = NMGlobalHelper::getMainWindow();//this->getMainWindow();

    mModelRunThread = new QThread(this);
    connect(this, SIGNAL(widgetIsExiting()), mModelRunThread, SLOT(quit()));

    mModelRunThread->start();

 	mModelController = NMModelController::getInstance();
 	mModelController->moveToThread(mModelRunThread);
    mModelController->getLogger()->setHtmlMode(true);

 	connect(this, SIGNAL(requestModelExecution(const QString &)),
 			mModelController, SLOT(executeModel(const QString &)));
 	connect(this, SIGNAL(requestModelReset(const QString &)),
 			mModelController, SLOT(resetComponent(const QString &)));
 	connect(this, SIGNAL(requestModelAbortion()),
 			mModelController, SLOT(abortModel()), Qt::DirectConnection);
 	connect(mModelController, SIGNAL(signalIsControllerBusy(bool)),
 			this, SLOT(reportIsModelControllerBusy(bool)));
    connect(mModelController->getLogger(), SIGNAL(sendLogMsg(const QString &)),
            mainWin, SLOT(appendHtmlMsg(const QString &)));

 	mRootComponent = new NMSequentialIterComponent();
	mRootComponent->setObjectName("root");
	mRootComponent->setDescription(
			"Top level model component managed by the model view widget");
	this->mModelController->addComponent(
			qobject_cast<NMModelComponent*>(mRootComponent));
    //connect(mRootComponent, SIGNAL(NMModelComponentChanged()), this, SLOT(compProcChanged()));


    /* ====================================================================== */
    /* GET THE RASDAMAN CONNECTOR FROM THE MAIN WINDOW */
	/* ====================================================================== */


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
    mModelScene->setSceneRect(-50000,-50000,100000,100000);
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
    connect(mModelScene, SIGNAL(widgetTitleBarRightClicked(QGraphicsSceneMouseEvent *,
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
    connect(mModelScene, SIGNAL(signalModelFileDropped(const QString &, const QPointF &)),
            this, SLOT(processModelFileDrop(const QString &, const QPointF &)));
    connect(mModelScene, SIGNAL(signalItemCopy(const QList<QGraphicsItem*> &, const QPointF &,
                                               const QPointF &)),
            this, SLOT(copyComponents(const QList<QGraphicsItem*> &, const QPointF &,
                                      const QPointF &)));
    connect(mModelScene, SIGNAL(signalItemMove(const QList<QGraphicsItem*> &, const QPointF &,
                                               const QPointF &)),
            this, SLOT(moveComponents(const QList<QGraphicsItem*> &, const QPointF &,
                                      const QPointF &)));

	/* ====================================================================== */
    /* MODEL VIEW SETUP */
	/* ====================================================================== */
	mModelView = new QGraphicsView(mModelScene, this);
	mModelView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //mModelView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    //mModelView->setCacheMode(QGraphicsView::CacheBackground);
	mModelView->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
	mModelView->setDragMode(QGraphicsView::ScrollHandDrag);
    mModelView->setRubberBandSelectionMode(Qt::ContainsItemShape);
	mModelView->setRenderHint(QPainter::Antialiasing, true);
	mModelView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    //mModelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //mModelView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mModelView->setMouseTracking(true);
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
NMModelViewWidget::addWidget(QWidget* w)
{
    this->mModelScene->addWidget(w);
}

void
NMModelViewWidget::addItem(QGraphicsItem* item)
{
    this->mModelScene->addItem(item);
}


void
NMModelViewWidget::reportIsModelControllerBusy(bool busy)
{
	this->mbControllerIsBusy = busy;

    if (!busy)
    {

    }
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
    if (selItems.count() < 1)
	{
        NMDebugAI(<< "grouping not a single component does not make sense!" << endl);
        NMDebugCtx(ctx, << "done!");
		return;
	}

	QListIterator<QGraphicsItem*> it(selItems);
	QList<NMModelComponent*> selComps;
    //QRectF grpBnd;

	// get the host component of the selected items
	NMDebugAI(<< "selected items ..." << std::endl);
	NMIterableComponent* host = 0;
	unsigned int compCounter = 0;
	while(it.hasNext())
	{
		NMProcessComponentItem* procItem = 0;
		NMAggregateComponentItem* aggrItem = 0;
        QGraphicsProxyWidget* proxyWidget = 0;
        QGraphicsItem* item = it.next();

		procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
        proxyWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

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
        else if (proxyWidget != 0)
        {
            itemTitle = proxyWidget->objectName();
            comp = this->mModelController->getComponent(itemTitle);
        }

		if (comp != 0)
		{
			if (compCounter == 0)
				host = comp->getHostComponent();
			selComps.push_back(comp);
		}
        else // non-model-component-items
		{
            //NMDebugAI(<< "oops! The model controller couldn't find the component for '"
            //		<< itemTitle.toStdString() << "'!" << endl);
			continue;
		}

		// we only consider items sharing the same level
		if (host != comp->getHostComponent())
		{
			NMDebugAI(<< "all components have to be on the same level, i.e. sharing the same host!"
					<< endl);
			//NMDebugAI(<< "ignoring component '" << comp->objectName().toStdString()
			//		<< "', which is not on the grouping level!" << endl);
            NMDebugCtx(ctx, << "done!");
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

    // set the new aggregated component on the host's time level
    aggrComp->setTimeLevel(host->getTimeLevel());// + 1);

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
    aggrItem->updateTimeLevel(aggrComp->getTimeLevel());
    aggrItem->updateDescription(aggrComp->getDescription());

    NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(aggrComp);
    if (sic != 0)
    {
        aggrItem->updateNumIterations(1);
        connect(sic, SIGNAL(NumIterationsChanged(uint)),
                aggrItem, SLOT(updateNumIterations(uint)));
        connect(sic, SIGNAL(signalProgress(float)),
                aggrItem, SLOT(slotProgress(float)));
    }
    else
    {
        aggrItem->updateNumIterations(0);
    }

    connect(aggrComp, SIGNAL(ComponentDescriptionChanged(QString)),
            aggrItem, SLOT(updateDescription(QString)));
    connect(aggrComp, SIGNAL(TimeLevelChanged(short)),
            aggrItem, SLOT(updateTimeLevel(short)));
    connect(aggrComp, SIGNAL(signalExecutionStarted()),
            aggrItem, SLOT(slotExecutionStarted()));
    connect(aggrComp, SIGNAL(signalExecutionStopped()),
            aggrItem, SLOT(slotExecutionStopped()));

	it.toFront();
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		NMProcessComponentItem* pItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
        QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(item);
        QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

		if (pItem != 0)
		{
			aggrItem->addToGroup(pItem);
		}
		else if (aItem != 0)
		{
			aggrItem->addToGroup(aItem);
		}
        else if (ti != 0)
        {
            aggrItem->addToGroup(ti);
        }
        else if (pwi != 0)
        {
            aggrItem->addToGroup(pwi);
        }
	}

	// let the individual members of an aggregate component handle their
	// events themselves by default
	aggrItem->setHandlesChildEvents(false);

	// finally add the new group component item itself to the scene
    this->mModelScene->updateComponentItemFlags(aggrItem);
    QPointF npos;
    if (hostItem != 0)
    {
        NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(hostItem);

        npos = hostItem->mapFromScene(aggrItem->sceneBoundingRect().center());
        ai->addToGroup(aggrItem);
        aggrItem->relocate(npos);
    }
    else
    {
        this->mModelScene->addItem(aggrItem);
        npos = aggrItem->sceneBoundingRect().center();
        aggrItem->relocate(npos);
    }

    this->mModelScene->invalidate();


	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::collapseAggrItem()
{
    NMAggregateComponentItem* ai =
            qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);
    ai->collapse(true);
}

void
NMModelViewWidget::unfoldAggrItem()
{
    NMAggregateComponentItem* ai =
            qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);
    ai->collapse(false);
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

    QAction* actGroupTimeLevel = new QAction(this->mItemContextMenu);
    QString actGroupTimeLevelText = "Set Time Level ...";
    actGroupTimeLevel->setEnabled(false);
    actGroupTimeLevel->setText(actGroupTimeLevelText);
    this->mActionMap.insert(actGroupTimeLevelText, actGroupTimeLevel);

    QAction* actDeltaTimeLevel = new QAction(this->mItemContextMenu);
    QString actDeltaTimeLevelText = "Increase/Decrease Time Level ...";
    actDeltaTimeLevel->setEnabled(false);
    actDeltaTimeLevel->setText(actDeltaTimeLevelText);
    this->mActionMap.insert(actDeltaTimeLevelText, actDeltaTimeLevel);

    QAction* collapseComp = new QAction(this->mItemContextMenu);
    collapseComp->setText(tr("Collapse"));
    this->mActionMap.insert("Collapse", collapseComp);

    QAction* unfoldComp = new QAction(this->mItemContextMenu);
    unfoldComp->setText(tr("Unfold"));
    this->mActionMap.insert("Unfold", unfoldComp);

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

    QAction* fontAct = new QAction(this->mItemContextMenu);
    fontAct->setText(tr("Change Font ..."));
    this->mActionMap.insert("Change Font ...", fontAct);

    QAction* clrAct = new QAction(this->mItemContextMenu);
    clrAct->setText(tr("Change Colour ..."));
    this->mActionMap.insert("Change Colour ...", clrAct);

    this->mItemContextMenu->addAction(actDeltaTimeLevel);
    this->mItemContextMenu->addAction(actGroupTimeLevel);
	this->mItemContextMenu->addAction(groupSeqItems);
	this->mItemContextMenu->addAction(groupCondItems);
	this->mItemContextMenu->addAction(ungroupItems);
	this->mItemContextMenu->addSeparator();
    this->mItemContextMenu->addAction(unfoldComp);
    this->mItemContextMenu->addAction(collapseComp);
	this->mItemContextMenu->addAction(delComp);
	this->mItemContextMenu->addSeparator();
	this->mItemContextMenu->addAction(loadComp);
	this->mItemContextMenu->addAction(saveComp);
    this->mItemContextMenu->addSeparator();
    this->mItemContextMenu->addAction(fontAct);
    this->mItemContextMenu->addAction(clrAct);
    this->mItemContextMenu->addSeparator();
    this->mItemContextMenu->addAction(resetComp);
    this->mItemContextMenu->addAction(runComp);


	connect(runComp, SIGNAL(triggered()), this, SLOT(executeModel()));
	connect(resetComp, SIGNAL(triggered()), this, SLOT(resetModel()));
	connect(delComp, SIGNAL(triggered()), this, SLOT(deleteItem()));
    connect(actDeltaTimeLevel, SIGNAL(triggered()), this, SLOT(addDeltaTimeLevel()));
    connect(actGroupTimeLevel, SIGNAL(triggered()), this, SLOT(setGroupTimeLevel()));
    connect(groupSeqItems, SIGNAL(triggered()), this, SLOT(createSequentialIterComponent()));
	connect(groupCondItems, SIGNAL(triggered()), this, SLOT(createConditionalIterComponent()));
	connect(ungroupItems, SIGNAL(triggered()), this, SLOT(ungroupComponents()));
	connect(saveComp, SIGNAL(triggered()), this, SLOT(saveItems()));
    connect(loadComp, SIGNAL(triggered()), this, SLOT(callLoadItems()));
    connect(fontAct, SIGNAL(triggered()), this, SLOT(changeFont()));
    connect(clrAct, SIGNAL(triggered()), this, SLOT(changeColour()));
    connect(collapseComp, SIGNAL(triggered()), this, SLOT(collapseAggrItem()));
    connect(unfoldComp, SIGNAL(triggered()), this, SLOT(unfoldAggrItem()));

    // DEBUG
#ifdef DEBUG
    QAction* testAct = new QAction(this->mItemContextMenu);
    testAct->setText("test");
    this->mActionMap.insert("test", testAct);
    this->mItemContextMenu->addSeparator();
    this->mItemContextMenu->addAction(testAct);
    connect(testAct, SIGNAL(triggered()), this, SLOT(test()));
#endif
    // DEBUG

}

void
NMModelViewWidget::test()
{
   NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);
   NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(mLastItem);
   QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(mLastItem);
   QGraphicsProxyWidget* pw = qgraphicsitem_cast<QGraphicsProxyWidget*>(mLastItem);
   std::string title;

   if (ai)
   {
       title = ai->getTitle().toStdString();
   }
   else if (pi)
   {
       title = pi->getTitle().toStdString();
   }
   else if (ti)
   {
       title = QString("%1 ...").arg(ti->toPlainText().left(15)).toStdString();
   }
   else if (pw)
   {
       title = pw->objectName().toStdString();
   }

   NMDebugAI(<< "probing '" << title << "' ... " << std::endl);

   NMDebugAI(<< this->reportPoint(mLastItem->pos(), "item's pos") << std::endl);
   NMDebugAI(<< this->reportPoint(mLastItem->scenePos(), "item's scene pos") << std::endl);
   NMDebugAI(<< this->reportRect(mLastItem->boundingRect(), "item's rect") << std::endl);
   NMDebugAI(<< this->reportRect(mLastItem->sceneBoundingRect(), "item's scene rect") << std::endl);
}


void
NMModelViewWidget::processModelFileDrop(const QString& fileName,
                          const QPointF& scenePos)
{
    mLastScenePos = scenePos;
    this->loadItems(fileName);
}


void
NMModelViewWidget::changeFont(void)
{
    QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(mLastItem);
    if (ti != 0)
    {
        bool ok;
        ti->setFont(QFontDialog::getFont(&ok, ti->font(),
                     this, QString::fromLatin1("Select Label Font")));
    }
}

void
NMModelViewWidget::changeColour(void)
{
    QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(mLastItem);
    NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);

    if (ti != 0)
    {

        ti->setDefaultTextColor(QColorDialog::getColor(ti->defaultTextColor(),
                    this, QString::fromLatin1("Select Label Colour")));

    }
    else if (ai != 0)
    {
        ai->setColor(QColorDialog::getColor(ai->getColor(),
                    this, QString::fromLatin1("Select Component Colour"),
                    QColorDialog::ShowAlphaChannel));
    }
}

void NMModelViewWidget::callItemContextMenu(QGraphicsSceneMouseEvent* event,
		QGraphicsItem* item)
{
    bool running = NMModelController::getInstance()->isModelRunning();

	this->mLastScenePos = event->scenePos();
	this->mLastItem = item;

    QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();

    NMProcessComponentItem* pi   = qgraphicsitem_cast<NMProcessComponentItem*>(item);
    NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
    NMComponentLinkItem* li = qgraphicsitem_cast<NMComponentLinkItem*>(item);
    QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(item);
    QGraphicsProxyWidget* wi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

    QString title;
    bool dataBuffer = false;
    bool paraTable = false;
    if (selection.count() > 0)
    {
        title = QString("%1 Components").arg(selection.count());
    }
    else if (pi != 0)
    {
        title = pi->getTitle();
        dataBuffer = pi->getIsDataBufferItem();
    }
    else if (wi != 0)
    {
        title = wi->widget()->windowTitle();
        paraTable = true;
    }
    else if (ai != 0)
    {
        title = ai->getTitle();
    }
    else if (li != 0)
    {
        title = QString("input link");
    }
    else if (ti != 0)
    {
        title = QString("Label");
    }
    else
    {
        title = QString::fromUtf8("root");
    }

	// Execute && Reset model
    if (!running && ((ai != 0 || pi != 0 || paraTable || mLastItem == 0) && li == 0 && ti == 0))
	{
        if (dataBuffer || paraTable)
        {
            this->mActionMap.value("Execute")->setEnabled(true);
            mActionMap.value("Execute")->setText(QString::fromLatin1("Update %1").arg(title));
        }
        else
        {
            this->mActionMap.value("Execute")->setEnabled(true);
            mActionMap.value("Execute")->setText(QString::fromUtf8("Execute %1").arg(title));
        }

        if (!paraTable)
        {
            this->mActionMap.value("Reset")->setEnabled(true);
            mActionMap.value("Reset")->setText(QString("Reset %1").arg(title));
        }
        else
        {
            this->mActionMap.value("Reset")->setEnabled(false);
        }
	}
	else
	{
		this->mActionMap.value("Execute")->setEnabled(false);
        mActionMap.value("Execute")->setText(QString::fromUtf8("Execute"));
		this->mActionMap.value("Reset")->setEnabled(false);
        mActionMap.value("Reset")->setText(QString::fromUtf8("Reset"));
	}

	// GROUP
    if ((selection.count() > 0 || ai != 0) && !running)
	{
        int levelIndi = -1 ;
        if (selection.count() > 0)
            levelIndi = this->shareLevel(selection);

        if ((ai != 0 && selection.count() == 0))// || selection.count() == 1)
        {
            this->mActionMap.value("Increase/Decrease Time Level ...")->setEnabled(false);
            this->mActionMap.value("Set Time Level ...")->setEnabled(false);
            this->mActionMap.value("Ungroup Components")->setEnabled(true);
            this->mActionMap.value("Create Sequential Group")->setEnabled(false);
            //this->mActionMap.value("Create Conditional Group")->setEnabled(true);
        }
        else if (selection.count() >= 1 && levelIndi >= 0)
		{
			this->mActionMap.value("Create Sequential Group")->setEnabled(true);
			//this->mActionMap.value("Create Conditional Group")->setEnabled(true);
			if (levelIndi > 0)
            {
                this->mActionMap.value("Increase/Decrease Time Level ...")->setEnabled(true);
                this->mActionMap.value("Set Time Level ...")->setEnabled(true);
				this->mActionMap.value("Ungroup Components")->setEnabled(true);
            }
			else
            {
                this->mActionMap.value("Increase/Decrease Time Level ...")->setEnabled(false);
                this->mActionMap.value("Set Time Level ...")->setEnabled(false);
				this->mActionMap.value("Ungroup Components")->setEnabled(false);
            }
		}
		else
		{
			this->mActionMap.value("Create Sequential Group")->setEnabled(false);
			this->mActionMap.value("Create Conditional Group")->setEnabled(false);
			this->mActionMap.value("Ungroup Components")->setEnabled(false);
            this->mActionMap.value("Set Time Level ...")->setEnabled(false);
            this->mActionMap.value("Increase/Decrease Time Level ...")->setEnabled(false);
		}
	}
	else
	{
		this->mActionMap.value("Create Sequential Group")->setEnabled(false);
		this->mActionMap.value("Create Conditional Group")->setEnabled(false);
		this->mActionMap.value("Ungroup Components")->setEnabled(false);
        this->mActionMap.value("Set Time Level ...")->setEnabled(false);
        this->mActionMap.value("Increase/Decrease Time Level ...")->setEnabled(false);
	}

	// DELETE & SAVE AS
	if ((selection.count() > 0 || item != 0) && !running)
    {
        this->mActionMap.value("Delete")->setEnabled(true);
        mActionMap.value("Delete")->setText(QString("Delete %1").arg(title));
    }
	else
    {
		this->mActionMap.value("Delete")->setEnabled(false);
        mActionMap.value("Delete")->setText(QString::fromUtf8("Delete"));
    }

	// SAVE & LOAD
    if ((item != 0 && li == 0 && wi == 0) || item == 0)
    {
        this->mActionMap.value("Save As ...")->setEnabled(true);
        mActionMap.value("Save As ...")->setText(QString("Save %1 As ...").arg(title));
    }
	else
    {
        this->mActionMap.value("Save As ...")->setEnabled(false);
        mActionMap.value("Save As ...")->setText(QString::fromUtf8("Save As ..."));
    }

    if (!running && pi == 0 && li == 0 && selection.count() == 0 && ti == 0 && wi == 0)
    {
		this->mActionMap.value("Load ...")->setEnabled(true);
        mActionMap.value("Load ...")->setText(QString("Load Into %1 ...").arg(title));

        if (ai)
        {
            mActionMap.value("Collapse")->setText(QString("Collapse %1").arg(title));
            mActionMap.value("Unfold")->setText(QString("Unfold %1").arg(title));

            if (ai->isCollapsed())
            {
                mActionMap.value("Collapse")->setEnabled(false);
                mActionMap.value("Unfold")->setEnabled(true);
            }
            else
            {
                mActionMap.value("Collapse")->setEnabled(true);
                mActionMap.value("Unfold")->setEnabled(false);
            }
        }
    }
	else
    {
		this->mActionMap.value("Load ...")->setEnabled(false);
        mActionMap.value("Load ...")->setText(QString::fromUtf8("Load ..."));

        mActionMap.value("Collapse")->setText(QString("Collapse"));
        mActionMap.value("Collapse")->setEnabled(false);
        mActionMap.value("Unfold")->setText(QString("Unfold"));
        mActionMap.value("Unfold")->setEnabled(false);
    }

    // CHANGE FONT & COLOUR
    if ((ai != 0 || ti != 0) && selection.count() == 0)
    {
        if (ti != 0)
        {
            this->mActionMap.value("Change Font ...")->setEnabled(true);
            this->mActionMap.value("Change Font ...")->setText(QString("Change %1's Font ...").arg(title));
        }
        else
        {
            this->mActionMap.value("Change Font ...")->setEnabled(false);
            this->mActionMap.value("Change Font ...")->setText(QString("Change Font ..."));
        }

        this->mActionMap.value("Change Colour ...")->setEnabled(true);
        this->mActionMap.value("Change Colour ...")->setText(QString("Change %1's Colour ...").arg(title));
    }
    else
    {
        this->mActionMap.value("Change Font ...")->setEnabled(false);
        this->mActionMap.value("Change Font ...")->setText(QString("Change Font ..."));
        this->mActionMap.value("Change Colour ...")->setEnabled(false);
        this->mActionMap.value("Change Colour ...")->setText(QString("Change Colour ..."));
    }

	QPoint viewPt = this->mModelView->mapFromScene(this->mLastScenePos);
	this->mItemContextMenu->move(mModelView->mapToGlobal(viewPt));
	this->mItemContextMenu->exec();
}

void
NMModelViewWidget::deleteProcessComponentItem(NMProcessComponentItem* procItem)
{
	NMDebugCtx(ctx, << "...");

    NMProcessComponentItem* dpi = qgraphicsitem_cast<NMProcessComponentItem*>(mLastItem);
    if (dpi == procItem)
    {
        mLastItem = 0;
    }

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
        NMDebugAI(<< "failed to remove component '"
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

    bool bSaveRoot = false;
    if (items.isEmpty())
    {
        if (this->mLastItem != 0)
        {
            items.append(this->mLastItem);
        }
        else if (!this->mModelScene->items().isEmpty())
        {
            items.append(this->mModelScene->items());
            bSaveRoot = true;
        }
    }

    if (items.count() == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

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
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }
    else
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

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

    QFile fileLmx(fnLmx);
    if (!fileLmx.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        fileLmv.close();
        NMErr(ctx, << "unable to create file '" << fnLmx.toStdString()
                << "'!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QDomDocument doc;
    // call the actually working horse for exporting items ...
    this->exportModel(items, fileLmv, doc, bSaveRoot);

    // write xml file
    QTextStream out(&fileLmx);
    out << doc.toString(4);
    fileLmx.close();

    // write the lmv file
    fileLmv.close();


    NMDebugCtx(ctx, << "done!");
}


void
NMModelViewWidget::exportModel(const QList<QGraphicsItem*>& items,
                               QIODevice& device,
                               QDomDocument& doc,
                               bool bSaveRoot)
{
    QStringList savecomps;
    if (bSaveRoot)
    {
        savecomps.push_back("root");
    }

	// create a list of all components to be saved
    // create also a list of all labels on root level
    // since they don't belong to any NMAggregateComponentItem
    QList<QGraphicsTextItem*> rootLabels;
	foreach(QGraphicsItem* item, items)
	{
        QGraphicsTextItem* label = qgraphicsitem_cast<QGraphicsTextItem*>(item);
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
        // look after "root-labels"
        else if (label != 0)
        {
            if (    label->parentItem() == 0
                ||  !items.contains(label->parentItem()))
            {
                rootLabels.push_back(label);
            }
        }
	}

	NMDebugAI(<< "save comps '" << savecomps.join(" ").toStdString() << "'" << endl);

	//   keep track of all written links
	QList<NMComponentLinkItem*> writtenLinks;


    QDataStream lmv(&device);

    // ---------------
    // set LUMASS identifier
    lmv << QString::fromLatin1("LUMASS Model Visualisation File");
    //lmv << (qreal)0.93;
    lmv << NMGlobalHelper::getLUMASSVersion();

    lmv.setVersion(13);
    // --------------------------------------
    // save root labels

    foreach(QGraphicsTextItem* label, rootLabels)
    {
        lmv << (qint32)QGraphicsTextItem::Type;
        lmv << *label;
    }

    // --------------------------------------

    // add the root node to the document
    QDomElement modElem = doc.createElement("Model");
    modElem.setAttribute("description", "the one and only model element");
    doc.appendChild(modElem);


    // ------------------------------------------------------
    // save model component items (incl. any included labels)
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
        //NMProcessComponentItem* pi;
        //NMAggregateComponentItem* ai;
        //NMComponentLinkItem* li;
        //QGraphicsTextItem* ti;

		// we treat root differently, because it hasn't got an item
		// associated with it
		if (itemName.compare("root") == 0)
		{
			comp = this->mModelController->getComponent("root");
            //xmlS.serialiseComponent(comp, fnLmx, 4, false);
            xmlS.serialiseComponent(comp, doc);
			continue;
		}

        // if the item is not on the scene, it's probably
        // hidden (in a collapsed component) and we'll get it
        // as a child of one of the
        // other ones (if it's not we've got a bug ...)
        if (item == 0)
        {
            continue;
        }
        // if we've got hold of the item, let's just go ahead with
        // the export ...
        else
        {
            NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
            if (ai && ai->isCollapsed())
            {
                QList<QGraphicsItem*> aos;
                aos.append(ai);
                ai->getAllModelOffSprings(aos);
                foreach(QGraphicsItem* oi, aos)
                {
                    exportComponent(oi, lmv, doc, writtenLinks, savecomps);
                }
            }
            else
            {
                exportComponent(item, lmv, doc, writtenLinks, savecomps);
            }
        }
    }

    //fileLmv.close();

    //NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::exportComponent(QGraphicsItem* item,
                                   QDataStream &lmv,
                                   QDomDocument& doc,
                                   QList<NMComponentLinkItem *> &writtenLinks,
                                   QStringList& savecomps)
{
    NMModelSerialiser xmlS;
    NMModelComponent* comp = 0;
    switch (item->type())
    {
    case NMProcessComponentItem::Type:
        {
            NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
            comp = this->mModelController->getComponent(pi->getTitle());
            if (comp == 0)
            {
                NMErr(ctx, << "couldn't write '" << pi->getTitle().toStdString() << "' - skip it!");
                //NMDebugCtx(ctx, << "done!");
                return;
            }

            //xmlS.serialiseComponent(comp, fnLmx, 4, append);
            xmlS.serialiseComponent(comp, doc);
            lmv << (qint32)NMProcessComponentItem::Type;
            lmv << *pi;

            // save links
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
        {
            NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
            comp = this->mModelController->getComponent(ai->getTitle());
            if (comp == 0)
            {
                NMErr(ctx, << "couldn't write '" << ai->getTitle().toStdString() << "' - skip it!");
                //NMDebugCtx(ctx, << "done!");
                return;
            }

            //xmlS.serialiseComponent(comp, fnLmx, 4, append);
            xmlS.serialiseComponent(comp, doc);
            lmv << (qint32)NMAggregateComponentItem::Type;
            lmv << *ai;
        }
        break;

    case QGraphicsProxyWidget::Type:
        {
            QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);
            comp = this->mModelController->getComponent(pwi->objectName());
            if (comp == 0)
            {
                NMErr(ctx, << "couldn't write '" << pwi->objectName().toStdString() << "' - skip it!");
                return;
            }

            //NMParameterTable* pt = qobject_cast<NMParameterTable*>(comp);

            xmlS.serialiseComponent(comp, doc);
            lmv << (qint32)QGraphicsProxyWidget::Type;
            lmv << pwi->objectName();
            //lmv << pt->getFileName();
            //lmv << pt->getTableName();
            lmv << pwi->pos();
        }
        break;

    default:
        break;
    }
}

void
NMModelViewWidget::moveComponents(const QList<QGraphicsItem*>& moveList, const QPointF &source,
                                  const QPointF &target)
{
    NMDebugCtx(ctx, << "...");

    NMDebugAI(<< this->reportRect(this->mModelScene->sceneRect(), "scene rect") << std::endl);
    NMDebugAI(<< this->reportPoint(source, "move from") << std::endl);
    NMDebugAI(<< this->reportPoint(target, "move to") << std::endl);


    NMAggregateComponentItem* ai = 0;
    NMProcessComponentItem* pi = 0;
    QGraphicsTextItem* ti = 0;

    // the target item
    QGraphicsItem* targetItem = this->mModelScene->itemAt(target, this->mModelView->transform());

    // determine individual group delta
    QList<QPointF> deltas;

    NMDebugAI(<< "items on the move ..." << std::endl);
    QList<QGraphicsItem*> topList;
    foreach(QGraphicsItem* gi, moveList)
    {
        ai = qgraphicsitem_cast<NMAggregateComponentItem*>(gi);
        pi = qgraphicsitem_cast<NMProcessComponentItem*>(gi);
        ti = qgraphicsitem_cast<QGraphicsTextItem*>(ti);

        bool isTopLevel = true;
        foreach(QGraphicsItem* gii, moveList)
        {
            NMAggregateComponentItem* aii = qgraphicsitem_cast<NMAggregateComponentItem*>(gii);
            // check, whether move is possible
            //if (gii->childItems().contains(targetItem))
            if (aii && aii->getModelChildren().contains(targetItem))
            {
                // ouch we can't do that
                NMBoxErr("Move Components", "Cannot move components into itself!");
                NMDebugCtx(ctx, << "done!");
                return;
            }

            //if (gii->childItems().contains(gi))
            if (aii && aii->getModelChildren().contains(gi))
            {
                isTopLevel = false;
                break;
            }
        }
        if (isTopLevel)
        {
            topList << gi;
            if (ai)
            {
                deltas << (ai->sceneBoundingRect().center() - source);
            }
            else
            {
                deltas << (gi->scenePos() - source);
            }
        }
    }


    // get the new host of the components
    NMAggregateComponentItem* newHostItem = 0;
    NMIterableComponent* newHostComp = 0;

    if (targetItem != 0)
    {
        if (targetItem->type() == NMProcessComponentItem::Type)
        {
            NMErr(ctx, <<  "Can't drop anything on a process component!" << std::endl);
            NMDebugCtx(ctx, << "done!");
            return;
        }
        else if (targetItem->type() == QGraphicsTextItem::Type)
        {
            NMErr(ctx, <<  "Can't drop anything on a text label!" << std::endl);
            NMDebugCtx(ctx, << "done!");
            return;
        }
        else if (targetItem->type() == NMAggregateComponentItem::Type)
        {
            newHostItem = qgraphicsitem_cast<NMAggregateComponentItem*>(targetItem);
            newHostComp = qobject_cast<NMIterableComponent*>(this->componentFromItem(newHostItem));
        }
    }
    else
    {
        newHostItem = 0;
        newHostComp = qobject_cast<NMIterableComponent*>(NMModelController::getInstance()->getComponent("root"));
    }

    if (newHostComp == 0)
    {
        NMErr(ctx, << "Couldn't find a suitable new host to move to!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // re-group items to move group
    NMDebugAI( << "moving these top level items ..." << std::endl);
    foreach(QGraphicsItem* tli, topList)
    {
        NMModelComponent* comp = this->componentFromItem(tli);
        if (comp != 0)
        {
            NMDebugAI( << comp->objectName().toStdString() << " - "
                       << comp->getDescription().toStdString() << std::endl);

            NMIterableComponent* hostComp = comp->getHostComponent();
            if (    hostComp->objectName().compare(newHostComp->objectName()) != 0
                &&  newHostComp->objectName().compare(comp->objectName()) != 0
               )
            {
                hostComp->removeModelComponent(comp->objectName());
                newHostComp->addModelComponent(comp);
            }
        }

        NMAggregateComponentItem* hostItem = qgraphicsitem_cast<NMAggregateComponentItem*>(tli->parentItem());
        if (hostItem != 0)
        {
            hostItem->removeFromGroup(tli);
        }
        mModelScene->removeItem(tli);
    }

    // re-assemble the moving components at its new position and host
    int counter = 0;
    foreach(QGraphicsItem* tli, topList)
    {
        NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(tli);

        QPointF newPos = target + deltas.at(counter);
        if (newHostItem != 0)
        {
            if (ai)
            {
                if (newHostItem->getTitle().compare(ai->getTitle()) != 0)
                {
                    newHostItem->addToGroup(ai);
                    ai->relocate(newHostItem->mapFromScene(newPos));
                }
                else
                {
                    mModelScene->addItem(ai);
                    ai->relocate(newPos);
                }

            }
            else
            {
                newHostItem->addToGroup(tli);
                tli->setPos(newHostItem->mapFromScene(newPos));
            }
        }
        else
        {
            mModelScene->addItem(tli);
            if (ai)
            {
                ai->relocate(newPos);
            }
            else
            {
                tli->setPos(newPos);
            }
        }
        tli->ungrabMouse();
        ++counter;
    }
    this->mModelScene->invalidate();
    this->mModelScene->clearDragItems();
    NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::copyComponents(const QList<QGraphicsItem*>& copyList, const QPointF &source,
                                  const QPointF &target)
{
    NMDebugCtx(ctx, << "...");

    NMAggregateComponentItem* hostItem = qgraphicsitem_cast<NMAggregateComponentItem*>(
                this->mModelScene->itemAt(target, this->mModelView->transform()));

    NMIterableComponent* importHost = 0;
    if (hostItem)
    {
        importHost = qobject_cast<NMIterableComponent*>(
                        NMModelController::getInstance()->getComponent(hostItem->getTitle()));
    }

    // make a structured copy of the selected item
    // -> DomDocument -> ByteStream
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    QDomDocument doc;
    this->exportModel(copyList, buf, doc, false);

    // parse the DomDocument copy and create identical copy
    // with own identity (i.e. objectName is unique)
    NMModelSerialiser xmlS;
    QMap<QString, QString> nameRegister;
    xmlS.parseModelDocument(nameRegister, doc, importHost);

    // create a copy of the graphical item representation
    buf.seek(0);
    QDataStream itemStream(&buf);
    this->importModel(itemStream, nameRegister, false);

    // create a list of newly imported items
    QList<QGraphicsItem*> itemList;
    foreach(QGraphicsItem* origItem, copyList)
    {
        NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(origItem);
        NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(origItem);

        QGraphicsItem* ni = 0;
        if (pi)
        {
            ni = this->mModelScene->getComponentItem(nameRegister.value(pi->getTitle()));
        }
        else if (ai)
        {
            ni = this->mModelScene->getComponentItem(nameRegister.value(ai->getTitle()));
        }
        else
        {

            ni = this->mModelScene->itemAt(origItem->scenePos(), this->mModelView->transform());
            if (ni == 0)
            {
                ni = origItem;
            }
        }

        if (ni)
        {
            itemList << ni;
        }
    }

    // move the new components to the pointed at position
    this->moveComponents(itemList, source, target);

    NMDebugCtx(ctx, << "done!");
}


void
NMModelViewWidget::loadItems(const QString& fileName)
{
    QString fileNameString;
    if (fileName.isEmpty())
    {
        fileNameString = QFileDialog::getOpenFileName(this,
             tr("Load LUMASS Model Component"), "~", tr("LUMASS Model Component Files (*.lmx *.lmv)"));
        if (fileNameString.isNull()) return;
    }
    else
    {
        fileNameString = fileName;
    }


    // get the import component
    QString importHostName;
    NMModelComponent* hc = this->componentFromItem(mLastItem);
    NMIterableComponent* importHost = 0;
    if (hc != 0)
    {
        importHost = qobject_cast<NMIterableComponent*>(hc);
        importHostName = importHost->objectName();
    }
    else
    {
        importHostName = QString::fromLatin1("root");
    }


    QFileInfo fi(fileNameString);
    QString fnLmx = QString("%1/%2.lmx").arg(fi.absolutePath()).arg(fi.baseName());
    QString fnLmv = QString("%1/%2.lmv").arg(fi.absolutePath()).arg(fi.baseName());

    // read the data model
    QMap<QString, QString> nameRegister;
    NMModelSerialiser xmlS;

#ifdef BUILD_RASSUPPORT
    nameRegister = xmlS.parseComponent(fnLmx, importHost, this->mModelController, *this->mRasConn);
#else
    nameRegister = xmlS.parseComponent(fnLmx, importHost, this->mModelController);
#endif

    QFile fileLmv(fnLmv);
    if (!fileLmv.open(QIODevice::ReadOnly))
    {
        NMErr(ctx, << "unable to open file '" << fnLmv.toStdString()
                << "'!");
        //NMDebugCtx(ctx, << "done!");
        return;
    }
    QDataStream lmv(&fileLmv);

    this->importModel(lmv, nameRegister, true);
}

void
NMModelViewWidget::importModel(QDataStream& lmv,
                               const QMap<QString, QString>& nameRegister,
                               bool move)
{
    NMDebugCtx(ctx, << "...");


    NMDebugAI(<< "reading model view file ..." << endl);
    NMDebugAI(<< "---------------------------" << endl);

    // file identifier
    QString fileIdentifier = "";
    // indicates lumass version from which on
    // this file format was used
    qreal lmv_version = (qreal)0.91;

    if (lmv.version() < 12)
    {
        NMBoxErr("Unsupported File Version",
                 "This *.lmv file version is not supported "
                 "by LUMASS version > 0.9.3!");
        NMDebugCtx(ctx, << "done!");
		return;
    }

    // that's the one we used for writing
    lmv.setVersion(13);

    // check whether we can read the header and if so, what the lmv file version is
    lmv >> fileIdentifier;
    if (fileIdentifier.compare(QString::fromLatin1("LUMASS Model Visualisation File")) == 0)
    {
        lmv >> lmv_version;
        NMDebugAI(<< "LUMASS lmv_version = " << lmv_version << std::endl);
    }
    else
    {
        lmv.device()->seek(0);
        lmv.resetStatus();
    }

    // --------------------------------------
    // first iteration: create item objects
    // --------------------------------------

    QList<QString> importItems;
    QList<NMAggregateComponentItem*> collapsedItems;
    QList<QPointF> collapsedPos;
    QList<QGraphicsTextItem*> importLabels;
	NMIterableComponent* itComp = 0;
	NMProcess* procComp;
	while(!lmv.atEnd())
	{
		qint32 readType;
		lmv >> readType;
		//NMDebugAI(<< "item type is: " << (int)readType << endl);

        NMProcessComponentItem* pi;
        NMAggregateComponentItem* ai;
        QGraphicsTextItem* ti;
        switch(readType)
		{
        case (qint32)QGraphicsTextItem::Type:
            {
                ti = new QGraphicsTextItem();
                lmv >> *ti;
                ti->setTextInteractionFlags(Qt::NoTextInteraction | Qt::TextBrowserInteraction);
                ti->setFlag(QGraphicsItem::ItemIsMovable, true);
                ti->setOpenExternalLinks(true);
                this->mModelScene->updateComponentItemFlags(ti);
                this->mModelScene->addItem(ti);
                importLabels <<  ti;
            }
            break;

        case (qint32)NMProcessComponentItem::Type:
				{
                    pi = new NMProcessComponentItem(0, this->mModelScene);
					lmv >> *pi;
					QString itemTitle = nameRegister.value(pi->getTitle());

                    // check, if we got a valid item (must have name!)
                    if (itemTitle.isEmpty())
                    {
                        continue;
                    }
					pi->setTitle(itemTitle);
                    importItems << itemTitle;

                    QRegExp digDetec("(\\d+$)");
                    if (digDetec.indexIn(itemTitle) != -1)
                    {
                        bool bok;
                        int typeId = digDetec.cap(1).toInt(&bok);
                        if (bok)
                        {
                            pi->setTypeID(typeId);
                        }
                    }


					if (!pi->getIsDataBufferItem())
					{
						// establish across-thread-communication between GUI item and process component
						itComp = qobject_cast<NMIterableComponent*>(
								this->mModelController->getComponent(itemTitle));
                        if (itComp == 0 || itComp->getProcess() == 0)
                        {
                            NMErr(ctx, << "Ivalid process component detected '"
									<< itemTitle.toStdString() << "'!");
                            continue;
						}
						pi->setDescription(itComp->getDescription());
                        pi->setTimeLevel(itComp->getTimeLevel());

						procComp = itComp->getProcess();
						this->connectProcessItem(procComp, pi);
					}
                    // data buffer item
					else
					{
                        NMModelComponent* mcomp = this->mModelController->getComponent(itemTitle);
                        if (mcomp == 0)
                        {
                            NMErr(ctx, << "Ivalid data buffer component detected '"
                                    << itemTitle.toStdString() << "'!");
                            continue;
                        }

						connect(mcomp, SIGNAL(ComponentDescriptionChanged(const QString &)),
                                pi, SLOT(setDescription(const QString &)));
                        connect(mcomp, SIGNAL(TimeLevelChanged(short)),
                                pi, SLOT(updateTimeLevel(short)));
                        //pi->setIsDataBufferItem(true);
						pi->setDescription(mcomp->getDescription());
                        pi->setTimeLevel(mcomp->getTimeLevel());
					}
                    //pi->setFlag(QGraphicsItem::ItemIsMovable, true);
                    pi->update();
                    this->mModelScene->update(pi->mapRectToScene(pi->boundingRect()));
                    this->mModelScene->updateComponentItemFlags(pi);
                    this->mModelScene->addItem(pi);
				}
				break;

		case (qint32)NMAggregateComponentItem::Type:
				{
					ai = new NMAggregateComponentItem(0);

					QString title;
					QPointF pos;
					QColor color;
                    bool bCollapsed = false;
                    qint32 nkids;

                    lmv >> title >> pos >> color;
                    if (lmv_version >= 0.95)
                    {
                        lmv >> bCollapsed;
                    }
                    lmv >> nkids;

					ai->setTitle(nameRegister.value(title));
					ai->setPos(pos);
                    ai->relocate(pos);
					ai->setColor(color);
                    importItems <<  ai->getTitle();

                    // we treat each component as
                    // unfolded for now, but keep
                    // an eye on those which are
                    // actually collapsed
                    if (bCollapsed)
                    {
                        collapsedItems << ai;
                        collapsedPos << pos;
                    }

                    for (qint32 v=0; v < nkids; ++v)
					{
						QString dummy;
						lmv >> dummy;
                        // text labels are stored as (QString("TextLabel"), QGraphicsItem*) pair!
                        if (dummy.compare(QString::fromLatin1("TextLabel")) == 0)
                        {
                            QGraphicsTextItem* textItemDummy = new QGraphicsTextItem(0);
                            lmv >> *textItemDummy;
                            delete textItemDummy;
                            textItemDummy = 0;
                        }
					}

                    ai->setHandlesChildEvents(false);
                    this->mModelScene->updateComponentItemFlags(ai);
					this->mModelScene->addItem(ai);
				}

				break;

		case (qint32)NMComponentLinkItem::Type:
				{
					int srcIdx, tarIdx;
					QString srcName, tarName;
					lmv >> srcIdx >> srcName >> tarIdx >> tarName;

                    bool dyn;
                    if (lmv_version > (qreal)0.91)
                        lmv >> dyn;
				}
				break;

        case (qint32)QGraphicsProxyWidget::Type:
                {
                    NMSqlTableView* tv = 0;
                    QString ptName;
                    QString fileName;
                    QString tableName;
                    QPointF itemPos;

                    lmv >> ptName >> itemPos;

                    QString compName = nameRegister.value(ptName);
                    NMModelComponent* comp =
                            this->mModelController->getComponent(compName);
                    NMParameterTable* pt = qobject_cast<NMParameterTable*>(comp);
                    if (pt)
                    {
                        fileName = pt->getFileName();
                        tableName = pt->getTableName();

                        tv = NMGlobalHelper::getMainWindow()->importTable(
                                    fileName,
                                    LUMASSMainWin::NM_TABVIEW_SCENE,
                                    true,
                                    tableName);
                    }
                    importItems << compName;

                    // if importing the table failed, we have
                    // to get rid of the model component as well
                    if (tv == 0)
                    {
                        if (comp)
                        {
                            mModelController->removeComponent(compName);
                            QMap<QString, QString>& nR = const_cast< QMap<QString, QString>& >(nameRegister);
                            nR.remove(ptName);
                        }
                        NMErr(ctx, "Failed importing ParameterTable '"
                                   << ptName.toStdString() << "/"
                                   << compName.toStdString() << "'!" << std::endl);
                    }
                    else
                    {
                        QGraphicsProxyWidget* proxyWidget =
                                this->mModelScene->addWidget(tv, Qt::CustomizeWindowHint |
                                                    Qt::Window | Qt::WindowTitleHint);
                        proxyWidget->setFlag(QGraphicsItem::ItemIsMovable, true);
                        proxyWidget->setObjectName(compName);
                        proxyWidget->setPos(itemPos);
                        this->mModelScene->updateComponentItemFlags(proxyWidget);

                        connect(this->mModelScene, SIGNAL(widgetViewPortRightClicked(QGraphicsSceneMouseEvent*,QGraphicsItem*)),
                                tv, SLOT(processParaTableRightClick(QGraphicsSceneMouseEvent*,QGraphicsItem*)));
                        connect(this->mModelScene, SIGNAL(itemDblClicked(QGraphicsSceneMouseEvent*)),
                                tv, SLOT(processParaTableDblClick(QGraphicsSceneMouseEvent*)));
                    }
                }
                break;

		default:
            {
                std::stringstream msg;
                msg << "LUMASS detected an unknown graphics item "
                    << "in the LUMASS visualsation file (*.lmv)!\n"
                    << "Last successfully imported item '"
                    << importItems.last().toStdString() << "'. "
                    << "Check the imported model for errors!";


                NMBoxWarn("Unknown Graphics Item!",
                          msg.str());

                lmv.device()->close();
                return;
            }
		}
	}

    // --------------------------------------------
    // second iteration: link & group
    // --------------------------------------------

    // re-read header again to get to the point where the actual items are stored
    lmv.device()->seek(0);
    lmv.resetStatus();
    lmv >> fileIdentifier;
    if (fileIdentifier.compare(QString::fromLatin1("LUMASS Model Visualisation File")) == 0)
    {
        lmv >> lmv_version;
    }
    else
    {
        lmv.device()->seek(0);
        lmv.resetStatus();
    }

    // - establish links:
    //   since we don't know the order of components in the file
    //   because we didn't know it as we were saving the file
    //   we have to establish all links at the very end to make
    //   sure that all source/target items have already been
    //   processed and are available!
    // - establish group components
	while(!lmv.atEnd())
	{
		qint32 readType;
		lmv >> readType;
        //NMDebugAI(<< "item type is: " << (int)readType << endl);

        NMProcessComponentItem* pi = 0;
        NMAggregateComponentItem* ai = 0;
        NMComponentLinkItem* li = 0;
        QGraphicsTextItem* ti = 0;
		switch (readType)
		{
        case (qint32)QGraphicsTextItem::Type:
            ti = new QGraphicsTextItem();
            lmv >> *ti;
            delete ti;
            ti = 0;
            break;

		case (qint32)NMComponentLinkItem::Type:
				{
					int srcIdx, tarIdx;
					QString srcName, tarName;
					lmv >> srcIdx >> srcName >> tarIdx >> tarName;

                    bool dyn, bvis;
                    if (lmv_version > (qreal)0.91)
                    {
                        lmv >> dyn;
                    }

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
                        if (lmv_version > (qreal)0.91)
                        {
                            li->setIsDynamic(dyn);
                        }

                        si->addOutputLink(srcIdx, li);
						ti->addInputLink(tarIdx, li);
						this->mModelScene->addItem(li);

                        NMAggregateComponentItem* siParent = 0;
                        NMAggregateComponentItem* tiParent = 0;
                        if (si->parentItem())
                        {
                            siParent = qgraphicsitem_cast<NMAggregateComponentItem*>(si->parentItem());
                        }

                        if (ti->parentItem())
                        {
                            tiParent = qgraphicsitem_cast<NMAggregateComponentItem*>(ti->parentItem());
                        }
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
                delete pi;
                pi = 0;
				break;

		case (qint32)NMAggregateComponentItem::Type:
				{
                    QString title;
                    QPointF pos;
                    QColor color;
                    bool bCollapsed = false;
                    qint32 nkids;
                    lmv >> title >> pos >> color;

                    if (lmv_version >= 0.95)
                    {
                        lmv >> bCollapsed;
                    }

                    lmv >> nkids;

					title = nameRegister.value(title);
                    ai = qgraphicsitem_cast<NMAggregateComponentItem*>(
                            this->mModelScene->getComponentItem(title));

                    NMIterableComponent* c = qobject_cast<NMIterableComponent*>(
                                NMModelController::getInstance()->getComponent(title));
                    ai->updateDescription(c->getDescription());
                    ai->updateTimeLevel(c->getTimeLevel());
                    NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(c);
                    if (sic != 0)
                    {
                        ai->updateNumIterations(sic->getNumIterations());
                    }
                    else
                    {
                        ai->updateNumIterations(0);
                    }
                    // force update of activiy status of component
                    ai->slotProgress(c->getIterationStep());


                    QStringList subNames;
                    this->getSubComps(c, subNames);

                    if (ai == 0 || nkids < 1)
                        break;

                    NMProcessComponentItem* ipi = 0;
					NMAggregateComponentItem* iai = 0;
                    QGraphicsProxyWidget* pwi = 0;
                    for (unsigned int c=0; c < nkids; ++c)
					{
                        QString kname;
                        lmv >> kname;

                        if (kname.compare(QString::fromLatin1("TextLabel")) == 0)
                        {
                            ti = new QGraphicsTextItem(ai);
                            lmv >> *ti;
                            ti->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::TextBrowserInteraction);
                            ti->setFlag(QGraphicsItem::ItemIsMovable, true);
                            ti->setOpenExternalLinks(true);
                            mModelScene->addItem(ti);
                            ti->setPos(ai->mapFromScene(ti->pos()));

                            ai->addToGroup(ti);
                        }
                        else
                        {
                            kname = nameRegister.value(kname);
                            if (kname.isEmpty())
                            {
                                NMWarn(ctx, << kname.toStdString()
                                       << " is not a registered model component - we skip it!");
                                continue;
                            }

                            ipi = qgraphicsitem_cast<NMProcessComponentItem*>(
                                    this->mModelScene->getComponentItem(kname));
                            iai = qgraphicsitem_cast<NMAggregateComponentItem*>(
                                    this->mModelScene->getComponentItem(kname));
                            pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(
                                        this->mModelScene->getComponentItem(kname));

                            if (ipi != 0)
                                ai->addToGroup(ipi);
                            else if (iai != 0)
                                ai->addToGroup(iai);
                            else if (pwi != 0)
                                ai->addToGroup(pwi);
                        }
					}

                    connect(c, SIGNAL(ComponentDescriptionChanged(QString)),
                            ai, SLOT(updateDescription(QString)));
                    connect(c, SIGNAL(TimeLevelChanged(short)),
                            ai, SLOT(updateTimeLevel(short)));
                    connect(sic, SIGNAL(NumIterationsChanged(uint)),
                            ai, SLOT(updateNumIterations(uint)));

                    connect(c, SIGNAL(signalExecutionStarted()),
                            ai, SLOT(slotExecutionStarted()));
                    connect(c, SIGNAL(signalExecutionStopped()),
                            ai, SLOT(slotExecutionStopped()));
                    connect(sic, SIGNAL(signalProgress(float)),
                            ai, SLOT(slotProgress(float)));
				}
				break;

        case (qint32)QGraphicsProxyWidget::Type:
                {
                    QString ptName;
                    QPointF scenePos;

                    lmv >> ptName >> scenePos;
                }
                break;
		default:
			break;
		}
	}

    NMDebugAI(<< "exploded rect: " << this->reportRect(mModelScene->itemsBoundingRect(), "") << std::endl);


    // now we collapse all collapsed aggregate component items
    NMDebugAI(<< "establishing ancestory of ... " << std::endl);
    QList<NMAggregateComponentItem*> orderedItems;
    QList<QPointF> orderedPos;
    for (int a=0; a < collapsedItems.count(); ++a)
    {
        NMAggregateComponentItem* ai = collapsedItems.at(a);
        NMDebugAI( << "  ... " << ai->getTitle().toStdString() << std::endl);
        int pos = 0;
        for (int i=0; i < orderedItems.count(); ++i)
        {
            if (ai->isAncestorOf(orderedItems.at(i)))
            {
                ++pos;
            }
        }
        orderedItems.insert(pos, ai);
        orderedPos.insert(pos, collapsedPos.at(a));
    }

    NMDebug(<< std::endl);
    NMDebugAI(<< "kids to grand parents  ... " << std::endl);
    for (int i=0; i < orderedItems.count(); ++i)
    {
        NMAggregateComponentItem* ai = orderedItems.at(i);
        NMDebugAI(<< ai->getTitle().toStdString() << std::endl);
        ai->collapse(true);
        ai->setPos(ai->mapToParent(ai->mapFromScene(orderedPos.at(i))));
    }

    this->mModelScene->invalidate();

    NMDebugAI(<< "collapsed rect: " << this->reportRect(mModelScene->itemsBoundingRect(), "") << std::endl);

    if (move)
    {
        // determine import region
        QRectF importRegion;

        QList<QGraphicsItem*> moveItems;
        foreach(const QString& itemName, importItems)
        {
            QGraphicsItem* gi = this->mModelScene->getComponentItem(itemName);
            if (gi != 0)
            {
                importRegion = unionRects(importRegion, gi->sceneBoundingRect());
                moveItems <<  gi;
            }
        }

        foreach(QGraphicsItem* gti, importLabels)
        {
            importRegion = unionRects(importRegion, gti->sceneBoundingRect());
            moveItems << gti;
        }

        NMDebugAI(<< this->reportRect(importRegion, "import region") << std::endl);

        this->moveComponents(moveItems, importRegion.center(), mLastScenePos);
    }

    NMDebugAI(<< "moved & collapsed rect: " << this->reportRect(mModelScene->itemsBoundingRect(), "") << std::endl);

    NMDebugCtx(ctx, << "done!");
}


std::string
NMModelViewWidget::reportRect(const QRectF& rect, const char* msg)
{
    std::stringstream str;

    str << msg << " "
            << rect.center().x() << ","
            << rect.center().y() << " "
            << rect.width() << "x"
            << rect.height();

    return str.str();
}

std::string
NMModelViewWidget::reportLine(const QLineF& line, const char* msg)
{
    std::stringstream str;
    str << msg << " "
            << line.p1().x() << ","
            << line.p1().y() << " "
            << line.p2().x() << ","
            << line.p2().y();

    return str.str();
}

std::string
NMModelViewWidget::reportPoint(const QPointF& pt, const char* msg)
{
    std::stringstream str;
    str << msg << " "
            << pt.x() << ","
            << pt.y();

    return str.str();
}


void
NMModelViewWidget::zoomToContent()
{
    // go through the actual content of the scene, and determien the
    // convex hull of all items and set that as the scenes rect

    //this->mModelScene->setSceneRect(this->mModelScene->itemsBoundingRect());
    this->mModelView->fitInView(this->mModelScene->itemsBoundingRect(),
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
        QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

		if (pi != 0)
			comp = this->mModelController->getComponent(pi->getTitle());
		else if (ai != 0)
			comp = this->mModelController->getComponent(ai->getTitle());
        else if (pwi != 0)
            comp = this->mModelController->getComponent(pwi->objectName());
        else
            // in case of label item
            continue;

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
    if (allLevel && cnt > 0)
	{
		if (host != this->mRootComponent)
			ret = 1;
		else
			ret = 0;
	}

	return ret;
}

void
NMModelViewWidget::addDeltaTimeLevel()
{
    NMModelController* ctrl = NMModelController::getInstance();
    if (ctrl->isModelRunning())
    {
        NMDebugAI(<< "Cannot edit components while model is running!" << std::endl);
        return;
    }

    QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();
    if (selection.size() == 0)
    {
        NMDebugAI(<< "No group available for which to alter the time level!" << std::endl);
        return;
    }

    // get the minimum time level allowed
    NMModelComponent* pcomp;// = this->componentFromItem(selection.at(0));
//    if (pcomp == 0)
//    {
//        return;
//    }
//    int minLevel = pcomp->getHostComponent()->getTimeLevel();

    bool bok;
    int levelDelta = QInputDialog::getInt(this,
                         tr("Increase/Decrease Time Level"),
                         tr("Change time level by (+/-)"),
                         1,
                         -2147483647, 2147483647,
                         1,
                         &bok);
    if (bok)
    {
        foreach(QGraphicsItem* item, selection)
        {
            pcomp = this->componentFromItem(item);
            if (pcomp)
            {
                pcomp->setTimeLevel(pcomp->getTimeLevel() + levelDelta);
            }
        }
    }

}

void
NMModelViewWidget::setGroupTimeLevel()
{
    NMModelController* ctrl = NMModelController::getInstance();
    if (ctrl->isModelRunning())
    {
        NMDebugAI(<< "Cannot edit components while model is running!" << std::endl);
        return;
    }

    QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();
    if (selection.size() == 0)
    {
        NMDebugAI(<< "No group available to set the time level for!" << std::endl);
        return;
    }

    // get the minimum time level allowed
    NMModelComponent* pcomp = this->componentFromItem(selection.at(0));
    if (pcomp == 0)
    {
        return;
    }
    int minLevel = pcomp->getHostComponent()->getTimeLevel();

    bool bok;
    int userLevel = QInputDialog::getInt(this,
                         tr("Set Time Level"),
                         tr("Time Level"),
                         minLevel,
                         minLevel, 2147483647,
                         1,
                         &bok);
    if (bok)
    {
        foreach(QGraphicsItem* item, selection)
        {
            pcomp = this->componentFromItem(item);
            if (pcomp)
            {
                pcomp->setTimeLevel(userLevel);
            }
        }
    }
}

void NMModelViewWidget::ungroupComponents()
{
	NMDebugCtx(ctx, << "...");
	QList<QGraphicsItem*> selection = this->mModelScene->selectedItems();
	if (selection.count() == 0)
    {
        NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);
        if (ai != 0)
        {
            //QList<QGraphicsItem*> kids = ai->childItems();
            QList<QGraphicsItem*> kids = ai->getModelChildren();
            QList<QGraphicsItem*>::iterator iit = kids.begin();
            while(iit != kids.end())
            {
                if (    (*iit)->type() == NMAggregateComponentItem::Type
                    ||  (*iit)->type() == NMProcessComponentItem::Type
                    ||  (*iit)->type() == QGraphicsTextItem::Type
                    ||  (*iit)->type() == QGraphicsProxyWidget::Type
                   )
                {
                    selection.push_back(*iit);
                }
                ++iit;
            }
        }
        else
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }

        if (selection.count() == 0)
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }

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
        if (c != 0)
        {
            host->removeModelComponent(c->objectName());
            hosthost->addModelComponent(c);
            const unsigned int htl = hosthost->getTimeLevel();
            const unsigned int ctl = c->getTimeLevel();
            c->setTimeLevel(ctl < htl ? htl : ctl);

            if (this->mOpenEditors.contains(c))
            {
                this->mOpenEditors.value(c)->update();
            }
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
	if (item == 0)
	{
		return 0;
	}

	NMModelComponent* ret = 0;
	NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
	NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
    QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

    if (pi != 0)
		ret = this->mModelController->getComponent(pi->getTitle());
	else if (ai != 0)
		ret = this->mModelController->getComponent(ai->getTitle());
    else if (pwi != 0)
        ret = this->mModelController->getComponent(pwi->objectName());

	return ret;
}

QString NMModelViewWidget::getComponentItemTitle(QGraphicsItem* item)
{
	QString ret;
	NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
	NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
    QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

	if (pi != 0)
		ret = pi->getTitle();
	else if (ai != 0)
		ret = ai->getTitle();
    else if (pwi != 0)
        ret = pwi->objectName();

	return ret;
}

void NMModelViewWidget::deleteItem()
{
	NMDebugCtx(ctx, << "...");

	NMProcessComponentItem* procItem;
	NMAggregateComponentItem* aggrItem;
    NMComponentLinkItem* linkItem;
    QGraphicsTextItem* labelItem;
    QGraphicsProxyWidget* proxyWidget = 0;
    QList<QGraphicsProxyWidget*> widgetItems;

	QStringList delList;
    QList<QGraphicsTextItem*> delLabels;
	if (this->mModelScene->selectedItems().count())
	{
		foreach(QGraphicsItem* gi, this->mModelScene->selectedItems())
		{
			procItem = qgraphicsitem_cast<NMProcessComponentItem*>(gi);
			aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(gi);
            labelItem = qgraphicsitem_cast<QGraphicsTextItem*>(gi);
            proxyWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(gi);

			if (procItem != 0)
				delList.push_back(procItem->getTitle());
			else if (aggrItem != 0)
				delList.push_back(aggrItem->getTitle());
            // we only deal with root-labels here, any other labels
            // are dealt with in deleteAggregateComponent
            else if (labelItem != 0 && labelItem->parentItem() == 0)
                delLabels.push_back(labelItem);
            else if (proxyWidget != 0)
            {
                widgetItems.push_back(proxyWidget);
            }
		}
	}
    else if (this->mLastItem != 0)
	{
		procItem = qgraphicsitem_cast<NMProcessComponentItem*>(this->mLastItem);
		aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(this->mLastItem);
		linkItem = qgraphicsitem_cast<NMComponentLinkItem*>(this->mLastItem);
        labelItem = qgraphicsitem_cast<QGraphicsTextItem*>(this->mLastItem);
        proxyWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(this->mLastItem);
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
        else if (labelItem != 0)
        {
            delLabels.push_back(labelItem);
        }
        else if (proxyWidget != 0)
        {
            widgetItems.push_back(proxyWidget);
        }
	}

    int ndelitems = delList.count();
    ndelitems += delLabels.count();
    ndelitems += widgetItems.count();

    // ==================================================================
    // accidentally deleting components can be really
    // painful! (as I've just experienced)
    // Ask the user whether he's really sure to delete!
    // Maybe she/he is still a bit tired and under
    // pressure and the least thing he/she wants is
    // starting all over again!

    QString msg = QString(tr("Do you really want to delete %1 model %2?"))
                    .arg(ndelitems)
                    .arg((ndelitems ? "component" : "components"));

    QMessageBox::StandardButton answer = QMessageBox::question(this,
                                         tr("Delete Model Components"),
                                         msg);

    if (answer == QMessageBox::No)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }


    // ==================================================================

    // remove labels first, then deal with the rest
    foreach(QGraphicsTextItem* ti, delLabels)
    {
        QGraphicsTextItem* dti = qgraphicsitem_cast<QGraphicsTextItem*>(mLastItem);
        if (dti == ti)
        {
            mLastItem = 0;
        }
        this->mModelScene->removeItem(ti);
        delete ti;
        ti = 0;
    }

    foreach(QGraphicsProxyWidget* pw, widgetItems)
    {
        this->deleteProxyWidget(pw);
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

QStringList
NMModelViewWidget::dynamicInputs(QList<QStringList>& inputs)
{
    NMDebugCtx(ctx, << "...");

    QStringList dynMembers;

    foreach(const QStringList& stepList, inputs)
    {
        foreach(const QString& stepInput, stepList)
        {
            bool persistent = true;
            foreach(const QStringList& _stepList, inputs)
            {
                if (!_stepList.contains(stepInput))
                {
                    persistent = false;
                    dynMembers.push_back(stepInput);
                    break;
                }
            }
        }
    }

    // debug
    NMDebugAI( << "dynamic members..." << std::endl);
    foreach(const QString& mem, dynMembers)
    {
        NMDebugAI(<< "  >> " << mem.toStdString() << std::endl);
    }

    NMDebugCtx(ctx, << "done!");
    return dynMembers;
}

void
NMModelViewWidget::processProcInputChanged(QList<QStringList> inputs)
{
	NMDebugCtx(ctx, << "...");


    NMProcess* sender = qobject_cast<NMProcess*>(this->sender());
    if (sender == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QString senderName = sender->parent()->objectName();
    QGraphicsItem* gi = this->mModelScene->getComponentItem(senderName);
    if (gi == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }
    NMProcessComponentItem* procItem =
            qgraphicsitem_cast<NMProcessComponentItem*>(gi);
    if (procItem == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }
    QList<NMComponentLinkItem*> inputLinks = procItem->getInputLinks();

    // get a list of dynamic inputs
    QStringList dynamicInputs = this->dynamicInputs(inputs);


    //QStringList srclist;
    //	if (!inputs.isEmpty())
    //		srclist = inputs.at(0);
    foreach(const QStringList& srclist, inputs)
    {
        // strip off any position indices from the source input name
        QStringList list;
        foreach(const QString& src, srclist)
        {
            if (!src.isEmpty())
                list.push_back(src.split(":", QString::SkipEmptyParts).at(0));
        }

        // remove (persistent) input links
        for (int a=0; a < inputLinks.size(); ++a)
        {
            NMComponentLinkItem* link = inputLinks.at(a);

            if (    !list.contains(procItem->identifyInputLink(a))
                &&  !link->getIsDynamic()
               )
            {
                NMProcessComponentItem* src = link->sourceItem();
                src->removeLink(link);
                procItem->removeLink(link);
                if (link->scene() == qobject_cast<QGraphicsScene*>(this->mModelScene))
                {
                    this->mModelScene->removeItem(link);
                }
                link->deleteLater();
            }
        }

        // add inputs and update the ones already present
        for (int b=0; b < list.size(); ++b)
        {
            NMProcessComponentItem* si = 0;
            NMComponentLinkItem* li = 0;
            int inputLinkIndex = procItem->getInputLinkIndex(list.at(b));
            if (inputLinkIndex < 0)
            {
                si = qgraphicsitem_cast<NMProcessComponentItem*>(
                            this->mModelScene->getComponentItem(list.at(b)));
                if (si != 0)
                {
                    li = new NMComponentLinkItem(si, procItem, 0);
                    li->setZValue(this->mModelScene->getLinkZLevel());
                    if (dynamicInputs.contains(srclist.at(b)))
                    {
                        li->setIsDynamic(true);
                    }
                    si->addOutputLink(-1, li);
                    procItem->addInputLink(b, li);
                    this->mModelScene->addItem(li);
                }
            }
            // we also look after those, which are already present
            // since they might have change persistency
            else
            {
                li = procItem->getInputLinks().at(inputLinkIndex);
                if (dynamicInputs.contains(srclist.at(b)))
                {
                    li->setIsDynamic(true);
                }
                else
                {
                    li->setIsDynamic(false);
                }
            }
        }
    }

	NMDebugCtx(ctx, << "done!");
}


void
NMModelViewWidget::deleteLinkComponentItem(NMComponentLinkItem* linkItem)
{
    NMComponentLinkItem* dli = qgraphicsitem_cast<NMComponentLinkItem*>(mLastItem);
    if (dli == linkItem)
    {
        mLastItem = 0;
    }


	NMProcessComponentItem* targetItem = linkItem->targetItem();
	NMProcessComponentItem* sourceItem = linkItem->sourceItem();
	if (targetItem == 0 || sourceItem == 0)
		return;

	NMModelComponent* targetComp = this->mModelController->getComponent(targetItem->getTitle());
	if (targetComp == 0)
		return;

	NMIterableComponent* itComp =
			qobject_cast<NMIterableComponent*>(targetComp);
    NMDataComponent* dataComp =
            qobject_cast<NMDataComponent*>(targetComp);
    if (itComp)
    {
        NMProcess* proc = itComp->getProcess();
        if (proc == 0)
            return;

        proc->removeInputComponent(sourceItem->getTitle());
    }
    else if (dataComp)
    {
        dataComp->setInputs(QList<QStringList>());
    }
    else
    {
        return;
    }

	if (this->mOpenEditors.contains(targetComp))
		this->mOpenEditors.value(targetComp)->update();

	// remove all traces of the link
	targetItem->removeLink(linkItem);
	sourceItem->removeLink(linkItem);
	this->mModelScene->removeItem(linkItem);
    delete linkItem;
    linkItem = 0;
}

void
NMModelViewWidget::deleteAggregateComponentItem(NMAggregateComponentItem* aggrItem)
{
	NMDebugCtx(ctx, << "...");

    NMAggregateComponentItem* dai = qgraphicsitem_cast<NMAggregateComponentItem*>(mLastItem);
    if (dai == aggrItem)
    {
        mLastItem = 0;
    }

	// object's context, we need further down the track
	QString aggrTitle = aggrItem->getTitle();
	NMModelComponent* comp = mModelController->getComponent(aggrTitle);
	NMModelComponent* hostComp = comp->getHostComponent();

	// iterate over all subcomponents and delete them
    //QList<QGraphicsItem*> childItems = aggrItem->childItems();
    QList<QGraphicsItem*> childItems = aggrItem->getModelChildren();
    QMutableListIterator<QGraphicsItem*> it(childItems);
	NMModelComponent* pcomp = 0;
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		NMProcessComponentItem* procChildItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aggrChildItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
        QGraphicsTextItem* labelItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
        QGraphicsProxyWidget* proxyWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);

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
        else if (labelItem != 0)
        {
            mModelScene->removeItem(labelItem);
            delete labelItem;
            labelItem = 0;
        }
        else if (proxyWidget != 0)
        {
            this->deleteProxyWidget(proxyWidget);
        }
	}

	// finally remove the component itself
	QString aggrName = aggrItem->getTitle();
	mModelScene->removeItem(aggrItem);
	if (!mModelController->removeComponent(aggrName))
	{
        NMDebugAI(<< "failed to remove component '"
				<< aggrName.toStdString() << "'!");
	}

	this->deleteEmptyComponent(hostComp);

	NMDebugCtx(ctx, << "done!");
}

void
NMModelViewWidget::deleteProxyWidget(QGraphicsProxyWidget *pw)
{
    QGraphicsProxyWidget* dpwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(mLastItem);
    if (pw == dpwi)
    {
        mLastItem = 0;
    }


    QString name = pw->objectName();
    QString title = pw->windowTitle();
    if (!NMModelController::getInstance()->removeComponent(name))
    {
        NMErr(ctx, << "Failed to delete '" << pw->windowTitle().toStdString() << "'!");
    }
    this->mModelScene->removeItem(pw);
	NMGlobalHelper::getMainWindow()->deleteTableObject(title);
	
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
        if (item == mLastItem)
            mLastItem = 0;
		mModelController->removeComponent(comp->objectName());
	}

	NMDebugCtx(ctx, << "done!");
}

void NMModelViewWidget::editRootComponent()
{
	if (this->mRootComponent != 0)
		this->callEditComponentDialog(this->mRootComponent->objectName());
}

//void NMModelViewWidget::compProcChanged()
//{
////	NMDebugCtx(ctx, << "...");
////	NMModelComponent* comp = qobject_cast<NMModelComponent*>(this->sender());
////	NMProcess* proc = qobject_cast<NMProcess*>(this->sender());
////
////	NMDebugCtx(ctx, << "done!");
//}

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
    //NMModelComponent* comp = qobject_cast<NMModelComponent*>(proc->parent());
    NMIterableComponent* comp = qobject_cast<NMIterableComponent*>(proc->parent());
	connect(comp, SIGNAL(ComponentDescriptionChanged(const QString &)), procItem,
            SLOT(setDescription(const QString &)));
    connect(comp, SIGNAL(TimeLevelChanged(short)), procItem,
            SLOT(updateTimeLevel(short)));
}

void
NMModelViewWidget::createProcessComponent(NMProcessComponentItem* procItem,
		const QString& procName, QPointF scenePos)
{
	NMDebugCtx(ctx, << "...");

    //QString compName;
	QString tname = procName;
	unsigned int cnt = 1;
	while (this->mModelController->contains(tname))
	{
		tname = QString(tr("%1%2")).arg(procName).arg(cnt);
		++cnt;
	}
	NMDebugAI(<< "finale name of component is '" << tname.toStdString() << "'" << endl);
    procItem->setTypeID(cnt-1);

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
        connect(dataComp, SIGNAL(ComponentDescriptionChanged(const QString &)),
                procItem, SLOT(setDescription(const QString &)));
        connect(dataComp, SIGNAL(TimeLevelChanged(short)),
                procItem, SLOT(updateTimeLevel(short)));
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

    if (comp == 0)
	{
        NMErr(ctx, << "Component creation failed! proc=" << proc << " | comp=" << comp);
        NMDebugCtx(ctx, << "done!")
        return;
	}
    NMDebugAI(<< "and its object name is '" << comp->objectName().toStdString() << "'" << endl);

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
    procItem->setTimeLevel(host->getTimeLevel());


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
        }
        return;
    }

    if (mTreeCompEditor == 0)
    {
        LUMASSMainWin* otbwin = NMGlobalHelper::getMainWindow();//this->getMainWindow();
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
    //mTreeCompEditor->show();
}

void NMModelViewWidget::callEditComponentDialog(const QString& compName)
{
    if (    compName.isEmpty()
        ||  compName.startsWith(QString::fromLatin1("ParameterTable"))
       )
    {
		return;
    }

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
    else
    {
        this->mOpenEditors.value(comp)->activateWindow();
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
            mTreeCompEditor->setObject(0);
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
    // store last item


    //mLastItem = this->mModelScene->itemAt(this->mModelView->mapToScene(event->pos()), this->mModelView->transform());

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
    else if (e->type() == QEvent::MouseButtonPress)
    {
        emit modelViewActivated(this);
    }



//    else if (e->type() == QEvent::Drop)
//    {
//        QDropEvent* de = static_cast<QDropEvent*>(e);
//        if (de != 0)
//        {
//            //mLastScenePos = this->mModelView->mapToScene(de->pos());
//            //mLastItem = this->mModelScene->itemAt(mLastScenePos, this->mModelView->transform());
//        }
//    }
    return false;
}

//LUMASSMainWin*
//NMModelViewWidget::getMainWindow(void)
//{
//    LUMASSMainWin* mainWin = 0;
//    QWidgetList tlw = qApp->topLevelWidgets();
//    QWidgetList::ConstIterator it = tlw.constBegin();
//    for (; it != tlw.constEnd(); ++it)
//    {
//        QWidget* w = const_cast<QWidget*>(*it);
//        if (w->objectName().compare("LUMASSMainWin") == 0)
//        {
//            mainWin = qobject_cast<LUMASSMainWin*>(w);
//        }
//    }

//    return mainWin;
//}
