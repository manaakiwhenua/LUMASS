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
	long numin = 0;
	foreach(QItemSelectionRange range, newSel)
	{
		numin += range.bottom() - range.top() + 1;
		d->ranges.append(range);
	}

	long numidx=0;
	foreach(const QItemSelectionRange& r, d->ranges)
	{
		numidx += r.bottom() - r.top() + 1;
	}
	if (numidx < 0)
		numidx = 0;

	NMDebugAI(<< "FASTTRACK: added " << numidx << " of " << numin << " rows" << std::endl);

	emit selectionChanged(d->ranges, old);
}

const QItemSelection
NMFastTrackSelectionModel::getSelection(void) const
{
	Q_D(const NMFastTrackSelectionModel);



	NMDebugAI(<< "FASTTRACK: ranges num: " << d->ranges.indexes().size() << std::endl);
	NMDebugAI(<< "FASTTRACK: curSel num: " << d->currentSelection.indexes().size() << std::endl);

	return d->ranges;
}

//void
//NMFastTrackSelectionModel::toggleRow(int row)
//{
//	Q_D(NMFastTrackSelectionModel);
//
//	QItemSelection newSel = d->ranges;
//
//	foreach(QItemSelectionRange r, newSel)
//	{
//		if (row)
//	}
//}

//void
//NMFastTrackSelectionModel::swapSelection(void)
//{
//	//QItem
//
//	QItemSelection invSel;
//	const int maxrow = this->mProxy2Source.size()-1;
//	int invstart = -1;
//	int invend   = -1;
//
//	const int numranges = selection.size();
//	int rnum=0;
//	while(rnum < numranges)
//	{
//		const int top = selection.at(rnum).top();
//		const int bottom = selection.at(rnum).bottom();
//		if (top < 0 || top > maxrow || bottom < 0 || bottom > maxrow)
//			continue;
//
//		if (top == invend+1)
//		{
//			invstart = bottom + 1;
//			if (invstart > maxrow)
//			{
//				break;
//			}
//			invend = invstart;
//		}
//
//		if (rnum < numranges-1)
//		{
//			invend = selection.at(rnum+1).top()-1;
//		}
//		else
//		{
//			invend = maxrow;
//		}
//
//		QModelIndex sidx = this->createIndex(invstart, 0, 0);
//		QModelIndex eidx = this->createIndex(invend, 0, 0);
//		invSel.append(QItemSelectionRange(sidx, eidx));
//
//		++rnum;
//	}
//
//
//	if (	numranges == 0
//		||  (invstart == -1 && invend == -1)
//	   )
//	{
//		invstart = 0;
//		invend = maxrow;
//		QModelIndex sidx = this->createIndex(invstart, 0, 0);
//		QModelIndex eidx = this->createIndex(invend, 0, 0);
//		invSel.append(QItemSelectionRange(sidx, eidx));
//	}
//
//}



/***********************************************************************************
 * 	 COPY OF QT's QItemSelectionPrivate implementation
 ***********************************************************************************/

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/



//void QItemSelectionModelPrivate::initModel(QAbstractItemModel *model)
//{
//    this->model = model;
//    if (model) {
//        Q_Q(QItemSelectionModel);
//        QObject::connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
//                q, SLOT(_q_rowsAboutToBeRemoved(QModelIndex,int,int)));
//        QObject::connect(model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
//                q, SLOT(_q_columnsAboutToBeRemoved(QModelIndex,int,int)));
//        QObject::connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
//                q, SLOT(_q_rowsAboutToBeInserted(QModelIndex,int,int)));
//        QObject::connect(model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
//                q, SLOT(_q_columnsAboutToBeInserted(QModelIndex,int,int)));
//        QObject::connect(model, SIGNAL(layoutAboutToBeChanged()),
//                q, SLOT(_q_layoutAboutToBeChanged()));
//        QObject::connect(model, SIGNAL(layoutChanged()),
//                q, SLOT(_q_layoutChanged()));
//    }
//}
//
///*!
//    \internal
//
//    returns a QItemSelection where all ranges have been expanded to:
//    Rows: left: 0 and right: columnCount()-1
//    Columns: top: 0 and bottom: rowCount()-1
//*/
//
//QItemSelection QItemSelectionModelPrivate::expandSelection(const QItemSelection &selection,
//                                                           QItemSelectionModel::SelectionFlags command) const
//{
//    if (selection.isEmpty() && !((command & QItemSelectionModel::Rows) ||
//                                 (command & QItemSelectionModel::Columns)))
//        return selection;
//
//    QItemSelection expanded;
//    if (command & QItemSelectionModel::Rows) {
//        for (int i = 0; i < selection.count(); ++i) {
//            QModelIndex parent = selection.at(i).parent();
//            int colCount = model->columnCount(parent);
//            QModelIndex tl = model->index(selection.at(i).top(), 0, parent);
//            QModelIndex br = model->index(selection.at(i).bottom(), colCount - 1, parent);
//            //we need to merge because the same row could have already been inserted
//            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
//        }
//    }
//    if (command & QItemSelectionModel::Columns) {
//        for (int i = 0; i < selection.count(); ++i) {
//            QModelIndex parent = selection.at(i).parent();
//            int rowCount = model->rowCount(parent);
//            QModelIndex tl = model->index(0, selection.at(i).left(), parent);
//            QModelIndex br = model->index(rowCount - 1, selection.at(i).right(), parent);
//            //we need to merge because the same column could have already been inserted
//            expanded.merge(QItemSelection(tl, br), QItemSelectionModel::Select);
//        }
//    }
//    return expanded;
//}
//
///*!
//    \internal
//*/
//void QItemSelectionModelPrivate::_q_rowsAboutToBeRemoved(const QModelIndex &parent,
//                                                         int start, int end)
//{
//    Q_Q(QItemSelectionModel);
//    finalize();
//
//    // update current index
//    if (currentIndex.isValid() && parent == currentIndex.parent()
//        && currentIndex.row() >= start && currentIndex.row() <= end) {
//        QModelIndex old = currentIndex;
//        if (start > 0) // there are rows left above the change
//            currentIndex = model->index(start - 1, old.column(), parent);
//        else if (model && end < model->rowCount(parent) - 1) // there are rows left below the change
//            currentIndex = model->index(end + 1, old.column(), parent);
//        else // there are no rows left in the table
//            currentIndex = QModelIndex();
//        emit q->currentChanged(currentIndex, old);
//        emit q->currentRowChanged(currentIndex, old);
//        if (currentIndex.column() != old.column())
//            emit q->currentColumnChanged(currentIndex, old);
//    }
//
//    QItemSelection deselected;
//    QItemSelection newParts;
//    QItemSelection::iterator it = ranges.begin();
//    while (it != ranges.end()) {
//        if (it->topLeft().parent() != parent) {  // Check parents until reaching root or contained in range
//            QModelIndex itParent = it->topLeft().parent();
//            while (itParent.isValid() && itParent.parent() != parent)
//                itParent = itParent.parent();
//
//            if (itParent.isValid() && start <= itParent.row() && itParent.row() <= end) {
//                deselected.append(*it);
//                it = ranges.erase(it);
//            } else {
//                ++it;
//            }
//        } else if (start <= it->bottom() && it->bottom() <= end    // Full inclusion
//                   && start <= it->top() && it->top() <= end) {
//            deselected.append(*it);
//            it = ranges.erase(it);
//        } else if (start <= it->top() && it->top() <= end) {      // Top intersection
//            deselected.append(QItemSelectionRange(it->topLeft(), model->index(end, it->left(), it->parent())));
//            *it = QItemSelectionRange(model->index(end + 1, it->left(), it->parent()), it->bottomRight());
//            ++it;
//        } else if (start <= it->bottom() && it->bottom() <= end) {    // Bottom intersection
//            deselected.append(QItemSelectionRange(model->index(start, it->right(), it->parent()), it->bottomRight()));
//            *it = QItemSelectionRange(it->topLeft(), model->index(start - 1, it->right(), it->parent()));
//            ++it;
//        } else if (it->top() < start && end < it->bottom()) { // Middle intersection
//            // If the parent contains (1, 2, 3, 4, 5, 6, 7, 8) and [3, 4, 5, 6] is selected,
//            // and [4, 5] is removed, we need to split [3, 4, 5, 6] into [3], [4, 5] and [6].
//            // [4, 5] is appended to deselected, and [3] and [6] remain part of the selection
//            // in ranges.
//            const QItemSelectionRange removedRange(model->index(start, it->right(), it->parent()),
//                                                    model->index(end, it->left(), it->parent()));
//            deselected.append(removedRange);
//            QItemSelection::split(*it, removedRange, &newParts);
//            it = ranges.erase(it);
//        } else
//            ++it;
//    }
//    ranges.append(newParts);
//
//    if (!deselected.isEmpty())
//        emit q->selectionChanged(QItemSelection(), deselected);
//}
//
///*!
//    \internal
//*/
//void QItemSelectionModelPrivate::_q_columnsAboutToBeRemoved(const QModelIndex &parent,
//                                                            int start, int end)
//{
//    Q_Q(QItemSelectionModel);
//
//    // update current index
//    if (currentIndex.isValid() && parent == currentIndex.parent()
//        && currentIndex.column() >= start && currentIndex.column() <= end) {
//        QModelIndex old = currentIndex;
//        if (start > 0) // there are columns to the left of the change
//            currentIndex = model->index(old.row(), start - 1, parent);
//        else if (model && end < model->columnCount() - 1) // there are columns to the right of the change
//            currentIndex = model->index(old.row(), end + 1, parent);
//        else // there are no columns left in the table
//            currentIndex = QModelIndex();
//        emit q->currentChanged(currentIndex, old);
//        if (currentIndex.row() != old.row())
//            emit q->currentRowChanged(currentIndex, old);
//        emit q->currentColumnChanged(currentIndex, old);
//    }
//
//    // update selections
//    QModelIndex tl = model->index(0, start, parent);
//    QModelIndex br = model->index(model->rowCount(parent) - 1, end, parent);
//    q->select(QItemSelection(tl, br), QItemSelectionModel::Deselect);
//    finalize();
//}
//
///*!
//    \internal
//
//    Split selection ranges if columns are about to be inserted in the middle.
//*/
//void QItemSelectionModelPrivate::_q_columnsAboutToBeInserted(const QModelIndex &parent,
//                                                             int start, int end)
//{
//    Q_UNUSED(end);
//    finalize();
//    QList<QItemSelectionRange> split;
//    QList<QItemSelectionRange>::iterator it = ranges.begin();
//    for (; it != ranges.end(); ) {
//        if ((*it).isValid() && (*it).parent() == parent
//            && (*it).left() < start && (*it).right() >= start) {
//            QModelIndex bottomMiddle = model->index((*it).bottom(), start - 1, (*it).parent());
//            QItemSelectionRange left((*it).topLeft(), bottomMiddle);
//            QModelIndex topMiddle = model->index((*it).top(), start, (*it).parent());
//            QItemSelectionRange right(topMiddle, (*it).bottomRight());
//            it = ranges.erase(it);
//            split.append(left);
//            split.append(right);
//        } else {
//            ++it;
//        }
//    }
//    ranges += split;
//}
//
///*!
//    \internal
//
//    Split selection ranges if rows are about to be inserted in the middle.
//*/
//void QItemSelectionModelPrivate::_q_rowsAboutToBeInserted(const QModelIndex &parent,
//                                                          int start, int end)
//{
//    Q_UNUSED(end);
//    finalize();
//    QList<QItemSelectionRange> split;
//    QList<QItemSelectionRange>::iterator it = ranges.begin();
//    for (; it != ranges.end(); ) {
//        if ((*it).isValid() && (*it).parent() == parent
//            && (*it).top() < start && (*it).bottom() >= start) {
//            QModelIndex middleRight = model->index(start - 1, (*it).right(), (*it).parent());
//            QItemSelectionRange top((*it).topLeft(), middleRight);
//            QModelIndex middleLeft = model->index(start, (*it).left(), (*it).parent());
//            QItemSelectionRange bottom(middleLeft, (*it).bottomRight());
//            it = ranges.erase(it);
//            split.append(top);
//            split.append(bottom);
//        } else {
//            ++it;
//        }
//    }
//    ranges += split;
//}
//
///*!
//    \internal
//
//    Split selection into individual (persistent) indexes. This is done in
//    preparation for the layoutChanged() signal, where the indexes can be
//    merged again.
//*/
//void QItemSelectionModelPrivate::_q_layoutAboutToBeChanged()
//{
//    savedPersistentIndexes.clear();
//    savedPersistentCurrentIndexes.clear();
//
//    // optimization for when all indexes are selected
//    // (only if there is lots of items (1000) because this is not entirely correct)
//    if (ranges.isEmpty() && currentSelection.count() == 1) {
//        QItemSelectionRange range = currentSelection.first();
//        QModelIndex parent = range.parent();
//        tableRowCount = model->rowCount(parent);
//        tableColCount = model->columnCount(parent);
//        if (tableRowCount * tableColCount > 1000
//            && range.top() == 0
//            && range.left() == 0
//            && range.bottom() == tableRowCount - 1
//            && range.right() == tableColCount - 1) {
//            tableSelected = true;
//            tableParent = parent;
//            return;
//        }
//    }
//    tableSelected = false;
//
//    QModelIndexList indexes = ranges.indexes();
//    QModelIndexList::const_iterator it;
//    for (it = indexes.constBegin(); it != indexes.constEnd(); ++it)
//        savedPersistentIndexes.append(QPersistentModelIndex(*it));
//    indexes = currentSelection.indexes();
//    for (it = indexes.constBegin(); it != indexes.constEnd(); ++it)
//        savedPersistentCurrentIndexes.append(QPersistentModelIndex(*it));
//}
//
///*!
//    \internal
//
//    Merges \a indexes into an item selection made up of ranges.
//    Assumes that the indexes are sorted.
//*/
//static QItemSelection mergeIndexes(const QList<QPersistentModelIndex> &indexes)
//{
//    QItemSelection colSpans;
//    // merge columns
//    int i = 0;
//    while (i < indexes.count()) {
//        QModelIndex tl = indexes.at(i);
//        QModelIndex br = tl;
//        while (++i < indexes.count()) {
//            QModelIndex next = indexes.at(i);
//            if ((next.parent() == br.parent())
//                 && (next.row() == br.row())
//                 && (next.column() == br.column() + 1))
//                br = next;
//            else
//                break;
//        }
//        colSpans.append(QItemSelectionRange(tl, br));
//    }
//    // merge rows
//    QItemSelection rowSpans;
//    i = 0;
//    while (i < colSpans.count()) {
//        QModelIndex tl = colSpans.at(i).topLeft();
//        QModelIndex br = colSpans.at(i).bottomRight();
//        QModelIndex prevTl = tl;
//        while (++i < colSpans.count()) {
//            QModelIndex nextTl = colSpans.at(i).topLeft();
//            QModelIndex nextBr = colSpans.at(i).bottomRight();
//
//            if (nextTl.parent() != tl.parent())
//                break; // we can't merge selection ranges from different parents
//
//            if ((nextTl.column() == prevTl.column()) && (nextBr.column() == br.column())
//                && (nextTl.row() == prevTl.row() + 1) && (nextBr.row() == br.row() + 1)) {
//                br = nextBr;
//                prevTl = nextTl;
//            } else {
//                break;
//            }
//        }
//        rowSpans.append(QItemSelectionRange(tl, br));
//    }
//    return rowSpans;
//}
//
///*!
//    \internal
//
//    Merge the selected indexes into selection ranges again.
//*/
//void QItemSelectionModelPrivate::_q_layoutChanged()
//{
//    // special case for when all indexes are selected
//    if (tableSelected && tableColCount == model->columnCount(tableParent)
//        && tableRowCount == model->rowCount(tableParent)) {
//        ranges.clear();
//        currentSelection.clear();
//        int bottom = tableRowCount - 1;
//        int right = tableColCount - 1;
//        QModelIndex tl = model->index(0, 0, tableParent);
//        QModelIndex br = model->index(bottom, right, tableParent);
//        currentSelection << QItemSelectionRange(tl, br);
//        tableParent = QModelIndex();
//        tableSelected = false;
//        return;
//    }
//
//    if (savedPersistentCurrentIndexes.isEmpty() && savedPersistentIndexes.isEmpty()) {
//        // either the selection was actually empty, or we
//        // didn't get the layoutAboutToBeChanged() signal
//        return;
//    }
//    // clear the "old" selection
//    ranges.clear();
//    currentSelection.clear();
//
//    // sort the "new" selection, as preparation for merging
//    qStableSort(savedPersistentIndexes.begin(), savedPersistentIndexes.end());
//    qStableSort(savedPersistentCurrentIndexes.begin(), savedPersistentCurrentIndexes.end());
//
//    // update the selection by merging the individual indexes
//    ranges = mergeIndexes(savedPersistentIndexes);
//    currentSelection = mergeIndexes(savedPersistentCurrentIndexes);
//
//    // release the persistent indexes
//    savedPersistentIndexes.clear();
//    savedPersistentCurrentIndexes.clear();
//}
//
//
