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
 * NMSelectableSortFilterProxyModel.h
 *
 *  Created on: 23/12/2011
 *      Author: alex
 */

#ifndef NMSELECTABLESORTFILTERPROXYMODEL_H_
#define NMSELECTABLESORTFILTERPROXYMODEL_H_
#define ctxSelSortFilter "NMSelectableSortFilterProxyModel"

#include <QtGui/qsortfilterproxymodel.h>

#include "nmlog.h"

class NMSelectableSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	NMSelectableSortFilterProxyModel(QObject *parent=0);
	~NMSelectableSortFilterProxyModel();

	Qt::ItemFlags flags(const QModelIndex &index) const;

	void sort(int column, Qt::SortOrder order);

};

#endif // ifndef
