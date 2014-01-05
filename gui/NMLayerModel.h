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
//#include "NMLayer.h"

#include <QObject>
#include <QStandardItem>
#include <QAbstractItemModel>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <QSharedPointer>

class NMLayer;

/*!	\brief The NMLayerModel class manages map layers and provides and interface
 * 		   to control how a layer is displayed in the geospatial map view. The
 * 		   general structure of a map layer is given in the table below:
 *
 * 	<table>
 * 		<tr> 	<td><b>Level</b></td> <td><b>row</b></td> <td><b>internalPointer()</b></td> <td><b>internalId()</b></td></tr>
 * 		<tr>       <td><i>layer entry</i></td> <td><i>layer index</i></td> <td><i>pointer to layer</i></td> <td><i></i></td></tr>
 * 		<tr>       <td>layer entry</td> <td>0</td>   <td>layer address</td> <td></td></tr>
 * 		<tr>       <td>layer entry</td> <td>1</td>   <td>layer address</td> <td></td></tr>
 * 		<tr>       <td>layer entry</td> <td>...</td> <td>layer address</td> <td></td></tr>
 *  	<tr>       <td><i>legend item</i></td> <td><i>legend item index </i></td> <td><i>NULL</i></td> <td><i>item identifier</i></td></tr>
 * 		<tr>       <td><i>legend metadata</i></td> <td><i>data item and index</i></td> <td><i>NULL</i></td> <td><i>item identifier</i></td></tr>
 *  	<tr>       <td>legend admin</td> <td>0: Value Field</td>     <td>0</td> <td>-170</td></tr>
 *  	<tr>       <td>legend admin</td> <td>1: Descr Field</td>     <td>0</td> <td>-171</td></tr>
 *  	<tr>       <td>legend admin</td> <td>2: LegendType</td>      <td>0</td> <td>-172</td></tr>
 *  	<tr>       <td>legend admin</td> <td>3: LegendClassType</td> <td>0</td> <td>-173</td></tr>
 *  	<tr>       <td>legend admin</td> <td>4: ColourRamp</td>      <td>0</td> <td>-174</td></tr>
 *  	<tr>       <td>legend admin</td> <td>5: Lower</td>           <td>0</td> <td>-175</td></tr>
 *  	<tr>       <td>legend admin</td> <td>6: Upper</td>           <td>0</td> <td>-176</td></tr>
 *  	<tr>       <td>legend admin</td> <td>7: Nodata</td>          <td>0</td> <td>-177</td></tr>

 *  	<tr>       <td>...</td> <td></td>          <td></td> <td></td></tr>
 * 	</table>
 *
 */

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

public slots:
	void layerDataSetChanged(const NMLayer* layer);

private:

	// the map holding the actual layer objects
	// note that the index 0 denotes the first layer
	// in the list
	QList<QSharedPointer<NMLayer > > mLayers;

	// creates a colour Icon representing the value at table entry
	// at index idx
	QIcon createLegendIcon(NMLayer* layer, int legendRow);

};

#endif /* NMLAYERMODEL_H_ */
