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

//#include <qsortfilterproxymodel.h>
#include <qabstractproxymodel.h>

#include "nmlog.h"

#include <QItemSelection>
#include <QDateTime>

class NMSelectableSortFilterProxyModel : public QAbstractProxyModel //public QSortFilterProxyModel
{
	Q_OBJECT

public:
	NMSelectableSortFilterProxyModel(QObject *parent=0);
	~NMSelectableSortFilterProxyModel();

	Qt::ItemFlags flags(const QModelIndex &index) const;

	/**
	 *  Set/Get the underlying source model.
	 */
	void setSourceModel(QAbstractItemModel* sourceModel);
	QAbstractItemModel* sourceModel(void) const {return mSourceModel;}

	/**
	 *  Control which source rows are actually available
	 *  as source for display, selection, and filtering
	 *  offered by this model.
	 */
	void hideSource(const QList<int>& rows);
	void showSource(const QList<int>& rows);

	/**
	 *  Returns the number of unhidden source rows.
	 */
	int sourceRowCount(void) const;

	/**
	 *  Returns an item selection, which comprises all
	 *  actually considered source rows
	 *  (i.e. excluding any hidden rows) regardless of
	 *  any filter settings. This is handy for toggling
	 *  selections (switch selection) when hidden rows
	 *  are presented.
	 */
	QItemSelection getSourceSelection(bool rowsonly=true);


	/**
	 *  Turn filter mode on or off; the filter operates
	 *  on the actual available source rows offered by
	 *  this model (i.e. excluding any hidden rows);
	 */
	void setFilterOn(bool yesno);
	bool isFilterOn(void)
		{return this->mbFilterOn;}

	/**
	 * 	Controls the which source rows are part of the
	 * 	filter or not. Rows 'in' the filter are visible
	 * 	to any ItemView or other consumer, which accesses
	 * 	the model by it's index, data, mapToSource, mapFromSource
	 * 	functions.
	 */
	void addToFilter(const QItemSelection& proxySelection);
	void removeFromFilter(const QItemSelection& proxySelection);
	void clearFilter(void);


	/**
	 *  NOTE: mapping indexes accounts for any active filter
	 *        applied to the model! i.e.
	 *        - source gets mapped to filtered proxy
	 *        - filtered proxy gets mapped to source
	 */
	QModelIndex mapFromSource(const QModelIndex& srcIdx) const;
	QModelIndex mapToSource(const QModelIndex& proxyIdx) const;

	/**
	 *  NOTE: mapping of selections is done regardless of any active filter;
	 *        i.e. a source selection gets mapped to an unfiltered proxy selection
	 *        and an unfiltered proxy selection  gets mapped to a source selection
	 */
	QItemSelection mapSelectionFromSource(const QItemSelection& sourceSelection) const;
	QItemSelection mapRowSelectionFromSource(const QItemSelection& sourceSelection,
			bool rowsonly=true) const;
	//QItemSelection mapSelectionFromSource(const QModelIndexList& sourceList) const;
	//QItemSelection mapSelectionToSourceFromRaw(const QItemSelection& rawSelection) const;
	QItemSelection mapSelectionToSource(const QItemSelection& proxySelection) const;
	void itemSelectionFromIndexList(const std::vector<int>& list,
			QItemSelection& isel, bool rowsonly=true) const;

	/**
	 * 	Toggles the given item selection (i.e. selected rows, for column=0),
	 * 	assuming either a proxy selection or a source selection;
	 */
	QItemSelection swapRowSelection(const QItemSelection& selection,
			bool bSourceSelection,
			bool rowsonly=true) const;

	void sort(int column, Qt::SortOrder order);


	QModelIndex index(int row, int column, const QModelIndex& idx=QModelIndex()) const;
	QModelIndex parent(const QModelIndex& idx) const;
	int rowCount(const QModelIndex& idx=QModelIndex()) const;
	int columnCount(const QModelIndex& idx=QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role) const;
	QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value,
			int role=Qt::DisplayRole);
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role);
	bool insertColumns(int column, int count, const QModelIndex& parent=QModelIndex());
	bool removeColumns(int column, int count, const QModelIndex& parent=QModelIndex());

	const QList<int>* getRaw2Source(void)
		{return &mRaw2Source;}


	// deprecated, they're not really in use in this implementation!
	void setDynamicSortFilter(bool dynamic){};
	void setFilterRegExp(const QRegExp& regexp){};
	void setFilterKeyColumn(int column){};

	void notifyLayoutUpdate() {emit layoutChanged();}

protected:

	QAbstractItemModel* mSourceModel;
	//QList<bool> mHidden;

	/**
	 *  contains for each index of the original
	 *  source model the index of the 'mapped'
	 *  (i.e. unhidden) source index (in view
	 *  of the SortFilterProxyModel.
	 */
	QList<int> mRaw2Source;

	// proxy-source maps, which DO NOT
	// contain hidden rows, i.e.
	// size() = mSourceModel.size() - mHiddenSource.size()
	QList<int> mSource2Raw;
	QList<int> mProxy2Source;
	QList<int> mSource2Proxy;

	// list indicating filtered rows (i.e. visible)
	// size is <= mProxy2Source.size()
	QList<int> mFilter2Proxy;
	// we'll have another bool to control whether the
	// filter should be used or not to explicitly allow for
	// an empty filter display in the tableView
	bool mbFilterOn;

	int mNumHidden;
	Qt::SortOrder mSortOrder;
	int mSortColumn;

	void resetMapping(void);
	QModelIndex mapToRaw(const QModelIndex& index) const;

//protected slots:
//	void handleColumnsChanged(const QModelIndex& parent, int start, int end);

private:

	void printSelRanges(const QItemSelection& selection,
			const QString& msg) const;


	// both values are expected to be of the same type
	inline bool lessThan(const QVariant& valLeft, const QVariant& valRight)
	{
		bool bok;
		switch(valLeft.type())
		{
		case QVariant::LongLong:
			return valLeft.toLongLong(&bok) < valRight.toLongLong(&bok);
			break;
		case QVariant::Int:
			return valLeft.toInt(&bok)      < valRight.toInt(&bok);
			break;
		case QVariant::UInt:
			return valLeft.toUInt(&bok)     < valRight.toUInt(&bok);
			break;
		case QVariant::Double:
			return valLeft.toDouble(&bok)   < valRight.toDouble(&bok);
			break;
		case QVariant::DateTime:
			return valLeft.toDateTime()     < valRight.toDateTime();
			break;
		default:
			return QString::localeAwareCompare(valLeft.toString(), valRight.toString())
					< 0 ? true : false;
			break;
		}

		return false;
	}

	inline bool greaterThan(const QVariant& valLeft, const QVariant& valRight)
	{
		bool bok;
		switch(valLeft.type())
		{
		case QVariant::LongLong:
			return valLeft.toLongLong(&bok) > valRight.toLongLong(&bok);
			break;
		case QVariant::Int:
			return valLeft.toInt(&bok)      > valRight.toInt(&bok);
			break;
		case QVariant::UInt:
			return valLeft.toUInt(&bok)     > valRight.toUInt(&bok);
			break;
		case QVariant::Double:
			return valLeft.toDouble(&bok)   > valRight.toDouble(&bok);
			break;
		case QVariant::DateTime:
			return valLeft.toDateTime()     > valRight.toDateTime();
			break;
		default:
			return QString::localeAwareCompare(valLeft.toString(), valRight.toString())
					> 0 ? true : false;
			break;
		}

		return false;
	}


	// we do quick sort and expect that
	// the given bounds are within the limit!
	inline void qsort(int left, int right)
	{
		int le=left, ri=right;
		const int middle = (le + ri) / 2;

		const QModelIndex midx = this->mSourceModel->index(
				mSource2Raw.at(mProxy2Source.at(middle)), mSortColumn, QModelIndex());
		const QVariant mval = mSourceModel->data(midx, Qt::DisplayRole);

		do
		{
			switch (mSortOrder)
			{
			case Qt::DescendingOrder:
				{
					QModelIndex leidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(le)), mSortColumn, QModelIndex());
					while (greaterThan(mSourceModel->data(leidx, Qt::DisplayRole), mval))
					{
						++le;
						leidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(le)), mSortColumn, QModelIndex());
					}

					QModelIndex riidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(ri)), mSortColumn, QModelIndex());
					while (lessThan(mSourceModel->data(riidx, Qt::DisplayRole), mval))
					{
						--ri;
						riidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(ri)), mSortColumn, QModelIndex());
					}
				}
				break;

			case Qt::AscendingOrder:
				{
					QModelIndex leidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(le)), mSortColumn, QModelIndex());
					while (lessThan(mSourceModel->data(leidx, Qt::DisplayRole), mval))
					{
						++le;
						leidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(le)), mSortColumn, QModelIndex());
					}

					QModelIndex riidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(ri)), mSortColumn, QModelIndex());
					while (greaterThan(mSourceModel->data(riidx, Qt::DisplayRole), mval))
					{
						--ri;
						riidx = this->mSourceModel->index(mSource2Raw.at(mProxy2Source.at(ri)), mSortColumn, QModelIndex());
					}
				}
				break;

			default:
				break;
			}

			if (le <= ri)
			{

				mProxy2Source.swap(le, ri);
				++le;
				--ri;
			}
		} while (le <= ri);
		if (left < ri) qsort(left, ri);
		if (right > le) qsort(le, right);
	}
};

#endif // ifndef
