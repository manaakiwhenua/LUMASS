/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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
 * NMFastTrackSelectionModel.cpp
 *
 *  Created on: 19/11/2013
 *      Author: alex
 */

#include "NMFastTrackSelectionModel.h"
#include <private/qitemselectionmodel_p.h>
#include "nmlog.h"


class NMFastTrackSelectionModelPrivate : public QItemSelectionModelPrivate
{
	Q_DECLARE_PUBLIC(NMFastTrackSelectionModel)
public:
	NMFastTrackSelectionModelPrivate() {}
	~NMFastTrackSelectionModelPrivate() {}

	void cleanup()
	{
		finalize();
	}
};

NMFastTrackSelectionModel::NMFastTrackSelectionModel(QAbstractItemModel* model)
	: QItemSelectionModel(*new NMFastTrackSelectionModelPrivate, model)
{

}

NMFastTrackSelectionModel::NMFastTrackSelectionModel(QAbstractItemModel* model, QObject* parent)
	: QItemSelectionModel(*new NMFastTrackSelectionModelPrivate, model)
{
	this->setParent(parent);
}


NMFastTrackSelectionModel::~NMFastTrackSelectionModel()
{
}

void
NMFastTrackSelectionModel::setSelection(const QItemSelection& newSel)
{
	//NMDebugCtx(this->objectName().toStdString(), << "...");

	Q_D(NMFastTrackSelectionModel);

	d->currentSelection.clear();
	d->currentCommand = QItemSelectionModel::NoUpdate;

	QItemSelection old = d->ranges;

	d->ranges.clear();
	d->ranges.append(newSel);
	//d->ranges.clear();
	//long numin = 0;
	//foreach(QItemSelectionRange range, newSel)
	//{
	//	//numin += range.bottom() - range.top() + 1;
	//	d->ranges.append(range);
	//}

	//long numidx=0;
	//foreach(const QItemSelectionRange& r, d->ranges)
	//{
	//	numidx += r.bottom() - r.top() + 1;
	//}
	//if (numidx < 0)
	//	numidx = 0;

	//NMDebugAI(<< "set " << numidx << " rows (->selection)" << std::endl);

	//NMDebugCtx(this->objectName().toStdString(), << "done!");
	emit selectionChanged(d->ranges, old);
}

QItemSelection
NMFastTrackSelectionModel::getSelection(void)
{
	//NMDebugCtx(this->objectName().toStdString(), << "...");

	Q_D(NMFastTrackSelectionModel);

	//d->cleanup();
	//d->currentSelection.clear();
	//d->currentCommand = NoUpdate;

	//NMDebugAI(<< "FASTTRACK: ranges num: " << d->ranges.indexes().size() << std::endl);
	//NMDebugAI(<< "FASTTRACK: curSel num: " << d->currentSelection.indexes().size() << std::endl);

	//long numidx=0;
	//foreach(const QItemSelectionRange& r, d->ranges)
	//{
	//	numidx += r.bottom() - r.top() + 1;
	//}
	//if (numidx < 0)
	//	numidx = 0;
    //
	//NMDebugAI(<< "got " << numidx << " selected rows (selection->)" << std::endl);
    //
	//NMDebugCtx(this->objectName().toStdString(), << "done!");

	QItemSelection ret = d->ranges;
	return ret;
}

void
NMFastTrackSelectionModel::toggleRow(int row, int column, const QModelIndex& parent)
{
	Q_D(NMFastTrackSelectionModel);

	if (row < 0 || row > model()->rowCount(parent)-1)
		return;

	//NMDebugCtx(this->objectName().toStdString(), << "...");

	// merge any outstanding current selections
	d->cleanup();

//	const int& maxcol = column < 0  || column > model()->columnCount(parent)
//			?  model()->columnCount(parent) : column;
//	const int& mincol = maxcol != column ? 0 : column;
    const int& maxcol = model()->columnCount(parent)-1;
    const int& mincol = 0;
    const int& maxrow = model()->rowCount(QModelIndex())-1;

	// iterate through the current selection, check if row was
	// selected and adjust selection accordingly
	bool bRowWasSelected = false;
	QItemSelection newSel;
	foreach(const QItemSelectionRange& old, d->ranges)
	{
		if (old.parent() != parent)
			newSel.append(old);

		const int& top = old.top();
		const int& bottom = old.bottom();
		if (top < 0 || bottom < 0 || top > maxrow || bottom > maxrow)
			continue;
		int ntop = -1;
		int nbottom = -1;
		if (top != bottom)
		{
			if (row == top)
			{
				// stay in the table and avoid 'negative' selections
				ntop = top+1 > maxrow || top+1 > bottom ? -1 : top+1;
				if (ntop > 0)
				{
					const QModelIndex sidx = model()->index(ntop, mincol, parent);
					const QModelIndex eidx = model()->index(bottom, maxcol, parent);
					newSel.append(QItemSelectionRange(sidx, eidx));
					bRowWasSelected = true;
				}
			}
			else if (row == bottom)
			{
				nbottom = bottom-1;
				const QModelIndex sidx = model()->index(top, mincol, parent);
				const QModelIndex eidx = model()->index(nbottom, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx, eidx));
				bRowWasSelected = true;
			}
			else if (row > top && row < bottom)
			{
				const QModelIndex sidx1 = model()->index(top, mincol, parent);
				const QModelIndex eidx1 = model()->index(row-1, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx1, eidx1));

				const QModelIndex sidx = model()->index(row+1, mincol, parent);
				const QModelIndex eidx = model()->index(bottom, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx, eidx));
				bRowWasSelected = true;
			}
			else
			{
				newSel.append(old);
			}
		}
		else
		{
			if (row != top)
			{
				newSel.append(old);
			}
			else
			{
				bRowWasSelected = true;
			}
		}
	}


	const QModelIndex sidx = model()->index(row, mincol, parent);
	const QModelIndex eidx = model()->index(row, maxcol, parent);
	QItemSelectionRange rowrange(sidx, eidx);

	if (!bRowWasSelected)
	{
		newSel.append(rowrange);
	}

	d->currentSelection.clear();
	d->currentCommand = NoUpdate;
	QItemSelection old = d->ranges;
	d->ranges = newSel;

	//NMDebugCtx(this->objectName().toStdString(), << "done!");
	emit selectionChanged(newSel, old);
}

