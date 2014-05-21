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

class NMProcessComponentItem: public QObject, public QGraphicsItem
{
	Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)

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

	QPolygonF getShapeAsPolygon(void) const;
	//QPainterPath shape(void) const;

	void setDescription(const QString& descr)
		{this->updateDescription(descr);}
	QString getDescription(void)
		{return this->mDescription;}

	void setIsDataBufferItem(bool isbuffer);
	bool getIsDataBufferItem(void)
		{return mbIsDataBuffer;}

    void setTimeLevel(short level)
        {this->mTimeLevel = level;}
    short getTimeLevel(void)
        {return this->mTimeLevel;}

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

public slots:
	void updateProgress(float progr);
	void reportExecutionStarted(const QString& proc);
	void reportExecutionStopped(const QString& proc);
	void updateDescription(const QString& descr);
    void updateTimeLevel(short level);

private:

	static const std::string ctx;

	bool mbIsDataBuffer;

	float mProgress;
	bool mbIsExecuting;
	NMModelScene* mScene;
	QString mTitle;
	QString mDescription;
    QList<NMComponentLinkItem*> mInputLinks;
    QList<NMComponentLinkItem*> mOutputLinks;

	int mSingleLineHeight;
	int mDoubleLineHeight;
	int mMaxTextWidth;
	QPixmap mIcon;
	QRectF mIconRect;
	QRectF mIconBnd;
	QRectF mTextRect;
    QRectF mTimeLevelRect;
    QRectF mClockRect;
    QLineF mPointer1;
    QLineF mPointer2;


    short mTimeLevel;

	QFont mFont;

};

/*	\brief Serialising a NMProcessComponentItem.
 */
QDataStream& operator<<(QDataStream &, const NMProcessComponentItem &);
QDataStream& operator>>(QDataStream &, NMProcessComponentItem &);


#endif /* NMPROCESSCOMPONENTITEM_H_ */
