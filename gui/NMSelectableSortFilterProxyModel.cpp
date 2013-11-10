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
		  mNumHidden(0)
{
}

NMSelectableSortFilterProxyModel::~NMSelectableSortFilterProxyModel()
{
}

QModelIndex
NMSelectableSortFilterProxyModel::index(int row, int column, const QModelIndex& parent) const
{
	if (	row < 0 || row > mProxy2Source.size()-1
		||	column < 0 || column > this->mSourceModel->columnCount(QModelIndex())-1)
		return QModelIndex();

	return this->createIndex(row, column, 0);
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

	//int sect = section;
	if (orientation == Qt::Vertical && role == Qt::DisplayRole)
	{
		//sect = mSource2Raw[mProxy2Source[section]];
		return section;
	}

	return this->mSourceModel->headerData(section, orientation, role);
}

QVariant
NMSelectableSortFilterProxyModel::data(const QModelIndex& index, int role) const
{
	if (mSourceModel == 0)
		return QVariant();

	//QModelIndex rawIdx = mapToRaw(index);
	//NMDebugAI(<< "fetching data from " << rawIdx.row()<< std::endl);

	return this->mSourceModel->data(mapToRaw(index), role);
}

QModelIndex
NMSelectableSortFilterProxyModel::mapToRaw(const QModelIndex& idx) const
{
	//NMDebugCtx(ctxSelSortFilter, << "...");
	if (idx.column() < 0 || idx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid column: " << idx.column());
		//NMDebugCtx(ctxSelSortFilter, << "done!");
		return QModelIndex();
	}

	if (idx.row() < 0 || idx.row() > mProxy2Source.size()-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid row! Abort!");
		//NMDebugCtx(ctxSelSortFilter, << "done!");
		return QModelIndex();
	}

	//NMDebugAI(<< "mapping: proxy -> source -> raw = " << idx.row()
	//		  << " -> " << mProxy2Source[idx.row()] << " -> "
	//		  << mSource2Raw[mProxy2Source[idx.row()]] << std::endl);

	//NMDebugCtx(ctxSelSortFilter, << "...");
	return this->mSourceModel->index(mSource2Raw[mProxy2Source[idx.row()]],
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
	mProxy2Source.clear();
	mSource2Proxy.clear();
	mHiddenSource.clear();

	for (int i=0; i < mSourceModel->rowCount(QModelIndex()); ++i)
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

//void
//NMSelectableSortFilterProxyModel::hideSource(int sourceRow)
//{
//	if (	this->mSourceModel == 0
//		||  sourceRow < 0
//		||  sourceRow >= this->mHiddenSource.size())
//		return;
//
//	if (mHiddenSource(sourceRow))
//		return;
//
//	mHiddenSource[sourceRow] = true;
//	++mNumHidden;
//
//
//
//	emit this->reset();
//}

void
NMSelectableSortFilterProxyModel::hideSource(const QList<int>& rows)
{
	if (this->mSourceModel == 0)
		return;

	for (int i=0; i < rows.size(); ++i)
	{
		if (rows[i] < 0 || rows[i] > mHiddenSource.size()-1)
			continue;

		if (!mHiddenSource[rows[i]])
		{
			mHiddenSource[rows[i]] = true;
			++mNumHidden;
		}
	}
	resetMapping();

	emit this->reset();

}

void
NMSelectableSortFilterProxyModel::resetMapping(void)
{
	mSource2Raw.clear();
	mProxy2Source.clear();
	mSource2Proxy.clear();

	for (int i=0, row=0; i < mHiddenSource.size(); ++i)
	{
		if (mHiddenSource[i])
			continue;

		mSource2Raw << i;
		mProxy2Source << row;
		mSource2Proxy << row;
		++row;
	}

	// first rows of mapping
	//for (int r=0; r < 150; ++r)
	//{
	//	NMDebugAI(<< "p2s s2p s2r " << mProxy2Source[r] << " " <<
	//			mSource2Proxy[r] << " " << mSource2Raw[r] << std::endl);
	//}
}

void
NMSelectableSortFilterProxyModel::showSource(const QList<int>& rows)
{
	if (this->mSourceModel == 0)
		return;

	//emit beginResetModel();
	for (int i=0; i < rows.size(); ++i)
	{
		if (rows[i] < 0 || rows[i] > mHiddenSource.size()-1)
			continue;

		if (mHiddenSource[rows[i]])
		{
			mHiddenSource[rows[i]] = false;
			--mNumHidden;
		}
	}

	resetMapping();
	emit this->reset();
	//emit endResetModel();
	//this->reset();
}

//void
//NMSelectableSortFilterProxyModel::showSource(int sourceRow)
//{
//	if (	this->mSourceModel == 0
//		||  sourceRow < 0
//		||  sourceRow >= this->mHiddenSource.size())
//		return;
//
//	if (!mHiddenSource(sourceRow))
//		return;
//
//	mHiddenSource[sourceRow] = false;
//	--mNumHidden;
//
//	emit this->reset();
//}


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
	//NMDebugCtx(ctxSelSortFilter, << "...");
	if (srcIdx.column() < 0 || srcIdx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid column: " << srcIdx.column());
		return QModelIndex();
	}

	if (srcIdx.row() < 0 || srcIdx.row() > this->mSource2Proxy.size()-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid row: " << srcIdx.row());
		return QModelIndex();
	}


	//NMDebugAI(<< "mapping: source -> proxy: " << srcIdx.row() << " -> "
	//			<< mSource2Proxy[srcIdx.row()] << std::endl);

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	return this->index(mSource2Proxy[srcIdx.row()], srcIdx.column(), QModelIndex());
}

QModelIndex
NMSelectableSortFilterProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
	//NMDebugCtx(ctxSelSortFilter, << "...");
	if (proxyIdx.column() < 0 || proxyIdx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		NMDebugAI(<< __FUNCTION__ << ": Invalid column: " << proxyIdx.column());
		//NMDebugCtx(ctxSelSortFilter, << "done!");
		return QModelIndex();
	}

	if (proxyIdx.row() < 0 || proxyIdx.row() > this->mProxy2Source.size()-1)
	{
		NMErr(ctxSelSortFilter, << "Invalid row: " << proxyIdx.row());
		//NMDebugCtx(ctxSelSortFilter, << "done!");

		return QModelIndex();
	}

	//NMDebugAI(<< "mapping: proxy -> source: " << proxyIdx.row() << " -> "
	//			<< mProxy2Source[proxyIdx.row()] << std::endl);

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	return this->mSourceModel->index(mProxy2Source[proxyIdx.row()], proxyIdx.column(), QModelIndex());
}


QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionToSource(const QItemSelection& proxySelection) const
{
	QItemSelection srcSel;
	if (mSourceModel == 0)
		return srcSel;

	int start = -1;
	int end = -1;
	for (int i=0; i < this->mSource2Proxy.size(); ++i)
	{
		const QModelIndex idx = this->index(mSource2Proxy[i], 0, QModelIndex());
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
				if (i == this->mSource2Proxy.size()-1)
				{
					QModelIndex sidx = this->index(start, 0, QModelIndex());
					QModelIndex eidx = this->index(end, 0, QModelIndex());
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
				QModelIndex sidx = this->index(start, 0, QModelIndex());
				QModelIndex eidx = this->index(end, 0, QModelIndex());
				srcSel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denote just by the start index
			else
			{
				QModelIndex sidx = this->index(start, 0, QModelIndex());
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
	NMDebugCtx(ctxSelSortFilter, << "...");

	QItemSelection proxySel;
	if (mSourceModel == 0)
		return proxySel;

	int start = -1;
	int end = -1;
	for (int i=0; i < this->mProxy2Source.size(); ++i)
	{
		const QModelIndex idx = this->index(mProxy2Source[i], 0, QModelIndex());

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
				if (i == this->mProxy2Source.size()-1)
				{
					QModelIndex sidx = this->index(start, 0, QModelIndex());
					QModelIndex eidx = this->index(end, 0, QModelIndex());
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
				QModelIndex sidx = this->index(start, 0, QModelIndex());
				QModelIndex eidx = this->index(end, 0, QModelIndex());
				proxySel.select(sidx, eidx);

				start = -1;
				end = -1;
				continue;
			}
			// we've got a single item selection, denoted just by the start index
			else
			{
				QModelIndex sidx = this->index(start, 0, QModelIndex());
				proxySel.select(sidx, sidx);

				start = -1;
				end = -1;
				continue;
			}
		}
	}

	// we store the selection for the show only selected rows filter
	//mProxySelection = proxySel.indexes();

	NMDebugCtx(ctxSelSortFilter, << "done!");
	return proxySel;
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
	QModelIndex topcol = this->index(mProxy2Source[0], column, QModelIndex());
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

	mSortOrder = order;
	mSortColumn = column;

	QString colName = this->headerData(column,
			Qt::Horizontal, Qt::DisplayRole).toString();

	QString sorder = order == Qt::AscendingOrder ? QString("asc") : QString("desc");
	NMDebugAI(<< "Sort " << colName.toStdString() << " "
			  << sorder.toStdString() << std::endl);

	//DEBUG
	//NMDebugAI(<< "original order: " << std::endl);
	//for (int i=0; i < this->mSourceModel->rowCount(QModelIndex()); ++i)
	//{
	//	QModelIndex idx = this->mSourceModel->index(i, mSortColumn, QModelIndex());
	//	QVariant val = this->mSourceModel->data(idx, Qt::DisplayRole);
	//	NMDebug(<< val.toString().toStdString() << " ");
	//}
	//NMDebug(<< std::endl);

	emit layoutAboutToBeChanged();

	qsort(0, this->mProxy2Source.size()-1);

	// ToDo::
	// this might go somewhere else and we only generate mSource2Proxy
	// when we really need it
	for (int i=0; i < mProxy2Source.size(); ++i)
	{
		mSource2Proxy[mProxy2Source[i]] = i;
	}
	emit layoutChanged();


	//NMDebugAI(<< "sorted order: " << std::endl);
	//for (int i=0; i < this->mSourceModel->rowCount(QModelIndex()); ++i)
	//{
	//	QModelIndex idx = this->index(i, mSortColumn, QModelIndex());
	//	QVariant val = this->mSourceModel->data(mapToSource(idx), Qt::DisplayRole);
	//	NMDebug(<< val.toString().toStdString() << " ");
	//}
	//NMDebug(<< std::endl);


	NMDebugCtx(ctxSelSortFilter, << "done!");
}


