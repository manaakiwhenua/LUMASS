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
 * NMQtOtbAttributeTableModel.cpp
 *
 *  Created on: 25/10/2013
 *      Author: alex
 */

#include "NMQtOtbAttributeTableModel.h"

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(QObject* parent)
{
	this->setParent(parent);
}

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(
		otb::AttributeTable::Pointer tab, QObject* parent)
{
	this->setParent(parent);
	if (tab.IsNotNull())
		this->mTable = tab;
}

NMQtOtbAttributeTableModel::~NMQtOtbAttributeTableModel()
{
}

int
NMQtOtbAttributeTableModel::rowCount(const QModelIndex& parent) const
{
	if (mTable.IsNull())
		return 0;

	return mTable->GetNumRows();
}

int
NMQtOtbAttributeTableModel::columnCount(const QModelIndex& parent) const
{
	if (mTable.IsNull())
		return 0;

	return mTable->GetNumCols();
}

QVariant
NMQtOtbAttributeTableModel::data(const QModelIndex& index, int role)
{
	if (mTable.IsNull())
		return QVariant();

	// we don't need to check indices here, 'cause the table is doing
	// that for us and returns nodata values in case we're out of bounds
	// (nodata is by default the default assigned value upon delcaration)

	TableColumnType type = mTable->GetColumnType(index.column());
	switch(type)
	{
	case ATTYPE_STRING:
		return QVariant(mTable->GetStrValue(index.column(), index.row()));
		break;
	case ATTYPE_INT:
		return QVariant(mTable->GetIntValue(index.column(), index.row()));
		break;
	case ATTYPE_DOUBLE:
		return QVariant(mTable->GetDblValue(index.column(), index.row()));
		break;
	default:
		return QVariant();
		break;
	}

	return QVariant();
}

QVariant headerData(int section, Qt::Orientation orientation,
		int role=Qt::DisplayRole) const;
bool setData(const QModelIndex& index, const QVariant& value,
		int role=Qt::EditRole);
Qt::ItemFlags flags(const QModelIndex& index) const;
bool insertColumn(int column, const QModelIndex& parent=QModelIndex());
bool removeColumn(int column, const QModelIndex& parent=QModelIndex());

// additional public interface to
void setTable(otb::AttributeTable::Pointer table);



