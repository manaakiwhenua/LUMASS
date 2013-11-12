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
#include "nmlog.h"
#include "NMSelectableSortFilterProxyModel.h"

#include <QItemSelectionRange>

NMSelectableSortFilterProxyModel::NMSelectableSortFilterProxyModel(
		QObject *parent)
		: QAbstractProxyModel(parent),
		  mSourceModel(0),
		  mSortOrder(Qt::AscendingOrder),
		  mSortColumn(0),
		  mNumHidden(0),
		  mbFilterOn(false)
{
}

NMSelectableSortFilterProxyModel::~NMSelectableSortFilterProxyModel()
{
}

QModelIndex
NMSelectableSortFilterProxyModel::index(int row, int column, const QModelIndex& parent) const
{
	if (	row < 0 || row > this->rowCount(QModelIndex())-1
		||	column < 0 || column > this->mSourceModel->columnCount(QModelIndex())-1)
		return QModelIndex();

	return this->createIndex(row, column, 0);
}

QModelIndex
NMSelectableSortFilterProxyModel::parent(const QModelIndex& idx) const
{
	return QModelIndex();
}

void
NMSelectableSortFilterProxyModel::setFilterOn(bool yesno)
{
	if (mbFilterOn == yesno)
		return;

	this->mbFilterOn = yesno;
	emit this->reset();
}

int
NMSelectableSortFilterProxyModel::sourceRowCount(void)
{
	if (mSourceModel == 0)
		return 0;

	return this->mSource2Proxy.size();
}

int
NMSelectableSortFilterProxyModel::rowCount(const QModelIndex& parent) const
{
	if (mSourceModel == 0)
		return 0;

	if (mbFilterOn)
		return mFilter2Proxy.size();
	else
		return mProxy2Source.size();
}

int
NMSelectableSortFilterProxyModel::columnCount(const QModelIndex& parent) const
{
	if (mSourceModel == 0)
		return 0;

	return mSourceModel->columnCount(parent);
}

QVariant
NMSelectableSortFilterProxyModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
	if (mSourceModel == 0)
		return QVariant();

	if (orientation == Qt::Vertical && role == Qt::DisplayRole)
		return section;

	return this->mSourceModel->headerData(section, orientation, role);
}

QVariant
NMSelectableSortFilterProxyModel::data(const QModelIndex& index, int role) const
{
	if (mSourceModel == 0)
		return QVariant();

	return this->mSourceModel->data(mapToRaw(index), role);
}

QModelIndex
NMSelectableSortFilterProxyModel::mapToRaw(const QModelIndex& idx) const
{
	if (idx.column() < 0 || idx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid column: " << idx.column());
		return QModelIndex();
	}

	int nrows = this->rowCount();
	int lookuprow = idx.row();
	if (lookuprow < 0 || lookuprow > nrows-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid row: " << idx.row());
		return QModelIndex();
	}

	if (mFilter2Proxy.size() > lookuprow)
		lookuprow = mFilter2Proxy.at(lookuprow);

	return this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(lookuprow)),
			idx.column(), QModelIndex());
}

bool
NMSelectableSortFilterProxyModel::setData(const QModelIndex& index,
		const QVariant& value,
		int role)
{
	if (mSourceModel == 0)
		return false;

	if (this->mSourceModel->setData(mapToRaw(index), value, role))
	{
		emit dataChanged(index, index);
		return true;
	}
	else
		return false;
}

void
NMSelectableSortFilterProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	if (sourceModel == 0 || sourceModel == mSourceModel)
		return;

	emit beginResetModel();
	if (mSourceModel != 0)
	{
		disconnect(mSourceModel, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
	}

	this->mSourceModel = sourceModel;
	const int nrows = mSourceModel->rowCount(QModelIndex());
	mProxy2Source.clear();  mProxy2Source.reserve(nrows);
	mSource2Proxy.clear();  mSource2Proxy.reserve(nrows);
	mHiddenSource.clear();  mHiddenSource.reserve(nrows);
	mFilter2Proxy.clear();

	for (int i=0; i < nrows; ++i)
	{
		mProxy2Source << i;
		mSource2Proxy << i;
		mSource2Raw << i;
		mHiddenSource << false;
	}
	mNumHidden = 0;

	connect(mSourceModel, SIGNAL(modelReset()), this, SIGNAL(modelReset()));

	this->reset();
	emit endResetModel();
}

void
NMSelectableSortFilterProxyModel::clearFilter(void)
{
	if (mFilter2Proxy.size() > 0)
	{
		mFilter2Proxy.clear();
		if (this->mbFilterOn)
		{
			emit this->reset();
		}
	}
}

void
NMSelectableSortFilterProxyModel::hideSource(const QList<int>& rows)
{
	if (this->mSourceModel == 0)
		return;

	const int nrows = rows.size();
	for (int i=0; i < nrows; ++i)
	{
		if (rows.at(i) < 0 || rows.at(i) > mHiddenSource.size()-1)
			continue;

		if (!mHiddenSource.at(rows.at(i)))
		{
			mHiddenSource[rows.at(i)] = true;
			++mNumHidden;
		}
	}
	resetMapping();

	emit this->reset();
}

void
NMSelectableSortFilterProxyModel::addToFilter(const QItemSelection& proxySel)
{
	if (this->mSourceModel == 0 || proxySel.count() == 0)
		return;

	foreach(const QItemSelectionRange& range, proxySel)
	{
		if (range.top() < 0 || range.bottom() > this->mProxy2Source.size()-1)
			continue;

		for (int i=range.top(); i <= range.bottom(); ++i)
		{
			if (i > mFilter2Proxy.size()-1)
			{
				mFilter2Proxy << i;
			}
			else
			{
				if (mFilter2Proxy.at(i) != i)
				{
					mFilter2Proxy.insert(i, i);
				}
			}
		}
	}

	emit this->reset();
}


void
NMSelectableSortFilterProxyModel::resetMapping(void)
{
	const int nrows = mHiddenSource.size();
	// we've got to clear everything here, since it might all change
	  mSource2Raw.clear();    mSource2Raw.reserve(nrows);
	mProxy2Source.clear();  mProxy2Source.reserve(nrows);
	mSource2Proxy.clear();  mSource2Proxy.reserve(nrows);
	mFilter2Proxy.clear();

	for (int i=0, row=0; i < nrows; ++i)
	{
		if (mHiddenSource[i])
			continue;

		mSource2Raw << i;
		mProxy2Source << row;
		mSource2Proxy << row;
		++row;
	}
}

void
NMSelectableSortFilterProxyModel::showSource(const QList<int>& rows)
{
	if (this->mSourceModel == 0)
		return;

	const int nrows = rows.size();

	//emit beginResetModel();
	for (int i=0; i < nrows; ++i)
	{
		if (rows.at(i) < 0 || rows.at(i) > mHiddenSource.size()-1)
			continue;

		if (mHiddenSource.at(rows.at(i)))
		{
			mHiddenSource[rows.at(i)] = false;
			--mNumHidden;
		}
	}

	resetMapping();
	emit this->reset();
}

void
NMSelectableSortFilterProxyModel::removeFromFilter(const QItemSelection& srcSel)
{
	if (this->mSourceModel == 0)
		return;

	foreach(const QItemSelectionRange& range, srcSel)
	{
		if (range.top() < 0 || range.bottom() > this->mProxy2Source.size()-1)
			continue;

		for (int i=range.top(); i <= range.bottom(); ++i)
		{
			mFilter2Proxy.removeOne(i);
		}
	}

	emit this->reset();
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
	if (srcIdx.column() < 0 || srcIdx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid column: " << srcIdx.column());
		return QModelIndex();
	}

	if (srcIdx.row() < 0 || srcIdx.row() > this->mSource2Proxy.size()-1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid row: " << srcIdx.row());
		return QModelIndex();
	}

	int row = mSource2Proxy.at(srcIdx.row());
	if (mbFilterOn)
		row = mFilter2Proxy.indexOf(row);

	return this->index(row, srcIdx.column(), QModelIndex());
}

QModelIndex
NMSelectableSortFilterProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
	if (proxyIdx.column() < 0 || proxyIdx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid column: " << proxyIdx.column() << std::endl);
		return QModelIndex();
	}

	int nrows = this->rowCount();
	int lookuprow = proxyIdx.row();
	if (lookuprow < 0 || lookuprow > nrows-1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid row: " << lookuprow << std::endl);
		return QModelIndex();
	}

	if (mFilter2Proxy.size() > lookuprow)
		lookuprow = mFilter2Proxy.at(lookuprow);

	return this->mSourceModel->index(mProxy2Source.at(lookuprow), proxyIdx.column(), QModelIndex());
}

QItemSelection
NMSelectableSortFilterProxyModel::getSourceSelection(void)
{
	QItemSelection srcSel;
	if (mSourceModel == 0)
		return srcSel;

	int start = -1;
	int end = -1;
	for (int i=0; i < this->mHiddenSource.size(); ++i)
	{
		// inside a selection
		if (!mHiddenSource.at(i))
		{
			// if we haven't got a selection range, we start a new one
			if (start == -1)
			{
				start = i;
				end = -1;
				continue;
			}
			// if we've got already a selection range, we extend its end
			else if (start != -1)
			{
				end = i;
				// in case this is the last row, we complete the selection range
				// and add it to the ItemSelection
				if (i == this->mHiddenSource.size()-1)
				{
					QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
					QModelIndex eidx = this->mSourceModel->index(end, 0, QModelIndex());
					srcSel.select(sidx, eidx);
				}
				continue;
			}
		}
		// we're currently not in a selection
		else
		{
			// if we've got a valid end index, we
			// complete the selection range and add it to the
			// ItemSelection
			if (end != -1)
			{
				QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
				QModelIndex eidx = this->mSourceModel->index(end, 0, QModelIndex());
				srcSel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denote just by the start index
			else
			{
				QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
				srcSel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}
	return srcSel;
}

QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionToSource(const QItemSelection& proxySelection) const
{
	QItemSelection srcSel;
	if (mSourceModel == 0 || proxySelection.count() == 0)
		return srcSel;

	const int nrows = mSource2Proxy.size();
	int start = -1;
	int end = -1;
	for (int i=0; i < nrows; ++i)
	{
		const QModelIndex idx = this->createIndex(mSource2Proxy.at(i), 0, 0);

		// inside a selection
		if (proxySelection.contains(idx))
		{
			// if we haven't got a selection range, we start a new one
			if (start == -1)
			{
				start = i;
				end = -1;
				continue;
			}
			// if we've got already a selection range, we extend its end
			else if (start != -1)
			{
				end = i;
				// in case this is the last row, we complete the selection range
				// and add it to the ItemSelection
				if (i == nrows-1)
				{
					QModelIndex sidx = this->createIndex(start, 0, 0);
					QModelIndex eidx = this->createIndex(end, 0, 0);
					srcSel.select(sidx, eidx);
				}
				continue;
			}
		}
		// we're currently not in a selection
		else
		{
			// if we've got a valid end index, we
			// complete the selection range and add it to the
			// ItemSelection
			if (end != -1)
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				QModelIndex eidx = this->createIndex(end, 0, 0);
				srcSel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denote just by the start index
			else
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				srcSel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}
	return srcSel;
}


QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
	//NMDebugCtx(ctxSelSortFilter, << "...");

	QItemSelection proxySel;
	if (mSourceModel == 0 || sourceSelection.count() == 0)
		return proxySel;

	const int nrows = this->mProxy2Source.size();
	int start = -1;
	int end = -1;
	for (int i=0; i < nrows; ++i)
	{
		const QModelIndex idx = this->createIndex(mProxy2Source.at(i), 0, 0);
		//const QModelIndex idx = this->mapToSource(_idx);

		//NMDebugAI(<< "proxy-id: " << i << " source-id: " << idx.row() << std::endl);

		// inside a selection
		if (sourceSelection.contains(idx))
		{
			//NMDebugAI(<< "detected selection at: proxy=" << i << " source=" << idx.row() << std::endl);

			// if we haven't got a selection range started yet, we do it now
			if (start == -1)
			{
				start = i;
				end = -1;
				continue;
			}
			// if we've got already a selection range, we extend its end
			else if (start != -1)
			{
				end = i;
				// in case this is the last row, we complete the selection range
				// and add it to the ItemSelection
				if (i == nrows-1)
				{
					QModelIndex sidx = this->createIndex(start, 0, 0);
					QModelIndex eidx = this->createIndex(end, 0, 0);
					proxySel.select(sidx, eidx);
				}
				continue;
			}
		}
		// we're currently not in a selection
		else
		{
			// if we've got a valid end index, we
			// complete the selection range and add it to the
			// ItemSelection
			if (end != -1)
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				QModelIndex eidx = this->createIndex(end, 0, 0);
				proxySel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denoted just by the start index
			else
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				proxySel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}

	// we store the selection for the show only selected rows filter
	//mProxySelection = proxySel.indexes();

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	return proxySel;
}

QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionFromSource(const QModelIndexList& sourceList) const
{
	QItemSelection proxySel;
	if (mSourceModel == 0 || sourceList.count() == 0)
		return proxySel;

	const int nrows = this->mProxy2Source.size();
	int start = -1;
	int end = -1;
	for (int i=0; i < nrows; ++i)
	{
		const QModelIndex idx = this->createIndex(mProxy2Source.at(i), 0, 0);
		//const QModelIndex idx = this->mapToSource(_idx);

		//NMDebugAI(<< "proxy-id: " << i << " source-id: " << idx.row() << std::endl);

		// inside a selection
		if (sourceList.contains(idx))
		{
			//NMDebugAI(<< "detected selection at: proxy=" << i << " source=" << idx.row() << std::endl);

			// if we haven't got a selection range started yet, we do it now
			if (start == -1)
			{
				start = i;
				end = -1;
				continue;
			}
			// if we've got already a selection range, we extend its end
			else if (start != -1)
			{
				end = i;
				// in case this is the last row, we complete the selection range
				// and add it to the ItemSelection
				if (i == nrows-1)
				{
					QModelIndex sidx = this->createIndex(start, 0, 0);
					QModelIndex eidx = this->createIndex(end, 0, 0);
					proxySel.select(sidx, eidx);
				}
				continue;
			}
		}
		// we're currently not in a selection
		else
		{
			// if we've got a valid end index, we
			// complete the selection range and add it to the
			// ItemSelection
			if (end != -1)
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				QModelIndex eidx = this->createIndex(end, 0, 0);
				proxySel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denoted just by the start index
			else
			{
				QModelIndex sidx = this->createIndex(start, 0, 0);
				proxySel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}

	return proxySel;
}

QItemSelection
NMSelectableSortFilterProxyModel::getSourceSelectionFromSourceList(const QModelIndexList& sourceList) const
{
	QItemSelection srcSel;
	if (mSourceModel == 0 || sourceList.count() == 0)
		return srcSel;

	const int nrows = this->mSource2Proxy.size();
	int start = -1;
	int end = -1;
	for (int i=0; i < nrows; ++i)
	{
		const QModelIndex idx = this->mSourceModel->index(i, 0, QModelIndex());

		// inside a selection
		if (sourceList.contains(idx))
		{
			// if we haven't got a selection range, we start a new one
			if (start == -1)
			{
				start = i;
				end = -1;
				continue;
			}
			// if we've got already a selection range, we extend its end
			else if (start != -1)
			{
				end = i;
				// in case this is the last row, we complete the selection range
				// and add it to the ItemSelection
				if (i == nrows-1)
				{
					QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
					QModelIndex eidx = this->mSourceModel->index(end, 0, QModelIndex());
					srcSel.select(sidx, eidx);
				}
				continue;
			}
		}
		// we're currently not in a selection
		else
		{
			// if we've got a valid end index, we
			// complete the selection range and add it to the
			// ItemSelection
			if (end != -1)
			{
				QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
				QModelIndex eidx = this->mSourceModel->index(end, 0, QModelIndex());
				srcSel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denote just by the start index
			else
			{
				QModelIndex sidx = this->mSourceModel->index(start, 0, QModelIndex());
				srcSel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}
	return srcSel;
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

	// if we've done this sorting recently, we don't do it again
	if (column == mSortColumn && order == mSortOrder)
		return;

	// check column
	QModelIndex topcol = this->mSourceModel->index(mProxy2Source.at(0), column, QModelIndex());
	if (!topcol.isValid())
	{
		NMWarn("ctxSelSortFilterProxy", << "Invalid column specified! Abort sorting!");
		NMDebugCtx("ctxSelSortFilterProxy", << "done!");
		return;
	}

	QVariant value = this->mSourceModel->data(topcol, Qt::DisplayRole);
	if (!value.isValid())
	{
		NMWarn("ctxSelSortFilterProxy", << "Invalid data type specified! Abort sorting!");
		NMDebugCtx("ctxSelSortFilterProxy", << "done!");
		return;
	}

	mSortOrder = order;
	mSortColumn = column;

	QString colName = this->headerData(column,
			Qt::Horizontal, Qt::DisplayRole).toString();

	QString sorder = order == Qt::AscendingOrder ? QString("asc") : QString("desc");
	NMDebugAI(<< "Sort " << colName.toStdString() << " "
			  << sorder.toStdString() << std::endl);

	emit layoutAboutToBeChanged();

	// note: for now sort on the unhidden source array rather than
	// just on the selection
	const int psize = mProxy2Source.size();
	qsort(0, psize-1);

	// ToDo::
	// this might go somewhere else and we only generate mSource2Proxy
	// when we really need it
	for (int i=0; i < psize; ++i)
	{
		mSource2Proxy[mProxy2Source.at(i)] = i;
	}
	emit layoutChanged();


	NMDebugCtx(ctxSelSortFilter, << "done!");
}


