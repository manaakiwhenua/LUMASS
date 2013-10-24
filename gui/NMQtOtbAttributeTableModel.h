/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMQtOtbAttributeTableModel.h
 *
 *  Created on: 25/10/2013
 *      Author: alex
 */

#ifndef NMQTOTBATTRIBUTETABLEMODEL_H_
#define NMQTOTBATTRIBUTETABLEMODEL_H_

#define ctxQtOtbTabModel "NMQtOtbAttributeTableModel"
#include "nmlog.h"
#include "otbAttributeTable.h"

#include <qabstractitemmodel.h>

class NMQtOtbAttributeTableModel: public QAbstractTableModel
{
public:
	NMQtOtbAttributeTableModel(QObject* parent=0);
	NMQtOtbAttributeTableModel(otb::AttributeTable::Pointer tab,
			QObject* parent=0);
	virtual ~NMQtOtbAttributeTableModel();

	// (re-)implemented from QAbstractTableModel
	int rowCount(const QModelIndex& parent=QModelIndex()) const;
	int columnCount(const QModelIndex& parent=QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role=Qt::DisplayRole);
	QVariant headerData(int section, Qt::Orientation orientation,
			int role=Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value,
			int role=Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool insertColumn(int column, const QModelIndex& parent=QModelIndex());
	bool removeColumn(int column, const QModelIndex& parent=QModelIndex());

	// additional public interface to
	void setTable(otb::AttributeTable::Pointer table);
	otb::AttributeTable::Pointer getTable(void)
		{return this->mTable;}

private:
	otb::AttributeTable::Pointer mTable;

};

#endif /* NMQTOTBATTRIBUTETABLEMODEL_H_ */
