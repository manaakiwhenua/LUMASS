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
#include "nmlog.h"

NMProcessComponentItem::NMProcessComponentItem(QGraphicsItem* parent,
		NMModelScene* scene)
	: QGraphicsItem(parent), mContextMenu(0) ,
	  mProgress(0.0), mbIsExecuting(false)
{
	ctx = "NMProcessComponentItem";
	this->mScene = scene;

	mIcon.load(":model-icon.png");
	mBndRect = QRectF(-45, -45, 90, 90);
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
}

QRectF NMProcessComponentItem::boundingRect(void) const
{
	//QRectF rr(mBndRect);
	//rr.adjust(-5.0, -5.0, 10.0, 10.0);
	return mBndRect;
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
NMProcessComponentItem::paint(QPainter* painter,
		const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{

	if(mbIsExecuting)
	{
		QSizeF psize = mBndRect.size();
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
	    //fade.setColorAt(0.6, QColor(0,0,0,127));
	    fade.setColorAt(0, QColor(0,0,0,90));
	    bgPainter.fillRect(img.rect(), fade);

		painter->drawImage(mBndRect, img);
	    painter->setRenderHint(QPainter::Antialiasing, true);

		// draw boundary
		painter->setBrush(Qt::NoBrush);
		QPen pen = QPen(QBrush(Qt::darkGray), 1, Qt::SolidLine);
		painter->setPen(pen);
		painter->drawRoundRect(QRectF(-45,-45,90,90), 10, 10);

		// draw icon
		//QImage execIcon(mIcon.toImage());
		//QPainter iconPainter(&execIcon);
		//iconPainter.fillRect(QRectF(QPointF(0,0), mIcon.size()), QColor(255,0,0,128));
		//iconPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		////iconPainter.fillRect(QRectF(QPointF(0,0), mIcon.size()), QColor(0,0,0,128));
		//painter->drawImage(QRectF(-40,-40,64,64), execIcon);
		painter->drawPixmap(QRectF(-40,-40,64,64), mIcon, QRectF(0,0,64,64));

		if (mProgress > 0)
		{
			painter->setPen(QPen(QBrush(Qt::darkRed), 1, Qt::SolidLine));
			mFont.setItalic(true);
			painter->setFont(mFont);
			QString strProg = QString("%1").arg(this->mProgress, 3, 'f', 0);
			painter->drawText(QRectF(10,-43,20,15), Qt::AlignLeft, strProg);
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
		painter->drawRoundRect(QRectF(-45,-45,90,90), 10, 10);

		// draw icon
		painter->drawPixmap(QRectF(-40,-40,64,64), mIcon, QRectF(0,0,64,64));
	}

	// draw the title
	painter->setBrush(Qt::NoBrush);
	painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
	mFont.setItalic(false);
	mFont.setBold(true);
	painter->setFont(mFont);
	painter->drawText(QRectF(-40,25,80,15), Qt::AlignCenter, mTitle);
}



QDataStream& operator<<(QDataStream& data, const NMProcessComponentItem& item)
{
	//qDebug() << "proc comp: op<< begin ..." << endl;
	NMProcessComponentItem& i = const_cast<NMProcessComponentItem&>(item);
	//data << (qint32)NMProcessComponentItem::Type;
	data << i.getTitle();
	data << i.scenePos();
	//qDebug() << "proc comp: op<< end!" << endl;
	return data;
}

QDataStream& operator>>(QDataStream& data, NMProcessComponentItem& item)
{
	//qDebug() << "proc comp: op>> begin ..." << endl;
	QPointF pos;
	QString title;
	data >> title;
	data >> pos;
	item.setTitle(title);
	item.setPos(pos);

	//qDebug() << "proc comp: op>> end!" << endl;
	return data;
}






