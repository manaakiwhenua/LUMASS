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
 * NMProcCompList.cpp
 *
 *  Created on: 19/06/2012
 *      Author: alex
 */

#include "NMProcCompList.h"
#include <QDrag>
#include <QMimeData>


NMProcCompList::NMProcCompList(QWidget* parent)
	: QListWidget(parent)
{
	ctx = "NMProcCompList";

    this->addItem(QString::fromLatin1("CastImage"));
    //this->addItem(QString::fromLatin1("CombineTwo"));
    this->addItem(QString::fromLatin1("CostDistanceBuffer"));
    this->addItem(QString::fromLatin1("DataBuffer"));
    this->addItem(QString::fromLatin1("FocalDistanceWeight"));
    this->addItem(QString::fromLatin1("ImageReader"));
    this->addItem(QString::fromLatin1("ImageWriter"));
    this->addItem(QString::fromLatin1("MapAlgebra"));
    this->addItem(QString::fromLatin1("NeighbourCounter"));
    this->addItem(QString::fromLatin1("ParameterTable"));
    this->addItem(QString::fromLatin1("RandomImage"));
    this->addItem(QString::fromLatin1("ResampleImage"));
    this->addItem(QString::fromLatin1("SQLProcessor"));
    this->addItem(QString::fromLatin1("SummarizeZones"));
    this->addItem(QString::fromLatin1("UniqueCombination"));
    this->addItem(QString::fromLatin1("ExternalExec"));
    this->addItem(QString::fromLatin1("TextLabel"));
}

NMProcCompList::~NMProcCompList()
{
}

void NMProcCompList::mouseMoveEvent(QMouseEvent* event)
{
	QModelIndex idx = this->indexAt(mDragStart);
	QString itText = idx.data(Qt::DisplayRole).toString();

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
    QString mimeText = QString::fromLatin1("_NMProcCompList_:%1").arg(itText);
    mimeData->setText(mimeText.toStdString().c_str());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction, Qt::CopyAction);

}

void NMProcCompList::mousePressEvent(QMouseEvent* event)
{
	this->mDragStart = event->pos();
	QModelIndex idx = this->indexAt(event->pos());
	QVariant itText = idx.data(Qt::DisplayRole);


	NMDebugAI(<< "I've got " << itText.toString().toStdString()
			  << " on the hook!" << std::endl);


}
