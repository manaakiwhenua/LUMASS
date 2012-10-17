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

#include "vtkLookupTable.h"
#include "vtkMapper.h"

NMLayerModel::NMLayerModel(QObject* parent)
{
//	NMDebugCtx(ctxNMLayerModel, << "...");

//	NMDebugCtx(ctxNMLayerModel, << "done!");
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

	// update the layer model ---------------------------------------
	this->layoutAboutToBeChanged();

	this->mLayers.insert(nLayers, pL);

	emit layoutChanged();

//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return 1;
}

int NMLayerModel::changeItemLayerPos(int oldpos, int newpos)
{
	if (oldpos == newpos ||
		oldpos < 0 || oldpos > this->rowCount(QModelIndex())-1 ||
		newpos < 0 || newpos > this->rowCount(QModelIndex())-1    )
		return oldpos;

	int srctreepos = this->toTreeModelRow(oldpos);
	int desttreepos = this->toTreeModelRow(newpos);

	QSharedPointer<NMLayer> pL = this->mLayers.at(srctreepos);
	this->mLayers.removeAt(srctreepos);

	this->layoutAboutToBeChanged();

	this->removeRow(srctreepos);
	this->mLayers.insert(desttreepos, pL);

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
//	NMDebugCtx(ctxNMLayerModel, << "...");

    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    // top level items (i.e. layer entries)
    if (index.internalPointer() != 0)
    {
    	// checkboxes (selctability and visibility)
//    	if (index.column() < 2)
    		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable | Qt::ItemIsSelectable;
//    	else
//    		flags |= Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
    }


//    NMDebugCtx(ctxNMLayerModel, << "done!");
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
//	NMDebugCtx(ctxNMLayerModel, << "...");

	// leave when we are dealing with legend items
	if (!index.parent().isValid())
	{
		if (role == Qt::CheckStateRole)
		{
			NMLayer* l = (NMLayer*)index.internalPointer();
			l->isVisible() ? l->setVisible(false) : l->setVisible(true);
		}
	}

	emit dataChanged(index, index);

//	NMDebugCtx(ctxNMLayerModel, << "done!"	<< endl);
	return true;
}

QVariant NMLayerModel::data(const QModelIndex& index, int role) const
{
//	NMDebugCtx(ctxNMLayerModel, << "...");

	if (!index.isValid())
	{
//		NMDebugCtx(ctxNMLayerModel, << "done!");
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
	}
	else 	// return general layer info
	{
		l = (NMLayer*)index.internalPointer();
		switch (role)
		{
		case Qt::DisplayRole:
			retVar = QVariant(l->objectName());
			break;
		case Qt::CheckStateRole:
			retVar = l->isVisible() ? Qt::Checked : Qt::Unchecked;
			break;
		case Qt::FontRole:
			QFont font;
			l->hasChanged() ? font.setItalic(true) : font.setItalic(false);
			retVar = font;
			break;
		}

	}

//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return retVar;
}

QIcon NMLayerModel::createLegendIcon(NMLayer* layer, int legendRow)
{
//	NMDebugCtx(ctxNMLayerModel, << "...");

	double rgba[4];
	if (!layer->getLegendColour(legendRow, rgba))
	{
//		NMDebugCtx(ctxNMLayerModel, << "done!");
		return QIcon();
	}

	QColor clr;
	clr.setRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);

	QPixmap pix(30,30);
	pix.fill(clr);

	QIcon icon(pix);

//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return icon;
}

int NMLayerModel::rowCount(const QModelIndex& parent) const
{
//	NMDebugCtx(ctxNMLayerModel, << "...");

	int ret = this->mLayers.size();

	if (!parent.isValid())
	{
//		NMDebugCtx(ctxNMLayerModel, << "done!");
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

//	NMDebugCtx(ctxNMLayerModel, << "done!");
	return ret;
}

int NMLayerModel::columnCount(const QModelIndex& parent) const
{
	int ncols = 1;

	if (parent.isValid())
		ncols = 2;

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
