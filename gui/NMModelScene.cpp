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
    mMode = NMS_IDLE;
    mbSceneMove = false;
    mLinkHitTolerance = 15;
	mLinkZLevel = 10000;
	mLinkLine = 0;
}

NMModelScene::~NMModelScene()
{
}

void
NMModelScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    NMDebugCtx(ctx, << "...");
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString leaveText = event->mimeData()->text();
        if (leaveText.startsWith(QString::fromLatin1("_NMModelScene_:")))
        {
            if (mDragItemList.count() == 1)
            {
                NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(
                            mDragItemList.at(0));
                if (pi != 0)
                {
                    if (pi->getIsDataBufferItem())
                    {
                        leaveText = QString::fromLatin1("_NMModelScene_:%1").arg(pi->getTitle());
                        QMimeData* md = const_cast<QMimeData*>(event->mimeData());
                        md->setText(leaveText);
                        event->setMimeData(md);
                        event->accept();
                    }
                }
            }
        }
    }
    mDragItemList.clear();
    NMDebugCtx(ctx, << "done!");

    //QGraphicsScene::dragLeaveEvent(event);
}

void
NMModelScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    NMDebugCtx(ctx, << "...");

    if (    event->mimeData()->hasFormat("text/plain")
        ||  event->mimeData()->hasUrls())
    {
        QString fileName;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (    url.isLocalFile()
                &&  (   url.toLocalFile().endsWith(QString::fromLatin1("lmv"))
                     || url.toLocalFile().endsWith(QString::fromLatin1("lmx"))
                    )
               )
            {
                fileName = url.toLocalFile();
                break;
            }
        }

        NMDebugAI(<< event->mimeData()->text().toStdString());
        QString mimeText = event->mimeData()->text();
        if (    mimeText.startsWith(QString::fromLatin1("_NMProcCompList_:"))
            ||  mimeText.startsWith(QString::fromLatin1("_NMModelScene_:"))
            ||  mimeText.startsWith(QString::fromLatin1("_ModelComponentList_:"))
            ||  !fileName.isEmpty()
           )
        {
            NMDebug(<< " - supported!" << std::endl);
            event->acceptProposedAction();
        }
        else
        {
            NMDebug(<< " - unfortunately not supported!" << std::endl);
        }
    }

    NMDebugCtx(ctx, << "done!");
}

void
NMModelScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
//    QGraphicsScene::dragMoveEvent(event);
//    NMDebugCtx(ctx, << "...");
    if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();

//    NMDebugCtx(ctx, << "done!");
//    else
//    {
//        QGraphicsItem* i = this->getComponentItem(event->mimeData()->text());
//        if (i != 0)
//        {
//            event->acceptProposedAction();
//        }
//    }
}

void
NMModelScene::toggleLinkToolButton(bool linkMode)
{
	if (linkMode)
	{
		this->views().at(0)->setDragMode(QGraphicsView::NoDrag);
		this->views().at(0)->setCursor(Qt::CrossCursor);
        this->setProcCompMoveability(false);
		this->mMode = NMS_LINK;
	}
	else
    {
        this->setProcCompMoveability(true);
        this->views().at(0)->setCursor(Qt::OpenHandCursor);
        this->mMode = NMS_MOVE;
    }
}

void
NMModelScene::updateComponentItemFlags(QGraphicsItem *item)
{
    switch(mMode)
    {
    case NMS_LINK:
        //item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        break;
//    case NMS_MOVE:
//        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
//        item->setFlag(QGraphicsItem::ItemIsMovable, false);
//        break;
    case NMS_SELECT:
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
//        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        break;
    case NMS_IDLE:
    default:
//        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        break;
    }

    if (mbSceneMove)
    {
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
    else
    {
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
    }
}

void NMModelScene::setProcCompSelectability(bool selectable)
{
	QList<QGraphicsItem*> allItems = this->items();
	QListIterator<QGraphicsItem*> it(allItems);
    //NMProcessComponentItem* item;
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
    //NMProcessComponentItem* item;
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
        //this->views().at(0)->setDragMode(QGraphicsView::NoDrag);
        //this->mMode = NMS_MOVE;
        mbSceneMove = true;
        this->setProcCompMoveability(false);
	}
	else
	{
        //this->views().at(0)->setDragMode(QGraphicsView::ScrollHandDrag);
        //this->mMode = NMS_IDLE;
        mbSceneMove = false;
        this->setProcCompMoveability(true);
	}
    this->invalidate();
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
    mMousePos = event->scenePos();
    if (    event->mimeData()->hasFormat("text/plain")
        ||  event->mimeData()->hasUrls())
	{
		QString dropText = event->mimeData()->text();
        QStringList dropsplit = dropText.split(':');
        QString dropSource = dropsplit.at(0);
        QString dropItem;
        if (dropsplit.count() > 1)
        {
            dropItem = dropsplit.at(1);
        }

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

        // do something depending on the source of the object

        // internal drag'n'drop for copying or moving of objects
        if (dropItem.isEmpty())
        {
            NMDebugAI(<< "nothing to be done - no drop item provided!" << std::endl);
            NMDebugCtx(ctx, << "done!");
            return;
        }
        else if (dropSource.startsWith(QString::fromLatin1("_NMModelScene_")))
        {
            //event->acceptProposedAction();
            QPointF dropPos = event->scenePos();
            switch(event->dropAction())
            {
            case Qt::MoveAction:
                NMDebugAI( << "moving: " << mDragItemList.count() << " items" << std::endl);
                emit signalItemMove(mDragItemList, mDragStartPos, dropPos);
                break;
            case Qt::CopyAction:
                NMDebugAI( << "copying: " << mDragItemList.count() << " items" << std::endl);
                emit signalItemCopy(mDragItemList, mDragStartPos, dropPos);
                break;
            }
            this->mDragItemList.clear();
        }
        else if (dropSource.startsWith(QString::fromLatin1("_NMProcCompList_")))
        {
            if (dropItem.compare(QString::fromLatin1("TextLabel")) == 0)
            {
                QGraphicsItem* pi = this->itemAt(event->scenePos(), this->views()[0]->transform());
                NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(pi);
                QGraphicsTextItem* labelItem = new QGraphicsTextItem(ai);
                labelItem->setHtml(QString::fromLatin1("<b>Text Label</b>"));
                labelItem->setTextInteractionFlags(Qt::NoTextInteraction);
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
                //event->acceptProposedAction();
            }
            else
            {
                NMProcessComponentItem* procItem = new NMProcessComponentItem(0, this);
                procItem->setTitle(dropItem);
                procItem->setDescription(dropItem);
                procItem->setPos(event->scenePos());
                if (dropItem.compare("DataBuffer") == 0)
                {
                    procItem->setIsDataBufferItem(true);
                }
                procItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                //event->acceptProposedAction();

                NMDebugAI(<< "asking for creating '" << dropItem.toStdString() << "' ..." << endl);
                emit processItemCreated(procItem, dropItem, event->scenePos());
            }
        }
        else if (event->mimeData()->hasUrls())
        {
            // we grab the first we can get hold of
            QString fileName;
            foreach(const QUrl& url, event->mimeData()->urls())
            {
                if (    url.isLocalFile()
                    &&  (   url.toLocalFile().endsWith(QString::fromLatin1("lmv"))
                         || url.toLocalFile().endsWith(QString::fromLatin1("lmx"))
                        )
                   )
                {
                    fileName = url.toLocalFile();
                    break;
                }
            }

            if (!fileName.isEmpty())
            {
                QFileInfo finfo(fileName);
                if (finfo.isFile())
                {
                    NMDebugAI(<< "gonna import model file: " << fileName.toStdString() << std::endl);
                    //event->acceptProposedAction();

                    emit signalModelFileDropped(fileName);
                }
            }
        }
        else
        {
            NMDebugAI(<< "No valid drag source detected!" << std::endl);
        }
	}
    //event->accept();
    if (!mbSceneMove)
    {
        this->setProcCompMoveability(true);
    }
    mDragItemList.clear();
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
        QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
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
        else if (textItem != 0)
        {
            textItem->setTextInteractionFlags(Qt::TextEditorInteraction |
                                              Qt::TextBrowserInteraction);
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
    NMDebugCtx(ctx, << "...");
    mMousePos = event->scenePos();
    mDragItemList.clear();
    QGraphicsItem* item = this->itemAt(event->scenePos(), this->views()[0]->transform());
    QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
    NMProcessComponentItem* procItem = qgraphicsitem_cast<NMProcessComponentItem*>(item);
    NMAggregateComponentItem* aggrItem = qgraphicsitem_cast<NMAggregateComponentItem*>(item);

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
            {
                // SELECTION 'MODE'
                if (event->modifiers() & Qt::ControlModifier)
                {
                    this->views().at(0)->setCursor(Qt::PointingHandCursor);
                    this->views().at(0)->setDragMode(QGraphicsView::RubberBandDrag);
                    //this->setProcCompMoveability(false);
                    this->setProcCompSelectability(true);
                    this->setLinkCompSelectability(false);
                }
                // INFO/DRAG 'MODE'
                else
                {
                    this->views().at(0)->setDragMode(QGraphicsView::ScrollHandDrag);
                    if (item != 0)
                    {
                        this->views().at(0)->setCursor(Qt::ClosedHandCursor);

                        if (procItem)
                        {
                            emit itemLeftClicked(procItem->getTitle());
                        }
                        else if (aggrItem)
                        {
                            emit itemLeftClicked(aggrItem->getTitle());
                        }
                        else if (textItem)
                        {
                            textItem->setTextInteractionFlags(Qt::NoTextInteraction
                                                              | Qt::TextBrowserInteraction);
                        }
                    }
                    else
                    {
                        this->views().at(0)->setCursor(Qt::OpenHandCursor);
                        this->setProcCompSelectability(false);
                        emit itemLeftClicked(QString::fromUtf8("root"));
                    }
                }
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
    NMDebugCtx(ctx, << "done!");
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

    //mMousePos = event->scenePos();

	switch(mMode)
	{
	case NMS_LINK:
		if (mLinkLine == 0)
			break;
		mLinkLine->setLine(QLineF(mLinkLine->line().p1(), event->scenePos()));
		break;

	case NMS_MOVE:
    case NMS_SELECT:
    default:

        {
            QGraphicsItem* dragItem = this->itemAt(mMousePos, this->views()[0]->transform());
            if (    (event->buttons() & Qt::LeftButton)
                &&  dragItem != 0
                &&  (   QApplication::keyboardModifiers() & Qt::AltModifier
                     || QApplication::keyboardModifiers() & Qt::ShiftModifier
                    )
               )
            {
                mDragStartPos = event->scenePos();
                mDragItemList.clear();
                mDragItemList = this->selectedItems();
                if (mDragItemList.count() == 0)
                {
                    mDragItemList.push_back(dragItem);
                }
                QRectF selRect;
                foreach(const QGraphicsItem* gi, mDragItemList)
                {
                    selRect = selRect.united(gi->mapRectToScene(gi->boundingRect()));
                }
                QPixmap dragPix = QPixmap::grabWidget(this->views()[0],
                        this->views()[0]->mapFromScene(selRect).boundingRect());
                dragPix = dragPix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                QDrag* drag = new QDrag(this);
                QMimeData* mimeData = new QMimeData;
                QString mimeText = QString("_NMModelScene_:%1").arg(mDragItemList.count());
                mimeData->setText(mimeText);

                drag->setMimeData(mimeData);
                drag->setPixmap(dragPix);
                if (event->modifiers() & Qt::ShiftModifier)
                {
                    NMDebugAI(<< " >> moving ..." << std::endl);
                    //drag->setDragCursor(dragPix, Qt::MoveAction);
                    drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::MoveAction);
                }
                else if (event->modifiers() & Qt::AltModifier)
                {
                   NMDebugAI(<< " >> copying ..." << std::endl);
                   //drag->setDragCursor(dragPix, Qt::CopyAction);
                   drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction);
                }
                NMDebugAI(<< "drag start - " << mimeText.toStdString() << std::endl);
                this->setProcCompMoveability(false);
            }
        }
        this->invalidate();
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
            link->setZValue(this->mLinkZLevel);
            addItem(link);
            this->invalidate();
            emit linkItemCreated(link);
		}

		break;

	default:
        {
            if (event->modifiers() & Qt::ControlModifier)
            {
                this->views().at(0)->setCursor(Qt::PointingHandCursor);
                this->views().at(0)->setDragMode(QGraphicsView::RubberBandDrag);
                this->setProcCompSelectability(true);
                this->setLinkCompSelectability(false);
                mMode = NMS_SELECT;
            }
            else
            {
                this->views().at(0)->setDragMode(QGraphicsView::ScrollHandDrag);
                this->views().at(0)->setCursor(Qt::OpenHandCursor);
                this->setProcCompSelectability(false);
                mMode = NMS_IDLE;
            }
            //            if (!mbSceneMove)
            //            {
            //                this->setProcCompMoveability(true);
            //            }
            //            else
            //            {
            //                this->setProcCompMoveability(false);
            //            }
            this->mDragItemList.clear();
        }
		break;
	}
    if (!mbSceneMove)
    {
        this->setProcCompMoveability(true);
    }
	QGraphicsScene::mouseReleaseEvent(event);
}



