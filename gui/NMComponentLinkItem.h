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
 * NMComponentLinkItem.h
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#ifndef NMCOMPONENTLINKITEM_H_
#define NMCOMPONENTLINKITEM_H_

#include <qgraphicsitem.h>
#include "NMModelComponent.h"
#include "NMProcessComponentItem.h"
#include "NMAggregateComponentItem.h"
#include <QPainterPath>

class NMProcessComponentItem;

class NMComponentLinkItem: public QObject, public QGraphicsPathItem
{
	Q_OBJECT
public:
	enum { Type = QGraphicsItem::UserType + 30 };
	NMComponentLinkItem(NMProcessComponentItem* sourceItem,
			NMProcessComponentItem* targetItem,
			QGraphicsItem* parent=0);
	virtual ~NMComponentLinkItem();

	int type() const
		{return Type;}
	NMProcessComponentItem* sourceItem() const
		{return this->mSourceItem;}
	NMProcessComponentItem* targetItem() const
		{return this->mTargetItem;}

	void setSourceItem(const NMProcessComponentItem* sourceItem);
	void setTargetItem(const NMProcessComponentItem* targetItem);

	QPainterPath shape(void) const
		{return this->mPath;}
	QRectF boundingRect(void) const
		{return this->mBndBox;}

protected:
	void paint(QPainter* painter,
			const QStyleOptionGraphicsItem* option,
			QWidget* widget=0);

private:
	NMProcessComponentItem* mSourceItem;
	NMProcessComponentItem* mTargetItem;
	double mHeadSize;
	QGraphicsScene* mScene;
	QRectF mBndBox;
	QPainterPath mPath;

};

QDataStream& operator<<(QDataStream &, const NMComponentLinkItem &);


#endif /* NMCOMPONENTLINKITEM_H_ */
