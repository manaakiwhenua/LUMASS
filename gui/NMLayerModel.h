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
 * NMLayerModel.h
 *
 *  Created on: 11/04/2011
 *      Author: alex
 */

#ifndef NMLAYERMODEL_H_
#define NMLAYERMODEL_H_

#define ctxNMLayerModel "NMLayerModel"
#include "nmlog.h"
#include "NMLayer.h"

#include <QObject>
#include <QStandardItem>
#include <QAbstractItemModel>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <QSharedPointer>

class NMLayerModel : public QAbstractItemModel
{
	Q_OBJECT


public:
	NMLayerModel(QObject* parent=0);
	virtual ~NMLayerModel();

	// public interface
	//void insertItem(int pos, QString name);
	int insertItemLayer(int pos, NMLayer* layer);
	int pileItemLayer(NMLayer* layer);
	int getItemLayerCount(void);
	NMLayer* getItemLayer(int idx);
	NMLayer* getItemLayer(QString layerName);
	QModelIndex getItemLayerModelIndex(QString layerName);
	int changeItemLayerPos(int oldpos, int newpos);
	void removeLayer(NMLayer* layer);

	// required tree model interface
	QModelIndex index(int row, int column, const QModelIndex& parent) const;
	QModelIndex parent(const QModelIndex& index) const;
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex& index, int role) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role=Qt::EditRole);

	// extended functionality
//	Qt::DropActions supportedDropActions(void) const
//		{return Qt::CopyAction | Qt::MoveAction;};
//	QStringList mimeTypes() const;
//	QMimeData* mimeData(const QModelIndexList& indexes) const;
//	bool removeRows(int row, int count, const QModelIndex& parent=QModelIndex());

private:

	// the map holding the actual layer objects
	// note that the index 0 denotes the first layer
	// in the list
	QList<QSharedPointer<NMLayer > > mLayers;

	/* conversion from tree model rows into layerStack
	 * indices
	 *
	 * tree model	layer stack
	 * row	0		index   3
	 * 		1				2
	 * 		2				1
	 * 		3				0
	 *
	 *  layerStackPos = itemCount - treePos - 1
	 *  treePos		  = itemCount - layerStackPos - 1
	 */
	int toLayerStackIndex(int treePos);
	int toTreeModelRow(int stackIndex);

	// creates a colour Icon representing the value at table entry
	// at index idx
	QIcon createLegendIcon(NMLayer* layer, int legendRow);

};

#endif /* NMLAYERMODEL_H_ */
