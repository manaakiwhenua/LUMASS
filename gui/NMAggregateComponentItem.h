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
#include <QGraphicsItemGroup>
#include <QFontMetrics>
#include "NMModelScene.h"

class NMModelScene;

class NMAggregateComponentItem: public QObject, public QGraphicsItemGroup
{

    Q_OBJECT
    //Q_INTERFACES(QGraphicsItemGroup)

public:
    NMAggregateComponentItem(bool bParallel, QGraphicsItem* parent=0);
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
    bool isCollapsed(){return mIsCollapsed;}
    //void setIsCollapsed(const bool& bCollapsed){mIsCollapsed = bCollapsed;}

    bool isParallel(){return mIsParallel;}

    QRectF boundingRect(void) const;

    QRectF iconRect(void) const;
    //void setIconRect(const QRectF& rect){mIconRect = rect;}

    void normaliseAt(const QPointF& pos);

    void getAncestors(QList<NMAggregateComponentItem*>& ancestors);
    void getEldestCollapsedAncestor(NMAggregateComponentItem*& eldest);

    NMAggregateComponentItem* getModelParent(void);
    void setModelParent(NMAggregateComponentItem* parent)
    {mModelParent = parent;}

    QList<QGraphicsItem*> getModelChildren(void);
    void setModelChildren(QList<QGraphicsItem*> modelChildren)
    {mHiddenItems = modelChildren;}

    void getAllModelOffSprings(QList<QGraphicsItem*>& offsprings);

    void setFont(const QFont& font){mFont = font;}
    QFont getFont(void){return mFont;}
    void setFontPtSize(const int pts) {mFont.setPointSize(pts);}
    int getFontPtSize(void){return mFont.pointSize();}


public slots:
    void updateDescription(const QString& descr);
    void updateTimeLevel(short level);
    void updateNumIterations(unsigned int iter);

    void slotExecutionStarted();
    void slotExecutionStopped();
    void slotProgress(float progress);
    void collapse(bool bCollapse);

    // moves the group (i.e. all kids)
    // to the new target location, which
    // depicts the centre of the bouding rect
    // in partent coordinates; this also
    // normalises the coordinates of the group
    // with pos() to be in the center of the
    // child items' bounding rect
    void relocate(const QPointF& target);

private:

    void updateActivityStatus(void)
    {
        mIsActive = mProgress > mNumIterations ? false : true;
    }

    void preparePainting(const QRectF& bndRect);

    inline void renderText(const QRectF& rect,
                           const Qt::AlignmentFlag& flag,
                           const QString& text, QPainter& p)
    {
        QTransform ot = p.worldTransform();
        QTransform wt = p.worldTransform();
        QRectF r = rect;
        wt.translate(rect.right(), 0);
        p.setWorldTransform(wt);

        r.setLeft(-rect.width());
        r.setRight(0);
        p.drawText(r, flag, text);

        p.setWorldTransform(ot);
    }

    QPixmap mCollapsedPix;

    QString mTitle;
	QColor mColor;
	int rgb[3];

    QFont mFont;
    QString mDescription;
    unsigned int mNumIterations;
    short mTimeLevel;

    float mProgress;
    bool mIsExecuting;
    bool mIsCollapsed;
    bool mIsActive;

    bool mIsParallel;

    QRectF mItemBnd;
    QRectF mDash;

    QRectF mTimeLevelRect;
    QRectF mNumIterRect;
    QRectF mDescrRect;

    //QPainterPath mIterSymbol;
    QRectF mIterSymbolRect;
    QLineF mHeadTopTop;
    QLineF mHeadTopBottom;
    QLineF mHeadBottomTop;
    QLineF mHeadBottomBottom;

    qreal headBase;
    qreal mStartAngle;
    qreal mSpanAngle;

    qreal dpr;
    qreal smallGap;
    qreal bigGap;

    QRectF mClockRect;
    QLineF mPointer1;
    QLineF mPointer2;

    QRectF mIconRect;

//    QMap<QString, QPointF> mAggrCompPos;
//    QList< QPair<QGraphicsTextItem*, QPointF> > mTextItems;
    //QMap< QGraphicsItem*, QPointF> mHiddenItems;
    QList<QGraphicsItem*> mHiddenItems;

    NMAggregateComponentItem* mModelParent;


    int dx1, dy1, dx2, dy2;

    static const std::string ctx;

};

QDataStream& operator<<(QDataStream& data, const QGraphicsTextItem& item);
QDataStream& operator>>(QDataStream& data, QGraphicsTextItem& item);
QDataStream& operator<<(QDataStream& data, const NMAggregateComponentItem &item);

#endif /* NMAGGREGATECOMPONENTITEM_H_ */
