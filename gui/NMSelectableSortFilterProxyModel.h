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
#include <QDateTime>

class NMSelectableSortFilterProxyModel : public QAbstractProxyModel //public QSortFilterProxyModel
{
	Q_OBJECT

public:
	NMSelectableSortFilterProxyModel(QObject *parent=0);
	~NMSelectableSortFilterProxyModel();

	Qt::ItemFlags flags(const QModelIndex &index) const;

	void setSourceModel(QAbstractItemModel* sourceModel);
	QAbstractItemModel* sourceModel(void) const {return mSourceModel;}

	QModelIndex mapFromSource(const QModelIndex& srcIdx) const;
	QModelIndex mapToSource(const QModelIndex& proxyIdx) const;
	QItemSelection mapSelectionFromSource(const QItemSelection& sourceSelection) const;
	QItemSelection mapSelectionToSource(const QItemSelection& proxySelection) const;

	void sort(int column, Qt::SortOrder order);


	QModelIndex index(int row, int column, const QModelIndex& idx) const;
	QModelIndex parent(const QModelIndex& idx) const;
	int rowCount(const QModelIndex& idx=QModelIndex()) const;
	int columnCount(const QModelIndex& idx) const;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role) const;
	QVariant data(const QModelIndex& index, int role) const;
	bool setData(const QModelIndex& index, const QVariant& value,
			int role);




	void setDynamicSortFilter(bool dynamic){};
	void setFilterRegExp(const QRegExp& regexp){};
	void setFilterKeyColumn(int column){};

protected:

	QAbstractItemModel* mSourceModel;
	QItemSelection* mSrcSelection;
	QItemSelection* mProxySelection;
	QList<int> mProxy2Source; // sorted row index of source model
	QList<int> mSource2Proxy;

	Qt::SortOrder mSortOrder;
	int mSortColumn;

	// we only do row-based comparison
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
		//++nmlog::nmindent;

		//NMDebugAI(<< "--> " << left << " right" << std::endl);

		int le=left, ri=right;
		int middle = (le + ri) / 2;

		const QModelIndex midx = this->mSourceModel->index(
				mProxy2Source[middle], mSortColumn, QModelIndex());
		const QVariant mval = mSourceModel->data(midx, Qt::DisplayRole);

		do
		{
			switch (mSortOrder)
			{
			case Qt::DescendingOrder:
				{
					QModelIndex leidx = this->mSourceModel->index(mProxy2Source[le], mSortColumn, QModelIndex());
					while (greaterThan(mSourceModel->data(leidx, Qt::DisplayRole), mval))
					{
						++le;
						leidx = this->mSourceModel->index(mProxy2Source[le], mSortColumn, QModelIndex());
					}

					QModelIndex riidx = this->mSourceModel->index(mProxy2Source[ri], mSortColumn, QModelIndex());
					while (lessThan(mSourceModel->data(riidx, Qt::DisplayRole), mval))
					{
						--ri;
						riidx = this->mSourceModel->index(mProxy2Source[ri], mSortColumn, QModelIndex());
					}
				}
				break;

			case Qt::AscendingOrder:
				{
					QModelIndex leidx = this->mSourceModel->index(mProxy2Source[le], mSortColumn, QModelIndex());
					while (lessThan(mSourceModel->data(leidx, Qt::DisplayRole), mval))
					{
						++le;
						leidx = this->mSourceModel->index(mProxy2Source[le], mSortColumn, QModelIndex());
					}

					QModelIndex riidx = this->mSourceModel->index(mProxy2Source[ri], mSortColumn, QModelIndex());
					while (greaterThan(mSourceModel->data(riidx, Qt::DisplayRole), mval))
					{
						--ri;
						riidx = this->mSourceModel->index(mProxy2Source[ri], mSortColumn, QModelIndex());
					}
				}
				break;

			default:
				break;
			}

			if (le <= ri)
			{
				//NMDebugAI(<< "  swap " << le << " " << ri << std::endl);
				mProxy2Source.swap(le, ri);
				++le;
				--ri;
			}
		} while (le <= ri);
		if (left < ri) qsort(left, ri);
		if (right > le) qsort(le, right);

		//NMDebugAI( << "<-- " << le << " " << ri << std::endl);
		//--nmlog::nmindent;
	}
};

#endif // ifndef
