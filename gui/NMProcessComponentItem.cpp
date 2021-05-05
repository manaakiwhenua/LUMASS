 /**************************m****************************************************
 * Created by Alexander Herzig.g
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
 * NMProcessComponentItem.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include "NMProcessComponentItem.h"
#include <QApplication>
#include <QTime>
#include <QDebug>
#include <QFontMetrics>
#include "nmlog.h"

//#include "valgrind/callgrind.h"

const std::string NMProcessComponentItem::ctx = "NMProcessComponentItem";

NMProcessComponentItem::NMProcessComponentItem(QGraphicsItem* parent,
		NMModelScene* scene)
    : QGraphicsItem(parent),
      mProgress(0.0), mbIsExecuting(false), mbIsDataBuffer(false),
      mTimeLevel(0), mTypeID(0),
      mModelParent(0),
      mDescription("")
{
	this->mScene = scene;

    mSingleLineHeight = 15;
    mDoubleLineHeight = 30;
    mMaxTextWidth = 150;

    // will be overriden with setTitle
    // once we know what type of
    // item this is
    mIcon.load(":model-icon.png");

    mFont = QFont("Arial", 10);
    mFont.setBold(true);

    initRectsNSizes();
}

NMProcessComponentItem::~NMProcessComponentItem()
{
}

void
NMProcessComponentItem::initRectsNSizes()
{
    qreal dpr = 1;//qApp->devicePixelRatio();
    qreal smallGap = 2*dpr;
    qreal bigGap = 5*dpr;

    mIcon.scaledToWidth(64*dpr);

    QFontMetrics fm(mFont);

    //mIconRect = QRectF(-37, -37, 74, 74);
    mIconRect = QRectF(-30*dpr, -26*dpr, 50*dpr, 50*dpr);

    //	mIconBnd = QRectF(mIconRect.left()-5, mIconRect.top()-5,
    //			          mIconRect.width()+10, mIconRect.height()+10);

    mIconBnd = QRectF(mIconRect.left()-12*dpr, mIconRect.top()-16*dpr,
                      mIconRect.width()+24*dpr, mIconRect.height()+24*dpr);


    mClockRect = QRectF(mIconBnd.left()+smallGap, mIconBnd.top()+bigGap,
                        fm.height()*0.6,fm.height()*0.6);

    qreal timewidth = fm.width(QString("%1").arg(9999));
    mTimeLevelRect = QRectF(mClockRect.right()+smallGap,mIconBnd.top()+1.5,
                            timewidth,fm.height());

    qreal idwidth = fm.width(QString("%1").arg(9999));
    mIDRect = QRectF(mIconBnd.right()-idwidth-bigGap, mIconBnd.top()+1.5,
                     idwidth, fm.height());

    QPointF center = QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                             mClockRect.top() +(mClockRect.height()/2.0));

    mPointer1 = QLineF(center, QPointF(mClockRect.left()+(mClockRect.width()/2.0),
                                       mClockRect.top()+1));
    mPointer1.setAngle((qreal)85);
    mPointer2 = mPointer1;
    mPointer2.setLength(0.6*mPointer1.length());
    mPointer2.setAngle((qreal)-27.5);
}

void
NMProcessComponentItem::updateProgress(float progr)
{
	this->mProgress = progr;
	//NMDebugAI(<< mTitle.toStdString() << " => progress: " << progr << std::endl);
	this->update();
}

void
NMProcessComponentItem::setIsDataBufferItem(bool isbuffer)
{
	this->mbIsDataBuffer = isbuffer;
	if (isbuffer)
	{
		mIcon.load(":image_layer.png");
        //mIconRect.adjust(0, -4, 0, 0);
        //mIconBnd.adjust(0, +4, 0, 0);
	}
	else
		mIcon.load(":model-icon.png");
}

void NMProcessComponentItem::addInputLink(int idx, NMComponentLinkItem* link)
{
//	NMDebugCtx(ctx, << "...");
	if (link == 0)
		return;

	if (idx < 0 || idx > mInputLinks.count()-1)
		this->mInputLinks.push_back(link);
	else //if (idx >= 0 && idx < this->mInputLinks.size()-1)
		this->mInputLinks[idx] = link;
//	NMDebugAI( << "new no of in links: " << this->mInputLinks.size() << std::endl);

//	NMDebugCtx(ctx, << "done!");
}

void NMProcessComponentItem::addOutputLink(int idx, NMComponentLinkItem* link)
{
//	NMDebugCtx(ctx, << "...");

	if (link == 0)
		return;

	if (idx < 0 || idx > this->mOutputLinks.count()-1)
		this->mOutputLinks.push_back(link);
	else //if (idx >= 0 && idx < this->mOutputLinks.size()-1)
		this->mOutputLinks[idx] = link;

//	NMDebugAI( << "new no of out links: " << this->mOutputLinks.size() << std::endl);

//	NMDebugCtx(ctx, << "done!");
}

int NMProcessComponentItem::getInputLinkIndex(QString inputItemName)
{
	int ret = -1;

	for (unsigned int i = 0; i < this->mInputLinks.count(); ++i)
	{
		if (this->mInputLinks.at(i)->sourceItem()->getTitle()
				.compare(inputItemName) == 0)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

int NMProcessComponentItem::getOutputLinkIndex(QString outputItemName)
{
	int ret = -1;

	for (unsigned int i = 0; i < this->mOutputLinks.count(); ++i)
	{
		if (this->mOutputLinks.at(i)->targetItem()->getTitle()
				.compare(outputItemName) == 0)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

QString NMProcessComponentItem::identifyOutputLink(int idx)
{
	QString ret;
	if (idx < 0 || idx >= this->mOutputLinks.count())
		return ret;

	ret = this->mOutputLinks[idx]->targetItem()->getTitle();

	return ret;
}

QString NMProcessComponentItem::identifyInputLink(int idx)
{
	QString ret;
	if (idx < 0 || idx >= this->mInputLinks.count())
		return ret;

	ret = this->mInputLinks[idx]->sourceItem()->getTitle();

	return ret;
}

void NMProcessComponentItem::removeLink(NMComponentLinkItem* link)
{
	if (this->mInputLinks.contains(link))
		this->mInputLinks.removeOne(link);
	else if (this->mOutputLinks.contains(link))
		this->mOutputLinks.removeOne(link);
}

void NMProcessComponentItem::setTitle(const QString& title)
{
	if (!title.isEmpty())
		this->mTitle = title;

    if (title.contains(QStringLiteral("BMIModel"), Qt::CaseInsensitive))
    {
        mIcon.load(":bmi-model.png");
    }
    else if (title.contains(QString("ImageReader"), Qt::CaseInsensitive))
    {
        mIcon.load(":image-read.png");
    }
    else if (title.contains(QString("ImageWriter"), Qt::CaseInsensitive))
    {
        mIcon.load(":image-write.png");
    }
    else if (title.contains(QString("TableReader"), Qt::CaseInsensitive))
    {
        mIcon.load(":table-read.png");
    }
    else if (title.contains("DataBuffer", Qt::CaseInsensitive))
    {
        mIcon.load(":image_layer.png");
    }
    else if (title.contains("SQL", Qt::CaseInsensitive))
    {
        mIcon.load(":SQLite_Logo_4.png");
        //mIcon = mIcon.scaledToWidth(64, Qt::SmoothTransformation);
    }
    else
    {
        mIcon.load(":model-icon.png");
    }

    mIcon.scaledToWidth(64*qApp->devicePixelRatio());
}

QPolygonF
NMProcessComponentItem::getShapeAsPolygon(void) const
{
    QRectF gap = QRectF(mIconBnd.left(), mIconBnd.bottom(), //-2,
                        mIconBnd.width(), mSingleLineHeight);

	QPolygonF iconPoly(mIconBnd);
	QPolygonF jointPoly = iconPoly.united(gap);
	QPolygonF txtPoly(mTextRect);
	return jointPoly.united(txtPoly);
}

void
NMProcessComponentItem::reportExecutionStarted(const QString& proc)
{
	if (proc.compare(this->mTitle) != 0)
		return;
	this->mbIsExecuting = true;
    this->update();
}

void
NMProcessComponentItem::reportExecutionStopped(const QString& proc)
{
	if (proc.compare(this->mTitle) != 0)
		return;
	this->mbIsExecuting = false;
    this->update();
}

void
NMProcessComponentItem::updateTimeLevel(short level)
{
    this->mTimeLevel = level;
    this->update();
}

void
NMProcessComponentItem::setFont(const QFont& font)
{
    mFont = font;
    initRectsNSizes();
    prepareGeometryChange();
    this->updateDescription();
    this->update();
}

void
NMProcessComponentItem::setFontPtSize(const int pts)
{
    mFont.setPointSize(pts);
    initRectsNSizes();
    prepareGeometryChange();
    this->updateDescription();
    this->update();
}

void
NMProcessComponentItem::setDescription(const QString& descr)
{
    if (descr.compare(this->mDescription) != 0)
    {
        this->mDescription = descr;
        prepareGeometryChange();
        this->updateDescription();
        this->update();
        //this->scene()->update();//mapRectToScene(boundingRect()));
    }
}

void
NMProcessComponentItem::updateDescription()
{
    const QString& descr = this->mDescription;
    QFontMetricsF fm(mFont);
    qreal leading = fm.leading();
    qreal height = 0;

    qreal linewidth = fm.width(descr)+5;
    if (linewidth > mMaxTextWidth)
    {
        linewidth = mMaxTextWidth;
    }

    mTextLayout.setText(descr);
    mTextLayout.setFont(mFont);
    mTextLayout.beginLayout();
    while(true)
    {
        QTextLine line = mTextLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(linewidth);
        height += leading;
        line.setPosition(QPointF(0, height));
        height += line.height();
    }
    mTextLayout.endLayout();

    mTextRect = mTextLayout.boundingRect();
    mTextRect.moveTopLeft(QPointF(-(0.5*mTextRect.width())-4,
                          mIconBnd.bottom()+0.5*mSingleLineHeight));
}

NMAggregateComponentItem*
NMProcessComponentItem::getModelParent(void)
{
    if (mModelParent)
    {
        return mModelParent;
    }

    return qgraphicsitem_cast<NMAggregateComponentItem*>(parentItem());
}

void
NMProcessComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{

    //CALLGRIND_START_INSTRUMENTATION;
    qreal dpr = 1;//qApp->devicePixelRatio();
    QFontMetrics fm(mFont);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setFont(mFont);

	if(mbIsExecuting)
	{
		QSizeF psize = mIconBnd.size();
		QPointF centre(psize.width()*0.5, psize.height()*0.5);
		QImage img(psize.toSize(), QImage::Format_ARGB32_Premultiplied);
		img.fill(0);
	    QPainter bgPainter(&img);

	    QRadialGradient bw(centre, img.rect().height()*0.8, centre);
	    bw.setColorAt(0, QColor(255,255,255));
	    bw.setColorAt(0.6, QColor(255,255,255));
	    bw.setColorAt(1, QColor(100,0,0));
	    bgPainter.fillRect(img.rect(), bw);
	    bgPainter.setCompositionMode(QPainter::CompositionMode_DestinationOver);

	    QRadialGradient fade(centre, img.rect().height()*0.8, centre);
	    fade.setColorAt(1, QColor(0,0,0,90));
	    fade.setColorAt(0, QColor(0,0,0,90));
	    bgPainter.fillRect(img.rect(), fade);

		painter->drawImage(mIconBnd, img);
	    painter->setRenderHint(QPainter::Antialiasing, true);

		// draw boundary
		painter->setBrush(Qt::NoBrush);
        //QPen pen = QPen(QBrush(Qt::darkGray), 2, Qt::SolidLine);
        QPen pen = QPen(QBrush(Qt::darkRed), 2, Qt::SolidLine);
		painter->setPen(pen);
		painter->drawRoundRect(mIconBnd, 10, 10);

		// draw icon
        painter->drawPixmap(mIconRect.toRect(), mIcon, QRectF(0,0,64*dpr,64*dpr)); // 0,0,64,64

        if (mProgress > 0)
		{
            mFont.setBold(false);
			painter->setPen(QPen(QBrush(Qt::darkRed), 1, Qt::SolidLine));
            mFont.setItalic(false);
			painter->setFont(mFont);
            QString strProg = QString("%1\%").arg(this->mProgress, 3, 'f', 0);
            painter->drawText(QRectF(mIconBnd.right()-40*dpr,mIconBnd.top()+1.5,
                              fm.width(strProg),fm.height()),
                    Qt::AlignRight, strProg);
		}
	}
	else
	{
		// draw boundary
		painter->setBrush(Qt::white);
		QPen pen;
		if (this->isSelected())
			pen = QPen(QBrush(Qt::red), 2, Qt::SolidLine);
		else
            pen = QPen(QBrush(Qt::darkGray), 2, Qt::SolidLine);
		painter->setPen(pen);
		painter->drawRoundRect(mIconBnd, 10, 10);

		// draw icon
        painter->drawPixmap(mIconRect, mIcon, QRectF(0,0,64*dpr,64*dpr));
	}

    // -----------------------------
    // draw metadata elements
	painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
    mFont.setItalic(false);

    // the type id
    if (this->mTypeID > 0)
    {
        painter->setFont(mFont);
        painter->setPen(QPen(QBrush(Qt::darkGray), 2, Qt::SolidLine));
        painter->drawText(mIDRect, Qt::AlignRight, QString("%1").arg(mTypeID));
        painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
    }

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
    mFont.setBold(true);
    painter->setFont(mFont);
    painter->drawText(mTextRect,
                      Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap |
                      Qt::ElideRight,
                      mDescription);

    //CALLGRIND_STOP_INSTRUMENTATION;
    //CALLGRIND_DUMP_STATS;
}


QDataStream& operator<<(QDataStream& data, const NMProcessComponentItem& item)
{


    NMProcessComponentItem& i = const_cast<NMProcessComponentItem&>(item);
	data << i.getTitle();
	data << i.scenePos();
	data << i.getIsDataBufferItem();
	return data;
}

QDataStream& operator>>(QDataStream& data, NMProcessComponentItem& item)
{
	QPointF pos;
	QString title;
	bool databuffer;

    data >> title;
    data >> pos >> databuffer;

	item.setTitle(title);
	item.setPos(pos);
	item.setIsDataBufferItem(databuffer);
	return data;
}






