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

#include <QDebug>
#include "NMAggregateComponentItem.h"

NMAggregateComponentItem::NMAggregateComponentItem(QGraphicsItem* parent)
    : mProgress(0), mIsExecuting(false)
{
	this->setParentItem(parent);
	ctx = "NMAggregateComponentItem";
    this->mNumIterations = 0;
    this->mColor = QColor(qrand() % 256, qrand() % 256, qrand() % 256, 200);

    dx1 = 10;
    dy1 = 43;
    dx2 = 10;
    dy2 = 43;
    headBase = 3;
    mStartAngle = 95 * 16;
    mSpanAngle  = -315 * 16;
    mHeadAngleLeft = -165;
    mHeadAngleRight = -60;
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
    this->update();
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
    bnd.adjust(-dx1,-dy1,dx2,dy2);
    return bnd;
}

void
NMAggregateComponentItem::updateDescription(const QString& descr)
{
    if (descr.compare(this->mDescription) != 0)
    {
        this->mDescription = descr;
        this->update();
    }
}

void
NMAggregateComponentItem::updateTimeLevel(short level)
{
    if (level != this->mTimeLevel)
    {
        this->mTimeLevel = level;
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

    mDash = QRectF(mItemBnd.left(), bnd.top()+12, mItemBnd.width(),25);

    mClockRect = QRectF(mDash.left()+2, mDash.top()+9,8,8);
    QPointF center = QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                             mClockRect.top() +(mClockRect.height()/2.0));

    mPointer1 = QLineF(center, QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                                       mClockRect.top()+1));
    mPointer1.setAngle((qreal)85);
    mPointer2 = mPointer1;
    mPointer2.setLength(0.6*mPointer1.length());
    mPointer2.setAngle((qreal)-27.5);

    mFont = QFont("Arial", 11);
    QFontMetrics fm(mFont);
    qreal levelWidth = fm.width(QString("%1").arg(mTimeLevel));

    mTimeLevelRect = QRectF(mClockRect.right()+2, mDash.top()+4, levelWidth,15);

    mIterSymbolRect = QRectF(mTimeLevelRect.right()+2+(headBase/2.0), mDash.top()+9, 8,8);

    mIterSymbol.moveTo(mIterSymbolRect.center());
    mIterSymbol.arcTo(mIterSymbolRect, (this->mStartAngle/16.0), (this->mSpanAngle/16.0));

    QPointF arcEnd = mIterSymbol.currentPosition();

    mHeadLeft = QLineF(arcEnd, QPointF(arcEnd.x()+headBase, arcEnd.y()));
    mHeadRight = QLineF(arcEnd, QPointF(arcEnd.x()+headBase, arcEnd.y()));

    mHeadLeft.setAngle(mHeadAngleLeft);
    mHeadRight.setAngle(mHeadAngleRight);

    mIterSymbol.lineTo(mHeadLeft.p2());
    mIterSymbol.moveTo(arcEnd);
    mIterSymbol.lineTo(mHeadRight.p2());

    qreal numIterWidth = fm.width(QString("%1").arg(mNumIterations));
    if (mIsExecuting)
    {
        numIterWidth = fm.width(QString("%1 of %2").arg(mProgress).arg(mNumIterations));
    }

    mNumIterRect = QRectF(mIterSymbolRect.right()+2, mDash.top()+4, numIterWidth,15);

    qreal width = (mDash.right()-2) - (mIterSymbolRect.right()+2);
    mDescrRect = QRectF(mTimeLevelRect.right()+2, mDash.top()+3.5,
                        width, 20);
}

void
NMAggregateComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{
	QRectF bnd = this->boundingRect();
    preparePainting(bnd);

	// clip the image
    QPainterPath clipPath;
    clipPath.addRoundedRect(bnd.adjusted(1,1,-1.5,-1.5), 5, 5);
    painter->setClipPath(clipPath);

    // ============================================================================
    // 'TRANSPARENT' BACKGROUND
    // ============================================================================

    painter->setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter->setBrush(mColor);
    //painter->drawRect(bnd.adjusted(-2,-2,-1,-1));

    int l,r,t,b,h,w;
    l = bnd.toRect().left();
    r = bnd.toRect().right();
    t = bnd.toRect().top();
    b = bnd.toRect().bottom();
    w = bnd.toRect().width();
    h = bnd.toRect().height();

    int red, green, blue;
    red   = mColor.red();
    green = mColor.green();
    blue  = mColor.blue();

    // ============================================================================
    // EDGE GLARE
    // ============================================================================

    //painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
    //painter->setBrush(mColor);
    painter->drawRoundedRect(bnd, 5, 5);

    // ------------------------------------------------------------------------
    // blend the corners
//    QRadialGradient cornerglare;
////    cornerglare.setColorAt(0.00, mIsExecuting ? QColor(0,0,255,0)   : QColor(red,green,blue,0)  );
////    cornerglare.setColorAt(0.85, mIsExecuting ? QColor(0,0,255,127) : QColor(red,green,blue,127));
////    cornerglare.setColorAt(0.88, mIsExecuting ? QColor(0,0,255,200) : QColor(red,green,blue,200));
////    cornerglare.setColorAt(0.90, mIsExecuting ? QColor(0,0,255,150) : QColor(red,green,blue,150));
////    cornerglare.setColorAt(0.95, mIsExecuting ? QColor(0,0,255,100) : QColor(red,green,blue,100));
////    cornerglare.setColorAt(1.00, mIsExecuting ? QColor(0,0,255,0)   : QColor(red,green,blue,0)  );

//    // top left
//    QSize size(8,8);
//    QRect tl(QPoint(l,t), size);
//    cornerglare.setCenter(tl.bottomRight());
//    cornerglare.setFocalPoint(tl.bottomRight());
//        cornerglare.setRadius(::sqrt(128)*0.8);
//    painter->fillRect(tl, cornerglare);

//    // top right
//    QRect tr(QPoint(r-8,t), size);
//    cornerglare.setCenter(tr.bottomLeft());
//    cornerglare.setFocalPoint(tr.bottomLeft());
//         ////cornerglare.setRadius(::sqrt(128)*0.8);
//    painter->fillRect(tr, cornerglare);

//    // bottom left
//    QRect bl(QPoint(l,b-8), size);
//    cornerglare.setCenter(bl.topRight());
//    cornerglare.setFocalPoint(bl.topRight());
//    painter->fillRect(bl, cornerglare);

//    // bottom right
//    QRect br(QPoint(r-8, b-8), size);
//    cornerglare.setCenter(br.topLeft());
//    cornerglare.setFocalPoint(br.topLeft());
//    painter->fillRect(br, cornerglare);

    // ----------------------------------------------------------------------
    // glare the edges on top of it

    QLinearGradient glare(l+(w/2.0), t+8, l+(w/2.0), t);
    glare.setColorAt(0.00, mIsExecuting ? QColor(0,0,255,255) : QColor(red,green,blue,255));
    glare.setColorAt(0.85, mIsExecuting ? QColor(0,0,255,200) : QColor(red,green,blue,200));
    glare.setColorAt(0.88, mIsExecuting ? QColor(0,0,255,150) : QColor(red,green,blue,150));
    glare.setColorAt(0.90, mIsExecuting ? QColor(0,0,255,100) : QColor(red,green,blue,100));
    glare.setColorAt(0.95, mIsExecuting ? QColor(0,0,255,50 ) : QColor(red,green,blue,50 ));
    glare.setColorAt(1.00, mIsExecuting ? QColor(0,0,255,0  ) : QColor(red,green,blue,0  ));

    // glare top
    QRect glareRect = QRect(l,t,w,8);
    painter->fillRect(glareRect, glare);

    // glare bottom
    glare.setStart(l+(w/2.0), b-8);
    glare.setFinalStop(l+(w/2.0), b);
    glareRect = QRect(l,b-8,w,9);
    painter->fillRect(glareRect, glare);

    // glare left
    glare.setStart(QPointF(l+8, t+(h/2.0)));
    glare.setFinalStop(QPointF(l, t+(h/2.0)));
    glareRect = QRect(l,t, 8, h);
    painter->fillRect(glareRect, glare);

    // glare right
    glare.setStart(r-8, t+(h/2.0));
    glare.setFinalStop(r, t+(h/2.0));
    glareRect = QRect(r-8, t, 9, h);
    painter->fillRect(glareRect, glare);

	// ============================================================================
	// PUTTING IT ALL TOGETHER
	// ============================================================================

    QRectF wr = QRectF(bnd.left()+10, bnd.top()+12, bnd.width()-20,
                       mDash.height());

    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->setBrush(QColor(255,255,255,255));
    painter->drawRoundedRect(wr, 5, 5);

    // ------------------------------------------------
    // DRAW THE DASH
    // ------------------------------------------------

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine));
    painter->drawRoundedRect(mDash, 3, 3);

    // the clock icon
    painter->setPen(QPen(QBrush(Qt::black), 0.8, Qt::SolidLine));
    painter->drawEllipse(mClockRect);
    painter->drawLine(mPointer1);
    painter->drawLine(mPointer2);

    // the time level
    painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));

    mFont.setBold(false);
    painter->setFont(mFont);
    painter->drawText(mTimeLevelRect, Qt::AlignLeft, QString("%1").arg(mTimeLevel));

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
            painter->setPen(QPen(QBrush(Qt::blue), 2.5, Qt::SolidLine));
            painter->drawText(mNumIterRect, Qt::AlignLeft,
               QString("%1 of %2").arg(mProgress).arg(mNumIterations));
        }
        else
        {
            painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
            painter->drawText(mNumIterRect, Qt::AlignLeft, QString("%1").arg(mNumIterations));
        }
    }

    // the description
    painter->drawText(mDescrRect, Qt::AlignCenter, mDescription);

    // ------------------------------------------------
    // DRAW SELECTION MARKER
    // ------------------------------------------------

    if(this->isSelected())
	{
		painter->setPen(QPen(QBrush(Qt::red), 2, Qt::SolidLine));
		QPainterPath selRect;
        selRect.addRoundedRect(bnd.adjusted(3,3,-3,-3), 5, 5);
		painter->drawPolygon(selRect.toFillPolygon());
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
    data << item.pos();

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
	data << i.pos();
	data << i.getColor();

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


