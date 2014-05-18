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
#include <QTime>
#include <QDebug>
#include <QFontMetrics>
#include "nmlog.h"

const std::string NMProcessComponentItem::ctx = "NMProcessComponentItem";

NMProcessComponentItem::NMProcessComponentItem(QGraphicsItem* parent,
		NMModelScene* scene)
    : QGraphicsItem(parent), //mContextMenu(0) ,
      mProgress(0.0), mbIsExecuting(false), mbIsDataBuffer(false),
      mTimeLevel(0)
{
	this->mScene = scene;

	mSingleLineHeight = 17;
	mDoubleLineHeight = 34;
    mMaxTextWidth = 80;

    if (this->objectName().contains("Reader", Qt::CaseInsensitive))
    {
        mIcon.load(":data-read-icon.png");
    }
    else if (this->objectName().contains("Writer", Qt::CaseInsensitive))
    {
        mIcon.load(":data-write-icon.png");
    }
    else if (this->objectName().contains("DataBuffer", Qt::CaseInsensitive))
    {
        mIcon.load(":image_layer.png");
    }
    else
    {
        mIcon.load(":model-icon.png");
    }

    mIconRect = QRectF(-37, -37, 74, 74);

	mIconBnd = QRectF(mIconRect.left()-5, mIconRect.top()-5,
			          mIconRect.width()+10, mIconRect.height()+10);

	mTextRect = QRectF(mIconBnd.left(), mIconBnd.bottom()+5,
			           mIconBnd.width(), mSingleLineHeight);

	mFont = QFont("Arial", 10);
}

NMProcessComponentItem::~NMProcessComponentItem()
{
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
	}
	else
		mIcon.load(":model-icon.png");
}

void NMProcessComponentItem::addInputLink(int idx, NMComponentLinkItem* link)
{
	NMDebugCtx(ctx, << "...");
	if (link == 0)
		return;

	if (idx < 0 || idx > mInputLinks.count()-1)
		this->mInputLinks.push_back(link);
	else //if (idx >= 0 && idx < this->mInputLinks.size()-1)
		this->mInputLinks[idx] = link;
	NMDebugAI( << "new no of in links: " << this->mInputLinks.size() << std::endl);

	NMDebugCtx(ctx, << "done!");
}

void NMProcessComponentItem::addOutputLink(int idx, NMComponentLinkItem* link)
{
	NMDebugCtx(ctx, << "...");

	if (link == 0)
		return;

	if (idx < 0 || idx > this->mOutputLinks.count()-1)
		this->mOutputLinks.push_back(link);
	else //if (idx >= 0 && idx < this->mOutputLinks.size()-1)
		this->mOutputLinks[idx] = link;

	NMDebugAI( << "new no of out links: " << this->mOutputLinks.size() << std::endl);

	NMDebugCtx(ctx, << "done!");
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

    if (title.contains(QString("Reader"), Qt::CaseInsensitive))
    {
        mIcon.load(":data-read-icon.png");
    }
    else if (title.contains(QString("Writer"), Qt::CaseInsensitive))
    {
        mIcon.load(":data-save-icon.png");
    }
    else if (title.contains("DataBuffer", Qt::CaseInsensitive))
    {
        mIcon.load(":image_layer.png");
    }
    else
    {
        mIcon.load(":model-icon.png");
    }
}

QRectF
NMProcessComponentItem::boundingRect(void) const
{
	return this->getShapeAsPolygon().boundingRect();
}

QPolygonF
NMProcessComponentItem::getShapeAsPolygon(void) const
{
	QRectF gap = QRectF(mIconBnd.left(), mIconBnd.bottom()-2,
			            mIconBnd.width(), mSingleLineHeight);

	QPolygonF iconPoly(mIconBnd);
	QPolygonF jointPoly = iconPoly.united(gap);
	QPolygonF txtPoly(mTextRect);
	return jointPoly.united(txtPoly);
}

//QPainterPath NMProcessComponentItem::shape(void) const
//{
//	QPainterPath path;
//	path.addPolygon(this->getShapeAsPolygon());
//	return path;
//}


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
    this->mTimeLevel;
    this->update();
}

void
NMProcessComponentItem::updateDescription(
	const QString& descr)
{
	this->mDescription = descr;

	QFontMetricsF fm(mFont);
	QRectF fbr = fm.boundingRect(mDescription);

	if (fbr.width() > mIconBnd.width() && fbr.width() < mMaxTextWidth)
	{
		mTextRect.setLeft(-(fbr.width()*0.5));
		mTextRect.setRight(fbr.width()*0.5);
		mTextRect.setBottom(mTextRect.top()+mSingleLineHeight);
	}
	else if (fbr.width() >= mMaxTextWidth)
	{
		mTextRect.setLeft(-(mMaxTextWidth));
		mTextRect.setRight(mMaxTextWidth);
		mTextRect.setBottom(mTextRect.top()+mDoubleLineHeight);
	}
	else
	{
		mTextRect.setLeft(mIconBnd.left());
		mTextRect.setRight(mIconBnd.right());
		mTextRect.setBottom(mTextRect.top()+mSingleLineHeight);
	}

	this->update();
}


void
NMProcessComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{

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
		QPen pen = QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine);
		painter->setPen(pen);
		painter->drawRoundRect(mIconBnd, 10, 10);

		// draw icon
		//QImage execIcon(mIcon.toImage());
		//QPainter iconPainter(&execIcon);
		//iconPainter.fillRect(QRectF(QPointF(0,0), mIcon.size()), QColor(255,0,0,128));
		//iconPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		////iconPainter.fillRect(QRectF(QPointF(0,0), mIcon.size()), QColor(0,0,0,128));
		//painter->drawImage(QRectF(-40,-40,64,64), execIcon);
		painter->drawPixmap(mIconRect, mIcon, QRectF(0,0,64,64));

		if (mProgress > 0)
		{
			painter->setPen(QPen(QBrush(Qt::darkRed), 1, Qt::SolidLine));
			mFont.setItalic(true);
			painter->setFont(mFont);
            QString strProg = QString("%1\%").arg(this->mProgress, 3, 'f', 0);
            painter->drawText(QRectF(mIconBnd.right()-28,mIconBnd.top()+7,23,15),
					Qt::AlignLeft, strProg);
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
			pen = QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine);
		painter->setPen(pen);
		painter->drawRoundRect(mIconBnd, 10, 10);

		// draw icon
		painter->drawPixmap(mIconRect, mIcon, QRectF(0,0,64,64));
	}

	// draw description
	painter->setBrush(Qt::NoBrush);
	painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
	mFont.setItalic(false);
    painter->setFont(mFont);
    painter->drawText(QRectF(mIconBnd.left()+3,mIconBnd.top()+7,15,15),
                      Qt::AlignLeft, QString("%1").arg(mTimeLevel));
    mFont.setBold(true);
	painter->setFont(mFont);
	painter->drawText(mTextRect, Qt::AlignCenter | Qt::TextWordWrap,
				mDescription);

}

QDataStream& operator<<(QDataStream& data, const NMProcessComponentItem& item)
{
	NMProcessComponentItem& i = const_cast<NMProcessComponentItem&>(item);
	data << i.getTitle();
    data << i.getTimeLevel();
	data << i.scenePos();
	data << i.getIsDataBufferItem();
	return data;
}

QDataStream& operator>>(QDataStream& data, NMProcessComponentItem& item)
{
	QPointF pos;
	QString title;
	QString descr;
	bool databuffer;
    short timelevel;

    data >> title >> timelevel >> pos >> databuffer;

	item.setTitle(title);
    item.setTimeLevel(timelevel);
	item.setPos(pos);
	item.setIsDataBufferItem(databuffer);
	return data;
}






