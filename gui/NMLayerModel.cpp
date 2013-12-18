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
 * NMLayerModel.cpp
 *
 *  Created on: 11/04/2011
 *      Author: alex
 */

#include <NMLayerModel.h>

#include <QPixmap>
#include <QPainter>

#include "vtkLookupTable.h"
#include "vtkMapper.h"


NMLayerModel::NMLayerModel(QObject* parent)
{
}

NMLayerModel::~NMLayerModel()
{
}

int NMLayerModel::pileItemLayer(NMLayer* layer)
{
	//	NMDebugCtx(ctxNMLayerModel, << "...");

	if (layer == 0)
	{
		NMDebugInd(1, << "oops, received NULL layer!" << endl);
		//		NMDebugCtx(ctxNMLayerModel, << "done!");
		return 0;
	}

	// how many layers do we have
	int nLayers = this->mLayers.size();

	int stackindex = nLayers == 0 ? 0 : nLayers-1;
	int treeidx = this->toTreeModelRow(stackindex);
	QSharedPointer<NMLayer> pL(layer);
	pL->setLayerPos(nLayers);

	connect(pL.data(), SIGNAL(dataSetChanged(const NMLayer *)),
			this, SLOT(layerDataSetChanged(const NMLayer *)));

	// update the layer model ---------------------------------------
	this->layoutAboutToBeChanged();

	this->mLayers.insert(nLayers, pL);

	emit layoutChanged();

	//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return 1;
}

void
NMLayerModel::layerDataSetChanged(const NMLayer* layer)
{
	if (layer == 0)
		return;

	QModelIndex idx = this->getItemLayerModelIndex(layer->objectName());
	emit dataChanged(idx, idx);
}

int NMLayerModel::changeItemLayerPos(int oldpos, int newpos)
{
	if (oldpos == newpos ||
		oldpos < 0 || oldpos > this->rowCount(QModelIndex())-1 ||
		newpos < 0 || newpos > this->rowCount(QModelIndex())-1    )
		return oldpos;

	emit layoutAboutToBeChanged();

	QSharedPointer<NMLayer> pL = this->mLayers.takeAt(oldpos);
	this->mLayers.insert(newpos, pL);
	emit layoutChanged();

	// update each layer's position within the layer stack
	for (int i=0; i < this->mLayers.size(); ++i)
		this->mLayers[i]->setLayerPos(i);

	return oldpos;
}

void NMLayerModel::removeLayer(NMLayer* layer)
{
	//	NMDebugCtx(ctxNMLayerModel, << "...");

	// get the stack position of this layer
	int layerPos = layer->getLayerPos();
	int treeRow =  this->toTreeModelRow(layerPos);

	disconnect(layer, SIGNAL(dataSetChanged(const NMLayer *)),
			this, SLOT(layerDataSetChanged(const NMLayer *)));

	// remove layer from internal list
	QString ln = layer->objectName();
	this->mLayers.removeAt(layerPos);
	this->removeRow(treeRow);
	this->reset();
	NMDebugAI(<< "removing layer: " << ln.toStdString() << endl);
	NMDebugAI(<< "layerPos = " << layerPos << endl);
	NMDebugAI(<< "treeRow = " << treeRow << endl);

	// update each layer's position within the layer stack
	for (int i=0; i < this->mLayers.size(); ++i)
		this->mLayers[i]->setLayerPos(i);

	//	NMDebugCtx(ctxNMLayerModel, << "done!");
}

Qt::ItemFlags NMLayerModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    // top level items (i.e. layer entries)
    if (index.internalPointer() != 0)
    {
    	// checkboxes (selctability and visibility)
   		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable |
    				 Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
    				 Qt::ItemIsDropEnabled;
    }

    return flags;
}

QModelIndex NMLayerModel::index(int row, int column, const QModelIndex& parent) const
{
	//	NMDebugCtx(ctxNMLayerModel, << "...");

	QModelIndex idx;

    NMLayer* l;
    if (!parent.isValid())
    {
       	NMLayerModel* lm = const_cast<NMLayerModel*>(this);
       	int stackpos = lm->toLayerStackIndex(row);
       	NMLayer* l = lm->getItemLayer(stackpos);
       	idx = createIndex(row, column, (void*) l);
    }
    else
    {
    	idx = createIndex(row, column, parent.row());
    }

    //    NMDebugCtx(ctxNMLayerModel, << "done!");
    return idx;
}

bool NMLayerModel::setData(const QModelIndex& index,
		const QVariant& value, int role)
{
	// leave when we are dealing with legend items
	if (!index.parent().isValid())
	{
		if (role == Qt::CheckStateRole)
		{
			NMLayer* l = (NMLayer*)index.internalPointer();
			if (value.toString() == "VIS")
				l->isVisible() ? l->setVisible(false) : l->setVisible(true);
			else if (value.toString() == "SEL")
				l->isSelectable() ? l->setSelectable(false) : l->setSelectable(true);
		}
	}

	emit dataChanged(index, index);

	return true;
}

QVariant NMLayerModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	QVariant retVar;
	NMLayer* l;
	QString sName;

	int col = index.column();
	int row = index.row();

	// return legend related info (i.e. icon or category name)
	if (index.parent().isValid())
	{
		l = (NMLayer*)index.parent().internalPointer();
		NMLayerModel* lm = const_cast<NMLayerModel*>(this);
		if (role == Qt::DisplayRole)
		{
			if (col == 0)
			{
				if (l->getLegendItemCount() == 1)
				{
					retVar = tr("all Features");
				}
				else
					retVar = QVariant(l->getLegendName(row));
			}
			else if (col == 1)
			{
				if (l->getLegendItemCount() == 1)
				{
					retVar = tr("");
				}
				else
				{
					retVar = QVariant(QString(tr("%1 - %2")).arg(l->getLegendItemLowerValue(row)).
							arg(l->getLegendItemUpperValue(row)));
				}
			}
		}
		else if (role == Qt::DecorationRole)
		{
			if (col == 0)
				retVar = lm->createLegendIcon(l, row);
		}
		else if (role == Qt::SizeHintRole)
		{
			retVar = QSize(16, 20);
		}
	}
	else 	// return general layer info
	{
		l = (NMLayer*)index.internalPointer();
		switch (role)
		{
		case Qt::DisplayRole:
			{
				if (col == 0)
					retVar = l->objectName();
			}
			break;

		case Qt::FontRole:
			{
				QFont font;
				l->hasChanged() ? font.setItalic(true) : font.setItalic(false);
				retVar = font;
			}
			break;

		case Qt::SizeHintRole:
			retVar = QSize(32, 22);
			break;

		case Qt::DecorationRole:
			{
				if (col == 0)
				{
					QImage pix(32,16, QImage::Format_ARGB32_Premultiplied);
					pix.fill(0);

					QPainter painter(&pix);
					QPixmap selImg = l->isSelectable() ?
							QPixmap(":edit-select_enabled.png") : QPixmap(":edit-select.png");
					QPixmap layerImg = l->getLayerIcon().pixmap(QSize(16,16));
					painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
					painter.drawPixmap(QRect(0,0,16,16), layerImg);
					painter.drawPixmap(QRect(17,0,16,16), selImg);

					QPixmap rPix(32, 16);
					rPix.convertFromImage(pix);

					QIcon rIcon(rPix);
					retVar = rIcon;
				}
				break;
			}
		}
	}

	return retVar;
}

QIcon NMLayerModel::createLegendIcon(NMLayer* layer, int legendRow)
{

	double rgba[4];
	if (!layer->getLegendColour(legendRow, rgba))
	{
		return QIcon();
	}

	QColor clr;
	clr.setRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);

	QPixmap pix(32,32);
	pix.fill(clr);

	QIcon icon(pix);

	return icon;
}

int NMLayerModel::rowCount(const QModelIndex& parent) const
{
	int ret = this->mLayers.size();

	if (!parent.isValid())
	{
		return ret;
	}

	// if we've got an internal pointer, parent
	// must be valid and therefore we have to
	// report the number of child rows (i.e. legend
	// items)
	NMLayerModel* lm = const_cast<NMLayerModel*>(this);
	int stackpos = lm->toLayerStackIndex(parent.row());
	NMLayer* l = lm->mLayers[stackpos].data();

	ret = l->getLegendItemCount();

	return ret;
}

int NMLayerModel::columnCount(const QModelIndex& parent) const
{
	int ncols = 1;
	//int ncols = 2;

	if (parent.isValid())
	{
		ncols = 2;
	}

	return ncols;
}

QModelIndex NMLayerModel::parent(const QModelIndex& index) const
{
	//	NMDebugCtx(ctxNMLayerModel, << "...");

	QModelIndex parentIdx;

	// if we've got an internal pointer attached,
	// index is already a parent
	if (index.internalId() >= 0 && index.internalId() < this->mLayers.size())
	{
		int row = index.internalId();
		NMLayerModel* lm = const_cast<NMLayerModel*>(this);
		int stackpos = lm->toLayerStackIndex(row);
		NMLayer* l = this->mLayers[stackpos].data();
		parentIdx = this->createIndex(row, 0, (void*)l);
	}

	//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return parentIdx;
}

NMLayer* NMLayerModel::getItemLayer(int idx)
{
	if (idx < 0 || idx >= this->mLayers.size())
		return 0;

	return this->mLayers[idx].data();
}

NMLayer* NMLayerModel::getItemLayer(QString layerName)
{
	NMLayer* l = 0;
	for (int i=0; i < this->mLayers.size(); ++i)
	{
		if (this->mLayers[i]->objectName() == layerName)
		{
			l = this->mLayers[i].data();
			break;
		}
	}

	return l;
}

int NMLayerModel::getItemLayerCount()
{
	return this->mLayers.size();
}



int NMLayerModel::toTreeModelRow(int stackIndex)
{
	int nLayers = this->mLayers.size();

	if (nLayers == 0)
		return 0;

	if (stackIndex < 0 || stackIndex >= nLayers)
		return 0;

	return nLayers - stackIndex - 1;
}

int NMLayerModel::toLayerStackIndex(int treePos)
{
	int nrows = this->rowCount(QModelIndex());

	if (nrows == 0)
		return 0;

	if (treePos < 0 || treePos >= nrows)
		return 0;

	return nrows - treePos - 1;
}

QModelIndex NMLayerModel::getItemLayerModelIndex(QString layerName)
{
	QModelIndex lidx;

	int stackid = -1;
	NMLayer* l = this->getItemLayer(layerName);

	if (l == 0)
		return lidx;

	stackid = l->getLayerPos();
	int treepos = this->toTreeModelRow(stackid);

	return this->index(treepos, 0, QModelIndex());
}
