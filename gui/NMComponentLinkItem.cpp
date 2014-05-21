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
 * NMComponentLinkItem.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include "nmlog.h"
#include "NMComponentLinkItem.h"
#include "math.h"
#include <QDebug>
#include <QTime>

NMComponentLinkItem::NMComponentLinkItem(NMProcessComponentItem* sourceItem,
		NMProcessComponentItem* targetItem,
		QGraphicsItem* parent)
    : QGraphicsPathItem(parent), mIsDynamic(false)
{
	this->mSourceItem = sourceItem;
	this->mTargetItem = targetItem;
	this->mHeadSize = 10;

//	this->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

NMComponentLinkItem::~NMComponentLinkItem()
{
}

void NMComponentLinkItem::setSourceItem(const NMProcessComponentItem* sourceItem)
{
	if (sourceItem != 0)
		this->mSourceItem = const_cast<NMProcessComponentItem*>(sourceItem);
}

void NMComponentLinkItem::setTargetItem(const NMProcessComponentItem* targetItem)
{
	if (targetItem != 0)
		this->mTargetItem = const_cast<NMProcessComponentItem*>(targetItem);
}

void
NMComponentLinkItem::setIsDynamic(bool dynamic)
{
    if (this->mIsDynamic != dynamic)
    {
        this->mIsDynamic = dynamic;
        this->update();
    }
}

void
NMComponentLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{
	if (mSourceItem->collidesWithItem(mTargetItem))
		return;

	// determine coordinates and bounding rectangle
	QPointF sp = mapFromItem(mSourceItem, 0, 0);
	QPointF tp = mapFromItem(mTargetItem, 0, 0);
	QRectF srcBnd = mapFromItem(mSourceItem, mSourceItem->boundingRect()).boundingRect();
	QRectF tarBnd = mapFromItem(mTargetItem, mTargetItem->boundingRect()).boundingRect();
	this->mBndBox = srcBnd.united(tarBnd);

	// determine target intersection point
	QPointF ip;
	QPolygonF poly(mapFromItem(mTargetItem, mTargetItem->getShapeAsPolygon()));
	QLineF linkLine(sp, tp);
	QLineF polyEdge;
	QPointF p1, p2;
	for (unsigned int i=0; i < poly.count()-1; ++i)
	{
		p1 = poly.at(i);
		p2 = poly.at(i+1);
		polyEdge = QLineF(p1, p2);
		if (polyEdge.intersect(linkLine, &ip) == QLineF::BoundedIntersection)
				break;
	}

	// determine source intersection point
	QPointF sip;
	QPolygonF spoly(mapFromItem(mSourceItem, mSourceItem->getShapeAsPolygon()));
	for (unsigned int i=0; i < spoly.count()-1; ++i)
	{
		p1 = spoly.at(i);
		p2 = spoly.at(i+1);
		polyEdge = QLineF(p1, p2);
		if (polyEdge.intersect(linkLine, &sip) == QLineF::BoundedIntersection)
				break;
	}

	// create the head
	QLineF shortBase(ip, sp);
	shortBase.setLength(mHeadSize);
	QLineF shortBase2(shortBase.p2(), shortBase.p1());
	QLineF normal1 = shortBase2.normalVector();
	normal1.setLength(mHeadSize / 2.0);
	QLineF normal2(normal1.p2(), normal1.p1());
	normal2.setLength(mHeadSize);

	QPolygonF head;
	head << ip << normal2.p1() << normal2.p2();

	// create bezier curve
	QPointF ep(shortBase.p2());
	QPainterPath path(sip);
	path.lineTo(ep);

	// check, whether any component item is colliding with
	// the direct line
	//QList<QGraphicsItem*> closeItems = this->collidingItems(Qt::IntersectsItemBoundingRect);
    //
	//foreach(const QGraphicsItem* i, closeItems)
	//{
	//	if (i->type() == NMProcessComponentItem::Type    ||
	//		i->type() == NMAggregateComponentItem::Type)
	//	{
	//		if (i->collidesWithPath(path, Qt::IntersectsItemShape))
	//		{
	//			NMDebug(<< QTime::currentTime().msec() << ": line hits!" << std::endl);
	//		}
	//	}
	//}

	//path.cubicTo(QPointF(shortBase.p2().x(), sip.y()),
	//		QPointF(sip.x(), shortBase.p2().y()), shortBase.p2());

	// draw elements
	QPen pen;
    //if (this->isSelected())
    //	pen = QPen(QBrush(Qt::red), 2);
    //else
    if (this->mIsDynamic)
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::DashLine);
    else
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::DashLine);
	painter->setPen(pen);
	painter->drawPath(path);
	painter->setBrush(QColor(80,80,80));
	painter->drawPolygon(head);

	QPainterPath npath = path;
	npath.addPolygon(head);
	this->mPath = npath;
	setPath(mPath);
}

QDataStream& operator<<(QDataStream &data, const NMComponentLinkItem &item)
{
	NMComponentLinkItem& i = const_cast<NMComponentLinkItem&>(item);
	//data << (qint32)NMComponentLinkItem::Type;

	data << (qint32)i.sourceItem()->getOutputLinkIndex(i.sourceItem()->getTitle());
	data << i.sourceItem()->getTitle();

	data << (qint32)i.targetItem()->getInputLinkIndex(i.targetItem()->getTitle());
	data << i.targetItem()->getTitle();

	return data;
}

