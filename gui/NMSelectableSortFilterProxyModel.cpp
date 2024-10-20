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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include <algorithm>
#include <vector>

#include <QItemSelectionRange>
#include "NMGlobalHelper.h"

NMSelectableSortFilterProxyModel::NMSelectableSortFilterProxyModel(
		QObject *parent)
		: QAbstractProxyModel(parent),
		  mSourceModel(0),
		  mSortOrder(Qt::AscendingOrder),
		  mSortColumn(0),
		  mNumHidden(0),
		  mbFilterOn(false)
{
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(true);

    connect(mLogger, SIGNAL(sendLogMsg(QString)), NMGlobalHelper::getLogWidget(),
            SLOT(insertHtml(QString)));
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

    return this->createIndex(row, column, (void*)0);
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
	//emit this->reset()
	emit layoutChanged();
}

int
NMSelectableSortFilterProxyModel::sourceRowCount(void) const
{
	if (mSourceModel == 0)
		return 0;

	int sr = this->mSource2Proxy.size();

	return sr;
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
        NMLogError(<< ctxSelSortFilter << ": Invalid column: " << idx.column());
		return QModelIndex();
	}

	int nrows = this->rowCount();
	int lookuprow = idx.row();
	if (lookuprow < 0 || lookuprow > nrows-1)
	{
        NMLogError(<< ctxSelSortFilter << ": Invalid row: " << idx.row());
		return QModelIndex();
	}

	//if (mFilter2Proxy.size() > lookuprow)
	if (mbFilterOn)
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
	mRaw2Source.clear();  mRaw2Source.reserve(nrows);
	mFilter2Proxy.clear();

	for (int i=0; i < nrows; ++i)
	{
		mProxy2Source << i;
		mSource2Proxy << i;
		mSource2Raw << i;
		mRaw2Source << i;
	}
	mNumHidden = 0;

	connect(mSourceModel, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
//	connect(mSourceModel, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
//			this, SLOT(handleColumnsChanged(const QModelIndex &, int, int)));
//	connect(mSourceModel, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
//			this, SLOT(handleColumnsChanged(const QModelIndex &, int, int)));

    // commented due to transition from Qt4 to Qt5
    //this->reset();
	emit endResetModel();
}

//void
//NMSelectableSortFilterProxyModel::handleColumnsChanged(const QModelIndex& parent,
//		int start, int end)
//{
//	//emit layoutChanged();
//}

bool
NMSelectableSortFilterProxyModel::insertColumns(int column, int count,
		const QModelIndex& parent)
{
	beginInsertColumns(QModelIndex(), column, column);
	if (!this->mSourceModel->insertColumns(column, count, parent))
		return false;
	endInsertColumns();
	return true;
}

bool
NMSelectableSortFilterProxyModel::removeColumns(int column, int count,
		const QModelIndex& parent)
{
	beginRemoveColumns(QModelIndex(), column, column);
	if (!this->mSourceModel->removeColumns(column, count, parent))
		return false;
	endRemoveColumns();
	return true;
}

void
NMSelectableSortFilterProxyModel::clearFilter(void)
{
	if (mFilter2Proxy.size() > 0)
	{
		mFilter2Proxy.clear();
		if (this->mbFilterOn)
		{
			emit layoutChanged();
		}
	}
}

void
NMSelectableSortFilterProxyModel::hideSource(const QList<int>& rows)
{
	if (	this->mSourceModel == 0
        ||  rows.size() <= this->mSourceModel->rowCount(QModelIndex())
	   )
	{
		return;
	}

	//NMDebugCtx(ctxSelSortFilter, << "...");

	for (int r=0; r < rows.size(); ++r)
	{
		mRaw2Source[rows.at(r)] = -1;
	}

	resetMapping();
	this->clearFilter();

	//NMDebugAI(<< "hid " << mNumHidden << " of " << mRaw2Source.size() << " raw rows " << std::endl);

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	//emit this->reset();
}

void
NMSelectableSortFilterProxyModel::addToFilter(const QItemSelection& proxySel)
{
	if (this->mSourceModel == 0 || proxySel.count() == 0)
		return;

	emit layoutAboutToBeChanged();
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
	emit layoutChanged();
	//emit this->reset();
}

bool
NMSelectableSortFilterProxyModel::setHeaderData(int section,
		Qt::Orientation orientation, const QVariant& value, int role)
{
	bool ret = false;
	ret = this->mSourceModel->setHeaderData(section, orientation, value, role);
	if (ret)
	{
		emit headerDataChanged(Qt::Horizontal, section, section);
		return true;
	}
	else
		return false;
}

void
NMSelectableSortFilterProxyModel::resetMapping(void)
{
	NMDebugCtx(ctxSelSortFilter, << "...");

    beginResetModel();

	const int nrows = this->mSourceModel->rowCount(QModelIndex());
	mSource2Raw.clear();			mSource2Raw.reserve(nrows);
	mSource2Proxy.clear();			mSource2Proxy.reserve(nrows);
	mProxy2Source.clear();			mProxy2Source.reserve(nrows);
	mProxy2Source.clear(); 			mProxy2Source.reserve(nrows);

	long row = 0;
	//NMDebugAI(<< "mRaw2Source indices ..." << std::endl);
	for (int i=0; i < nrows; ++i)
	{
		if (mRaw2Source.at(i) == -1)
		{
			//NMDebug(<< mRaw2Source.at(i) << " ");
			continue;
		}

		mRaw2Source[i] = row;
		mSource2Raw << i;
		mProxy2Source << row;
		mSource2Proxy << row;
		++row;

		//NMDebug(<< mRaw2Source.at(i) << " ");
	}
	NMDebug(<< std::endl);

    // added due to qt4 to qt5 transition
    endResetModel();

	//NMDebugAI(<< "un-hidden source rows: " << row << " (mSource2Raw.size()=" << mSource2Raw.size() << ") raw rows " << std::endl);
	NMDebugCtx(ctxSelSortFilter, << "done!");

    // commented due to qt4 to qt5 transition
    //emit this->reset();
}

void
NMSelectableSortFilterProxyModel::showSource(const QList<int>& rows)
{
	if (	this->mSourceModel == 0
		||  rows.size() == 0)
		return;


	for (int i=0; i < rows.size(); ++i)
	{
		if (mRaw2Source.at(rows.at(i)) == -1)
		{
			mRaw2Source[rows.at(i)] = rows.at(i);
		}
	}

    this->clearFilter();
    resetMapping();
	//NMDebugAI(<< "hid " << mNumHidden << " of " << mRaw2Source.size() << " raw rows " << std::endl);

	NMDebugCtx(ctxSelSortFilter, << "done!");

    // commented due to qt4 to qt5 transition
    //emit this->reset();
}

void
NMSelectableSortFilterProxyModel::removeFromFilter(const QItemSelection& srcSel)
{
	if (this->mSourceModel == 0)
		return;

    // added due to qt4 to qt5 transition
    //emit beginResetModel();

    foreach(const QItemSelectionRange& range, srcSel)
	{
		if (range.top() < 0 || range.bottom() > this->mProxy2Source.size()-1)
			continue;

		for (int i=range.top(); i <= range.bottom(); ++i)
		{
			mFilter2Proxy.removeOne(i);
		}
	}

    // added due to qt4 to qt5 transition
    //emit endResetModel();

    // commented due to qt4 to qt5 transition
    //emit this->reset();
}


Qt::ItemFlags NMSelectableSortFilterProxyModel::flags(
		const QModelIndex &index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
        return QFlags<Qt::ItemFlag>();
}

QModelIndex
NMSelectableSortFilterProxyModel::mapFromSource(const QModelIndex& srcIdx) const
{
	if (srcIdx.column() < 0 || srcIdx.column() > mSourceModel->columnCount(QModelIndex())-1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid column: " << srcIdx.column());
		return QModelIndex();
	}

	//if (srcIdx.row() < 0 || srcIdx.row() > this->mSource2Proxy.size()-1)
	if (	srcIdx.row() < 0
		||  srcIdx.row() > this->mRaw2Source.size()-1
		||  this->mRaw2Source.at(srcIdx.row()) == -1)
	{
		//NMDebugAI(<< __FUNCTION__ << ": Invalid row: " << srcIdx.row());
		return QModelIndex();
	}

	int row = mSource2Proxy.at(mRaw2Source.at(srcIdx.row()));
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

	//if (mFilter2Proxy.size() > lookuprow)
	if (mbFilterOn)
		lookuprow = mFilter2Proxy.at(lookuprow);

	return this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(lookuprow)), proxyIdx.column(), QModelIndex());
}

QItemSelection
NMSelectableSortFilterProxyModel::getSourceSelection(bool rowsonly)
{
	//NMDebugCtx(ctxSelSortFilter, << "...");

	QItemSelection srcSel;
	if (mSourceModel == 0)
		return srcSel;

	//NMDebugAI(<< "generate index list ...");
	std::vector<int> srcidx(mSource2Raw.size());
	for (int i=0; i < mSource2Raw.size(); ++i)
		srcidx[i] = mSource2Raw.at(i);
	//NMDebug( << " -> " << srcidx.size() << " items" << std::endl);

	//NMDebugAI(<< "sorting list ... ");
	//std::stable_sort(srcidx.begin(), srcidx.end());
	//NMDebug(<< "got " << srcidx.size() << " sorted items" << std::endl);

	//NMDebugAI(<< "convert list to item selection (range list) ... " << std::endl);
	this->itemSelectionFromIndexList(srcidx, srcSel, rowsonly);
	//this->printSelRanges(srcSel, "Source selection ranges ...");

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	return srcSel;

}

QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionToSource(const QItemSelection& proxySelection) const
{
	//NMDebugCtx(ctxSelSortFilter, << "...");
	QItemSelection srcSel;
	if (mSourceModel == 0 || proxySelection.count() == 0)
		return srcSel;

	std::vector<int> srcidx;
	foreach(const QItemSelectionRange& range, proxySelection)
	{
		for (int row=range.top(); row <= range.bottom(); ++row)
		{
			srcidx.push_back(mSource2Raw.at(mProxy2Source.at(row)));
		}
	}
	if (srcidx.size() > 1)
		std::stable_sort(srcidx.begin(), srcidx.end());
	this->itemSelectionFromIndexList(srcidx, srcSel);

	//NMDebugCtx(ctxSelSortFilter, << "done!");
	return srcSel;
}

QItemSelection
NMSelectableSortFilterProxyModel::swapRowSelection(
		const QItemSelection& selection, bool bSourceSelection,
		bool rowsonly) const
{
	NMDebugCtx(ctxSelSortFilter, << "...");
	QItemSelection isel;
	const int numranges = selection.size();
	const int maxrow = bSourceSelection ? this->mRaw2Source.size()-1
			: this->mSource2Proxy.size()-1;
	//if (numranges == 0)
	//{
	//	const QModelIndex sidx = this->createIndex(0, 0, 0);
	//	const QModelIndex eidx = this->createIndex(maxrow, 0, 0);
	//	isel.append(QItemSelectionRange(sidx, eidx));
	//	NMDebugAI(<< "input selection empty -> complete table selected!" << std::endl);
	//	NMDebugCtx(ctxSelSortFilter, << "done!");
	//	return isel;
	//}

	// create list of selected indices
	std::vector<int> selidx;
	//NMDebugAI(<< "map source indices to proxies ..." << std::endl);

	int rangeindex=0;
	//NMDebugAI(<< "picking rows: ");
	for (int invr=0; invr <= maxrow; ++invr)
	{
		if (bSourceSelection && mRaw2Source.at(invr) == -1)
			continue;
		if (numranges > 0)
		{
			if (	invr >= selection.at(rangeindex).top()
				&&  invr <= selection.at(rangeindex).bottom())
			{
				if (invr == selection.at(rangeindex).bottom())
				{
					rangeindex = rangeindex < numranges - 1 ? rangeindex + 1 : rangeindex;
				}
				continue;
			}
		}

		//NMDebug(<< invr << "  ");
		selidx.push_back(invr);
	}
	//NMDebugAI( << std::endl);
	NMDebugAI(<< selidx.size() << " indices in swapped selection" << std::endl);

	// need to sort in any case?
	//std::stable_sort(selidx.begin(), selidx.end());

	// turn list of indices into selection ranges
	this->itemSelectionFromIndexList(selidx, isel, rowsonly);

	NMDebugCtx(ctxSelSortFilter, << "done!");
	return isel;
}


void
NMSelectableSortFilterProxyModel::itemSelectionFromIndexList(const std::vector<int>& list,
		QItemSelection& isel, bool rowsonly) const
{
	//NMDebugCtx(ctxSelSortFilter, << "...");
	int numsel = 0;
	const int& nrows = list.size();
	if (nrows == 0)
	{
		NMDebugAI(<< "Input index list empty! Get out of here!" << std::endl);
		NMDebugCtx(ctxSelSortFilter, << "done!");
		return;
	}

	const int& maxcolid = rowsonly ? 0 : this->mSourceModel->columnCount()-1;
	int start = -1;
	int end   = -1;
	for (int r=0; r < nrows; ++r)
	{
		if (start == -1)
		{
			start = list.at(r);
			end = start;

			if (r == nrows-1)
			{
                QModelIndex sidx = this->createIndex(start, 0, (void*)0);
                QModelIndex eidx = this->createIndex(end, maxcolid, (void*)0);
				isel.append(QItemSelectionRange(sidx, eidx));
				//NMDebugAI(<< "  added rows: " << start << " to " << end << std::endl);
				numsel += (end-start+1);
			}
		}
		// row is adjacent, so we extend the range
		else if (list.at(r) == end + 1)
		{
			++end;

			if (r == nrows-1)
			{
                QModelIndex sidx = this->createIndex(start, 0, (void*)0);
                QModelIndex eidx = this->createIndex(end, maxcolid, (void*)0);
				isel.append(QItemSelectionRange(sidx, eidx));
				//NMDebugAI(<< "  added rows: " << start << " to " << end << std::endl);
				numsel += (end-start+1);
			}
		}
		else
		{
            QModelIndex sidx = this->createIndex(start, 0, (void*)0);
            QModelIndex eidx = this->createIndex(end, maxcolid, (void*)0);
			isel.append(QItemSelectionRange(sidx, eidx));
			//NMDebugAI(<< "  added rows: " << start << " to " << end << std::endl);
			numsel += (end-start+1);

			start = list.at(r);
			end   = start;

			if (r == nrows-1 && nrows > 1)
			{
                QModelIndex sidx = this->createIndex(start, 0, (void*)0);
                QModelIndex eidx = this->createIndex(end, maxcolid, (void*)0);
				isel.append(QItemSelectionRange(sidx, eidx));
				//NMDebugAI(<< "  added rows: " << start << " to " << end << std::endl);
				numsel += (end-start+1);
			}
		}
	}
	//NMDebugAI(<< "selected " << numsel << " rows in total!" << std::endl);
	//NMDebugCtx(ctxSelSortFilter, << "done!");
}


QItemSelection
NMSelectableSortFilterProxyModel::mapRowSelectionFromSource(const QItemSelection &sourceSelection,
		bool rowsonly) const
{
	NMDebugCtx(ctxSelSortFilter, << "...");
	QItemSelection isel;
	if (sourceSelection.size() == 0)
	{
		NMDebugAI(<< "Input selection is empty." << std::endl);
		NMDebugCtx(ctxSelSortFilter, << "done!");
		return isel;
	}

	// create sorted list of selected proxy indices
	std::vector<int> selidx;//(mProxy2Source.size());
	NMDebugAI(<< "map source indices to proxies ..." << std::endl);
	foreach(const QItemSelectionRange& r, sourceSelection)
	{
		for(int row=r.top(); row <= r.bottom(); ++row)
		{
			const int rowidx = mSource2Proxy.at(mRaw2Source.at(row));
			if (rowidx < 0 || rowidx > mSource2Proxy.size()-1)
			{
				NMDebugAI(<< rowidx << " is outside mSource2Proxy! Skip!" << std::endl);
			}
			else
			{
				//NMDebugAI(<< "\t" << row << " -> " << mSource2Proxy.at(row) << std::endl);
				selidx.push_back(rowidx);
			}
		}
	}
	//NMDebugAI(<< selidx.size() << " rows to sort ... and map ..." << std::endl);
	if (selidx.size() > 1)
		std::stable_sort(selidx.begin(), selidx.end());

	// turn list of indices into selection ranges
	this->itemSelectionFromIndexList(selidx, isel, rowsonly);

	NMDebugCtx(ctxSelSortFilter, << "done!");
	return isel;
}


QItemSelection
NMSelectableSortFilterProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
	NMDebugCtx(ctxSelSortFilter, << "...");
	QItemSelection isel;
	if (sourceSelection.size() == 0)
	{
		NMDebugAI(<< "Input selection is empty." << std::endl);
		NMDebugCtx(ctxSelSortFilter, << "done!");
		return isel;
	}

	// create sorted list of selected proxy indices
	std::vector<int> selidx;//(mProxy2Source.size());
	NMDebugAI(<< "map source indices to proxies ..." << std::endl);
	foreach(const QItemSelectionRange& r, sourceSelection)
	{
		for(int row=r.top(); row <= r.bottom(); ++row)
		{
			const int rowidx = mSource2Proxy.at(mRaw2Source.at(row));
			if (rowidx < 0 || rowidx > mSource2Proxy.size()-1)
			{
				NMDebugAI(<< rowidx << " is outside mSource2Proxy! Skip!" << std::endl);
			}
			else
			{
				//NMDebugAI(<< "\t" << row << " -> " << mSource2Proxy.at(row) << std::endl);
				selidx.push_back(rowidx);
			}
		}
	}
	//NMDebugAI(<< selidx.size() << " rows to sort ... and map ..." << std::endl);
	if (selidx.size() > 1)
		std::stable_sort(selidx.begin(), selidx.end());

	// turn list of indices into selection ranges
	this->itemSelectionFromIndexList(selidx, isel);

	NMDebugCtx(ctxSelSortFilter, << "done!");
	return isel;
}

void
NMSelectableSortFilterProxyModel::printSelRanges(const QItemSelection& selection,
		const QString& msg) const
{
	int total = selection.count();
	NMDebugAI(<< msg.toStdString() << ": " << total << " indices total" << std::endl);
	int rcnt = 1;
	foreach(const QItemSelectionRange& range, selection)
	{
		NMDebugAI(<< "   range #" << rcnt << ":  " << range.width()
				                          << " x " << range.height() << std::endl);
		NMDebugAI(<< "     rows: " << range.top() << " - " << range.bottom() << std::endl);
		NMDebug(<< std::endl);
	}
}

//QItemSelection
//NMSelectableSortFilterProxyModel::mapSelectionFromSource(const QModelIndexList& sourceList) const
//{
//	QItemSelection proxySel;
//	if (mSourceModel == 0 || sourceList.count() == 0)
//		return proxySel;
//
//	const int nrows = this->mProxy2Source.size();
//	int start = -1;
//	int end = -1;
//	for (int i=0; i < nrows; ++i)
//	{
//		const QModelIndex idx = this->createIndex(mProxy2Source.at(i), 0, 0);
//		//const QModelIndex idx = this->mapToSource(_idx);
//
//		//NMDebugAI(<< "proxy-id: " << i << " source-id: " << idx.row() << std::endl);
//
//		// inside a selection
//		if (sourceList.contains(idx))
//		{
//			//NMDebugAI(<< "detected selection at: proxy=" << i << " source=" << idx.row() << std::endl);
//
//			// if we haven't got a selection range started yet, we do it now
//			if (start == -1)
//			{
//				start = i;
//				end = -1;
//				if (i == nrows-1)
//				{
//					QModelIndex sidx = this->createIndex(start, 0, 0);
//					proxySel.select(sidx, sidx);
//
//					start = -1;
//				}
//			}
//			// if we've got already a selection range, we extend its end
//			else //if (start != -1)
//			{
//				end = i;
//				// in case this is the last row, we complete the selection range
//				// and add it to the ItemSelection
//				if (i == nrows-1)
//				{
//					QModelIndex sidx = this->createIndex(start, 0, 0);
//					QModelIndex eidx = this->createIndex(end, 0, 0);
//					proxySel.select(sidx, eidx);
//				}
//			}
//		}
//		// we're currently not in a selection
//		else
//		{
//			// if we've got a valid end index, we
//			// complete the selection range and add it to the
//			// ItemSelection
//			if (end != -1)
//			{
//				QModelIndex sidx = this->createIndex(start, 0, 0);
//				QModelIndex eidx = this->createIndex(end, 0, 0);
//				proxySel.select(sidx, eidx);
//
//				start = -1;
//				end = -1;
//			}
//			// we've got a single item selection, denoted just by the start index
//			else if (start != -1)
//			{
//				QModelIndex sidx = this->createIndex(start, 0, 0);
//				proxySel.select(sidx, sidx);
//
//				start = -1;
//				end = -1;
//			}
//		}
//	}
//
//	return proxySel;
//}

//QItemSelection
//NMSelectableSortFilterProxyModel::mapSelectionToSourceFromRaw(
//		const QItemSelection& rawSelection) const
//{
//	QItemSelection srcSel;
//	if (mSourceModel == 0 || rawSelection.size() == 0)
//		return srcSel;
//
//	const int nrows = this->mSource2Raw.size();
//	int start = -1;
//	int end = -1;
//	for (int i=0; i < nrows; ++i)
//	{
//		const QModelIndex idx = this->mSourceModel->index(mSource2Raw.at(i), 0, QModelIndex());
//
//		// inside a selection
//		if (rawSelection.contains(idx) && !mRaw2Source.at(mSource2Raw.at(i)))
//		{
//			// if we haven't got a selection range, we start a new one
//			if (start == -1)
//			{
//				start = i;
//				end = -1;
//				if (i == nrows-1)
//				{
//					QModelIndex sidx = this->createIndex(start, 0, 0);
//					srcSel.select(sidx, sidx);
//					start = -1;
//				}
//			}
//			// if we've got already a selection range, we extend its end
//			else //if (start != -1)
//			{
//				end = i;
//				// in case this is the last row, we complete the selection range
//				// and add it to the ItemSelection
//				if (i == nrows-1)
//				{
//					QModelIndex sidx = this->createIndex(start, 0, 0);
//					QModelIndex eidx = this->createIndex(end, 0, 0);
//					srcSel.select(sidx, eidx);
//				}
//			}
//		}
//		// we're currently not in a selection
//		else
//		{
//			// if we've got a valid end index, we
//			// complete the selection range and add it to the
//			// ItemSelection
//			if (end != -1)
//			{
//				QModelIndex sidx = this->createIndex(start, 0, 0);
//				QModelIndex eidx = this->createIndex(end, 0, 0);
//				srcSel.select(sidx, eidx);
//
//				start = -1;
//				end = -1;
//			}
//			// we've got a single item selection, denote just by the start index
//			else if (start != -1)
//			{
//				QModelIndex sidx = this->createIndex(start, 0, 0);
//				srcSel.select(sidx, sidx);
//
//				start = -1;
//				end = -1;
//			}
//		}
//	}
//	return srcSel;
//}

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


