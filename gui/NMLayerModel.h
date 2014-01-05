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

/*!	\brief The NMLayerModel class manages map layers and provides and interactive
 * 		   legend to control the colour coding of the map in the map view. The
 * 		   general tree view structure used for the legend is given in the following
 * 		   table.
 *
 * 	<table>
 * 		<tr> 	<td><b>Tree Level</b></td> <td><b>Row #</b></td>  <td><b>internalId()</b></td> </tr>
 * 		<tr>    <td><i>level</i></td>          <td><i>layer number</i> </td> <td><i>item model identifier</i></td>      </tr>
 * 		<tr>    <td>0</td>                 <td>0</td>             <td>100</td>       </tr>
 * 		<tr>    <td>0</td>                 <td>1</td>             <td>200</td>       </tr>
 * 		<tr>    <td>0</td>                 <td>...</td>           <td>...</td>       </tr>
 * 		<tr>    <td>0</td>                 <td>n</td>             <td>(n+1)*100</td>       </tr>
 *  	<tr>    <td><i>level</i></td> <td><i>legend item number</i></td> <td><i>item model identifier</i></td> </tr>
 *      <tr>    <td>1</td>                 <td>0</td>             <td>101</td>          </tr>
 *      <tr>    <td>1</td>                 <td>1</td>             <td>101</td>          </tr>
 *      <tr>    <td>1</td>                 <td>...</td>           <td>...</td>          </tr>
 *      <tr>    <td>1</td>                 <td>m</td>             <td>(n+1)*100+1</td>  </tr>
 * 		<tr>    <td><i>level</i></td> <td><i>legend metadata item number: meaning</i></td> <td><i>item model identifier</i></td> </tr>
 *  	<tr>    <td>2</td>                 <td>0: Value Field</td>     <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>1: Descr Field</td>     <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>2: LegendType</td>      <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>3: LegendClassType</td> <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>4: ColourRamp</td>      <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>5: Lower</td>           <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>6: Upper</td>           <td>102</td> </tr>
 *  	<tr>    <td>2</td>                 <td>7: Nodata</td>          <td>(n+1)*100+2</td> </tr>
 * 	</table>
 *
 * 	For each legend item, the tree level, layer (i.e. top level) tree row number and relative tree row number can
 * 	be retrieved from the \c QModelIndex as follows:
 *
 *  \code
 *
 *  // let's take the currentIndex for example
 *  QModelIndex idx = currentIndex();
 *
 *  // retrieving tree model information for the current index
 *  int top_level_row_number = (idx.internalId() / 100) - 1;
 *  int tree_level           = idx.internalId() % 100;
 *  int level_row_number     = idx.row();
 *
 *  \endcode
 *
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
