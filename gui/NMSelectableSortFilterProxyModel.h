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

//#include <QtGui/qsortfilterproxymodel.h>
#include <QtGui/qabstractproxymodel.h>

#include "nmlog.h"

#include <QItemSelection>

class NMSelectableSortFilterProxyModel : public QAbstractProxyModel//public QSortFilterProxyModel
{
	Q_OBJECT

public:
	NMSelectableSortFilterProxyModel(QObject *parent=0);
	~NMSelectableSortFilterProxyModel();

	Qt::ItemFlags flags(const QModelIndex &index) const;

	void setSourceModel(QAbstractItemModel* sourceModel)
			{this->mSourceModel = sourceModel;}
	QAbstractItemModel* sourceModel(void) const {return mSourceModel;}

	QModelIndex mapFromSource(const QModelIndex& srcIdx) const;
	QModelIndex mapToSource(const QModelIndex& proxyIdx) const;
	QItemSelection mapSelectionFromSource(const QItemSelection& sourceSelection) const;
	QItemSelection mapSelectionToSource(const QItemSelection& proxySelection) const;

	void sort(int column, Qt::SortOrder order);

protected:

	QAbstractItemModel* mSourceModel;
	QItemSelection* mSrcSelection;
	QItemSelection* mProxySelection;
	QVector<int> mSortedSource;

	// we only do row-based comparison
	void lessThan(int left, int right){};

	void qsort(int left, int right){};
};

#endif // ifndef
