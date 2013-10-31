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

vtkQtEditableTableModelAdapter::vtkQtEditableTableModelAdapter(QObject* parent)
	: vtkQtTableModelAdapter(parent)
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

bool
vtkQtEditableTableModelAdapter::setHeaderData(int section, Qt::Orientation orientation,
			const QVariant& value, int role)
{
	if (section < 0 || section > this->table()->GetNumberOfColumns()-1)
		return false;

	// we don't care about the orientation, or the role, but interpret
	// the value as string and set the name of the associated column
	vtkSmartPointer<vtkTable> tab = this->table();

	//this->beginResetModel();
	tab->GetColumn(section)->SetName(value.toString().toStdString().c_str());
	// setTable takes care about resetting the model
	this->setTable(tab);
	//this->endResetModel();

	return true;
}

bool
vtkQtEditableTableModelAdapter::insertColumns(int column, int count,
		const QModelIndex& parent)
{
	// we ever only append just one column at a time, so
	// just ignore column and count; we expect the
	// the parents internalPoniter to point to a
	// QVariant::Type

	QVariant::Type* type = (QVariant::Type*)parent.internalPointer();
	if (type == 0)
		return false;

	vtkAbstractArray* nar = 0;
	switch (*type)
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

	// to be on the safe side, we give a dummy name
	vtkSmartPointer<vtkTable> tab = this->table();
	QString name = QString("Col_%1").arg(tab->GetNumberOfColumns()+1);
	nar->SetName(name.toStdString().c_str());

	// setTable takes care about resetting the model
	//this->beginResetModel();
	// add the column to the table
	tab->AddColumn(nar);
	this->setTable(tab);
	//this->endResetModel();

	return true;
}

bool
vtkQtEditableTableModelAdapter::removeColumns(int column, int count,
		const QModelIndex& parent)
{
	// we just remove column and ignore the count parameter

	if (column < 0 || column > this->table()->GetNumberOfColumns()-1)
		return false;


	vtkSmartPointer<vtkTable> tab = this->table();
	//this->beginResetModel();
	tab->RemoveColumn(column);
	this->setTable(tab);
	//this->endResetModel();

	return true;
}

