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
#include <cstdio>
#include <ctime>
#include <random>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlIndex>
#include <QSqlError>
#include <QSqlDriver>
#include <QUuid>

#include "nmqsql_sqlite_p.h"

#include "NMGlobalHelper.h"

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

const std::string NMSelSortSqlTableProxyModel::ctx = "NMSelSortSqlTableProxyModel";

NMSelSortSqlTableProxyModel::NMSelSortSqlTableProxyModel(QObject *parent)
    : mSourceModel(nullptr),
      mLastFilter(""),
      mLastSelRecsOnly(false),
      mUpdateProxySelection(false),
      mUpdateSourceSelection(false),
      mLastSelCount(0),
      mTempTableName(""),
      mSourcePK(""),
      mProxyPK(""),
      mPKIsFstCol(true),
      mMinPKVal(1)
{
    this->setParent(parent);
    mLastColSort.first = -1;
    mLastColSort.second = Qt::AscendingOrder;
    mTempTableName = this->getRandomString();
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(true);

    connect(mLogger, SIGNAL(sendLogMsg(QString)), NMGlobalHelper::getLogWidget(),
            SLOT(insertHtml(QString)));

}

NMSelSortSqlTableProxyModel::~NMSelSortSqlTableProxyModel()
{
    this->mSourceModel = 0;
}

void
NMSelSortSqlTableProxyModel::removeTempTables(void)
{
    if (!openWriteModel())
    {
        // assume we're done!
        return;
    }
    QSqlDriver* drv = mSourceModel->database().driver();
    QString delQuery = QString("drop table if exists %1")
            .arg(drv->escapeIdentifier(mTempTableName, QSqlDriver::TableName));
    QSqlQuery dq(mSourceModel->database());
    if (!dq.exec(delQuery))
    {
        NMLogError(<< "Failed removing temp tables from '"
                   << mSourceModel->getDatabaseName().toStdString()
                   << "': "
                   << dq.lastError().text().toStdString());
    }
    dq.finish();
    openReadModel();
}

void
NMSelSortSqlTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{

    // disconnect current model from proxy signals
//    if (mSourceModel)
//    {
//        disconnect(mSourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
//                   this, SIGNAL(rowsInserted(QModelIndex,int,int)));
//        disconnect(mSourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)),
//                   this, SIGNAL(columnsInserted(QModelIndex,int,int)));

//        disconnect(mSourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
//                   this, SIGNAL(rowsRemoved(QModelIndex,int,int)));
//        disconnect(mSourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)),
//                   this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
//    }

    mSourceModel = qobject_cast<NMSqlTableModel*>(sourceModel);
    if (mSourceModel == 0)
    {
        return;
    }

    // connect new model to proxy signals
    QAbstractProxyModel::setSourceModel(sourceModel);
    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)),
               sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)));
    connect(this, SIGNAL(columnsInserted(QModelIndex,int,int)),
               sourceModel, SIGNAL(columnsInserted(QModelIndex,int,int)));
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
               sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

    connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)),
               sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    connect(this, SIGNAL(columnsRemoved(QModelIndex,int,int)),
               sourceModel, SIGNAL(columnsRemoved(QModelIndex,int,int)));

    mSourcePK = getSourcePK();
    mProxyPK = "nm_proxy_id";
}

QItemSelection
NMSelSortSqlTableProxyModel::getProxySelection()
{
    if (mUpdateProxySelection)
    {
        updateSelection(mProxySelection, true);
    }
    return mProxySelection;
}

QItemSelection
NMSelSortSqlTableProxyModel::getSourceSelection()
{
    if (mUpdateSourceSelection)
    {
        updateSelection(mSourceSelection, false);
    }
    return mSourceSelection;
}

QItemSelection
NMSelSortSqlTableProxyModel::getSelectAll()
{
    QItemSelection sel;
    if (mSourceModel == 0)
    {
        return sel;
    }

    QModelIndex top = this->createIndex(0, 0);
    QModelIndex bottom = this->createIndex(getNumTableRecords()-1, mSourceModel->columnCount()-1);
    sel.append(QItemSelectionRange(top, bottom));
    return sel;
}

bool
NMSelSortSqlTableProxyModel::createColumnIndex(int colidx)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    if (!this->openWriteModel())
    {
        return false;
    }

    QSqlDriver* drv = mSourceModel->database().driver();
    QString colname = mSourceModel->headerData(
                            colidx, Qt::Horizontal).toString();
    QString escpCol = drv->escapeIdentifier(colname,
                                            QSqlDriver::FieldName);

    QString idxname = QString("%1_idx").arg(colname);
    QString escpIdx = drv->escapeIdentifier(idxname,
                                            QSqlDriver::FieldName);

    QString escpTabName = drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName);

    QString idxQuery = QString("Create index if not exists %1 on %2 (%3)")
                .arg(escpIdx).arg(escpTabName).arg(escpCol);
    NMLogDebug(<< ctx << ": idx creation query: " << idxQuery.toStdString() << " on " << colname.toStdString());

    QString table = mSourceModel->tableName();
    mSourceModel->clear();

    QSqlDatabase db = mSourceModel->database();
    db.transaction();
    QSqlQuery qobj(db);
    if (!qobj.exec(idxQuery))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << qobj.lastError().text().toStdString());
        qobj.finish();
        qobj.clear();
        db.rollback();
        openReadModel();
        return false;
    }
    qobj.finish();
    qobj.clear();
    db.commit();
    openReadModel();

    this->updateSourceModel(table);

    return true;
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



    if (!mSourceModel->select())
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : failed sorting column #" << column << " \n"
                  << "Try indexing the column and try again.");
    }

    mLastColSort.first = column;
    mLastColSort.second = order;

    emit layoutChanged();

    mUpdateProxySelection = true;
    mUpdateSourceSelection = true;

    // create new mapping table here
    if (!createMappingTable())
    {
        // complain ...

        return;
    }
}

bool
NMSelSortSqlTableProxyModel::selectRows(const QString &queryString, bool showSelRecsOnly)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    QString qstr = queryString;

    bool chngLayout = showSelRecsOnly ? true : mLastSelRecsOnly ? true : false;
    mLastSelRecsOnly = showSelRecsOnly;

    if (chngLayout)
    {
        emit layoutAboutToBeChanged();

        this->resetSourceModel();

        if (showSelRecsOnly)
        {
            mSourceModel->setFilter(qstr);
        }
        else
        {
            mSourceModel->setFilter(QString(""));
        }

        if (mLastColSort.first >= 0)
        {
            mSourceModel->setSort(mLastColSort.first, mLastColSort.second);
        }

        // if the filter is invalid, we just re-set
        // the previous selection and get out of here
        if (!mSourceModel->select())
        {
            mSourceModel->setFilter(mLastFilter);
            mSourceModel->select();
            emit layoutChanged();
            return false;
        }

        emit layoutChanged();
    }

    mLastFilter = qstr;
    if (!mLastFilter.isEmpty())
    {
        mUpdateProxySelection = true;
        mUpdateSourceSelection = true;
    }
    else
    {
        clearSelection();
    }
    return true;
}

void
NMSelSortSqlTableProxyModel::resetSourceModel()
{
    mSourceModel->setFilter("");
    //mSourceModel->setSort(0, Qt::AscendingOrder);
    mSourceModel->select();
}

void
NMSelSortSqlTableProxyModel::clearSelection(void)
{
    if (mSourceModel)
    {
        emit layoutAboutToBeChanged();
        this->resetSourceModel();
        mLastFilter = "";
        mProxySelection.clear();
        mSourceSelection.clear();
        mUpdateProxySelection = false;
        mUpdateSourceSelection = false;
        mLastSelCount = 0;
        emit layoutChanged();
    }
}

QString
NMSelSortSqlTableProxyModel::getSourcePK()
{
    QString primaryKey = "";
    if (mSourceModel == 0)
    {
        return primaryKey;
    }

    QSqlIndex pk = mSourceModel->primaryKey();
    if (!pk.isEmpty())
    {
        primaryKey = pk.fieldName(0);
    }
    else
    {
        // look for  rowidx field
        bool bfound = false;
        for (int i=0; i < this->columnCount(); ++i)
        {
            primaryKey = this->mSourceModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            if (primaryKey.compare("rowidx") == 0)
            {
                bfound = true;
                break;
            }
        }

        if (!bfound)
        {
            primaryKey = "rowid";
        }
    }

    QString fstCol = mSourceModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    if (primaryKey.compare(fstCol, Qt::CaseInsensitive) != 0)
    {
        mPKIsFstCol = false;
    }
    else
    {
        mPKIsFstCol = true;
    }

    QSqlDriver* drv = mSourceModel->database().driver();

    // get the min PK value (0- or 1-based)
    QString queryStr = QString("select min(%1) from %2")
                       .arg(drv->escapeIdentifier(primaryKey, QSqlDriver::FieldName))
                       .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName));

    QSqlQuery queryObj(mSourceModel->database());
    if (!queryObj.exec(queryStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << queryObj.lastError().text().toStdString() << std::endl);
        queryObj.finish();
        return primaryKey;
    }

    if (queryObj.next())
    {
        mMinPKVal = queryObj.value(0).toInt();
        NMLogDebug(<< ctx << ": min PK value = " << mMinPKVal);
    }
    queryObj.finish();

    return primaryKey;
}

bool
NMSelSortSqlTableProxyModel::updateSelection(QItemSelection& sel, bool bProxySelection)
{
    if (mLastFilter.isEmpty())// && mLastColSort.first == -1)
    {
        sel.clear();
        // nothing to update here, really
        return true;
    }

    // ==========================================================================
    //  DEFINE SELECTION UPDATE QUERY
    // ==========================================================================

    // check, whether we've got a mapping table at all
    bool tmpTable = false;
    //QStringList allTables = mTempDb.tables();
    QStringList allTables = mSourceModel->database().tables();
    if (!allTables.contains(mTempTableName))
    {
        if (!this->createMappingTable())
        {
            NMLogError(<< ctx << "::" << __FUNCTION__ << "() : "
                       << "Failed to create selection mapping table!");
            return false;
        }
    }
    tmpTable = true;

    // define query items
    QString selectColumn = mSourcePK;
    QString tableName = mSourceModel->tableName();
    int proxyCorr = -1 * mMinPKVal;

    if (bProxySelection && tmpTable)
    {
        selectColumn = mProxyPK;
        proxyCorr = -1;
        tableName = mTempTableName;
    }

    QString whereClause = QString("where %1").arg(mLastFilter);
    QString orderByClause = QString("order by %1 asc").arg(selectColumn);
    QAbstractItemModel* model = this;

    QSqlDriver* drv = mSourceModel->database().driver();
    QString queryStr = QString("select %1 from %2 %3 %4")
                       .arg(drv->escapeIdentifier(selectColumn, QSqlDriver::FieldName))
                       .arg(drv->escapeIdentifier(tableName, QSqlDriver::TableName))
                       .arg(whereClause)
                       .arg(orderByClause);

    QSqlDatabase srcDb = mSourceModel->database();
    srcDb.transaction();
    QSqlQuery queryObj(srcDb);
    queryObj.setForwardOnly(true);
    if (!queryObj.exec(queryStr))
    {
        queryObj.finish();
        queryObj.clear();
        srcDb.rollback();
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << queryObj.lastError().text().toStdString() << std::endl);
        return false;
    }

    // ==========================================================================
    //  UPDATE ITEM SELECTION
    // ==========================================================================

    // init some vars
    sel.clear();
    mLastSelCount = 0;
    int mincol = 0;
    int maxcol = model->columnCount()-1;

    // select the first row
    int top = -1;
    int bottom = -1;

    // select the first reported record
    if (queryObj.next())
    {
        top = queryObj.value(0).toInt() + proxyCorr;
        bottom = top;
    }

    // extend selection as appropriate and/or create new ones
    while (queryObj.next())
    {
        const int v = queryObj.value(0).toInt() + proxyCorr;
        if (v == bottom + 1)
        {
            bottom = v;
        }
        // write selection range and start new range
        else
        {
            //NMDebugAI(<< "#" << cnt << ": " << top << " - " << bottom << std::endl);
            mLastSelCount += bottom - top + 1;
            const QModelIndex tl = model->index(top, mincol, QModelIndex());
            const QModelIndex br = model->index(bottom, maxcol, QModelIndex());
            sel.append(QItemSelectionRange(tl, br));
            top = v;
            bottom = v;
        }
    }
    queryObj.finish();
    queryObj.clear();
    srcDb.commit();

    // close the last open selection, if any ...
    if (top != -1 && bottom != -1)
    {
        mLastSelCount += bottom - top + 1;
        const QModelIndex tl = model->index(top, mincol, QModelIndex());
        const QModelIndex br = model->index(bottom, maxcol, QModelIndex());
        sel.append(QItemSelectionRange(tl, br));
    }

    if (bProxySelection)
    {
        mUpdateProxySelection = false;
    }
    else
    {
        mUpdateSourceSelection = false;
    }

    return true;
}

QString
NMSelSortSqlTableProxyModel::getRandomString(int len)
{
    if (len < 1)
    {
        return QString();
    }

    std::random_device rand_rd;
    std::mt19937 rand_mt(rand_rd());
    std::uniform_int_distribution<int> rand_1_1e6(1, 1e6);
    std::uniform_int_distribution<int> rand_48_57(48, 57);
    std::uniform_int_distribution<int> rand_65_90(65, 90);
    std::uniform_int_distribution<int> rand_97_122(97, 122);

    //std::srand(std::time(0));
    char* nam = new char[len+1];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (rand_1_1e6(rand_mt) % 2 == 0)
            {
                nam[i] = rand_65_90(rand_mt);
            }
            else
            {
                nam[i] = rand_97_122(rand_mt);
            }
        }
        else
        {
            if (rand_1_1e6(rand_mt) % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (rand_1_1e6(rand_mt) % 5 == 0)
            {
                nam[i] = rand_65_90(rand_mt);
            }
            else if (rand_1_1e6(rand_mt) % 3 == 0)
            {
                nam[i] = rand_97_122(rand_mt);
            }
            else
            {
                nam[i] = rand_48_57(rand_mt);
            }
        }
    }
    nam[len] = '\0';
    QString ret = nam;
    delete[] nam;


    return ret;
}

bool
NMSelSortSqlTableProxyModel::insertColumn(const QString& name,
                                          const QVariant::Type& type)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    if (    name.isEmpty()
        ||  type == QVariant::Invalid
       )
    {
        return false;
    }

    if (!this->openWriteModel())
    {
        //endInsertColumns();
        return false;
    }


    int colidx = mSourceModel->columnCount();
    QString table = mSourceModel->tableName();

    QString typeStr;
    switch (type)
    {
    case QVariant::Int:
    case QVariant::LongLong:
        typeStr = "INTEGER";
        break;
    case QVariant::Double:
        typeStr = "REAL";
        break;

    case QVariant::String:
    default:
        typeStr = "TEXT";
        break;
    }

    beginInsertColumns(QModelIndex(), colidx, colidx);

    QSqlDriver* drv = mSourceModel->database().driver();
    //mSourceModel->clear();
    QString qStr = QString("Alter table %1 add column %2 %3")
                        .arg(drv->escapeIdentifier(table, QSqlDriver::TableName))
                        .arg(drv->escapeIdentifier(name, QSqlDriver::FieldName))
                        .arg(typeStr);

    QSqlDatabase db = mSourceModel->database();
    db.transaction();
    QSqlQuery q(db);
    bool ret = true;
    if (!q.exec(qStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << q.lastError().text().toStdString());
        db.rollback();
        ret = false;
    }
    q.finish();
    q.clear();
    db.commit();

    openReadModel();
    this->updateSourceModel(table);

    endInsertColumns();

    return ret;
}

bool
NMSelSortSqlTableProxyModel::joinTable(const QString& joinTableName,
                                       const QString& joinFieldName,
                                       const QString& tarFieldName)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    int colidx = mSourceModel->columnCount();
    QString tarTableName = mSourceModel->tableName();

    beginInsertColumns(QModelIndex(), colidx, colidx);
    mSourceModel->clear();



    // -------------------------------------------------------------
    // first, we create a real sqlite temporary table from the
    // spatialite virtualtable

    QString tempJoinTableName = this->getRandomString(10);

    QSqlDatabase db = mSourceModel->database();
    QSqlDriver* drv = db.driver();
    std::stringstream ssql;
    ssql << "CREATE TEMP TABLE " << tempJoinTableName.toStdString() << " AS "
         << "SELECT * FROM " << drv->escapeIdentifier(joinTableName, QSqlDriver::TableName).toStdString() << ";";

    db.transaction();
    QSqlQuery query(mSourceModel->database());
    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMLogDebug(<< ctx << ": " << " last query:\n" << ssql.str());
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - create temp table: " << query.lastError().text().toStdString());
        query.finish();
        query.clear();
        db.rollback();
        return false;
    }
    query.finish();
    query.clear();
    db.commit();

    QString idxName = QString("%1_idx").arg(joinFieldName);
    ssql.str("");
    ssql << "CREATE INDEX IF NOT EXISTS " << drv->escapeIdentifier(idxName, QSqlDriver::FieldName).toStdString()
         << " on " << drv->escapeIdentifier(tempJoinTableName, QSqlDriver::TableName).toStdString()
         << " (" << drv->escapeIdentifier(joinFieldName, QSqlDriver::FieldName).toStdString() << ");";

    db.transaction();
    QSqlQuery idxQuery(db);
    if (!idxQuery.exec(QString(ssql.str().c_str())))
    {
        NMLogDebug(<< ctx << ": " << "last query:\n" << ssql.str() << std::endl);
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - create index: " << idxQuery.lastError().text().toStdString());
        idxQuery.finish();
        idxQuery.clear();
        db.rollback();
        return false;
    }
    idxQuery.finish();
    idxQuery.clear();
    db.commit();


    // ---------------------------------------------------------------
    // now, we create temporary join table ...

    QString tempTableName = this->getRandomString(10);

    ssql.str("");
    ssql << "CREATE TEMP TABLE " << tempTableName.toStdString() << " AS "
         << "SELECT * FROM " << drv->escapeIdentifier(tarTableName, QSqlDriver::TableName).toStdString() << " "
         << "LEFT OUTER JOIN " << drv->escapeIdentifier(tempJoinTableName, QSqlDriver::TableName).toStdString() << " "
         << " ON " << drv->escapeIdentifier(tarTableName, QSqlDriver::TableName).toStdString()
         << "." << drv->escapeIdentifier(tarFieldName, QSqlDriver::FieldName).toStdString()
         << " = " << drv->escapeIdentifier(tempJoinTableName, QSqlDriver::TableName).toStdString()
         << "." << drv->escapeIdentifier(joinFieldName, QSqlDriver::FieldName).toStdString()
         << ";";


    db.transaction();
    QSqlQuery joinTable(mSourceModel->database());
    if (!joinTable.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - create temp joined table: " << joinTable.lastError().text().toStdString());
        joinTable.finish();
        joinTable.clear();
        db.rollback();
        return false;
    }
    joinTable.finish();
    joinTable.clear();
    db.commit();

    ssql.str("");
    ssql << "DROP TABLE " << drv->escapeIdentifier(tarTableName, QSqlDriver::TableName).toStdString() << ";";
    db.transaction();
    QSqlQuery dropTable(db);
    if (!dropTable.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - drop original target table: " << dropTable.lastError().text().toStdString());
        dropTable.finish();
        dropTable.clear();
        db.rollback();
        return false;
    }
    dropTable.finish();
    dropTable.clear();
    db.commit();


    ssql.str("");
    ssql << "CREATE TABLE " << drv->escapeIdentifier(tarTableName, QSqlDriver::TableName).toStdString() << " AS "
         << "SELECT * FROM " << tempTableName.toStdString() << ";";
    db.transaction();
    QSqlQuery tarTable(db);
    if (!tarTable.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - create joined target table: " << tarTable.lastError().text().toStdString());
        tarTable.finish();
        tarTable.clear();
        db.rollback();
        return false;
    }
    tarTable.finish();
    tarTable.clear();
    db.commit();


    mSourceModel->setTable(drv->escapeIdentifier(tarTableName, QSqlDriver::TableName));
    if (mLastColSort.first >= 0)
    {
        mSourceModel->setSort(mLastColSort.first, mLastColSort.second);
    }
    if (mLastSelRecsOnly)
    {
        mSourceModel->setFilter(mLastFilter);
    }
    else
    {
        mSourceModel->setFilter("");
    }
    mSourceModel->select();
    mUpdateProxySelection = true;
    mUpdateSourceSelection = true;

    endInsertColumns();

    return true;
}

bool
NMSelSortSqlTableProxyModel::addRow()
{
    return addRows(1);
}

bool
NMSelSortSqlTableProxyModel::openWriteModel(void)
{
    if (this->mSourceModel == nullptr)
    {
        return false;
    }

//    QString dbname = mSourceModel->database().databaseName();
//    if (dbname.compare(NMGlobalHelper::getMainWindow()->getSessionDbFileName()) == 0)
//    {
//        return true;
//    }

    const QString connectOptions = QStringLiteral("QSQLITE_ENABLE_SHARED_CACHE;"
                                                  "QSQLITE_INIT_SPATIALITE;"
                                                  "QSQLITE_OPEN_URI");
    QSqlDatabase db = mSourceModel->database();
    if (db.isValid() && db.isOpen())
    {
        if (!db.connectOptions().contains(QStringLiteral("READONLY")))
        {
            return true;
        }
    }

//    QString cname = NMGlobalHelper::getMainWindow()->getDbConnection(dbname);
//    QSqlDatabase db = QSqlDatabase::database(cname);


    db.close();
    db.setConnectOptions(connectOptions);
    if (!db.open())
    {
        NMLogError(<< this->ctx
            << ": Failed opening write connection to the table: "
            << db.lastError().text().toStdString());
        return false;
    }

    return true;
}

void
NMSelSortSqlTableProxyModel::openReadModel(void)
{
    if (this->mSourceModel == nullptr)
    {
        return;
    }

//    QString dbname = mSourceModel->database().databaseName();
//    if (dbname.compare(NMGlobalHelper::getMainWindow()->getSessionDbFileName()) == 0)
//    {
//        return;
//    }

    const QString connectOptions = QStringLiteral("QSQLITE_OPEN_READONLY;"
                                                  "QSQLITE_ENABLE_SHARED_CACHE;"
                                                  "QSQLITE_INIT_SPATIALITE;"
                                                  "QSQLITE_OPEN_URI");
    QSqlDatabase db = mSourceModel->database();
    if (db.isValid() && db.isOpen())
    {
        if (db.connectOptions().contains(QStringLiteral("READONLY")))
        {
            return;
        }
    }

    db.close();
    db.setConnectOptions(connectOptions);

//    QString cname = NMGlobalHelper::getMainWindow()->getDbConnection(dbname, false);
//    QSqlDatabase db = QSqlDatabase::database(cname);

    if (!db.open())
    {
        NMLogError(<< this->ctx
            << ": Failed opening a read-only connection to the table: "
            << db.lastError().text().toStdString());
        return;
    }
    mSourceModel->select();

    return;
}


bool
NMSelSortSqlTableProxyModel::addRows(unsigned int nrows)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    if (!this->openWriteModel())
    {
        return false;
    }

    // query max pk value
    QString idColName;
    if (!mPKIsFstCol)
    {
        QModelIndex mid = mSourceModel->index(0, 0);
        QVariant idVal = mSourceModel->data(mid, Qt::DisplayRole);
        if (    idVal.type() == QVariant::String
            ||  idVal.type() == QVariant::Bool
            ||  idVal.type() == QVariant::Char
            ||  idVal.type() == QVariant::ByteArray
           )
        {
            NMLogError(<< ctx << ": " << "Failed to add rows! Please double check primary key!");
            return false;
        }
        idColName = mSourceModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    }
    else
    {
        idColName = this->mSourcePK;
    }

    QSqlDriver* drv = mSourceModel->database().driver();
    QString maxPKSql = QString("SELECT max(%1) from %2")
                            .arg(drv->escapeIdentifier(idColName, QSqlDriver::FieldName))
                            .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName));

    QSqlQuery qMaxPK(mSourceModel->database());
    if (!qMaxPK.exec(maxPKSql))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - select max index value: " << qMaxPK.lastError().text().toStdString()
                   << qMaxPK.lastError().text().toStdString());
        qMaxPK.finish();
        this->openReadModel();
        return false;
    }
    qMaxPK.next();
    int maxPK = qMaxPK.value(0).toInt();
    qMaxPK.finish();
    qMaxPK.clear();
    ++maxPK;

    // insert new rows
    QString ssql = QString("INSERT INTO %1 (%2) VALUES (?)")
                            .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName))
                            .arg(drv->escapeIdentifier(idColName, QSqlDriver::FieldName));

    QSqlDatabase db = mSourceModel->database();
    db.transaction();
    QSqlQuery qInsert(db);
    qInsert.prepare(ssql);

    QVariantList ids;
    for (unsigned int row=0; row < nrows; ++row, ++maxPK)
    {
        ids << maxPK;
    }
    qInsert.addBindValue(ids);

    // in case we haven't got any records yet,
    // we make sure that the row index is 0-based
    //    if (rowcount == 0)
    //    {

    //        ssql = QString("INSERT INTO %1 (%2) VALUES (?)")
    //                        .arg(mSourceModel->tableName())
    //                        .arg(idColName);
    //                        //.arg(rowcount);
    //        QVariant
    //    }
    //    else
    //    {
    //        ssql = QString("INSERT INTO %1 DEFAULT VALUES")
    //                .arg(mSourceModel->tableName());
    //    }


    //if (!qInsert.exec(ssql))
    if (!qInsert.execBatch(QSqlQuery::ValuesAsRows))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - insert rows: " << qInsert.lastError().text().toStdString()
              << qInsert.lastError().text().toStdString());
        qInsert.finish();
        qInsert.clear();
        db.rollback();
        this->openReadModel();
        return false;
    }
    qInsert.finish();
    qInsert.clear();
    db.commit();
    this->openReadModel();

    QString table = this->mSourceModel->tableName();
    this->mSourceModel->clear();
    this->updateSourceModel(table);

    //mSourceModel->select();
	

    return true;
}

void
NMSelSortSqlTableProxyModel::updateSourceModel(const QString& table)
{
    mSourceModel->setTable(table);
    if (mLastColSort.first >= 0)
    {
        mSourceModel->setSort(mLastColSort.first, mLastColSort.second);
    }
    if (mLastSelRecsOnly)
    {
        mSourceModel->setFilter(mLastFilter);
    }
    else
    {
        mSourceModel->setFilter("");
    }
    mSourceModel->select();
    mUpdateProxySelection = true;
    mUpdateSourceSelection = true;
}

void
NMSelSortSqlTableProxyModel::reloadSourceModel(void)
{
    if (mSourceModel == nullptr)
    {
        return;
    }

    QString tableName = mSourceModel->tableName();
    mSourceModel->clear();
    this->updateSourceModel(tableName);

}

int
NMSelSortSqlTableProxyModel::updateData(int colidx, const QString &column,
                                        const QString& expr,
                                        QString &error)
{
    int ret = 0;
    if (mSourceModel == 0 || column.isEmpty() || expr.isEmpty())
    {
        return ret;
    }

    QString whereClause = "";
    if (!mLastFilter.isEmpty())
    {
        if (expr.indexOf(QStringLiteral("where") >= 0))
        {
            error = QString("Double where clause! Please clear the current selection "
                            " before using a where clause in your update statement!");
            return ret;
        }

        whereClause = QString("where %1").arg(mLastFilter);
    }

    if (!this->openWriteModel())
    {
        return ret;
    }

    QSqlDatabase db = mSourceModel->database();

    // parse the calculate expression for external DBs
    // that need to be attached
    QStringList externalDbs = NMGlobalHelper::identifyExternalDbs(db, expr);
    NMGlobalHelper::attachMultipleDbs(db, externalDbs);

    QSqlDriver* drv = db.driver();
    QString uStr = QString("update %1 set %2 = %3 %4")
                    .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName))
                    .arg(drv->escapeIdentifier(column, QSqlDriver::FieldName))
                    .arg(expr)
                    .arg(whereClause);


    db.transaction();
    QSqlQuery qUpdate(db);
    if (!qUpdate.exec(uStr))
    {
        error = QString("%1::%2() : %3")
                .arg(ctx.c_str()).arg(__FUNCTION__)
                .arg(qUpdate.lastError().text());
        qUpdate.finish();
        qUpdate.clear();
        db.rollback();
        this->openReadModel();
        return ret;
    }
    ret = qUpdate.numRowsAffected();
    qUpdate.finish();
    qUpdate.clear();
    db.commit();

    // since this closes the database in the process, it
    // also autmatically detaches any attached databases
    this->openReadModel();

    QModelIndex tl = createIndex(0, colidx);
    QModelIndex br = createIndex(ret-1, colidx);
    emit dataChanged(tl, br);
    return ret;
}

bool
NMSelSortSqlTableProxyModel::removeColumn(const QString& name)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    if (name.isEmpty())
    {
        return false;
    }

    if (!this->openWriteModel())
    {
        return false;
    }

    QSqlDriver* drv = mSourceModel->database().driver();

    int ncols = mSourceModel->columnCount();
    int delidx = -1;
    QStringList remainCols;
    for (int i=0; i < ncols; ++i)
    {
        QVariant vcol = this->mSourceModel->headerData(i, Qt::Horizontal, Qt::DisplayRole);
        if (vcol.isValid())
        {
            if (vcol.toString().compare(name, Qt::CaseInsensitive) == 0)
            {
                delidx = i;
            }
            else
            {
                remainCols << drv->escapeIdentifier(vcol.toString(), QSqlDriver::FieldName);
            }
        }
    }

    std::string collist = remainCols.join(',').toStdString();
    std::string backuptable = this->getRandomString().toStdString();
    std::string tablename = drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName).toStdString();
    std::stringstream ssql;
    ssql << "CREATE TEMPORARY TABLE " << backuptable << " AS SELECT " << collist << " FROM main." << tablename << ";"
         << "DROP TABLE main." << tablename << ";"
         << "CREATE TABLE main." << tablename << " as SELECT * FROM " << backuptable << ";"
         << "DROP TABLE " << backuptable << ";";

    QStringList queries = QString::fromLatin1(ssql.str().c_str()).split(';', Qt::SkipEmptyParts);
    QStringList errortypes;
    errortypes << QStringLiteral("create temporary backup table:")
               << QStringLiteral("drop original table:")
               << QStringLiteral("create new table:")
               << QStringLiteral("drop backup table:");

    // since we're recreating the original table (i.e. delete it first in the process)
    // we need to release all reference to it; however, we still need the write connection to the db
    mSourceModel->clear();

    beginRemoveColumns(QModelIndex(), delidx, delidx);
    bool ret = true;

    QSqlDatabase db = mSourceModel->database();
    for (int i=0; i < queries.size(); ++i)
    {
        QString qu = queries.at(i);
        NMDebugAI(<< "Query: " << qu.toStdString() << std::endl);
        db.transaction();
        QSqlQuery dml(db);
        if (!dml.exec(qu))
        {
            NMLogError(<< ctx << "::" << __FUNCTION__ << "() - " << errortypes.at(i).toStdString() << dml.lastError().text().toStdString());
            ret = false;
            dml.finish();
            dml.clear();
            db.rollback();
            break;
        }
        else
        {
            NMDebugAI(<< "Successful!" << std::endl);
        }
        dml.finish();
        dml.clear();
        db.commit();
    }

    this->openReadModel();

    // adjust selection and ordering if applicable
    if (mLastColSort.first == delidx)
    {
        mLastColSort.first = -1;
    }
    // if the column just deleted featured in the last
    // filter expression in any capacity, we just clear the filter
    if (mLastFilter.contains(name, Qt::CaseInsensitive))
    {
        mLastFilter.clear();
    }
    endRemoveColumns();
    this->updateSourceModel(tablename.c_str());

    return ret;
}

//bool
//NMSelSortSqlTableProxyModel::joinTable(const QString &sourceFileName,
//                                       const QString &joinField,
//                                       const QStringList &joinSrcFields)
//{
//    if (mSourceModel == 0)
//    {
//        return false;
//    }

//    QString vttablename = this->getRandomString();

//    std::stringstream ssql;
//    ssql << "CREATE VIRTUAL TABLE " << vttablename.toStdString()
//         << " USING VirtualText('" << sourceFileName.toStdString() << "', "
//         << "'CP1252', 1, POINT, DOUBLEQUOTE, ',')";

//	return true;

//}

bool
NMSelSortSqlTableProxyModel::createMappingTable(void)
{
    // ==================================================================
    // drop any previously created mapping tables
    // ==================================================================

    if (!openWriteModel())
    {
        NMLogError(<< "NMSelSortSqlTableProxyModel::createMappingTable() - openWriteModel failed!");
        return false;
    }

    QSqlDatabase db = mSourceModel->database();
    {
        QStringList tables = db.tables();
        if (tables.contains(mTempTableName, Qt::CaseInsensitive))
        {
            QString qstr = QString("drop table if exists %1")
                    .arg(db.driver()->escapeIdentifier(mTempTableName, QSqlDriver::TableName));
            db.transaction();
            QSqlQuery qobj(db);
            if (!qobj.exec(qstr))
            {
                NMLogError(<< ctx << "::" << __FUNCTION__ << "() - drop mapping table: " << qobj.lastError().text().toStdString() << std::endl);
                qobj.finish();
                qobj.clear();
                db.rollback();
                return false;
            }
            qobj.finish();
            qobj.clear();
            db.commit();
        }
    }

    // ==================================================================
    // query the table creating SQL and use for creating temp table
    // ==================================================================
    QString tabName = mSourceModel->tableName().replace("\"", "");
    QString tmpStruct = QString("Select sql from sqlite_master where type='table' and name='%1'")
                            .arg(tabName);
    db.transaction();
    QSqlQuery queryStruct(db);
    if (!queryStruct.exec(tmpStruct))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - fetching source table SQL: " << queryStruct.lastError().text().toStdString() << std::endl);
        queryStruct.finish();
        queryStruct.clear();
        db.rollback();
        return false;
    }
    queryStruct.next();
    QString orgTableSql = queryStruct.value(0).toString();
    queryStruct.finish();
    queryStruct.clear();
    db.commit();

    // if the first column of the table happens to be the primary
    // key, we take all column definitions after the
    // primary key (fst col) to create the mapping table
    int pos = -1;
    if (mPKIsFstCol)
    {
        pos = orgTableSql.indexOf(',');
        orgTableSql = orgTableSql.right(orgTableSql.length()-pos);
    }
    // if the table doesn't have an explicit primary key, we
    // use rowid (SQLITE-specifc) for mapping and take all
    // column definitions
    else
    {
        pos = orgTableSql.indexOf('(')+1;
        orgTableSql = orgTableSql.right(orgTableSql.length()-pos);
        orgTableSql.prepend(", ");
    }


    //QString tmpCreate = QString("Create temp table if not exists %1 ")
    QString tmpCreate = QString("Create temp table if not exists %1 ")
                        .arg(db.driver()->escapeIdentifier(mTempTableName, QSqlDriver::TableName));
    tmpCreate += QString("(%1 integer primary key, %2 integer").arg(mProxyPK)
                                                       .arg(mSourcePK);
    tmpCreate += orgTableSql;

    // have to account for the case that the input table had
    // its primary key defined at the end of the column definition ...
    // ... so looking for superfluous second PIRMARY statement
    pos = tmpCreate.indexOf("primary", 0, Qt::CaseInsensitive);
    int pos2 = tmpCreate.indexOf("primary", pos+1, Qt::CaseInsensitive);
    if (pos2 > pos)
    {
        pos = tmpCreate.lastIndexOf(',');
        tmpCreate = tmpCreate.left(pos);
        tmpCreate += ")";
    }

    // had the case that the column definition is not closed, i.e. we're
    // missing the closing bracket ')', so double check that, just in case ....
    if (!tmpCreate.endsWith(')'))
    {
        tmpCreate += ")";
    }


    db.transaction();
    QSqlQuery queryTmpCreate(db);
    if (!queryTmpCreate.exec(tmpCreate))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - create new empty mapping table: " << queryTmpCreate.lastError().text().toStdString());
        queryTmpCreate.finish();
        queryTmpCreate.clear();
        db.rollback();
        return false;
    }
    queryTmpCreate.finish();
    queryTmpCreate.clear();
    db.commit();

    QStringList tmptables = db.tables();

    // ==================================================================
    // populate the table
    // ==================================================================

    // list of original columns
    QSqlDriver* drv = db.driver();
    QString columns = "(";

    if (!mPKIsFstCol)
    {
        columns += "rowid, ";
    }

    for (int c=0; c < mSourceModel->columnCount(); ++c)
    {
        //srcCol = mSourceModel->headerData(c, Qt::Horizontal).toString();

        columns += drv->escapeIdentifier(mSourceModel->headerData(c, Qt::Horizontal).toString(), QSqlDriver::FieldName);
        if (c < mSourceModel->columnCount() - 1)
        {
            columns += ", ";
        }
    }
    columns += ")";

    // order by clause
    QString orderByClause = "";
    if (mLastColSort.first >= 0)
    {
        QString sOrderColumn = drv->escapeIdentifier(mSourceModel->headerData(mLastColSort.first, Qt::Horizontal).toString(), QSqlDriver::FieldName);
        QString qsSortOrder = mLastColSort.second == Qt::AscendingOrder ? "ASC" : "DESC";
        orderByClause = QString("order by %1 %2").arg(sOrderColumn).arg(qsSortOrder);
    }

    QString tmpInsert;
    if (mPKIsFstCol)
    {
        tmpInsert = QString("Insert into %1 %2 select * from %3 %4")
                            .arg(drv->escapeIdentifier(mTempTableName, QSqlDriver::TableName))
                            .arg(columns)
                            .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName))
                            .arg(orderByClause);
    }
    else
    {
        tmpInsert = QString("Insert into %1 %2 select rowid, * from %3 %4")
                            .arg(drv->escapeIdentifier(mTempTableName, QSqlDriver::TableName))
                            .arg(columns)
                            .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName))
                            .arg(orderByClause);
    }

    db.transaction();
    QSqlQuery queryInsert(db);
    if (!queryInsert.exec(tmpInsert))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - populate mapping table: " << queryInsert.lastError().text().toStdString());
        queryInsert.finish();
        queryInsert.clear();
        db.rollback();
        return false;
    }
    queryInsert.finish();
    queryInsert.clear();
    db.commit();

    openReadModel();

    return true;
}

QModelIndex
NMSelSortSqlTableProxyModel::mapFromSource(const QModelIndex& srcIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

    QModelIndex retIdx;
    // no need of expensive mapping unless the model has been sorted
    if (mLastColSort.first == -1)
    {
        return this->mSourceModel->index(srcIdx.row(), srcIdx.column(), srcIdx.parent());
    }

    QSqlDriver* drv = mSourceModel->database().driver();
    QString qstr = QString("Select %1 from %2 where %3 = %4;")
                   .arg(drv->escapeIdentifier(mProxyPK, QSqlDriver::FieldName))
                   .arg(drv->escapeIdentifier(mTempTableName, QSqlDriver::TableName))
                   .arg(drv->escapeIdentifier(mSourcePK, QSqlDriver::FieldName))
                   .arg(srcIdx.row());

    NMLogDebug(<< ctx << ": " << "tv mapFromSource (srcIdx): " << qstr.toStdString());


    //    /// debug
    //    NMDebugAI(<< "mTempDb.isOpen() = " << mTempDb.isOpen() << std::endl);
    //    if (mTempDb.isOpen())
    //    {
    //        NMDebugAI(<< "  tables: " << mTempDb.tables().join(' ').toStdString() << std::endl);
    //    }

    QSqlDatabase db = mSourceModel->database();//mTempDb;
    db.transaction();
    QSqlQuery qProxy(db);
    if (!qProxy.exec(qstr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << qProxy.lastError().text().toStdString());
        qProxy.finish();
        qProxy.clear();
        db.rollback();
        return retIdx;
    }

    if (qProxy.next())
    {
        // the proxy index, i.e. the row number in the table view is always 0-based;
        // the 'proxy-mapping-index' in the temp table is 1-based and since this
        // is the onve we're querying here, we've got to subtract 1 to get to
        // row number in the table view
        int r = qProxy.value(0).toInt()-1;
        NMLogDebug(<< ctx << ": " << ">> mapped proxyIdx + srcToProxy: ");
        retIdx = this->createIndex(r, srcIdx.column());
    }
    qProxy.finish();
    qProxy.clear();
    db.commit();
    return retIdx;
}

QModelIndex
NMSelSortSqlTableProxyModel::mapToSource(const QModelIndex& proxyIdx) const
{
    if (mSourceModel == 0)
    {
        return QModelIndex();
    }

    QModelIndex retIdx;
    // no need of expensive mapping unless the source table has been sorted
    if (mLastColSort.first == -1)
    {
        return this->index(proxyIdx.row(), proxyIdx.column(), QModelIndex());
    }

    // NOTE: since we utilise the autoincrementing primary key, the
    // proxy index in the mapping-table is 1-based!
    QSqlDriver* drv = mSourceModel->database().driver();
    QString qstr = QString("Select %1 from %2 where %3 = %4;")
                   .arg(drv->escapeIdentifier(mSourcePK, QSqlDriver::FieldName))
                   .arg(drv->escapeIdentifier(mTempTableName, QSqlDriver::TableName))
                   .arg(drv->escapeIdentifier(mProxyPK, QSqlDriver::FieldName))
                   .arg(proxyIdx.row());//+1);

    NMLogDebug(<< ctx << ": " << "tv mapToSource (proxyIdx): " << qstr.toStdString());

    QSqlDatabase db = mSourceModel->database();//mTempDb;
    db.transaction();
    QSqlQuery qProxy(db);
    if (!qProxy.exec(qstr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << qProxy.lastError().text().toStdString());
        qProxy.finish();
        qProxy.clear();
        db.rollback();
        return retIdx;
    }

    if (qProxy.next())
    {
        int r = qProxy.value(0).toInt();
        retIdx = this->createIndex(r, proxyIdx.column());
    }
    qProxy.finish();
    qProxy.clear();
    db.commit();
    return retIdx;
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
    return this->createIndex(row, column, (void*)this);
    //return this->mSourceModel->ind
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
NMSelSortSqlTableProxyModel::getNumTableRecords()
{
    int rows = -1;

    if (mSourceModel == 0)
    {
        return rows;
    }

    QSqlIndex pk = mSourceModel->primaryKey();
    QString primaryKey;
    if (!pk.isEmpty())
    {
        primaryKey = pk.name();
        if (primaryKey.isEmpty())
        {
            primaryKey = "*";
        }
    }
    else
    {
        primaryKey = mSourceModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    }

    QSqlDatabase db = mSourceModel->database();
    QSqlDriver* drv = db.driver();
    QString qstr = QString("Select count(%1) from %2").arg(drv->escapeIdentifier(primaryKey, QSqlDriver::FieldName))
                                                      .arg(drv->escapeIdentifier(mSourceModel->tableName(), QSqlDriver::TableName));

    db.transaction();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    if (q.exec(qstr))
    {
        q.next();
        rows = q.value(0).toInt();
    }
    else
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() : " << q.lastError().text().toStdString());
    }
    q.finish();
    q.clear();
    db.commit();

    return rows;
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
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
        return QFlags<Qt::ItemFlag>();
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







