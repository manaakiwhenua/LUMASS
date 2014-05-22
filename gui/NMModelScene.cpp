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
 * NMModelScene.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include <QMimeData>
#include <QString>
#include <QObject>
#include <QStatusBar>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QDebug>

#include "NMModelScene.h"
#include "NMModelViewWidget.h"
#include "NMProcessComponentItem.h"
#include "NMAggregateComponentItem.h"
#include "otbmodellerwin.h"
#include "nmlog.h"

class NMModelViewWidget;

const std::string NMModelScene::ctx = "NMModelScene";


NMModelScene::NMModelScene(QObject* parent)
	: QGraphicsScene(parent)
{
	//ctx = "NMModelScene";
    mMode = NMS_MOVE;
	mLinkHitTolerance = 15;
	mLinkZLevel = 10000;
	mLinkLine = 0;
}

NMModelScene::~NMModelScene()
{
}

void NMModelScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat("text/plain"))
		event->acceptProposedAction();
}

void NMModelScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat("text/plain"))
		event->acceptProposedAction();
}

void
NMModelScene::toggleLinkToolButton(bool linkMode)
{
	if (linkMode)
	{
		this->views().at(0)->setDragMode(QGraphicsView::NoDrag);
		this->views().at(0)->setCursor(Qt::CrossCursor);
		this->mMode = NMS_LINK;
	}
	else
		this->mMode = NMS_IDLE;
}

void
NMModelScene::updateComponentItemFlags(QGraphicsItem *item)
{
    switch(mMode)
    {
    case NMS_MOVE:
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        break;
    case NMS_LINK:
        break;
    case NMS_SELECT:
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        break;
    case NMS_IDLE:
        break;
    }
}

void NMModelScene::setProcCompSelectability(bool selectable)
{
	QList<QGraphicsItem*> allItems = this->items();
	QListIterator<QGraphicsItem*> it(allItems);
	NMProcessComponentItem* item;
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		if (item != 0)
		{
			item->setFlag(QGraphicsItem::ItemIsSelectable, selectable);
		}
	}
}

void NMModelScene::setLinkCompSelectability(bool selectable)
{
	QList<QGraphicsItem*> allItems = this->items();
	QListIterator<QGraphicsItem*> it(allItems);
	NMComponentLinkItem* item;
	while(it.hasNext())
	{
		item = qgraphicsitem_cast<NMComponentLinkItem*>(it.next());
		if (item != 0)
			item->setFlag(QGraphicsItem::ItemIsSelectable, selectable);
	}
}


void NMModelScene::setProcCompMoveability(bool moveable)
{
	QList<QGraphicsItem*> allItems = this->items();
	QListIterator<QGraphicsItem*> it(allItems);
	NMProcessComponentItem* item;
	while(it.hasNext())
	{
		it.next()->setFlag(QGraphicsItem::ItemIsMovable, moveable);
	}
}


void NMModelScene::toggleSelToolButton(bool selMode)
{
	if (selMode)
	{
		this->views().at(0)->setDragMode(QGraphicsView::RubberBandDrag);
		this->views().at(0)->setCursor(Qt::PointingHandCursor);
		this->mMode = NMS_SELECT;
		this->setProcCompSelectability(true);
		this->setLinkCompSelectability(false);
	}
	else
	{
		this->views().at(0)->setDragMode(QGraphicsView::NoDrag);
		this->mMode = NMS_IDLE;
		this->setProcCompSelectability(false);
		//this->setLinkCompSelectability(false);
	}
}

void NMModelScene::toggleMoveToolButton(bool moveMode)
{
	if (moveMode)
	{
		this->views().at(0)->setDragMode(QGraphicsView::ScrollHandDrag);
		this->mMode = NMS_MOVE;
		this->setProcCompMoveability(true);
	}
	else
	{
		this->views().at(0)->setDragMode(QGraphicsView::NoDrag);
		this->mMode = NMS_IDLE;
		this->setProcCompMoveability(false);
	}
}

QGraphicsItem*
NMModelScene::getComponentItem(const QString& name)
{
	QGraphicsItem* retItem = 0;
	QList<QGraphicsItem*> allItems = this->items();
	QListIterator<QGraphicsItem*> it(allItems);
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		NMProcessComponentItem* procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

		if (procItem != 0)
		{
			if (procItem->getTitle().compare(name) == 0)
				return procItem;
		}
		else if (aggrItem != 0)
		{
			if (aggrItem->getTitle().compare(name) == 0)
				return aggrItem;
		}
	}

	return retItem;
}

void NMModelScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
	NMDebugCtx(ctx, << "...");
	if (event->mimeData()->hasFormat("text/plain"))
	{
		QString dropText = event->mimeData()->text();
		//NMDebugCtx(ctx, << "dropText = " << dropText.toStdString() << std::endl);

		if (NMModelController::getInstance()->isModelRunning())
        {
			QMessageBox::information(0, "Invalid user request!",
					"You cannot create new model components\nwhile a "
					"model is being executed! Please try again later!");

			NMErr(ctx, << "You cannot create new model components while there is "
					     "a model running. Please try again later!");
			NMDebugCtx(ctx, << "done!");
			return;
		}

        if (dropText.compare(QString::fromLatin1("TextLabel")) == 0)
        {
            QGraphicsItem* pi = this->itemAt(event->scenePos(), this->views()[0]->transform());
            NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(pi);
            QGraphicsTextItem* labelItem = new QGraphicsTextItem(ai);
            labelItem->setHtml(QString::fromLatin1("<h2><b>Text Label</b></h2>"));
            labelItem->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::TextBrowserInteraction);
            labelItem->setFlag(QGraphicsItem::ItemIsMovable, true);
            labelItem->setOpenExternalLinks(true);
            if (ai != 0)
            {
                ai->addToGroup(labelItem);
                labelItem->setPos(ai->mapFromScene(event->scenePos()));
            }
            else
            {
                this->addItem(labelItem);
                labelItem->setPos(event->scenePos());
            }
        }
        else
        {
            NMProcessComponentItem* procItem = new NMProcessComponentItem(0, this);
            procItem->setTitle(dropText);
            procItem->setDescription(dropText);
            procItem->setPos(event->scenePos());
            if (dropText.compare("DataBuffer") == 0)
            {
                procItem->setIsDataBufferItem(true);
            }
            procItem->setFlag(QGraphicsItem::ItemIsMovable, true);
            event->acceptProposedAction();

            NMDebugAI(<< "asking for creating '" << dropText.toStdString() << "' ..." << endl);
            emit processItemCreated(procItem, dropText, event->scenePos());
        }
	}

	NMDebugCtx(ctx, << "done!");
}

void
NMModelScene::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    mMousePos = event->scenePos();
    //qDebug() << "wheelEvent(): scene pos: " << mMousePos;
    emit zoom(event->delta());
}

void
NMModelScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
        QGraphicsItem* item = this->itemAt(event->scenePos(), this->views()[0]->transform());
		NMProcessComponentItem* procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
		NMAggregateComponentItem* aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
		if (item == 0)
		{
			emit rootComponentDblClicked();
		}
		else if (procItem != 0)
		{
			emit procAggregateCompDblClicked(procItem->getTitle());
		}
		else if (aggrItem != 0)
		{
			emit procAggregateCompDblClicked(aggrItem->getTitle());
		}
	}
	else
	{
		QGraphicsScene::mouseDoubleClickEvent(event);
	}
}

void
NMModelScene::serialiseItems(QList<QGraphicsItem*> items, QDataStream& data)
{

}

void
NMModelScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem* item = this->itemAt(event->scenePos(), this->views()[0]->transform());
    NMProcessComponentItem* procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
    NMAggregateComponentItem* aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);;

	if (event->button() == Qt::LeftButton)
	{
		switch(mMode)
		{
		case NMS_LINK:
			mLinkLine = new QGraphicsLineItem(
					QLineF(event->scenePos(),
							event->scenePos()));
			mLinkLine->setPen(QPen(Qt::darkGray, 2));
			this->addItem(mLinkLine);
			break;

		default:
            if (procItem)
            {
                emit itemLeftClicked(procItem->getTitle());
            }
            else if (aggrItem)
            {
                emit itemLeftClicked(aggrItem->getTitle());
            }
            else if (item == 0)
            {
                emit itemLeftClicked(QString::fromUtf8("root"));
            }
			break;
		}
		QGraphicsScene::mousePressEvent(event);
	}
	else if (event->button() == Qt::RightButton)
	{
		QGraphicsItem* sendItem = 0;

		// first, we check, whether we've got a link on the hook
		sendItem = this->getLinkItem(event->scenePos());
		if (sendItem == 0 && item != 0)
        {
			sendItem = item;
        }

		emit itemRightBtnClicked(event, sendItem);
	}
	else
	{
		QGraphicsScene::mousePressEvent(event);
	}
}

NMComponentLinkItem* NMModelScene::getLinkItem(QPointF pos)
{
    QGraphicsItem* item = this->itemAt(pos, this->views()[0]->transform());
	NMComponentLinkItem* link = qgraphicsitem_cast<NMComponentLinkItem*>(item);

	if (link == 0)
	{
		QPointF p = pos;
		qreal dxy = this->mLinkHitTolerance / 2.0;
		qreal wh = this->mLinkHitTolerance;
		QList<QGraphicsItem*> listItems = this->items(p.x()-dxy, p.y()-dxy, wh, wh,
				Qt::IntersectsItemShape, Qt::DescendingOrder);
		foreach(QGraphicsItem* i, listItems)
		{
			link = qgraphicsitem_cast<NMComponentLinkItem*>(i);
			if (link != 0)
			{
				NMProcessComponentItem* src = const_cast<NMProcessComponentItem*>(link->sourceItem());
				NMProcessComponentItem* tar = const_cast<NMProcessComponentItem*>(link->targetItem());
				NMDebugAI(<< "link from " << src->getTitle().toStdString()
						<< " to " << tar->getTitle().toStdString() << endl);
				break;
			}
		}
	}

	return link;
}

void
NMModelScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    // update coordinate label in status bar
    QPointF sp = event->scenePos();
    QGraphicsItem* item = this->itemAt(sp, this->views()[0]->transform());
    NMAggregateComponentItem* ai = 0;
    NMProcessComponentItem* pi = 0;
    qreal x = sp.x();
    qreal y = sp.y();
    QString title = "Scene";
    if (item != 0)
    {
        pi = qgraphicsitem_cast<NMProcessComponentItem*>(item);
        if (pi != 0)
        {
            if (pi->parentItem() != 0)
            {
                ai = qgraphicsitem_cast<NMAggregateComponentItem*>(pi->parentItem());
            }
        }
        else
        {
             ai = qgraphicsitem_cast<NMAggregateComponentItem*>(item);
        }

        if (ai != 0)
        {
            title = ai->getTitle();
            QPointF aip = ai->mapFromScene(sp);
            x = aip.x();
            y = aip.y();
        }
    }

    QString pos = QString("Scene Position - X: %1 Y: %2 || %3 Position - X: %4 Y: %5")
            .arg(event->scenePos().x())
            .arg(event->scenePos().y())
            .arg(title)
            .arg(x)
            .arg(y);
    NMModelViewWidget* vw = qobject_cast<NMModelViewWidget*>(this->views().at(0)->parent());
    OtbModellerWin* mainWin = 0;
    if (vw != 0)
    {
        mainWin = vw->getMainWindow();
        if (mainWin != 0)
        {
            mainWin->updateCoordLabel(pos);
        }
    }

    mMousePos = event->scenePos();

	switch(mMode)
	{
	case NMS_LINK:
		if (mLinkLine == 0)
			break;
		mLinkLine->setLine(QLineF(mLinkLine->line().p1(), event->scenePos()));
		break;

	case NMS_MOVE:
		this->invalidate();
		break;

	default:
		break;
	}

    QGraphicsScene::mouseMoveEvent(event);
}

void
NMModelScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	NMProcessComponentItem* srcComp = 0;
	NMProcessComponentItem* tarComp = 0;
	NMComponentLinkItem* link = 0;
	QList<QGraphicsItem*> srcList;
	QList<QGraphicsItem*> tarList;
	switch(mMode)
	{
	case NMS_LINK:
		if (mLinkLine == 0)
			break;

		srcList = items(mLinkLine->line().p1());
		if (srcList.count() && srcList.first() == mLinkLine)
			srcList.removeFirst();
		tarList = items(mLinkLine->line().p2());
		if (srcList.count() && tarList.first() == mLinkLine)
			tarList.removeFirst();

		removeItem(mLinkLine);
		delete mLinkLine;
		mLinkLine = 0;

		if (srcList.count() > 0 && tarList.count() > 0 &&
			srcList.first() != tarList.first())
		{
			srcComp =
					qgraphicsitem_cast<NMProcessComponentItem*>(srcList.first());
			tarComp =
					qgraphicsitem_cast<NMProcessComponentItem*>(tarList.first());

			if (srcComp == 0 || tarComp == 0)
				break;
			int st = srcComp->type();
			int tt = tarComp->type();

			NMDebugAI(<< "types are: " << st << " "
					<< tt << std::endl);

			link = new NMComponentLinkItem(
					srcComp, tarComp, 0);

			srcComp->addOutputLink(-1, link);
			tarComp->addInputLink(-1, link);

			addItem(link);
			link->setZValue(this->mLinkZLevel);
			emit linkItemCreated(link);
		}

		break;

	default:
		break;
	}

	QGraphicsScene::mouseReleaseEvent(event);
}



