/******************************************************************************
* Created by Alexander Herzig
* Copyright 2015 Landcare Research New Zealand Ltd
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
* NMSelSortSqlTableProxyModel.cpp
*
*  Created on: 29/09/2015
*      Author: alex
*/

#include "NMSelSortSqlTableProxyModel.h"
#include "nmlog.h"

#include <QSqlDatabase>
#include <QSqlQuery>

const std::string NMSelSortSqlTableProxyModel::ctx = "NMSelSortSqlTableProxyModel";

NMSelSortSqlTableProxyModel::NMSelSortSqlTableProxyModel(QObject *parent)
    : mSourceModel(0),
      mTempTableName(""),
      mLastFilter(""),
      mLastSelRecsOnly(false)
{
    this->setParent(parent);
    mLastColSort.first = -1;
    mLastColSort.second = Qt::AscendingOrder;
}

NMSelSortSqlTableProxyModel::~NMSelSortSqlTableProxyModel()
{
}

void
NMSelSortSqlTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (mSourceModel)
    {
        disconnect(mSourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SIGNAL(rowsInserted(QModelIndex,int,int)));
        disconnect(mSourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SIGNAL(columnsInserted(QModelIndex,int,int)));

        disconnect(mSourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SIGNAL(rowsRemoved(QModelIndex,int,int)));
        disconnect(mSourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
    }

    mSourceModel = qobject_cast<QSqlTableModel*>(sourceModel);
    if (mSourceModel)
    {
        QAbstractProxyModel::setSourceModel(sourceModel);

        connect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SIGNAL(rowsInserted(QModelIndex,int,int)));
        connect(sourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SIGNAL(columnsInserted(QModelIndex,int,int)));

        connect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SIGNAL(rowsRemoved(QModelIndex,int,int)));
        connect(sourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
    }
}

void
NMSelSortSqlTableProxyModel::sort(int column, Qt::SortOrder order)
{
    if (   mSourceModel == 0
        || (    mLastColSort.first == column
             && mLastColSort.second == order
           )
       )
    {
        return;
    }

    emit layoutAboutToBeChanged();

    mSourceModel->setSort(column, order);
    mSourceModel->select();
    mLastColSort.first = column;
    mLastColSort.second = order;

    emit layoutChanged();
}

QItemSelection
NMSelSortSqlTableProxyModel::selectRows(const QString &queryString, bool showSelRecsOnly)
{
    if (mSourceModel == 0)
    {
        return QItemSelection();
    }

    QString qstr = queryString;

    bool chngLayout = showSelRecsOnly ? true : mLastSelRecsOnly ? true : false;
    if (chngLayout)
    {
        emit layoutAboutToBeChanged();

        this->resetSourceModel();

        mSourceModel->setFilter(qstr);
        if (mLastColSort.first >= 0)
        {
            mSourceModel->setSort(mLastColSort.first, mLastColSort.second);
        }
        mSourceModel->select();
        mLastFilter = qstr;

        emit layoutChanged();

        if (!showSelRecsOnly)
        {
            updateInternalSelection();
            return mInternalSelection;
        }
    }
    // "selection only query"
    else
    {
        mLastFilter = qstr;

        updateInternalSelection();
        return mInternalSelection;
    }

    return QItemSelection();
}

void
NMSelSortSqlTableProxyModel::resetSourceModel()
{
    mSourceModel->setFilter("");
    mSourceModel->setSort(0, Qt::AscendingOrder);
    mSourceModel->select();
}

void
NMSelSortSqlTableProxyModel::clearSelection()
{
    if (mSourceModel)
    {
        emit layoutAboutToBeChanged();
        this->resetSourceModel();
        mLastFilter = "";
        emit layoutChanged();
    }
}

void
NMSelSortSqlTableProxyModel::updateInternalSelection()
{

}

//bool
//NMSelSortSqlTableProxyModel::canFetchMore(const QModelIndex &parent) const
//{
//    if (mSourceModel == 0)
//    {
//        return false;
//    }

//    return mSourceModel->canFetchMore(parent);
//}

//void
//NMSelSortSqlTableProxyModel::fetchMore(const QModelIndex &parent)
//{
//    if (mSourceModel && mSourceModel->canFetchMore(parent))
//    {
//        mSourceModel->fetchMore(parent);
//        this->resetInternalData();
//    }
//}

/*
void
NMSelSortSqlTableProxyModel::sort(int column, Qt::SortOrder order)
{
    NMDebugCtx(ctx, << "...");

    // been here and done it? -> quit
    if (    mSourceModel == 0
        ||  (   mLastColSort.first == column
             && mLastColSort.second == order
            )
       )
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // drop any previously sorted indices
    QString qstr;
    if (!mTempTableName.isEmpty())
    {
        QStringList tables = mSourceModel->database().tables();
        if (tables.contains(mTempTableName, Qt::CaseInsensitive))
        {
            qstr = QString("Drop table if exists %1;").arg(mTempTableName);
            QSqlQuery qobj(mSourceModel->database());
            if (!qobj.exec(qstr))
            {
                NMErr(ctx, << "Failed dropping previous mapping table"
                           << " '" << mTempTableName.toStdString() << "'!");
                NMDebugCtx(ctx, << "done!");
                return;
            }
        }
    }

    // ... it's geting serious now ...
    emit layoutAboutToBeChanged();


    // some info we need later for setting up the mapping table and
    // do the actual sort based on the given column
    QString sOrderColumn = mSourceModel->headerData(column, Qt::Horizontal).toString();
    QString qsSortOrder = order == Qt::AscendingOrder ? "asc" : "desc";
    mTempTableName = QString("s%1%2")
                     .arg(column)
                     .arg(qsSortOrder);

    // create temporary table mapping proxy to source index
    qstr = QString("Create temp table if not exists %1 ")
           .arg(mTempTableName);
    qstr += "(proxy integer primary key, source integer);";

    QSqlQuery qCreate(mSourceModel->database());
    if (!qCreate.exec(qstr))
    {
        NMErr(ctx, << "Failed creating new mapping table"
                   << " '" << mTempTableName.toStdString() << "'!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // populate the new mapping table

    // assuming that the pk is the first column in the source table
    QString pk = mSourceModel->headerData(0, Qt::Horizontal).toString();
    qstr = QString("Insert into %1 (source) select %2 from %3 order by %4 %5;")
           .arg(mTempTableName)
           .arg(pk)
           .arg(mSourceModel->tableName())
           .arg(sOrderColumn)
           .arg(qsSortOrder);

    QSqlQuery qSort(mSourceModel->database());
    if (!qSort.exec(qstr))
    {
        NMErr(ctx, << "Failed populating new mapping table"
                   << " '" << mTempTableName.toStdString() << "'!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    NMDebugAI( << "sorted " << qSort.numRowsAffected()
               << " rows ... " << std::endl);

    mLastColSort.first = column;
    mLastColSort.second = order;

    QStringList tabs = mSourceModel->database().tables();
    NMDebugAI( << "post sort tables: " << tabs.join(' ').toStdString() << std::endl);

    // debug only =========================================================== //

//    qstr = QString("Select * from %1;").arg(mTempTableName);
//    QSqlQuery qTest(mSourceModel->database());
//    qTest.exec(qstr);
//    NMDebugAI( << "   proxy | source" << std::endl);
//    while (qTest.next())
//    {
//        NMDebugAI( << "   " << qTest.value(0).toInt()
//                   << " | " << qTest.value(1).toInt() << std::endl);
//    }

    // debug only ===========================================================

    NMDebugAI(<< "proxy <-> source mapping established!" << std::endl);

    emit layoutChanged();

    NMDebugCtx(ctx, << "done!");
}
*/

QModelIndex
NMSelSortSqlTableProxyModel::mapFromSource(const QModelIndex& srcIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

    return this->mSourceModel->index(srcIdx.row(), srcIdx.column(), srcIdx.parent());

    /*
    // no need of expensive mapping unless the model has been sorted
    if (mTempTableName.isEmpty())
    {
        return this->mSourceModel->index(srcIdx.row(), srcIdx.column(), srcIdx.parent());
    }

    QString qstr = QString("Select proxy from %1 where source = %2;")
                   .arg(mTempTableName)
                   .arg(srcIdx.row());

    QSqlQuery qProxy(mSourceModel->database());
    if (!qProxy.exec(qstr))
    {
        //NMErr(ctx, << "Failed mapping source to proxy index!");
        return QModelIndex();
    }

    if (qProxy.next())
    {
        // NOTE: since we utilise the autoincrementing primary key, the
        // proxy index in the mapping-table is 1-based!
        int r = qProxy.value(0).toInt() - 1;
        return this->createIndex(r, srcIdx.column());
    }

    return QModelIndex();
    */
}

QModelIndex
NMSelSortSqlTableProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

    return mSourceModel->index(proxyIdx.row(), proxyIdx.column(), proxyIdx.parent());

    /*
    // no need of expensive mapping unless the model has been sorted
    if (mTempTableName.isEmpty())
    {
        return this->index(proxyIdx.row(), proxyIdx.column(), proxyIdx.parent());
    }

    // NOTE: since we utilise the autoincrementing primary key, the
    // proxy index in the mapping-table is 1-based!
    QString qstr = QString("Select source from %1 where proxy = %2;")
                   .arg(mTempTableName)
                   .arg(proxyIdx.row()+1);

    QSqlQuery qProxy(qstr, mSourceModel->database());
    if (!qProxy.exec())
    {
        //NMErr(ctx, << "Failed mapping proxy to source index!");
        return QModelIndex();
    }

    if (qProxy.next())
    {
        int r = qProxy.value(0).toInt();
        return this->mSourceModel->index(r, proxyIdx.column(), proxyIdx.parent());
    }

    return QModelIndex();
    */
}

//QItemSelection NMSelSortSqlTableProxyModel::mapSelectionFromSource(const QItemSelection& sourceSelection) const
//{
//    return QItemSelection();
//}

//QItemSelection NMSelSortSqlTableProxyModel::mapSelectionToSource(const QItemSelection& proxySelection) const
//{
//    return QItemSelection();
//}

QModelIndex
NMSelSortSqlTableProxyModel::index(int row, int column, const QModelIndex& parent) const
{
//    if (    mSourceModel == 0
//        ||  (row < 0 || row > mSourceModel->rowCount() - 1)
//        ||  (column < 0 || column > mSourceModel->columnCount() - 1)
//       )
//    {
//        return QModelIndex();
//    }

    if (mSourceModel)
    {
        return mSourceModel->index(row, column, parent);
    }

    return QModelIndex();

    //return this->createIndex(row, column);
}

QModelIndex
NMSelSortSqlTableProxyModel::parent(const QModelIndex& idx) const
{
    return QModelIndex();
}

int
NMSelSortSqlTableProxyModel::rowCount(const QModelIndex& parent) const
{
    if (mSourceModel == 0)
    {
        return 0;
    }

    return mSourceModel->rowCount(parent);
}

int
NMSelSortSqlTableProxyModel::columnCount(const QModelIndex& parent) const
{
    if (mSourceModel == 0)
    {
        return 0;
    }

    return mSourceModel->columnCount(parent);
}

Qt::ItemFlags
NMSelSortSqlTableProxyModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return 0;
}

QVariant
NMSelSortSqlTableProxyModel::headerData(int section, Qt::Orientation orientation,
        int role) const
{
    if (mSourceModel == 0)
    {
        return QVariant();
    }

    if (orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section + 1;
    }

    return this->mSourceModel->headerData(section, orientation, role);
}


QVariant
NMSelSortSqlTableProxyModel::data(const QModelIndex& index, int role) const
{
    if (mSourceModel == 0)
    {
        return QVariant();
    }
    return this->mSourceModel->data(mapToSource(index), role);
}







