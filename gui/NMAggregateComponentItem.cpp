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
{
	this->setParentItem(parent);
	ctx = "NMAggregateComponentItem";
	this->mColor = QColor(qrand() % 256, qrand() % 256, qrand() % 256);

    dx1 = 10;
    dy1 = 35;
    dx2 = 10;
    dy2 = 35;
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
NMAggregateComponentItem::preparePainting(const QRectF& bndRect)
{
    QRectF bnd = const_cast<QRectF&>(bndRect);
    mItemBnd = bnd;
    mItemBnd.adjust(dx1,dy1,-dx2,-dy2);

    mDash = QRectF(mItemBnd.left(), bnd.top()+17, mItemBnd.width(),25);

    mClockRect = QRectF(mDash.left()+2, mDash.top()+7.5,8,8);
    QPointF center = QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                             mClockRect.top() +(mClockRect.height()/2.0));

    mPointer1 = QLineF(center, QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                                       mClockRect.top()+1));
    mPointer1.setAngle((qreal)85);
    mPointer2 = mPointer1;
    mPointer2.setLength(0.6*mPointer1.length());
    mPointer2.setAngle((qreal)-27.5);

    mTimeLevelRect = QRectF(mClockRect.right()+2, mDash.top()+4, 30,15);
    mFont = QFont("Arial", 11);
    QFontMetrics fm(mFont);
    QRectF dsr = fm.boundingRect(mDescription);
    qreal width = dsr.width();
    qreal height = dsr.height();

    mDescrRect = QRectF(mDash.right()-width-2, mTimeLevelRect.top(),
                        width, 15);
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
	clipPath.addRoundRect(bnd.adjusted(1,1,-1.5,-1.5), 5);
	painter->setClipPath(clipPath);

	// ============================================================================
	// EDGE GLARE
	// ============================================================================
	QImage img(bnd.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	img.fill(0);
	QPainter imgPainter(&img);
	imgPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	// ------------------------------------------------------------------------
    // blend the corners
    QRadialGradient cornerglare;
    cornerglare.setColorAt(0.00, QColor(0,0,0,0));
    cornerglare.setColorAt(0.85, QColor(0,0,0,127));
	cornerglare.setColorAt(0.88, QColor(0,0,0,200));
	cornerglare.setColorAt(0.90, QColor(0,0,0,150));
    cornerglare.setColorAt(0.95, QColor(0,0,0,100));
	cornerglare.setColorAt(1.00, QColor(0,0,0,0));

	// top left
	QSize size(8,8);
	QRect tl(QPoint(0,0), size);
	cornerglare.setCenter(tl.bottomRight());
    cornerglare.setFocalPoint(tl.bottomRight());
    cornerglare.setRadius(::sqrt(128)*0.7);
    imgPainter.fillRect(tl, cornerglare);

    // top right
	QRect tr(QPoint(img.rect().right()-8,0), size);
	cornerglare.setCenter(tr.bottomLeft());
    cornerglare.setFocalPoint(tr.bottomLeft());
    //cornerglare.setRadius(::sqrt(128)*0.8);
    imgPainter.fillRect(tr, cornerglare);

    // bottom left
	QRect bl(QPoint(0,img.rect().bottom()-8), size);
	cornerglare.setCenter(bl.topRight());
    cornerglare.setFocalPoint(bl.topRight());
    imgPainter.fillRect(bl, cornerglare);

    // bottom right
	QRect br(QPoint(img.rect().right()-8, img.rect().bottom()-8), size);
	cornerglare.setCenter(br.topLeft());
    cornerglare.setFocalPoint(br.topLeft());
    imgPainter.fillRect(br, cornerglare);

	// ----------------------------------------------------------------------
	// glare the edges on top of it
    // glare top
	QRect glareRect = img.rect().adjusted(8,0,-9,-(img.rect().height()-8));
	QLinearGradient glare(img.rect().center().x(), img.rect().top()+8,
			img.rect().center().x(), img.rect().top());
	glare.setColorAt(0.00, QColor(0,0,0,0));
	glare.setColorAt(0.85, QColor(0,0,0,127));
	glare.setColorAt(0.88, QColor(0,0,0,200));
	glare.setColorAt(0.90, QColor(0,0,0,150));
	glare.setColorAt(0.95, QColor(0,0,0,100));
	glare.setColorAt(1.00, QColor(0,0,0,0));
	imgPainter.fillRect(glareRect, glare);

	// glare bottom
	glare.setStart(img.rect().center().x(), img.rect().bottom()-8);
    glare.setFinalStop(img.rect().center().x(), img.rect().bottom());
    glareRect = img.rect().adjusted(8, img.rect().height()-8, -9, 0);
    imgPainter.fillRect(glareRect, glare);

	// glare left
	glare.setStart(QPointF(img.rect().x()+8, img.rect().top() + img.rect().center().y()));
    glare.setFinalStop(QPointF(img.rect().x(), img.rect().top() + img.rect().center().y()));
    glareRect = img.rect().adjusted(0, 8, 8, -9);
    imgPainter.fillRect(glareRect, glare);

	// glare right
	glare.setStart(QPointF(img.rect().right()-8, img.rect().top() + img.rect().center().y()));
    glare.setFinalStop(QPointF(img.rect().right(), img.rect().top() + img.rect().center().y()));
    glareRect = img.rect().adjusted(img.rect().width()-8, 8, -1, -9);
    imgPainter.fillRect(glareRect, glare);

	// ============================================================================
	// TRANSPARENT BACKGROUND
	// ============================================================================

	QImage bgImg(bnd.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	bgImg.fill(0);
	QPainter bgImgPainter(&bgImg);
	bgImgPainter.fillRect(bgImg.rect().adjusted(-2,-2,-1,-1), mColor);
	bgImgPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	bgImgPainter.fillRect(bgImg.rect().adjusted(-2,-2,-1,-1), QColor(0,0,0,128));

    QRectF wr = QRectF(bgImg.rect().left()+2, bgImg.rect().top()+2, mDash.width(),
                       mDash.height());
    bgImgPainter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    bgImgPainter.fillRect(wr, QColor(255,255,255,179));


	// ============================================================================
	// PUTTING IT ALL TOGETHER
	// ============================================================================

	//bgImgPainter.drawImage(glareRect, img);
	painter->drawImage(bnd, bgImg);
	painter->drawImage(bnd, img);

    painter->setBrush(Qt::NoBrush);
	painter->setRenderHint(QPainter::Antialiasing, true);


    // ------------------------------------------------
    // DRAW THE DASH
    // ------------------------------------------------

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine));
    painter->drawRoundedRect(mDash, 10, 10);

    // the clock icon
    painter->setPen(QPen(QBrush(Qt::black), 0.5, Qt::SolidLine));
    painter->drawEllipse(mClockRect);
    painter->drawLine(mPointer1);
    painter->drawLine(mPointer2);

    // the time level
    painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));

    mFont.setBold(false);
    painter->setFont(mFont);
    painter->drawText(mTimeLevelRect, Qt::AlignLeft, QString("%1").arg(mTimeLevel));

    // the description
    //painter->setBrush(Qt::white);
    //painter->drawRoundedRect(mDash, 10, 10);

    painter->drawText(mDescrRect, Qt::AlignRight, mDescription);

    // ------------------------------------------------
    // DRAW SELECTION MARKER
    // ------------------------------------------------

    if(this->isSelected())
	{
		painter->setPen(QPen(QBrush(Qt::red), 2, Qt::SolidLine));
		QPainterPath selRect;
		selRect.addRoundRect(bnd.adjusted(3,3,-3,-3), 5);
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


