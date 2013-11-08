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
		//: QSortFilterProxyModel(parent), mSourceModel(0)
		: QAbstractProxyModel(parent), mSourceModel(0)
{
}

NMSelectableSortFilterProxyModel::~NMSelectableSortFilterProxyModel()
{
}


QModelIndex
NMSelectableSortFilterProxyModel::index(int row, int column, const QModelIndex& parent) const
{
	return this->createIndex(row, column);
}

QModelIndex
NMSelectableSortFilterProxyModel::parent(const QModelIndex& idx) const
{
	return QModelIndex();
}

int
NMSelectableSortFilterProxyModel::rowCount(const QModelIndex& parent) const
{
	if (mSourceModel == 0)
		return 0;

	return mSourceModel->rowCount(parent);
}

int
NMSelectableSortFilterProxyModel::columnCount(const QModelIndex& parent) const
{
	if (mSourceModel == 0)
		return 0;

	return mSourceModel->columnCount(parent);
}


void
NMSelectableSortFilterProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	if (sourceModel == 0 || sourceModel == mSourceModel)
		return;

	this->mSourceModel = sourceModel;
	mProxy2Source.clear();
	mSource2Proxy.clear();

	for (int i=0; i < mSourceModel->rowCount(QModelIndex()); ++i)
	{
		mProxy2Source << i;
		mSource2Proxy << i;
	}
}

Qt::ItemFlags NMSelectableSortFilterProxyModel::flags(
		const QModelIndex &index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return 0;
}

QModelIndex
NMSelectableSortFilterProxyModel::mapFromSource(const QModelIndex& srcIdx) const
{
	if (!srcIdx.isValid())
		return QModelIndex();

	int row = srcIdx.row();
	if (mSource2Proxy.size() == this->mSourceModel->rowCount(QModelIndex()))
	{
		row = mSource2Proxy[srcIdx.row()];
	}
	return this->index(row, srcIdx.column(), QModelIndex());
}

QModelIndex
NMSelectableSortFilterProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
	if (!proxyIdx.isValid())
		return QModelIndex();

	int row = proxyIdx.row();
	if (mProxy2Source.size() == this->mSourceModel->rowCount(QModelIndex()))
	{
		row = mProxy2Source[proxyIdx.row()];
	}
	return this->mSourceModel->index(row, proxyIdx.column(), QModelIndex());
}


QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionToSource(const QItemSelection &proxySelection) const
{
	return QItemSelection();
}


QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
	return QItemSelection();
}


void
NMSelectableSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
	NMDebugCtx(ctxSelSortFilter, << "...");

	if (this->mSourceModel == 0)
	{
		NMDebugCtx(ctxSelSortFilter, << "done!");
		return;
	}

	// get the type of column
	QModelIndex topcol = this->index(0, column, QModelIndex());
	if (!topcol.isValid())
	{
		NMWarn("ctxSelSortFilterProxy", << "Invalid column specified! Abort sorting!");
		NMDebugCtx("ctxSelSortFilterProxy", << "done!");
		return;
	}

	QVariant value = this->data(topcol, Qt::DisplayRole);
	if (!value.isValid())
	{
		NMWarn("ctxSelSortFilterProxy", << "Invalid data type specified! Abort sorting!");
		NMDebugCtx("ctxSelSortFilterProxy", << "done!");
		return;
	}

	QString colName = this->headerData(column,
			Qt::Horizontal, Qt::DisplayRole).toString();
	mSortOrder = order;
	QString sorder = order == Qt::AscendingOrder ? QString("asc") : QString("desc");
	NMDebugAI(<< "Sort " << colName.toStdString() << " "
			  << sorder.toStdString() << std::endl);

	qsort(0, this->mSourceModel->rowCount(QModelIndex()));

	for (int i=0; i < mProxy2Source.size(); ++i)
	{
		mSource2Proxy[mProxy2Source[i]] = i;
	}

	NMDebugCtx("ctxSelSortFilterProxy", << "done!");
}


