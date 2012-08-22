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
 * NMProcessComponentItem.h
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#ifndef NMPROCESSCOMPONENTITEM_H_
#define NMPROCESSCOMPONENTITEM_H_

#include <string>
#include <iostream>
#include <sstream>

#include <qgraphicsitem.h>
#include <qobject.h>
#include <QObject>
#include <QList>
#include <QString>
#include <QMenu>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPixmap>
#include <QDebug>

#include "nmlog.h"
#include "NMComponentLinkItem.h"
#include "NMModelScene.h"
#include "NMModelComponent.h"
#include "NMProcess.h"

class NMModelScene;
class NMComponentLinkItem;

class NMProcessComponentItem: public QGraphicsItem
{

public:
	NMProcessComponentItem(QGraphicsItem* parent=0,
			NMModelScene* scene=0);
	virtual ~NMProcessComponentItem();

	// UserType is 65536
	enum {Type = QGraphicsItem::UserType + 10};

	int type(void) const
		{return Type;}

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
			QWidget* widget);

	void setTitle(const QString& title);
	QString getTitle(void)
		{return this->mTitle;}

	void addInputLink(int idx, NMComponentLinkItem* link);
	void addOutputLink(int idx, NMComponentLinkItem* link);

	QString identifyOutputLink(int idx);
	QString identifyInputLink(int idx);

	int getInputLinkIndex(QString inputItemName);
	int getOutputLinkIndex(QString outputItemName);

	void removeLink(NMComponentLinkItem* link);

	QList<NMComponentLinkItem*> getOutputLinks(void)
		{return this->mOutputLinks;}

	QList<NMComponentLinkItem*> getInputLinks(void)
		{return this->mInputLinks;}


private:

	std::string ctx;

	NMModelScene* mScene;
	QString mTitle;
	QMenu* mContextMenu;
	QList<NMComponentLinkItem*> mInputLinks;
	QList<NMComponentLinkItem*> mOutputLinks;

	QPixmap mIcon;
	QRectF mBndRect;
	QFont mFont;

};

/*	\brief Serialising a NMProcessComponentItem.
 */
QDataStream& operator<<(QDataStream &, const NMProcessComponentItem &);
QDataStream& operator>>(QDataStream &, NMProcessComponentItem &);


#endif /* NMPROCESSCOMPONENTITEM_H_ */
