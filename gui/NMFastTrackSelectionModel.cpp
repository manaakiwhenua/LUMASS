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
	Q_D(NMFastTrackSelectionModel);

	d->currentSelection.clear();
	d->currentCommand = QItemSelectionModel::NoUpdate;

	QItemSelection old = d->ranges;

	d->ranges.clear();
	d->ranges = newSel;
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
    //
	//NMDebugAI(<< "FASTTRACK: added " << numidx << " of " << numin << " rows" << std::endl);

	emit selectionChanged(d->ranges, old);
}

const QItemSelection
NMFastTrackSelectionModel::getSelection(void)
{
	Q_D(NMFastTrackSelectionModel);


	d->cleanup();
	//d->currentSelection.clear();
	//d->currentCommand = NoUpdate;

	//NMDebugAI(<< "FASTTRACK: ranges num: " << d->ranges.indexes().size() << std::endl);
	//NMDebugAI(<< "FASTTRACK: curSel num: " << d->currentSelection.indexes().size() << std::endl);

	return d->ranges;
}

void
NMFastTrackSelectionModel::toggleRow(int row, int column, const QModelIndex& parent)
{
	Q_D(NMFastTrackSelectionModel);

	const QModelIndex idx = model()->index(row, column, parent);
	if (!idx.isValid())
		return;

	d->cleanup();

	const int& maxcol = column < 0  || column > model()->columnCount(parent)
			?  model()->columnCount(parent) : column;
	const int& mincol = maxcol != column ? 0 : column;
	const int& maxrow = model()->rowCount(QModelIndex());
	QItemSelection newSel;
	foreach(const QItemSelectionRange& old, d->ranges)
	{
		if (old.parent() != parent)
			newSel.append(old);

		const int& top = old.top();
		const int& bottom = old.bottom();
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
				}
			}
			else if (row == bottom)
			{
				nbottom = bottom-1;
				const QModelIndex sidx = model()->index(top, mincol, parent);
				const QModelIndex eidx = model()->index(nbottom, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx, eidx));
			}
			else if (row > top && row < bottom)
			{
				const QModelIndex sidx1 = model()->index(top, mincol, parent);
				const QModelIndex eidx1 = model()->index(row-1, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx1, eidx1));

				const QModelIndex sidx = model()->index(row+1, mincol, parent);
				const QModelIndex eidx = model()->index(bottom, maxcol, parent);
				newSel.append(QItemSelectionRange(sidx, eidx));
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
		}
	}

	const QModelIndex sidx = model()->index(row, mincol, parent);
	const QModelIndex eidx = model()->index(row, maxcol, parent);
	newSel.append(QItemSelectionRange(sidx, eidx));

	QItemSelection old = d->ranges;
	d->ranges = newSel;

	emit selectionChanged(newSel, old);

	//model()
}

