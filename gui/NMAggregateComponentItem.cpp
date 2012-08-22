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
 * NMAggregateComponentItem.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include <QDebug>
#include "NMAggregateComponentItem.h"

NMAggregateComponentItem::NMAggregateComponentItem(QGraphicsItem* parent,
		NMModelScene* scene)
	: mScene(scene)
{
	this->setParentItem(parent);
	ctx = "NMAggregateComponentItem";
	this->mColor = QColor(qrand() % 256, qrand() % 256, qrand() % 256);
}

NMAggregateComponentItem::~NMAggregateComponentItem()
{

}

QRectF NMAggregateComponentItem::boundingRect() const
{
	QList<QGraphicsItem*> childs = this->childItems();
	QListIterator<QGraphicsItem*> it(childs);
	QRectF bnd;
	while(it.hasNext())
	{
		QGraphicsItem* item = it.next();
		if (item->type() != NMComponentLinkItem::Type)
		{
			QRectF tbnd = QRectF(this->mapFromItem(item, item->boundingRect()).boundingRect());
			bnd = QRectF(bnd.united(tbnd));
		}
	}
	bnd.setLeft(bnd.left()-10);
	bnd.setRight(bnd.right()+10);
	bnd.setTop(bnd.top()-10);
	bnd.setBottom(bnd.bottom()+10);

	return bnd;
}

void
NMAggregateComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{
	QRectF bnd = this->boundingRect();

	QPen pen;
	if(this->isSelected())
		pen = QPen(QBrush(Qt::red), 2, Qt::SolidLine);
	else
		pen = QPen(Qt::NoPen);

	painter->setPen(pen);
	painter->setBrush(this->mColor);
	painter->drawRect(bnd);
}

bool
NMAggregateComponentItem::containsComponent(QString name)
{
	bool ret = false;

	foreach(QGraphicsItem* kid, this->childItems())
	{
		NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(kid);
		NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(kid);

		if (pi != 0)
		{
			if (pi->getTitle().compare(name) == 0)
			{
				ret = true;
				break;
			}
		}
		else if (ai != 0)
		{
			if (ai->getTitle().compare(name) == 0)
			{
				ret = true;
				break;
			}
			else
			{
				ret = ai->containsComponent(name);
				if (ret)
					break;
			}
		}
	}

	return ret;
}


QDataStream& operator<<(QDataStream &data, const NMAggregateComponentItem &item)
{
	NMAggregateComponentItem& i = const_cast<NMAggregateComponentItem&>(item);
	data << i.getTitle();
	data << i.pos();
	data << i.getColor();

	QList<QGraphicsItem*> kids = i.childItems();
	int nchild = kids.count();
	data << (qint32)nchild;

	for (unsigned int c = 0; c < nchild; ++c)
	{
		NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(kids.at(c));
		NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(kids.at(c));
		if (ai != 0)
			data << ai->getTitle();
		else if (pi != 0)
			data << pi->getTitle();
	}

	return data;
}







