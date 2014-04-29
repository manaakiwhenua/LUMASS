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
 * vtkQtEditableTableModelAdapter.cxx
 *
 *  Created on: 29/10/2013
 *      Author: alex
 */

#include "vtkQtEditableTableModelAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkStringArray.h"

#include <QVariant>

#include "nmlog.h"

vtkQtEditableTableModelAdapter::vtkQtEditableTableModelAdapter(QObject* parent)
    : NMVtkQtTableModelAdapter(parent)
{
}

vtkQtEditableTableModelAdapter::vtkQtEditableTableModelAdapter(vtkTable* table,
		QObject* parent)
	: vtkQtTableModelAdapter(table, parent)
{
}

vtkQtEditableTableModelAdapter::~vtkQtEditableTableModelAdapter()
{
}

Qt::ItemFlags
vtkQtEditableTableModelAdapter::flags(const QModelIndex &idx) const
{
	if (idx.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return 0;
}

bool
vtkQtEditableTableModelAdapter::setData(const QModelIndex& index,
		const QVariant& value,
		int role)
{
	if (	this->table() == 0
		||  !index.isValid())
	{
		return false;
	}

	if (	role == Qt::DisplayRole
		||	role == Qt::EditRole
	   )
	{
		vtkTable* tab = this->table();
		vtkAbstractArray* ar = tab->GetColumn(index.column());
		const int type = ar->GetDataType();
		bool bok;
		switch(type)
		{
			case VTK_BIT:
			case VTK_CHAR:
			case VTK_SIGNED_CHAR:
			case VTK_UNSIGNED_CHAR:
			case VTK_SHORT:
			case VTK_UNSIGNED_SHORT:
			case VTK_INT:
			case VTK_UNSIGNED_INT:
			case VTK_LONG:
			case VTK_UNSIGNED_LONG:
			case VTK_ID_TYPE:
			{
				long val = value.toInt(&bok);
				if (bok)
				{
					ar->SetVariantValue(index.row(), vtkVariant(val));
				}
				else
				{
					return false;
				}
			}
			break;

			case VTK_DOUBLE:
			{
				double val = value.toDouble(&bok);
				if (bok)
				{
					ar->SetVariantValue(index.row(), vtkVariant(val));
				}
				else
				{
					return false;
				}
			}
			break;

			default:
				ar->SetVariantValue(index.row(), vtkVariant(value.toString().toStdString().c_str()));
				break;
		}
		emit dataChanged(index, index);
		return true;
	}
	else if (role == Qt::DecorationRole)
	{
		return vtkQtTableModelAdapter::setData(index, value, role);
	}
	else
	{
		NMWarn("vtkQtEditableTableModelAdapter", << "can't edit this role " << role << std::endl);
		return false;
	}
}

QVariant
vtkQtEditableTableModelAdapter::data(const QModelIndex &idx, int role) const
{
	if (!idx.isValid())
		return QVariant();

	vtkTable* tab = vtkTable::SafeDownCast(this->GetVTKDataObject());
	const int type = tab->GetValue(idx.row(), idx.column()).GetType();

	switch(role)
	{
	case Qt::EditRole:
	case Qt::DisplayRole:
		{
			switch(type)
			{
			case VTK_ID_TYPE:
			case VTK_CHAR:
			case VTK_SIGNED_CHAR:
			case VTK_UNSIGNED_CHAR:
			case VTK_SHORT:
			case VTK_UNSIGNED_SHORT:
			case VTK_INT:
			case VTK_UNSIGNED_INT:
			case VTK_LONG:
			case VTK_UNSIGNED_LONG:
			case VTK_LONG_LONG:
				return QVariant((qlonglong)tab->GetValue(idx.row(), idx.column()).ToLongLong());
				break;

			case VTK_FLOAT:
			case VTK_DOUBLE:
				return tab->GetValue(idx.row(), idx.column()).ToDouble();
				break;

			default:
				return tab->GetValue(idx.row(), idx.column()).ToString().c_str();
				break;
			}
		}
		break;

	case Qt::TextAlignmentRole:
		{
			if (type == VTK_STRING)
			{
				return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
			}
			else
			{
				return QVariant(Qt::AlignRight | Qt::AlignVCenter);
			}
		}
		break;

	default:
		break;
	}

	return QVariant();
}


bool
vtkQtEditableTableModelAdapter::setHeaderData(int section, Qt::Orientation orientation,
			const QVariant& value, int role)
{
	if (	section < 0
		||  section > this->table()->GetNumberOfColumns()-1
		||  value.type() != QVariant::String
		||  role != Qt::DisplayRole
		)
		return false;

	// we don't care about the orientation, or the role, but interpret
	// the value as string and set the name of the associated column
	vtkSmartPointer<vtkTable> tab = this->table();

	tab->GetColumn(section)->SetName(value.toString().toStdString().c_str());

	emit headerDataChanged(Qt::Horizontal, section, section);

	return true;
}

bool
vtkQtEditableTableModelAdapter::insertColumns(int column, int count,
		const QModelIndex& parent)
{
	// we ever only append just one column at a time,
	// at the end of the table; so, we use the count
	// parameter to actually denote the column type
	// represented by a QVariant::Type

	QVariant::Type type = (QVariant::Type)count;
	if (type == 0)
		return false;

	vtkSmartPointer<vtkTable> tab = this->table();
	int ncols = tab->GetNumberOfColumns();
	vtkAbstractArray* nar = 0;

	// we don't support multi-component columns, which has
	// the advantage that we don't have to reset the model
	// but can do with some appropriate signals
	this->SetSplitMultiComponentColumns(false);

	beginInsertColumns(QModelIndex(), ncols, ncols);
	switch (type)
	{
	case QVariant::Int:
		nar = vtkAbstractArray::CreateArray(VTK_LONG);
		break;
	case QVariant::Double:
		nar = vtkAbstractArray::CreateArray(VTK_DOUBLE);
		break;
	case QVariant::String:
		nar = vtkAbstractArray::CreateArray(VTK_STRING);
		break;
	default:
		break;
	}
	nar->SetNumberOfComponents(1);
	nar->SetNumberOfTuples(tab->GetNumberOfRows());

	// to be on the safe side, we give a dummy name
	QString name = QString("Col_%1").arg(ncols+1);
	nar->SetName(name.toStdString().c_str());

	vtkDataArray* da = vtkDataArray::SafeDownCast(nar);
	if (da)
	{
		da->FillComponent(0, 0);
	}
	tab->AddColumn(nar);
	endInsertColumns();

	return true;
}

bool
vtkQtEditableTableModelAdapter::removeColumns(int column, int count,
		const QModelIndex& parent)
{
	// we just remove column and ignore the count parameter

	if (column < 0 || column > this->table()->GetNumberOfColumns()-1)
		return false;

	// we don't support multi-component columns, which has
	// the advantage that we don't have to reset the model
	// but can do with some appropriate signals
	this->SetSplitMultiComponentColumns(false);

	vtkSmartPointer<vtkTable> tab = this->table();
	beginRemoveColumns(parent, column, column);
	tab->RemoveColumn(column);
	endRemoveColumns();

	return true;
}

