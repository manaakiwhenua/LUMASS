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
 * NMSelectableSortFilterProxyModel.cpp
 *
 *  Created on: 23/12/2011
 *      Author: alex
 */

#include "NMSelectableSortFilterProxyModel.h"

#include "nmlog.h"

NMSelectableSortFilterProxyModel::NMSelectableSortFilterProxyModel(
		QObject *parent)
		: QSortFilterProxyModel(parent)
{

}

NMSelectableSortFilterProxyModel::~NMSelectableSortFilterProxyModel()
{

}

Qt::ItemFlags NMSelectableSortFilterProxyModel::flags(
		const QModelIndex &index) const
{
	return QSortFilterProxyModel::flags(index) | Qt::ItemIsSelectable;
}

QItemSelection
NMSelectableSortFilterProxyModel::mapRowSelectionToSource(const QItemSelection &proxySelection) const
{
	QModelIndexList proxyIndexes = proxySelection.indexes();
    QItemSelection sourceSelection;
    for (int i = 0; i < proxyIndexes.size(); ++i)
    {
        const QModelIndex proxyIdx = mapToSource(proxyIndexes.at(i));
        if (!proxyIdx.isValid() || proxyIdx.column() > 0)
            continue;
        sourceSelection << QItemSelectionRange(proxyIdx);

        //sourceSelection.merge(QItemSelection(proxyIdx, proxyIdx),
        //		QItemSelectionModel::Select);
    }
    return sourceSelection;
}


QItemSelection
NMSelectableSortFilterProxyModel::mapRowSelectionFromSource(const QItemSelection &sourceSelection) const
{
    QModelIndexList sourceIndexes = sourceSelection.indexes();
    QItemSelection proxySelection;
    for (int i = 0; i < sourceIndexes.size(); ++i)
    {
        const QModelIndex srcIdx = mapFromSource(sourceIndexes.at(i));
        if (!srcIdx.isValid() || srcIdx.column() > 0)
            continue;
        proxySelection << QItemSelectionRange(srcIdx);
        //proxySelection.merge(QItemSelection(srcIdx, srcIdx),
        //		QItemSelectionModel::Select);
    }
    return proxySelection;
}


//void
//NMSelectableSortFilterProxyModel::sort(int column, Qt::SortOrder order)
//{
//	NMDebugCtx("ctxSelSortFilterProxy", << "...");
//
//
//	NMDebugAI(<< "about to filter column #" << column << std::endl);
//	QSortFilterProxyModel::sort(column, order);
//
//	NMDebugCtx("ctxSelSortFilterProxy", << "done!");
//}


