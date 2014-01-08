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

#include "NMLayerModel.h"
#include "NMLayer.h"

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
	layer->setParent(this);
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
    if (!index.parent().isValid())
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
	QModelIndex idx;

	if (row < 0 || column > 1)
		return idx;

	int iid = -1;
	// top level (layer) item (i.e. no parent)
	if (!parent.isValid())
    {
       	if (row < this->mLayers.size() && column == 0)
       	{
			iid = (row + 1) * 100;
			idx = createIndex(row, 0, iid);
       	}
    }
	// legend item / legend admin item
    else
    {
    	int toplevelrow = (parent.internalId() / 100) - 1;
    	int level = parent.internalId() % 100;

    	if (level == 1)
    	{
    		if (parent.row() == 0)
    		{
				if (	 (column >= 0 && column < 2)
					&&   (row < 8)
				   )
				{
					iid = (toplevelrow + 1) * 100 + 2;
					idx = createIndex(row, column, iid);
				}
    		}
    	}
		else if (level == 0)
		{
			iid = (toplevelrow + 1) * 100 + 1;
			idx = createIndex(row, 0, iid);
		}
    }

    return idx;
}

bool NMLayerModel::setData(const QModelIndex& index,
		const QVariant& value, int role)
{
	if (!index.isValid())
		return false;

	const int toplevelrow = (index.internalId() / 100) - 1;
	const int stackpos = this->toLayerStackIndex(toplevelrow);
	const int level = index.internalId() % 100;

	// leave when we are dealing with legend items
	if (level == 0)
	{
		if (role == Qt::CheckStateRole)
		{
			NMLayer* l = this->getItemLayer(stackpos);
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
	QString sName;

	const int col = index.column();
	const int row = index.row();

	const int toplevelrow = (index.internalId() / 100) - 1;
	const int level = index.internalId() % 100;

	NMLayerModel* lm = const_cast<NMLayerModel*>(this);
	const int stackpos = lm->toLayerStackIndex(toplevelrow);
	NMLayer* l = lm->getItemLayer(stackpos);

	/////////////////////////// LEGEND CLOUR ITEMS ///////////////////////////////////////
	switch(level)
	{
	case 1:
	{
		switch (role)
		{
			case Qt::DisplayRole:
			{
				//col == 0 ? retVar = QVariant(l->getLegendName(row)) : QString();
				retVar = QVariant(l->getLegendName(row));
			}
			break;

			case Qt::ToolTipRole:
			{
				if (row == 0)
					retVar = tr("Double click to show/edit legend metadata");
			}
			break;

			case Qt::DecorationRole:
			{
				if (row > 0)
				{
					//col == 0 ? retVar = lm->createLegendIcon(l, row): QIcon();
					retVar = l->getLegendIcon(row);
				}
			}
			break;

			case Qt::FontRole:
				{
					QFont font;
					if (row == 0)
						font.setPointSize(9);
					else
						font.setPointSize(8);

					retVar = font;
				}
				break;


			case Qt::SizeHintRole:
			{
				if (row == 0)
				{
					retVar = QSize(0, 20);
				}
				else
				{
					if (l->getLegendType() == NMLayer::NM_LEGEND_RAMP && row == 3)
					{
						retVar = QSize(16, 60);
					}
					else
					{
						retVar = QSize(16, 20);
					}
				}
			}
			break;

			// herewith we provide the NMComponentListItemDelegate with the
			// specified legend type to paint ramp description
			case Qt::UserRole:
				retVar = l->getLegendTypeStr(l->getLegendType());
				break;

			case Qt::UserRole+1:
				retVar = QString(tr("%1")).arg(l->getUpper());
				break;

			case Qt::UserRole+2:
				retVar = QString(tr("%1")).arg(l->getLower());
				break;

			default:
				break;
		}
	}
	break;
	/////////////////////////// LEGEND ADMIN ITEMS ///////////////////////////////////////
	case 2:
	{
		NMLayer* pl = l;
		switch(role)
		{
			case Qt::FontRole:
			{
				QFont font;
				font.setPointSize(7);
				if (col == 0)
				{
					font.setItalic(true);
				}
				retVar = font;
			}
			break;

			case Qt::ForegroundRole:
			{
				if (col == 0)
					return QVariant(QColor(0,0,255));
			}
			break;

			case Qt::DisplayRole:
			{
				switch(row)
				{
				case 0: col == 0 ? retVar = "Value Field" : retVar = pl->getLegendValueField(); break;
				case 1:	col == 0 ? retVar = "Descr Field" : retVar = pl->getLegendDescrField(); break;
				case 2: col == 0 ? retVar = "Legend Type"
						: retVar = pl->getLegendTypeStr(pl->getLegendType()); break;
				case 3: col == 0 ? retVar = "Legend Class Type"
						: retVar = pl->getLegendClassTypeStr(pl->getLegendClassType()); break;
				case 4: col == 0 ? retVar = "Colour Ramp"
						: retVar = pl->getColourRampStr(pl->getColourRamp()); break;
				case 5: col == 0 ? retVar = "Lower" : retVar = QVariant(pl->getLower()); break;
				case 6: col == 0 ? retVar = "Upper" : retVar = QVariant(pl->getUpper()); break;
				case 7: col == 0 ? retVar = "Nodata" : retVar = QVariant(pl->getNodata()); break;
				}
			}
			break;

			case Qt::SizeHintRole:
			{
				retVar = QSize(32, 16);
			}
			break;
		}
	}
	break;
	/////////////////////////// LAYER ADMIN ITEMS ///////////////////////////////////////
	case 0:
	{
		if (col == 0)
		{
			switch (role)
			{
				case Qt::DisplayRole:
				{
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
	}
	break;

	default:
	break;

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
	int ret = 0;

	if (!parent.isValid())
	{
		return this->mLayers.size();
	}

	int toplevelrow = (parent.internalId() / 100) - 1;
	int level = parent.internalId() % 100;

	if (level == 1 && parent.row() == 0)
	{
		ret = 8;
	}
	else if (level == 0)
	{
		NMLayerModel* lm = const_cast<NMLayerModel*>(this);
		int stackpos = lm->toLayerStackIndex(toplevelrow);
		NMLayer* l = lm->getItemLayer(stackpos);
		ret = l->getLegendItemCount();
	}

	return ret;
}

int NMLayerModel::columnCount(const QModelIndex& parent) const
{
	return 1;
	//int ncols = 1;
    //
	//if (!parent.isValid())
	//	return ncols;
    //
	//const int level = parent.internalId() % 100;
	//if (level == 1 && parent.row() == 0)
	//	ncols = 2;
    //
	//return ncols;
}

QModelIndex NMLayerModel::parent(const QModelIndex& index) const
{
	QModelIndex parentIdx;

	if (!index.isValid())
		return parentIdx;

	const int toplevelrow = (index.internalId() / 100) - 1;
	const int level = index.internalId() % 100;

	int iid = 0;
	switch(level)
	{
		case 1: // legend item level
		{
			iid = (toplevelrow + 1) * 100; // + 0;
			parentIdx = this->createIndex(toplevelrow, 0, iid);
		}
		break;

		case 2: // legend item admin level (always associated with row=0 on level 1)
		{
			iid = (toplevelrow + 1) * 100 + 1;
			parentIdx = this->createIndex(0, 0, iid);
		}
		break;

		case 0:
		default:
		break;
	}

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
	int nrows = this->mLayers.size();

	if (nrows == 0)
		return -1;

	if (treePos < 0 || treePos >= nrows)
		return -1;

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
