 /*********************b*********************************************************
 * Created by Alexander nHerzig
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

#include <QApplication>
#include <QDebug>
#include "NMAggregateComponentItem.h"
#include "nmlog.h"

//#include "valgrind/callgrind.h"

const std::string NMAggregateComponentItem::ctx = "NMAggregateComponentItem";

NMAggregateComponentItem::NMAggregateComponentItem(QGraphicsItem* parent)
    : mProgress(0), mIsExecuting(false), mIsCollapsed(false)
{
	this->setParentItem(parent);
    this->mNumIterations = 0;
    this->mColor = QColor(qrand() % 256, qrand() % 256, qrand() % 256);

    dpr = qApp->devicePixelRatio();

    dx1 = 10*dpr;
    dy1 = 43*dpr;
    dx2 = 10*dpr;
    dy2 = 43*dpr;
    headBase = 3;
    mStartAngle = 95 * 16;
    mSpanAngle  = -315 * 16;
    mHeadAngleLeft = -165;
    mHeadAngleRight = -60;



    smallGap = 2*dpr;
    bigGap = 4*dpr;

    mCollapsedPix.load(":model-icon.png");
    mCollapsedPix.scaledToWidth(64*dpr);

    mDescription = this->objectName();
    mFont = QFont("Arial", 11);
}

NMAggregateComponentItem::~NMAggregateComponentItem()
{
}

void
NMAggregateComponentItem::slotExecutionStarted()
{
    this->mIsExecuting = true;
    this->update();
}

void
NMAggregateComponentItem::slotExecutionStopped()
{
    this->mIsExecuting = false;
    this->update();
}

void
NMAggregateComponentItem::slotProgress(float progress)
{
    this->mProgress = progress;
    //NMDebugAI(<< mTitle.toStdString() << " progress: " << mProgress << std::endl);
    this->update();
}



void
NMAggregateComponentItem::collapse(bool bCollapse)
{
    NMDebugCtx(ctx, << "...");

    QRectF oldBnd = this->mapRectToScene(this->boundingRect());

    QList<QGraphicsItem*> kids = this->childItems();
    foreach(QGraphicsItem* k, kids)
    {
        k->setVisible(!bCollapse);
    }
    mIsCollapsed = bCollapse;

    this->update();
    this->scene()->update(oldBnd);

    emit itemCollapsed();
    NMDebugCtx(ctx, << "done!");
}

void
NMAggregateComponentItem::relocate(const QPointF &target)
{
    // target comes in parent coord sys
    // target is new centre
    QRectF sceneBnd = this->mapToScene(this->boundingRect()).boundingRect();
    QPointF centre = sceneBnd.center();

    QGraphicsItem* pi = this->parentItem();
    QPointF nt = target;
    if (pi)
    {
        nt = pi->mapToScene(target);
    }

    QList<QPointF> npos;
    QList<QGraphicsItem*> kids = this->childItems();
    foreach(QGraphicsItem* ci, kids)
    {
        npos << nt + (ci->scenePos() - centre);
        //ci->setPos(this->mapFromScene(target + posDelta));
    }


//    QGraphicsItemGroup::setPos(target.x() - (sceneBnd.width()/2.0),
//                               target.y() - (sceneBnd.height()/2.0));

    QGraphicsItemGroup::setPos(this->mapFromScene(nt));
    for(int i=0; i < kids.count(); ++i)
    {
        kids.at(i)->setPos(this->mapFromScene(npos.at(i)));
    }
}

//QRectF NMAggregateComponentItem::boundingRect() const
//{
//    QRectF bnd = this->childrenBoundingRect();
//    bnd.adjust(-dx1,-dy1,dx2,dy2);
//    return bnd;
//}

void
NMAggregateComponentItem::updateDescription(const QString& descr)
{
    if (descr.compare(this->mDescription) != 0)
    {
        this->mDescription = descr;
        //this->preparePainting();
        this->update();
        this->scene()->update(mapRectToScene(boundingRect()));
    }
}

void
NMAggregateComponentItem::updateTimeLevel(short level)
{
    if (level != this->mTimeLevel)
    {
        this->mTimeLevel = level;
        //this->preparePainting();
        this->update();
    }
}

void
NMAggregateComponentItem::updateNumIterations(unsigned int iter)
{
    if (this->mNumIterations != iter)
    {
        this->mNumIterations = iter;
        this->update();
    }
}

void
NMAggregateComponentItem::preparePainting(const QRectF& bndRect)
{
    QRectF bnd = const_cast<QRectF&>(bndRect);
    mItemBnd = bnd;
    mItemBnd.adjust(dx1,dy1,-dx2,-dy2);

    QFontMetrics fm(mFont);

    qreal descrWidth = fm.width(mDescription);
    qreal descrHeight = fm.height();

    //    if (descrWidth > mItemBnd.width())
    //    {
    //        qreal diff = descrWidth - mItemBnd.width();
    //        mItemBnd.setLeft(mItemBnd.x()-(diff/2.0)-2);
    //        mItemBnd.setWidth(descrWidth+2);
    //        //mItemBnd.adjust
    //    }

    mDash = QRectF(mItemBnd.left(), bnd.top()+12*dpr, mItemBnd.width(),25*dpr);
    qreal width = (mDash.right()-smallGap) - (mNumIterRect.right()+bigGap);

    mClockRect = QRectF(mDash.left()+smallGap, mDash.top()+2*bigGap,
                        descrHeight*0.6, descrHeight*0.6);
    QPointF center = QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                             mClockRect.top() +(mClockRect.height()/2.0));

    mPointer1 = QLineF(center, QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                                       mClockRect.top()+1));
    mPointer1.setAngle((qreal)85);
    mPointer2 = mPointer1;
    mPointer2.setLength(0.6*mPointer1.length());
    mPointer2.setAngle((qreal)-27.5);

    qreal levelWidth = fm.width(QString("%1").arg(mTimeLevel));

    mTimeLevelRect = QRectF(mClockRect.right()+smallGap, mDash.top()+bigGap,
                            levelWidth, descrHeight);

    mIterSymbolRect = QRectF(mTimeLevelRect.right()+bigGap+(headBase/2.0),
                             mDash.top()+2*bigGap,
                             descrHeight*0.6, descrHeight*0.6);

    mIterSymbol.moveTo(mIterSymbolRect.center());
    mIterSymbol.arcTo(mIterSymbolRect, (this->mStartAngle/16.0),
                      (this->mSpanAngle/16.0));
    QPointF arcEnd = mIterSymbol.currentPosition();

    mHeadLeft = QLineF(arcEnd, QPointF(arcEnd.x()+headBase, arcEnd.y()));
    mHeadRight = QLineF(arcEnd, QPointF(arcEnd.x()+headBase, arcEnd.y()));

    mHeadLeft.setAngle(mHeadAngleLeft);
    mHeadRight.setAngle(mHeadAngleRight);

    mIterSymbol.lineTo(mHeadLeft.p2());
    mIterSymbol.moveTo(arcEnd);
    mIterSymbol.lineTo(mHeadRight.p2());

    qreal numIterWidth = fm.width(QString("%1").arg(mNumIterations));
    qreal numIterHeight = fm.height();
    if (mIsExecuting)
    {
        numIterWidth = fm.width(QString("%1 of %2").arg(mProgress).arg(mNumIterations));
    }

    mNumIterRect = QRectF(mIterSymbolRect.right()+smallGap, mDash.top()+bigGap,
                          numIterWidth, numIterHeight); //15);

    mDescrRect = QRectF(mNumIterRect.right()+bigGap, mDash.top()+bigGap,
                        width, descrHeight);//20);
    qreal dc = mDescrRect.center().x();
    mDescrRect.setLeft(dc-(descrWidth/2.0));
    mDescrRect.setRight(dc+(descrWidth/2.0));
}

QRectF
NMAggregateComponentItem::boundingRect(void) const
{
    QRectF bnd;
    if (mIsCollapsed)
    {
        QRectF fb = this->childrenBoundingRect();
        bnd = QRectF(fb.center().x() - (mCollapsedPix.width()/2.0),
                     fb.center().y() - (mCollapsedPix.height()/2.0),
                     mCollapsedPix.width(), mCollapsedPix.height());
    }
    else
    {
        bnd = this->childrenBoundingRect();

    }

    QFontMetrics fm(mFont);
    qreal minWidth = fm.height() + smallGap + fm.width(QString("%1").arg(mTimeLevel)) + bigGap +
                     fm.height() + smallGap + fm.width(QString("%1").arg(mNumIterations)) + bigGap +
                     fm.width(mDescription);

    if (minWidth > bnd.width())
    {
        qreal diff = minWidth - bnd.width();
        bnd.adjust(-(diff/2.0), 0, (diff/2.0), 0);
    }



    bnd.adjust(-this->dx1,-this->dy1, this->dx2, this->dy2);
    return bnd;
}


void
NMAggregateComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{
    // ============================================================================
    // short cuts && preparations
    // ============================================================================

    //CALLGRIND_START_INSTRUMENTATION;
    QRectF bnd = this->boundingRect();


//    QTransform wt = painter->worldTransform();
//    wt.translate(bnd.right(), 0);
//    painter->setWorldTransform(wt);

//    bnd.setLeft(-bnd.width());
//    bnd.setRight(0);
    //prepareGeometryChange();
    preparePainting(bnd);

    int l,r,t,b,h,w;
    l = bnd.toRect().left()    ;
    r = bnd.toRect().right()   ;
    t = bnd.toRect().top();
    b = bnd.toRect().bottom();
    w = bnd.toRect().width()  -16;
    h = bnd.toRect().height() -16;

    int red, green, blue;
    red   = mColor.red();
    green = mColor.green();
    blue  = mColor.blue();

    // background
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->setBrush(QColor(red, green, blue, 200));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(bnd.adjusted(0.25,0.25,-0.25,-0.25), 8, 8);

    // ============================================================================
    // EDGE EFFECT
    // ============================================================================

    painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);

    // blend the corners
    QRadialGradient cornerglare;
    cornerglare.setColorAt(0.00, QColor(255,255,255,255));
    cornerglare.setColorAt(0.50, QColor(255,255,255,200));
    cornerglare.setColorAt(0.75, QColor(255,255,255,180));
    cornerglare.setColorAt(0.85, QColor(255,255,255,150));
    cornerglare.setColorAt(1.00, QColor(255,255,255,100));


    // top left
    QSize size(16,16);
    QRect tl(QPoint(l,t), size);
    cornerglare.setCenter(QPoint(l+9, t+9));
    cornerglare.setFocalPoint(QPoint(l+9, t+9));
    cornerglare.setRadius(9);
    painter->setBrush(cornerglare);
    int start = 16*90;
    int span  = 16*90;
    painter->drawPie(tl, start, span);

    // top right
    QRect tr(QPoint(r-15, t), size);
    cornerglare.setCenter(QPoint(r-7, t+9));
    cornerglare.setFocalPoint(QPoint(r-7, t+9));
    painter->setBrush(cornerglare);
    start = 0;
    painter->drawPie(tr, start, span);

    // bottom left
    QRect bl(QPoint(l,b-15), size);
    cornerglare.setCenter(QPoint(l+9, b-8));
    cornerglare.setFocalPoint(QPoint(l+9, b-8));
    painter->setBrush(cornerglare);
    start = 16*180;
    painter->drawPie(bl, start, span);

    // bottom right
    QRect br(QPoint(r-15, b-15), size);
    cornerglare.setCenter(QPoint(r-7, b-8));
    cornerglare.setFocalPoint(QPoint(r-7, b-8));
    painter->setBrush(cornerglare);
    start = 16*270;
    painter->drawPie(br, start, span);
    
    // ----------------------------------------------------------------------
    // glare the edges on top of it
    
    QLinearGradient glare(l+(w/2.0), t+8, l+(w/2.0), t);
    glare.setColorAt(0.00, QColor(255,255,255,255));
    glare.setColorAt(0.50, QColor(255,255,255,200));
    glare.setColorAt(0.75, QColor(255,255,255,180));
    glare.setColorAt(0.85, QColor(255,255,255,150));
    glare.setColorAt(1.00, QColor(255,255,255,100));

    // glare top
    QRect glareRect = QRect(l+8,t,w,8);
    painter->fillRect(glareRect, glare);

    // glare left
    glare.setStart(QPointF(l+8, t+(h/2.0)));
    glare.setFinalStop(QPointF(l, t+(h/2.0)));
    glareRect = QRect(l,t+8, 8, h);
    painter->fillRect(glareRect, glare);

    // glare bottom
    glare.setStart(l+(w/2.0), b-8);
    glare.setFinalStop(l+(w/2.0), b+1);
    glareRect = QRect(l+8,b-7,w,8);
    painter->fillRect(glareRect, glare);


    // glare right
    glare.setStart(r-8, t+(h/2.0));
    glare.setFinalStop(r+1, t+(h/2.0));
    glareRect = QRect(r-7, t+8, 8, h);
    painter->fillRect(glareRect, glare);

	// ============================================================================
    // other stuff
	// ============================================================================
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    QRectF wr = QRectF(l+10, t+12, bnd.width()-20,
                       mDash.height());

    if (mIsExecuting)
    {
        painter->setBrush(QColor(255,240,240,255));
    }
    else
    {
        painter->setBrush(QColor(255,255,255,255));
    }
    painter->drawRoundedRect(wr, 5, 5);

    // ------------------------------------------------
    // DRAW THE DASH
    // ------------------------------------------------

    painter->setBrush(Qt::NoBrush);
    if (mIsExecuting)
    {
        painter->setPen(QPen(QBrush(Qt::darkRed), 1, Qt::SolidLine));
    }
    else
    {
        painter->setPen(QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine));
    }

    painter->drawRoundedRect(mDash, 5, 5);

    // the clock icon
    painter->setPen(QPen(QBrush(Qt::black), 0.8, Qt::SolidLine));
    painter->drawEllipse(mClockRect);
    painter->drawLine(mPointer1);
    painter->drawLine(mPointer2);

    // the time level
    painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));

    mFont.setBold(false);
    painter->setFont(mFont);

    //painter->drawText(mTimeLevelRect, Qt::AlignLeft, QString("%1").arg(mTimeLevel));
    this->renderText(mTimeLevelRect, Qt::AlignLeft, QString("%1").arg(mTimeLevel), *painter);

    // the iteration icon
    painter->setPen(QPen(QBrush(Qt::black), 0.8, Qt::SolidLine));
    //painter->drawPath(mIterSymbol);
    painter->drawArc(mIterSymbolRect, mStartAngle, mSpanAngle);
    painter->drawLine(mHeadLeft);
    painter->drawLine(mHeadRight);


    // the actual number of iterations
    if (mNumIterations > 0)
    {
        if (mIsExecuting)
        {
            //            mFont.setBold(true);
            //            painter->setPen(QPen(QBrush(Qt::darkRed), 2, Qt::SolidLine));
            //painter->drawText(mNumIterRect, Qt::AlignLeft,
              // QString("%1 of %2").arg(mProgress).arg(mNumIterations));

            this->renderText(mNumIterRect, Qt::AlignLeft,
               QString("%1 of %2").arg(mProgress).arg(mNumIterations), *painter);

            mFont.setBold(false);
        }
        else
        {
            painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
            //painter->drawText(mNumIterRect, Qt::AlignLeft, QString("%1").arg(mNumIterations));
            this->renderText(mNumIterRect, Qt::AlignLeft, QString("%1").arg(mNumIterations), *painter);
        }
    }

    // the description
    //painter->drawText(mDescrRect, Qt::AlignCenter, mDescription);
    this->renderText(mDescrRect, Qt::AlignCenter, mDescription, *painter);


    // ------------------------------------------------
    // DRAW ICON , IF COLLAPSED
    // ------------------------------------------------
    if (mIsCollapsed)
    {
        QRectF trex = QRectF(mDash.center().x()-32*dpr,
                             mDash.bottom()+3*bigGap,
                             64*dpr, 64*dpr);
        painter->drawPixmap(trex, mCollapsedPix, QRectF(0,0,64*dpr,64*dpr));
    }

    // ------------------------------------------------
    // DRAW SELECTION MARKER
    // ------------------------------------------------

    if(this->isSelected() || mIsExecuting)
	{
        if (mIsExecuting)
        {
            painter->setPen(QPen(QBrush(Qt::red), 4, Qt::SolidLine));
        }
        else
        {
            painter->setPen(QPen(QBrush(Qt::red), 2, Qt::SolidLine));
        }
        QPainterPath selRect;
        selRect.addRoundedRect(bnd.adjusted(3,3,-3,-3), 5, 5);
		painter->drawPolygon(selRect.toFillPolygon());
	}

    //CALLGRIND_STOP_INSTRUMENTATION;
    //CALLGRIND_DUMP_STATS;

}

void
NMAggregateComponentItem::getAncestors(QList<NMAggregateComponentItem*>& ancestors)
{
    NMAggregateComponentItem* ai =
            qgraphicsitem_cast<NMAggregateComponentItem*>(parentItem());
    if (ai)
    {
        ancestors.append(ai);
        ai->getAncestors(ancestors);
    }
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

/*!
    define QDataStream operator<< and operator>> for QGraphicsTextItem
*/

QDataStream& operator<<(QDataStream& data, const QGraphicsTextItem& item)
{
    data << item.toPlainText();
    data << item.font();
    data << item.defaultTextColor();
    data << item.scenePos();
    data << item.isVisible();

    return data;
}

QDataStream& operator>>(QDataStream& data, QGraphicsTextItem& item)
{
    QString text;
    QFont font;
    QColor color;
    QPointF pos;

    data >> text >> font >> color >> pos;

    item.setPlainText(text);
    item.setFont(font);
    item.setDefaultTextColor(color);
    item.setPos(pos);

    return data;
}

QDataStream& operator<<(QDataStream &data, const NMAggregateComponentItem &item)
{
	NMAggregateComponentItem& i = const_cast<NMAggregateComponentItem&>(item);
	data << i.getTitle();
    data << i.scenePos();
	data << i.getColor();
    data << i.isCollapsed();
    data << i.isVisible();

	QList<QGraphicsItem*> kids = i.childItems();
	int nchild = kids.count();
	data << (qint32)nchild;

	for (unsigned int c = 0; c < nchild; ++c)
	{
		NMAggregateComponentItem* ai = qgraphicsitem_cast<NMAggregateComponentItem*>(kids.at(c));
		NMProcessComponentItem* pi = qgraphicsitem_cast<NMProcessComponentItem*>(kids.at(c));
        QGraphicsTextItem* ti = qgraphicsitem_cast<QGraphicsTextItem*>(kids.at(c));
		if (ai != 0)
        {
			data << ai->getTitle();
        }
		else if (pi != 0)
        {
			data << pi->getTitle();
        }
        else if (ti != 0)
        {
            data << QString::fromLatin1("TextLabel");
            data << *ti;
        }
	}

	return data;
}


