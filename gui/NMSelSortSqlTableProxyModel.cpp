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
      mTempTableName("")
{
    this->setParent(parent);
}

NMSelSortSqlTableProxyModel::~NMSelSortSqlTableProxyModel()
{
}

void
NMSelSortSqlTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    mSourceModel = qobject_cast<QSqlTableModel*>(sourceModel);
}

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
            qstr = QString("Drop %1;").arg(mTempTableName);
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

    // define name for temporary table mapping proxy to source
    // index
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
    qstr = QString("Insert into %1 (source) select %2 from %3 order by %2 %4;")
           .arg(mTempTableName)
           .arg(pk)
           .arg(mSourceModel->tableName())
           .arg(qsSortOrder);

    QSqlQuery qSort(mSourceModel->database());
    if (!qSort.exec(qstr))
    {
        NMErr(ctx, << "Failed populating new mapping table"
                   << " '" << mTempTableName.toStdString() << "'!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    NMDebugAI(<< "proxy <-> source mapping established!" << std::endl);
    NMDebugCtx(ctx, << "done!");
}

QModelIndex
NMSelSortSqlTableProxyModel::mapFromSource(const QModelIndex& srcIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

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
        NMErr(ctx, << "Failed mapping source to proxy index!");
        return QModelIndex();
    }

    if (qProxy.next())
    {
        int r = qProxy.value(0).toInt();
        return this->createIndex(r, srcIdx.column());
    }
    else
    {
        NMErr(ctx, << "Failed mapping source to proxy index!");
    }

    return QModelIndex();
}

QModelIndex
NMSelSortSqlTableProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

    // no need of expensive mapping unless the model has been sorted
    if (mTempTableName.isEmpty())
    {
        return this->index(proxyIdx.row(), proxyIdx.column(), proxyIdx.parent());
    }

    QString qstr = QString("Select source from %1 where proxy = %2;")
                   .arg(mTempTableName)
                   .arg(proxyIdx.row());

    QSqlQuery qProxy(mSourceModel->database());
    if (!qProxy.exec(qstr))
    {
        NMErr(ctx, << "Failed mapping proxy to source index!");
        return QModelIndex();
    }

    if (qProxy.next())
    {
        int r = qProxy.value(0).toInt();
        return this->mSourceModel->index(r, proxyIdx.column(), proxyIdx.parent());
    }
    else
    {
        NMErr(ctx, << "Failed mapping proxy to source index!");
    }


    return QModelIndex();
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
    if (    mSourceModel == 0
        ||  (row < 0 || row > mSourceModel->rowCount() - 1)
        ||  (column < 0 || column > mSourceModel->columnCount() - 1)
       )
    {
        return QModelIndex();
    }

    return this->createIndex(row, column);
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












