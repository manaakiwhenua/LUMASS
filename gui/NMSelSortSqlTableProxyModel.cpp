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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlIndex>
#include <QSqlError>
#include <QSqlDriver>
#include <QUuid>

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
    : mSourceModel(0),
      mLastFilter(""),
      mLastSelRecsOnly(false),
      mUpdateProxySelection(false),
      mUpdateSourceSelection(false),
      mLastSelCount(0),
      mTempTableName(""),
      mSourcePK(""),
      mProxyPK("")

{
    this->setParent(parent);
    mLastColSort.first = -1;
    mLastColSort.second = Qt::AscendingOrder;
    mTempTableName = this->getRandomString();
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(true);

#ifdef DEBUG
    mLogger->setLogLevel(NMLogger::NM_LOG_DEBUG);
#endif

    connect(mLogger, SIGNAL(sendLogMsg(QString)), NMGlobalHelper::getLogWidget(),
            SLOT(insertHtml(QString)));

}

NMSelSortSqlTableProxyModel::~NMSelSortSqlTableProxyModel()
{
    if (mTempDb.isOpen())
    {
        mTempDb.close();
        QSqlDatabase::removeDatabase(mTempDb.connectionName());
    }
    this->mSourceModel = 0;
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
        primaryKey = mSourceModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    }

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
    QStringList allTables = mTempDb.tables();
    if (allTables.contains(mTempTableName))
    {
        tmpTable = true;
    }

    QString selectColumn = mProxyPK;
    QString whereClause = QString("where %1").arg(mLastFilter);
    QString orderByClause = "";
    QAbstractItemModel* model = this;
    QString tableName = mTempTableName;
    int proxyCorr = -1;
    if (!bProxySelection || !tmpTable)
    {
        //model = mSourceModel;
        selectColumn = mSourcePK;
        orderByClause = QString("order by %1 asc").arg(selectColumn);
        proxyCorr = 0;
        if (!tmpTable)
        {
            tableName = mSourceModel->tableName();
        }
    }

    QString queryStr = QString("select %1 from %2 %3 %4")
                       .arg(selectColumn)
                       .arg(tableName)
                       .arg(whereClause)
                       .arg(orderByClause);

    QSqlQuery queryObj(mTempDb);
    if (!tmpTable)
    {
        queryObj = QSqlQuery(mSourceModel->database());
    }
    queryObj.setForwardOnly(true);
    if (!queryObj.exec(queryStr))
    {
        NMLogError(<< ctx << queryObj.lastError().text().toStdString() << std::endl);
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

    //std::srand(std::time(0));
    char* nam = new char[len+1];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (::rand() % 2 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else
            {
                nam[i] = ::rand() % 26 + 97;
            }
        }
        else
        {
            if (::rand() % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (::rand() % 5 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else if (::rand() % 3 == 0)
            {
                nam[i] = ::rand() % 26 + 97;
            }
            else
            {
                nam[i] = ::rand() % 10 + 48;
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

    mSourceModel->clear();
    QString qStr = QString("Alter table %1 add column %2 %3")
                        .arg(table)
                        .arg(name)
                        .arg(typeStr);

    const QSqlDatabase& db = mSourceModel->database();
    QSqlQuery q(db);
    bool ret = true;
    if (!q.exec(qStr))
    {
        NMLogError(<< ctx << q.lastError().text().toStdString());
        ret = false;
    }

    // reset the source model with the
    // modified table
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

    QString tempJoinTableName = this->getRandomString(5);

    std::stringstream ssql;
    ssql << "CREATE TEMP TABLE " << tempJoinTableName.toStdString() << " AS "
         << "SELECT * FROM " << joinTableName.toStdString() << ";";

    QSqlQuery query(mSourceModel->database());
    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMDebugAI(<< "last query:\n" << ssql.str() << std::endl);
        NMLogError(<< ctx << query.lastError().text().toStdString());
        return false;
    }

    QString idxName = QString("%1_idx").arg(joinFieldName);
    ssql.str("");
    ssql << "CREATE INDEX IF NOT EXISTS " << idxName.toStdString()
         << " on " << tempJoinTableName.toStdString()
         << " (" << joinFieldName.toStdString() << ");";

    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMDebugAI(<< "last query:\n" << ssql.str() << std::endl);
        NMLogError(<< ctx << query.lastError().text().toStdString());
        return false;
    }


    // ---------------------------------------------------------------
    // now, we create temporary join table ...

    QString tempTableName = this->getRandomString(5);

    ssql.str("");
    ssql << "CREATE TEMP TABLE " << tempTableName.toStdString() << " AS "
         << "SELECT * FROM " << tarTableName.toStdString() << " "
         << "LEFT OUTER JOIN " << tempJoinTableName.toStdString() << " "
         << " ON " << tarTableName.toStdString() << "." << tarFieldName.toStdString()
         << " = " << tempJoinTableName.toStdString() << "." << joinFieldName.toStdString()
         << ";";



    //QSqlQuery query(mSourceModel->database());
    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << query.lastError().text().toStdString());
        return false;
    }

    ssql.str("");
    ssql << "DROP TABLE " << tarTableName.toStdString() << ";";
    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << query.lastError().text().toStdString());
        return false;
    }

    ssql.str("");
    ssql << "CREATE TABLE " << tarTableName.toStdString() << " AS "
         << "SELECT * FROM " << tempTableName.toStdString() << ";";
    if (!query.exec(QString(ssql.str().c_str())))
    {
        NMLogError(<< ctx << query.lastError().text().toStdString());
        return false;
    }

    mSourceModel->setTable(tarTableName);
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
NMSelSortSqlTableProxyModel::addRows(unsigned int nrows)
{
    if (mSourceModel == 0)
    {
        return false;
    }

    int rowcount = mSourceModel->rowCount();
    QString idColName = this->getSourcePK();
    QString ssql = QString("INSERT INTO %1 (%2) VALUES (?)")
                            .arg(mSourceModel->tableName())
                            .arg(idColName);

    QSqlQuery qInsert(mSourceModel->database());
    qInsert.prepare(ssql);

    QVariantList ids;
    for (unsigned int row=0; row < nrows; ++row, ++rowcount)
    {
        ids << rowcount;
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
        NMLogError(<< ctx << "Failed to add rows - "
              << qInsert.lastError().text().toStdString());
        return false;
    }
    mSourceModel->select();
	

    return true;
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
        whereClause = QString("where %1").arg(mLastFilter);
    }

    QString uStr = QString("update %1 set %2 = %3 %4")
                    .arg(mSourceModel->tableName())
                    .arg(column)
                    .arg(expr)
                    .arg(whereClause);

    QSqlQuery qUpdate(mSourceModel->database());
    if (!qUpdate.exec(uStr))
    {
        error = qUpdate.lastError().text();
        return ret;
    }
    mSourceModel->select();
    ret = qUpdate.numRowsAffected();

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
                remainCols << vcol.toString();
            }
        }
    }

    std::string collist = remainCols.join(',').toStdString();
    std::string backuptable = this->getRandomString().toStdString();
    std::string tablename = mSourceModel->tableName().toStdString();
    std::stringstream ssql;
    ssql << "CREATE TEMPORARY TABLE " << backuptable << " AS SELECT " << collist << " FROM main." << tablename << ";"
         << "DROP TABLE main." << tablename << ";"
         << "CREATE TABLE main." << tablename << " as SELECT * FROM " << backuptable << ";"
         << "DROP TABLE " << backuptable << ";";

    QStringList queries = QString::fromLatin1(ssql.str().c_str()).split(';', QString::SkipEmptyParts);

    beginRemoveColumns(QModelIndex(), delidx, delidx);

    bool ret = true;
    mSourceModel->clear();
    const QSqlDatabase& db = mSourceModel->database();
    QSqlQuery dml(db);
    foreach (const QString& qu, queries)
    {
        NMDebugAI(<< "Query: " << qu.toStdString() << std::endl);
        if (!dml.exec(qu))
        {
            NMLogError(<< ctx << dml.lastError().text().toStdString());
            ret = false;
            break;
        }
        else
        {
            NMDebugAI(<< "Successful!" << std::endl);
        }
    }

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


    // repopulate the table model
    mSourceModel->setTable(QString(tablename.c_str()));
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

    endRemoveColumns();
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
    if (mTempDb.isOpen())
    {
        //mTempTableModel->clear();
        //mTempDb.close();
        //mTempDb.open();
        QStringList tables = mTempDb.tables();
        if (tables.contains(mTempTableName, Qt::CaseInsensitive))
        {
            QString qstr = QString("Drop table if exists %1").arg(mTempTableName);
            QSqlQuery qobj(mTempDb);
            if (!qobj.exec(qstr))
            {
                NMLogError(<< ctx << qobj.lastError().text().toStdString() << std::endl);
                return false;
            }
        }
    }
    else
    {
        mTempDb = QSqlDatabase::addDatabase("QSQLITE", this->getRandomString());
        mTempDb.setDatabaseName(mSourceModel->getDatabaseName());
        if (!mTempDb.open())
        {
            NMLogError(<< ctx << ": " << mTempDb.lastError().text().toStdString() << std::endl);
            return false;
        }
    }

    // ==================================================================
    // query the table creating SQL and use for creating temp table
    // ==================================================================
    QString tmpStruct = QString("Select sql from sqlite_master where type='table' and name='%1'")
                            .arg(mSourceModel->tableName());
    QSqlQuery queryStruct(mSourceModel->database());
    if (!queryStruct.exec(tmpStruct))
    {
        NMLogError(<< ctx << ": " << queryStruct.lastError().text().toStdString() << std::endl);
        return false;
    }
    queryStruct.next();
    QString orgTableSql = queryStruct.value(0).toString();
    queryStruct.finish();
    queryStruct.clear();

    int pos = orgTableSql.indexOf(',');
    orgTableSql = orgTableSql.right(orgTableSql.length()-pos);
    QString tmpCreate = QString("Create temp table if not exists %1 ")
                        .arg(mTempTableName);
    tmpCreate += QString("(%1 integer primary key, %2").arg(mProxyPK)
                                                       .arg(mSourcePK);
    tmpCreate += orgTableSql;

    QSqlQuery queryTmpCreate(mTempDb);
    if (!queryTmpCreate.exec(tmpCreate))
    {
        NMLogError(<< ctx << ": " << queryTmpCreate.lastError().text().toStdString() << " Query: '"
                   << tmpCreate.toStdString() << "'" << std::endl);
        return false;
    }

    // ==================================================================
    // populate the table
    // ==================================================================

    // list of original columns
    QSqlDriver* drv = mSourceModel->database().driver();
    QString srcCol;
    QString columns = "(";
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
        QString sOrderColumn = mSourceModel->headerData(mLastColSort.first, Qt::Horizontal).toString();
        QString qsSortOrder = mLastColSort.second == Qt::AscendingOrder ? "ASC" : "DESC";
        orderByClause = QString("order by %1 %2").arg(sOrderColumn).arg(qsSortOrder);
    }

    QString tmpInsert = QString("Insert into %1 %2 select * from %3 %4")
                        .arg(mTempTableName)
                        .arg(columns)
                        .arg(mSourceModel->tableName())
                        .arg(orderByClause);

    QSqlQuery queryInsert(mTempDb);
    if (!queryInsert.exec(tmpInsert))
    {
        NMLogError(<< ctx << ": " << queryInsert.lastError().text().toStdString() << " Query: '"
                   << tmpInsert.toStdString() << "'" << std::endl);
        queryInsert.finish();
        return false;
    }
    return true;
}

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
                NMLogError(<< ctx << "Failed dropping previous mapping table"
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
        NMLogError(<< ctx << "Failed creating new mapping table"
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
        NMLogError(<< ctx << "Failed populating new mapping table"
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

    QModelIndex retIdx;
    // no need of expensive mapping unless the model has been sorted
    if (mLastColSort.first == -1)
    {
        return this->mSourceModel->index(srcIdx.row(), srcIdx.column(), srcIdx.parent());
    }

    QString qstr = QString("Select %1 from %2 where %3 = %4;")
                   .arg(mProxyPK)
                   .arg(mTempTableName)
                   .arg(mSourcePK)
                   .arg(srcIdx.row());


    //    /// debug
    //    NMDebugAI(<< "mTempDb.isOpen() = " << mTempDb.isOpen() << std::endl);
    //    if (mTempDb.isOpen())
    //    {
    //        NMDebugAI(<< "  tables: " << mTempDb.tables().join(' ').toStdString() << std::endl);
    //    }


    QSqlQuery qProxy(mTempDb);
    if (!qProxy.exec(qstr))
    {
        NMLogError(<< ctx << qProxy.lastError().text().toStdString() << std::endl);
        return retIdx;
    }

    if (qProxy.next())
    {
        // NOTE: since we utilise the autoincrementing primary key, the
        // proxy index in the mapping-table is 1-based!
        int r = qProxy.value(0).toInt() - 1;
        retIdx = this->createIndex(r, srcIdx.column());
    }

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
    QString qstr = QString("Select %1 from %2 where %3 = %4;")
                   .arg(mSourcePK)
                   .arg(mTempTableName)
                   .arg(mProxyPK)
                   .arg(proxyIdx.row()+1);

	NMMsg(<< "mapToSource: qstr: " << qstr.toStdString() << std::endl);

    QSqlQuery qProxy(mTempDb);
    if (!qProxy.exec(qstr))
    {
        NMLogError(<< ctx << qProxy.lastError().text().toStdString() << std::endl);
        return retIdx;
    }

    if (qProxy.next())
    {
        int r = qProxy.value(0).toInt();
        retIdx = this->createIndex(r, proxyIdx.column());
    }

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
    }
    else
    {
        primaryKey = mSourceModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
    }
    QString qstr = QString("Select count(%1) from %2").arg(primaryKey)
                                                      .arg(mSourceModel->tableName());
    QSqlQuery q(mSourceModel->database());
    q.setForwardOnly(true);
    if (q.exec(qstr))
    {
        q.next();
        rows = q.value(0).toInt();
    }
    else
    {
        NMLogError(<< ctx << q.lastError().text().toStdString() << std::endl);
    }



    //this->mlNumRecs = q.value(0).toInt();
    //this->updateSelectionAdmin(mlNumRecs);


//    QString qstr = QString("Select * from %1").arg(mProxyTable);
//    QSqlQuery q(mProxyDb);
//    if (q.exec(qstr))
//    {
//        rows = q.size();
//    }
//    else
//    {
//        rows = mSourceModel->rowCount();
//    }

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







