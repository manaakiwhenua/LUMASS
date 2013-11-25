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
#include "nmlog.h"

#include <QColor>

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(QObject* parent)
	: mKeyIdx(0)
{
	this->setParent(parent);
}

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(
		otb::AttributeTable::Pointer tab, QObject* parent)
	: mKeyIdx(-1)
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

void NMQtOtbAttributeTableModel::setKeyColumn(const QString& keyColumn)
{
	int colidx = this->mTable->ColumnExists(keyColumn.toStdString());
	if (colidx < 0)
		return;

	this->mKeyIdx = colidx;
}

void
NMQtOtbAttributeTableModel::setKeyColumn(int keyidx)
{
	if (keyidx < 0 || keyidx > this->mTable->GetNumCols()-1)
		return;

	this->mKeyIdx = keyidx;
}

QVariant
NMQtOtbAttributeTableModel::data(const QModelIndex& index, int role) const
{
	if ( mTable.IsNull() || !index.isValid() )
	{
		return QVariant();
	}

	// everything we do, depends on the column  type
	otb::AttributeTable::TableColumnType type = mTable->GetColumnType(index.column());
	switch(role)
	{
		case Qt::EditRole:
		case Qt::DisplayRole:
		{
			switch(type)
			{
			case otb::AttributeTable::ATTYPE_STRING:
				return QVariant(mTable->GetStrValue(index.column(), index.row()).c_str());
				break;
			case otb::AttributeTable::ATTYPE_INT:
				return QVariant((qlonglong)mTable->GetIntValue(index.column(), index.row()));
						//QString("%1")
						//.arg(mTable->GetIntValue(index.column(), index.row())));
				break;
			case otb::AttributeTable::ATTYPE_DOUBLE:
				return QVariant(mTable->GetDblValue(index.column(), index.row()));
						//QString("%L1")
						//.arg(mTable->GetDblValue(index.column(), index.row()), 0, 'G', 4));
				break;
			default:
				return QVariant();
				break;
			}
		}
		break;

		case Qt::TextAlignmentRole:
		{
			switch(type)
			{
			case otb::AttributeTable::ATTYPE_INT:
			case otb::AttributeTable::ATTYPE_DOUBLE:
				return QVariant(Qt::AlignRight | Qt::AlignVCenter);
				break;
			default:
				return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
				break;
			}
		}
		break;

		case Qt::ForegroundRole:
			{
				return QVariant(QColor(0,0,0));
			}
			break;



		default:
			break;
	}
	return QVariant();
}

QVariant
NMQtOtbAttributeTableModel::headerData(int section,
		Qt::Orientation orientation,
		int role) const
{
	if (mTable.IsNull() || role != Qt::DisplayRole)
		return QVariant();

	switch(orientation)
	{
	case Qt::Horizontal:
		return QVariant(mTable->GetColumnName(section).c_str());
		break;
	case Qt::Vertical:
		return QVariant(mTable->GetStrValue(mKeyIdx, section).c_str());
	default:
		break;
	}

	return QVariant();
}


bool
NMQtOtbAttributeTableModel::setData(const QModelIndex& index,
		const QVariant& value,
		int role)
{
	if (	mTable.IsNull()
		||  !index.isValid())
	{
		return false;
	}

	if (	role == Qt::DisplayRole
		||  role == Qt::EditRole
	   )
	{
		bool bok = true;
		switch(value.type())
		{
		case QVariant::Char:
		case QVariant::Int:
			mTable->SetValue(index.column(), index.row(), (long)value.toInt(&bok));
			break;

		case QVariant::Double:
			mTable->SetValue(index.column(), index.row(), value.toDouble(&bok));
			break;

		case QVariant::Date:
		case QVariant::DateTime:
		case QVariant::Time:
		case QVariant::Url:
		case QVariant::String:
			mTable->SetValue(index.column(), index.row(), value.toString().toStdString());
			break;

		default:
			return false;
			break;
		}
		emit dataChanged(index, index);
		return true;
	}

	return false;
}


Qt::ItemFlags
NMQtOtbAttributeTableModel::flags(const QModelIndex& index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return 0;
}


void
NMQtOtbAttributeTableModel::setTable(otb::AttributeTable::Pointer table)
{
	this->mTable = table;

	//this->beginResetModel();
	emit this->reset();
	//this->endResetModel();
}

bool
NMQtOtbAttributeTableModel::setHeaderData(int section, Qt::Orientation orientation,
		const QVariant& value, int role)
{
	// we don't care about the orientation, or the role, but interpret
	// the value as string and set the name of the associated column

	if (	section < 0
		||  section > this->mTable->GetNumCols()-1
		||  value.type() != QVariant::String
		||  role != Qt::DisplayRole
	   )
		return false;

	this->mTable->SetColumnName(section, value.toString().toStdString());

	emit headerDataChanged(Qt::Horizontal, section, section);

	return true;
}

bool
NMQtOtbAttributeTableModel::insertColumns(int column, int count,
		const QModelIndex& parent)
{
	// we ever only append just one column at a time,
	// at the end of the table; so, we use the count
	// parameter to actually denote the column type
	// represented by a QVariant::Type

	QVariant::Type type = (QVariant::Type)count;
	if (type == 0)
	{
		NMErr("NMQtOtbAttributeTableModel", << "Invalid column type!");
		NMDebugCtx("NMQtOtbAttributeTableModel", << "done!");
		return false;
	}

	int ncols = this->mTable->GetNumCols();
	QString name = QString("Col_%1").arg(ncols+1);

	beginInsertColumns(QModelIndex(), ncols, ncols);
	switch (type)
	{
	case QVariant::Int:
		this->mTable->AddColumn(name.toStdString(), otb::AttributeTable::ATTYPE_INT);
		break;
	case QVariant::Double:
		this->mTable->AddColumn(name.toStdString(), otb::AttributeTable::ATTYPE_DOUBLE);
		break;
	case QVariant::String:
		this->mTable->AddColumn(name.toStdString(), otb::AttributeTable::ATTYPE_STRING);
		break;
	default:
		break;
	}
	//emit this->reset();
	endInsertColumns();

	return true;
}

bool
NMQtOtbAttributeTableModel::removeColumns(int column, int count,
		const QModelIndex& parent)
{
	if (column < 0 || column > this->mTable->GetNumCols()-1)
		return false;

	beginRemoveColumns(parent, column, column);
	int ret = this->mTable->RemoveColumn(column);
	if (ret)
	{
		//emit this->reset();
		endRemoveColumns();
	}
	else
	{
		return false;
	}

	return true;
}





