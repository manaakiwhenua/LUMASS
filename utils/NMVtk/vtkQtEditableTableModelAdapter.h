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
 * vtkQtEditableTableModelAdapter.h
 *
 *  Created on: 29/10/2013
 *      Author: alex
 */

#ifndef VTKQTEDITABLETABLEMODELADAPTER_H_
#define VTKQTEDITABLETABLEMODELADAPTER_H_

#include "vtkQtTableModelAdapter.h"
#include "vtkTable.h"

class QVTK_EXPORT vtkQtEditableTableModelAdapter: public vtkQtTableModelAdapter
{
	Q_OBJECT

public:
	vtkQtEditableTableModelAdapter(QObject* parent=0);
	vtkQtEditableTableModelAdapter(vtkTable* table, QObject* parent=0);
	virtual ~vtkQtEditableTableModelAdapter();

	bool setHeaderData(int section, Qt::Orientation orientation,
			const QVariant& value, int role);
	bool insertColumns(int column, int count, const QModelIndex& parent);
	bool removeColumns(int column, int count, const QModelIndex& parent);


private:
	vtkQtEditableTableModelAdapter(const vtkQtEditableTableModelAdapter &);
	void operator=(const vtkQtEditableTableModelAdapter&);
};

#endif /* VTKQTEDITABLETABLEMODELADAPTER_H_ */
