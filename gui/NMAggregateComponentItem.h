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
 * NMAggregateComponentItem.h
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#ifndef NMAGGREGATECOMPONENTITEM_H_
#define NMAGGREGATECOMPONENTITEM_H_

#include <string>
#include <iostream>
#include <qgraphicsitem.h>
#include "NMModelScene.h"

class NMModelScene;

class NMAggregateComponentItem: public QGraphicsItemGroup
{
public:
	NMAggregateComponentItem(QGraphicsItem* parent=0);
	virtual ~NMAggregateComponentItem();

	// UserType is 65536
	enum {Type = QGraphicsItem::UserType + 20};

	int type(void) const
		{return Type;}

	void setTitle(const QString& title)
		{this->mTitle = title;}
	QString getTitle(void)
		{return this->mTitle;}

	QColor getColor(void)
		{return this->mColor;}
	void setColor(QColor color)
		{this->mColor = color;}

	void paint(QPainter* painter,
			const QStyleOptionGraphicsItem* option,
			QWidget* widget);

	bool containsComponent(QString name);

	QRectF boundingRect(void) const;

private:
	std::string ctx;
	QString mTitle;
	QColor mColor;
	int rgb[3];

};

QDataStream& operator<<(QDataStream& data, const QGraphicsTextItem& item);
QDataStream& operator>>(QDataStream& data, QGraphicsTextItem& item);
QDataStream& operator<<(QDataStream& data, const NMAggregateComponentItem &item);

#endif /* NMAGGREGATECOMPONENTITEM_H_ */
