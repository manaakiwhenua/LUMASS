/******************************************************************************
* Created by Alexander Herzig
* Copyright 2011-2016 Landcare Research New Zealand Ltd
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
 * NMMosra., cpp
 *
 *  Created on: 23/03/2011
 *      Author: alex
 */

#include "nmlog.h"
#include "itkNMLogEvent.h"
#include "NMLogger.h"
#include "NMSqlTableModel.h"

//#include "NMTableCalculator.h"
//#include "NMMfwException.h"
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <QObject>
#include <QMetaObject>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QList>
#include <QMap>
#include <QVector>
#include <QPointer>
#include <QStandardItem>
#include <QScopedPointer>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QSqlDriver>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlIndex>
#include <QSqlError>
#include <QVariantList>
#include <QStack>
#include <QSet>

#include "itkProcessObject.h"
#include "otbSQLiteTable.h"

#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkDelimitedTextWriter.h"

#include "NMMosra.h"

/// need to define NMMosra specific debug macros since this class
/// is used as part of the GUI as well as the modlling framework,
/// which sport different logging mechanisms; the macros below
/// cater for this unique situation

#define MosraLogError(arg) \
        { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_ERROR)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_ERROR, \
                        sstr.str().c_str()); \
        }

#define MosraLogWarn(arg)  \
        { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_WARN)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_WARN, \
                        sstr.str().c_str()); \
        }

#define MosraLogInfo(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_INFO)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_INFO, \
                        sstr.str().c_str()); \
       }

#define MosraLogDebug(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            if (mProcObj) mProcObj->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_DEBUG)); \
            else if (mLogger) mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                        NMLogger::NM_LOG_DEBUG, \
                        sstr.str().c_str()); \
       }

// need to have the macros defined before including the openGA related header(s)
#include "NMOpenGA.h"

////////////////////////////////
/// NMMosraDataSet implementation
////////////////////////////////

NMMosraDataSet::NMMosraDataSet(QObject* parent)
    : mProcObj(nullptr),
      mLogger(nullptr),
      mVtkDS(nullptr),
      mOtbTab(nullptr),
      mSqlMod(nullptr),
      mbTransaction(false),
      mTableName(""),
      mType(NM_MOSRA_DS_NONE)
{
    this->setParent(parent);
}

void
NMMosraDataSet::reset(void)
{
    this->mOtbTab = nullptr;
    this->mVtkDS = nullptr;
    this->mSqlMod = nullptr;
    this->mLogger = nullptr;
    this->mTableName.clear();
    this->mType = NM_MOSRA_DS_NONE;
}

bool
NMMosraDataSet::beginTransaction(void)
{

    if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            mbTransaction = sqltab->BeginTransaction();
        }
    }
    else if (mSqlMod != nullptr)
    {
        mSqlMod->clear();
        mbTransaction = mSqlMod->database().transaction();
    }
    else
    {
        mbTransaction = false;
    }

    return mbTransaction;
}

bool
NMMosraDataSet::endTransaction(void)
{
    if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            mbTransaction = sqltab->EndTransaction();
        }
    }
    else if (mSqlMod != nullptr)
    {
        mbTransaction = mSqlMod->database().commit();
        mSqlMod->setTable(mSqlMod->database().driver()->escapeIdentifier(mTableName, QSqlDriver::TableName));
        mSqlMod->select();
    }
    else
    {
        mbTransaction = true;
    }

    return !mbTransaction;
}

void
NMMosraDataSet::rollBack(void)
{
    if (mSqlMod != nullptr)
    {
        mSqlMod->database().rollback();
    }
}

void
NMMosraDataSet::setDataSet(otb::AttributeTable::Pointer otbtab)
{
    mVtkDS = nullptr;
    mSqlMod = nullptr;
    if (otbtab.IsNotNull())
    {
        mOtbTab = otbtab;
        mType = NM_MOSRA_DS_OTBTAB;
    }
    else
    {
        mOtbTab = nullptr;
        mSqlMod = nullptr;
        mType = NM_MOSRA_DS_NONE;
    }
}

void
NMMosraDataSet::setDataSet(vtkDataSet* vtkds)
{
    mOtbTab = nullptr;
    mSqlMod = nullptr;
    if (vtkds)
    {
        mVtkDS = vtkds;
        mType = NM_MOSRA_DS_VTKDS;
    }
    else
    {
        mVtkDS = nullptr;
        mSqlMod = nullptr;
        mType = NM_MOSRA_DS_NONE;
    }
}

void
NMMosraDataSet::setDataSet(QSqlTableModel *sqlmod)
{
    mOtbTab = nullptr;
    mVtkDS = nullptr;
    if (sqlmod)
    {
        mSqlMod = sqlmod;
        mTableName = sqlmod->tableName();
        mPrimaryKey = this->getNMPrimaryKey();
        mType = NM_MOSRA_DS_QTSQL;
    }
    else
    {
        mVtkDS = nullptr;
        mOtbTab = nullptr;
        mType = NM_MOSRA_DS_NONE;
        mColTypes.clear();
    }
}

bool
NMMosraDataSet::hasColumn(const QString &columnName)
{
    bool ret = false;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        if (mOtbTab->ColumnExists(columnName.toStdString()) >= 0)
        {
            ret = true;
        }
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr->HasArray(columnName.toStdString().c_str()))
            {
                ret = true;
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        for (int col=0; col < mSqlMod->columnCount(); ++col)
        {
            if (mSqlMod->headerData(col, Qt::Horizontal).toString()
                    .compare(columnName, Qt::CaseInsensitive) == 0)
            {
                ret = true;
            }
        }
        break;

    default:
        ret = false;
    }

    return ret;
}

int
NMMosraDataSet::getNumRecs()
{
    int recs = 0;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        recs = mOtbTab->GetNumRows();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                recs = dsAttr->GetAbstractArray(0)->GetNumberOfTuples();
            }
            break;
        }

    case NM_MOSRA_DS_QTSQL:
        {
            if (mSqlMod != nullptr)
            {
                QSqlDatabase db = mSqlMod->database();
                QSqlDriver* drv = db.driver();

                QString qStr = QString("SELECT count(*) from %1")
                                .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName));
                QSqlQuery q(db);
                if (!q.exec(qStr))
                {
                    MosraLogError(<< "Failed retrieving the number of records in '"
                                  << mTableName.toStdString() << "'!");
                    recs = 0;
                }
                else
                {
                    if (q.next())
                    {
                        recs = q.value(0).toInt();
                    }
                }
                q.finish();
            }
        }
        break;

    default:
        recs = 0;
    }

    return recs;
}

double
NMMosraDataSet::getDblValue(const QString &columnName, int row)
{
    double val = 0;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetDblValue(columnName.toStdString(), row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkDataArray* da = vtkDataArray::SafeDownCast(dsAttr->GetAbstractArray(columnName.toStdString().c_str()));
                if (da != nullptr)
                {
                    val = da->GetTuple1(row);
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            const QVariant value = getQSqlTableValue(columnName, row);
            if (value.isValid())
            {
                val = value.toDouble();
            }

//            int cidx = getColumnIndex(columnName);
//            QModelIndex mi = mSqlMod->index(row, cidx);
//            bool bok = false;
//            val = mSqlMod->data(mi, Qt::DisplayRole).toDouble(&bok);
//            if (!bok)
//            {
//                val = 0.0;
//            }
        }
        break;

    default:
        val = 0;
    }

    return val;
}

int
NMMosraDataSet::getIntValue(const QString &columnName, int row)
{
    int val = 0;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetIntValue(columnName.toStdString(), row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkDataArray* da = vtkDataArray::SafeDownCast(dsAttr->GetAbstractArray(columnName.toStdString().c_str()));
                if (da != nullptr)
                {
                    val = static_cast<int>(da->GetTuple1(row));
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            const QVariant value = getQSqlTableValue(columnName, row);
            if (value.isValid())
            {
                val = value.toInt();
            }

//            const int cidx = getColumnIndex(columnName);
//            QModelIndex mi = mSqlMod->index(row, cidx);
//            bool bok = false;
//            val = mSqlMod->data(mi, Qt::DisplayRole).toInt(&bok);
//            if (!bok)
//            {
//                val = 0;
//            }
        }
        break;

    default:
        val = 0;
    }

    return val;
}

QString
NMMosraDataSet::getStrValue(const QString &columnName, int row)
{
    QString val = "";
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetStrValue(columnName.toStdString(), row).c_str();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(columnName.toStdString().c_str()));
                if (sa != nullptr)
                {
                    val = sa->GetValue(row).c_str();
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            const QVariant value = getQSqlTableValue(columnName, row);
            if (value.isValid())
            {
                val = value.toString();
            }

//            const int cidx = getColumnIndex(columnName);
//            QModelIndex mi = mSqlMod->index(row, cidx);
//            val = mSqlMod->data(mi, Qt::DisplayRole).toString();
        }
        break;

    default:
        val = "";
    }

    return val;
}

/// int col impl
double
NMMosraDataSet::getDblValue(int col, int row)
{
    double val = 0;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetDblValue(col, row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkDataArray* da = vtkDataArray::SafeDownCast(dsAttr->GetAbstractArray(col));
                if (da != nullptr)
                {
                    val = da->GetTuple1(row);
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            const QString colname = this->getColumnName(col);
            const QVariant value = getQSqlTableValue(colname, row);
            if (value.isValid())
            {
                val = value.toDouble();
            }

//            QModelIndex mi = mSqlMod->index(row, col);
//            bool bok = false;
//            val = mSqlMod->data(mi, Qt::DisplayRole).toDouble(&bok);
//            if (!bok)
//            {
//                val = 0.0;
//            }
        }
        break;


    default:
        val = 0;
    }

    return val;
}

int
NMMosraDataSet::getIntValue(int col, int row)
{
    int val = 0;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetIntValue(col, row);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkDataArray* da = vtkDataArray::SafeDownCast(dsAttr->GetAbstractArray(col));
                {
                    val = static_cast<int>(da->GetTuple1(row));
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            const QString colname = this->getColumnName(col);
            const QVariant value = getQSqlTableValue(colname, row);
            if (value.isValid())
            {
                val = value.toInt();
            }

//            QModelIndex mi = mSqlMod->index(row, col);
//            bool bok = false;
//            val = mSqlMod->data(mi, Qt::DisplayRole).toInt(&bok);
//            if (!bok)
//            {
//                val = 0;
//            }
        }
        break;

    default:
        val = 0;
    }

    return val;
}

QString
NMMosraDataSet::getStrValue(int col, int row)
{
    QString val = "";
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->GetStrValue(col, row).c_str();
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr != nullptr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(col));
                if (sa != nullptr)
                {
                    val = sa->GetValue(row).c_str();
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
              const QString colname = this->getColumnName(col);
              const QVariant value = getQSqlTableValue(colname, row);
              if (value.isValid())
              {
                  val = value.toString();
              }
//            QModelIndex mi = mSqlMod->index(row, col);
//            val = mSqlMod->data(mi, Qt::DisplayRole).toString();
        }
        break;

    default:
        val = "";
    }

    return val;
}

QVariant
NMMosraDataSet::getQSqlTableValue(const QString &column, int row)
{
    QVariant val = QVariant::Invalid;
    if (mSqlMod == nullptr)
    {
        return val;
    }

    QSqlDatabase db = mSqlMod->database();
    QSqlDriver* drv = db.driver();

    QString qStr = QString("SELECT %1 from %2 where %3 = %4")
            .arg(drv->escapeIdentifier(column, QSqlDriver::FieldName))
            .arg(drv->escapeIdentifier(mSqlMod->tableName(), QSqlDriver::TableName))
            .arg(drv->escapeIdentifier(this->getNMPrimaryKey(), QSqlDriver::FieldName))
            .arg(row);

    QSqlQuery q(db);
    if (q.exec(qStr))
    {
        if (q.next())
        {
            val = q.value(0);
        }
    }
    q.finish();
    return val;
}



void
NMMosraDataSet::addColumn(const QString &colName, NMMosraDataSetDataType type)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        switch(type)
        {
        case NM_MOSRA_DATATYPE_INT:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_INT);
            break;
        case NM_MOSRA_DATATYPE_DOUBLE:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_DOUBLE);
            break;
        case NM_MOSRA_DATATYPE_STRING:
            mOtbTab->AddColumn(colName.toStdString(), otb::AttributeTable::ATTYPE_STRING);
            break;
        default:
            break;
        }

        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr && dsAttr->GetArray(0))
            {
                vtkAbstractArray* newar = 0;
                switch(type)
                {
                case NM_MOSRA_DATATYPE_INT:
                    newar = vtkAbstractArray::CreateArray(VTK_INT);
                    break;
                case NM_MOSRA_DATATYPE_DOUBLE:
                    newar = vtkAbstractArray::CreateArray(VTK_DOUBLE);
                    break;
                case NM_MOSRA_DATATYPE_STRING:
                    newar = vtkAbstractArray::CreateArray(VTK_STRING);
                    break;
                default:
                    break;
                }
                if (newar)
                {
                    newar->SetName(colName.toStdString().c_str());
                    newar->SetNumberOfComponents(1);
                    newar->SetNumberOfTuples(dsAttr->GetArray(0)->GetNumberOfTuples());
                    mVtkDS->GetCellData()->AddArray(newar);
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            QString typeStr;
            switch(type)
            {
            case NM_MOSRA_DATATYPE_INT:
                typeStr = "INTEGER";
                break;
            case NM_MOSRA_DATATYPE_DOUBLE:
                typeStr = "REAL";
                break;
            case NM_MOSRA_DATATYPE_STRING:
            default:
                typeStr = "TEXT";
                break;
            }

            //const int colidx = mSqlMod->columnCount();
            const QString tableName = mTableName;//mSqlMod->tableName();

            const QSqlDriver* drv = mSqlMod->database().driver();

            //mSqlMod->clear();
            const QString qStr = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                    .arg(drv->escapeIdentifier(tableName, QSqlDriver::TableName))
                    .arg(drv->escapeIdentifier(colName, QSqlDriver::FieldName))
                    .arg(typeStr);

            QSqlDatabase db = mSqlMod->database();
            QSqlQuery q(db);
            if (!q.exec(qStr))
            {
                MosraLogError(<< "Failed adding column '" << colName.toStdString() << "'!");
            }
            q.finish();

            if (mColTypes.empty())
            {
                this->getColumnType(colName);
            }
            else
            {
                mColTypes.insert(colName, type);
            }
        }
        break;


    default:
        break;
    }
}

void
NMMosraDataSet::setIntValue(const QString &colname, int row, int value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(),
                          static_cast<long long>(row),
                          static_cast<long long>(value));
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkIntArray* ia = vtkIntArray::SafeDownCast(
                            dsAttr->GetArray(colname.toStdString().c_str()));
                ia->SetValue(row, value);
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            QSqlDatabase db = mSqlMod->database();
            QSqlDriver* drv = db.driver();

            const QString whereClause = QString("WHERE %1 = %2")
                    .arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName))
                    .arg(row);

            const QString qStr = QString("UPDATE %1 SET %2 = %3 %4")
                    .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName))
                    .arg(drv->escapeIdentifier(colname, QSqlDriver::FieldName))
                    .arg(value)
                    .arg(whereClause);

            QSqlQuery qUpdate(db);
            if (!qUpdate.exec(qStr))
            {
                MosraLogError(<< "Failed to set value='" << value << "' for column '" << colname.toStdString()
                              << "' in row=" << row << "!");
            }
            qUpdate.finish();
        }
        break;

    default:
        break;
    }
}

void
NMMosraDataSet::setDblValue(const QString &colname, int row, double value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(), static_cast<long long>(row), value);
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(
                            dsAttr->GetArray(colname.toStdString().c_str()));
                da->SetValue(row, value);
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            QSqlDatabase db = mSqlMod->database();
            QSqlDriver* drv = db.driver();

            const QString whereClause = QString("WHERE %1 = %2")
                    .arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName))
                    .arg(row);

            const QString qStr = QString("UPDATE %1 SET %2 = %3 %4")
                    .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName))
                    .arg(drv->escapeIdentifier(colname, QSqlDriver::FieldName))
                    .arg(value)
                    .arg(whereClause);


            QSqlQuery qUpdate(db);
            if (!qUpdate.exec(qStr))
            {
                MosraLogError(<< "Failed to set value='" << value << "' for column '" << colname.toStdString()
                              << "' in row=" << row << "!");
            }
            qUpdate.finish();
        }
        break;


    default:
        break;
    }
}

void
NMMosraDataSet::setStrValue(const QString &colname, int row, const QString& value)
{
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        mOtbTab->SetValue(colname.toStdString(), row, value.toStdString());
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(
                            dsAttr->GetAbstractArray(colname.toStdString().c_str()));
                sa->SetValue(row, value.toStdString());
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            QSqlDatabase db = mSqlMod->database();
            QSqlDriver* drv = db.driver();

            const QString whereClause = QString("WHERE %1 = %2")
                    .arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName))
                    .arg(row);

            const QString qStr = QString("UPDATE %1 SET %2 = '%3' %4")
                    .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName))
                    .arg(drv->escapeIdentifier(colname, QSqlDriver::FieldName))
                    .arg(value)
                    .arg(whereClause);


            QSqlQuery qUpdate(db);
            if (!qUpdate.exec(qStr))
            {
                MosraLogError(<< "Failed to set value='" << value.toStdString() << "' for column '"
                              << colname.toStdString()
                              << "' in row=" << row << "!");
            }
            qUpdate.finish();
        }
        break;

    default:
        break;
    }
}

QString
NMMosraDataSet::getNMPrimaryKey()
{
    QString primaryKey = "";

    if (mSqlMod != nullptr)
    {
        QSqlIndex pk = mSqlMod->primaryKey();
        if (!pk.isEmpty() && !pk.fieldName(0).isEmpty())
        {
            primaryKey = pk.fieldName(0);
        }
        else
        {
            primaryKey = mSqlMod->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString();
        }

    }

    return primaryKey;
}


NMMosraDataSet::NMMosraDataSetDataType
NMMosraDataSet::getColumnType(const QString& colname)
{
    NMMosraDataSetDataType dtype = NM_MOSRA_DATATYPE_STRING;
    if (mVtkDS != nullptr)
    {
        vtkDataSetAttributes* dsa = mVtkDS->GetAttributes(vtkDataSet::CELL);
        const int colidx = this->getColumnIndex(colname);
        if (colidx >=0 && dsa)
        {
            const int atype = dsa->GetArray(colidx)->GetArrayType();
            switch(atype)
            {
            case VTK_INT:
                dtype = NM_MOSRA_DATATYPE_INT;
                break;
            case VTK_DOUBLE:
                dtype = NM_MOSRA_DATATYPE_DOUBLE;
                break;
            case VTK_STRING:
            default:
                dtype = NM_MOSRA_DATATYPE_STRING;
                break;
            }
        }
    }
    else if (mOtbTab != nullptr)
    {
        const int colidx = this->getColumnIndex(colname);
        const otb::AttributeTable::TableColumnType otype = mOtbTab->GetColumnType(colidx);
        switch(otype)
        {
        case otb::AttributeTable::ATTYPE_INT:
            dtype = NM_MOSRA_DATATYPE_INT;
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            dtype = NM_MOSRA_DATATYPE_DOUBLE;
            break;
        case otb::AttributeTable::ATTYPE_STRING:
        default:
            dtype = NM_MOSRA_DATATYPE_STRING;
            break;
        }
    }
    else if (mSqlMod != nullptr)
    {
        if (mColTypes.empty())
        {
            QSqlDatabase db = mSqlMod->database();
            const QString qStr = QString("SELECT sql from sqlite_master where tbl_name = '%1'")
                    .arg(mTableName);

            QSqlQuery sqlq(db);
            if (!sqlq.exec(qStr))
            {
                MosraLogError(<< "Failed retreiving data type of column '" << colname.toStdString()
                              << "'!");
                sqlq.finish();
                return dtype;
            }

            if (sqlq.next())
            {
                QString colname, coltype;
                QString tabsql = sqlq.value(0).toString();
                int fpos = tabsql.indexOf('(', 0);
                int epos = tabsql.indexOf(')', 0);
                int tpos = 0;
                tabsql = tabsql.mid(fpos+1, epos-1-fpos);
                while (fpos >=0)
                {
                    epos = tabsql.indexOf(' ', fpos+1);
                    colname = tabsql.mid(fpos+1, epos-1-fpos);
                    fpos = tabsql.indexOf(',', epos+1);
                    if ((tpos = tabsql.indexOf("primary key", epos+1, Qt::CaseInsensitive)) < fpos)
                    {
                        coltype = tabsql.mid(epos+1, tpos-1-epos);
                    }
                    else
                    {
                        coltype = tabsql.mid(epos+1, fpos-1-epos);
                    }

                    NMMosraDataSetDataType ctype;
                    if (   coltype.compare("INT", Qt::CaseInsensitive) == 0
                        || coltype.compare("INTEGER", Qt::CaseInsensitive) == 0
                       )
                    {
                        ctype = NM_MOSRA_DATATYPE_INT;
                    }
                    else if (coltype.compare("REAL", Qt::CaseInsensitive) == 0)
                    {
                        ctype = NM_MOSRA_DATATYPE_DOUBLE;
                    }
                    else if (coltype.compare("TEXT", Qt::CaseInsensitive) == 0)
                    {
                        ctype = NM_MOSRA_DATATYPE_STRING;
                    }
                    mColTypes.insert(colname.trimmed(), ctype);
                }
            }
            sqlq.finish();
        }

        dtype = mColTypes[colname];
    }

    return dtype;
}

QString
NMMosraDataSet::getColumnName(const int &idx)
{
    QString ret="";
    if (mVtkDS != nullptr)
    {
        vtkDataSetAttributes* dsa = mVtkDS->GetAttributes(vtkDataSet::CELL);
        vtkAbstractArray* aa = dsa->GetAbstractArray(idx);
        if (aa != nullptr)
        {
            ret = aa->GetName();
        }
    }
    else if (mOtbTab.IsNotNull())
    {
        ret = mOtbTab->GetColumnName(idx).c_str();
    }
    else if (mSqlMod != nullptr)
    {
        if (idx >=0 && idx < mSqlMod->columnCount())
        {
            ret = mSqlMod->headerData(idx, Qt::Horizontal).toString();
        }
    }

    return ret;
}

vtkSmartPointer<vtkTable>
NMMosraDataSet::getDataSetAsVtkTable()
{
    vtkSmartPointer<vtkTable> tab = nullptr;
    if (mVtkDS != nullptr)
    {
        tab = vtkSmartPointer<vtkTable>::New();
        tab->SetRowData(mVtkDS->GetAttributes(vtkDataSet::CELL));
    }
    else if (mSqlMod != nullptr)
    {
        tab = vtkSmartPointer<vtkTable>::New();
        const int nrows = this->getNumRecs();
        const int ncols = mSqlMod->columnCount();

        // create the table structure
        for (int col=0; col < ncols; ++col)
        {
            vtkSmartPointer<vtkAbstractArray> ar;
            const QString cname = getColumnName(col);
            const NMMosraDataSetDataType coltype = getColumnType(cname);
            switch(coltype)
            {
                case NM_MOSRA_DATATYPE_INT:
                    ar = vtkSmartPointer<vtkLongArray>::New();
                    break;
                case NM_MOSRA_DATATYPE_DOUBLE:
                    ar = vtkSmartPointer<vtkDoubleArray>::New();
                    break;
                case NM_MOSRA_DATATYPE_STRING:
                    ar = vtkSmartPointer<vtkStringArray>::New();
                    break;
                default:
                    break;

            }
            ar->SetNumberOfComponents(1);
            ar->SetNumberOfTuples(nrows);
            ar->SetName(cname.toStdString().c_str());
            tab->AddColumn(ar);
         }

        // populate the table
        const QSqlDatabase db = mSqlMod->database();
        QSqlDriver* drv = db.driver();

        QString qStr = QString("SELECT * FROM %1")
                .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName));

        QSqlQuery q(db);
        if (!q.exec(qStr))
        {
            MosraLogError(<< "Failed to fecth records for '"
                          << mTableName.toStdString() << "'!");
        }

        long rowcount = 0;
        while (q.next())
        {
            for (int field=0; field < ncols; ++field)
            {
                const QVariant vres = q.value(field);
                switch(vres.type())
                {
                case QVariant::Int:
                    tab->SetValue(rowcount, field, vtkVariant(vres.toInt()));
                    break;
                case QVariant::Double:
                    tab->SetValue(rowcount, field, vtkVariant(vres.toDouble()));
                    break;
                case QVariant::String:
                    tab->SetValue(rowcount, field, vtkVariant(vres.toString().toStdString()));
                    break;
                default:
                    break;
                }
            }
            ++rowcount;
        }
        q.finish();
    }
    else if (mOtbTab.IsNotNull())
    {
        if (mOtbTab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
        {
            otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
            long long nrows = sqltab->GetNumRows();

            tab = vtkSmartPointer<vtkTable>::New();

            // ------------------------------------------------
            // create table structure
            std::vector<std::string> colnames;
            for (int col=0; col < sqltab->GetNumCols(); ++col)
            {
                vtkSmartPointer<vtkAbstractArray> ar;
                switch(sqltab->GetColumnType(col))
                {
                    case otb::AttributeTable::ATTYPE_INT:
                        ar = vtkSmartPointer<vtkLongArray>::New();
                        break;
                    case otb::AttributeTable::ATTYPE_DOUBLE:
                        ar = vtkSmartPointer<vtkDoubleArray>::New();
                        break;
                    case otb::AttributeTable::ATTYPE_STRING:
                        ar = vtkSmartPointer<vtkStringArray>::New();
                        break;
                    default:
                        break;
                }
                ar->SetNumberOfComponents(1);
                ar->SetNumberOfTuples(nrows);

                colnames.push_back(sqltab->GetColumnName(col));
                ar->SetName(colnames[col].c_str());

                tab->AddColumn(ar);
            }

            // ------------------------------------------------------
            // populate table
            sqltab->BeginTransaction();
            sqltab->PrepareBulkGet(colnames, "");

            std::vector<otb::AttributeTable::ColumnValue> values;
            values.resize(colnames.size());

            for (long long row=0; row < nrows; ++row)
            {
                if (sqltab->DoBulkGet(values))
                {
                    for (int wcol=0; wcol < sqltab->GetNumCols(); ++wcol)
                    {
                        switch(sqltab->GetColumnType(wcol))
                        {
                        case otb::AttributeTable::ATTYPE_INT:
                            tab->SetValue(row, wcol, values[wcol].ival);
                            break;
                        case otb::AttributeTable::ATTYPE_DOUBLE:
                            tab->SetValue(row, wcol, values[wcol].dval);
                            break;
                        case otb::AttributeTable::ATTYPE_STRING:
                            tab->SetValue(row, wcol, values[wcol].tval);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
            sqltab->EndTransaction();

            // clean up
            //            for (int v=0; v < values.size(); ++v)
            //            {
            //                if (values[v].type == otb::AttributeTable::ATTYPE_STRING)
            //                {
            //                    delete[] (values[v].tval);
            //                }
            //            }
        }
    }
    return tab;
}

int
NMMosraDataSet::getColumnIndex(const QString &colName)
{
    int val = -1;
    switch(mType)
    {
    case NM_MOSRA_DS_OTBTAB:
        val = mOtbTab->ColumnExists(colName.toStdString());
        break;

    case NM_MOSRA_DS_VTKDS:
        {
            vtkDataSetAttributes* dsAttr = mVtkDS->GetAttributes(vtkDataSet::CELL);
            if (dsAttr)
            {
                if (dsAttr->GetAbstractArray(colName.toStdString().c_str(), val) == NULL)
                {
                    val = -1;
                }
            }
        }
        break;

    case NM_MOSRA_DS_QTSQL:
        {
            for (int i=0; i < mSqlMod->columnCount(); ++i)
            {
                const QString name = mSqlMod->headerData(i, Qt::Horizontal).toString();
                if (name.compare(colName, Qt::CaseInsensitive) == 0)
                {
                    val = i;
                    break;
                }
            }
        }
        break;

    default:
        val = -1;
    }

    return val;
}


bool
NMMosraDataSet::prepareRowUpdate(const QStringList &colnames, bool bInsert)
{
    bool ret = false;

    if (mVtkDS != nullptr)
    {
        // just store the column names
        mRowUpdateColumns = colnames;
        ret = true;
    }
    else if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            std::vector<std::string> vcolnames;
            foreach (const QString name, colnames)
            {
                vcolnames.push_back(name.toStdString());
            }
            ret = sqltab->PrepareBulkSet(vcolnames, bInsert);
        }
    }
    else if (mSqlMod != nullptr)
    {
        QSqlDatabase db = mSqlMod->database();
        QSqlDriver* drv = db.driver();

        QString qStr;
        if (bInsert)
        {
            qStr = QString("INSERT OR REPLACE INTO %1 (")
                    .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName));

            for (int c=0; c < colnames.size(); ++c)
            {
                qStr += drv->escapeIdentifier(colnames.at(c), QSqlDriver::FieldName);
                if (c < colnames.size()-1)
                {
                    qStr += ",";
                }
                else
                {
                    qStr += ") VALUES (";
                }
            }

            for (int v=0; v < colnames.size(); ++v)
            {
                qStr += "?";
                if (v < colnames.size()-1)
                {
                    qStr += ", ";
                }
                else
                {
                    qStr += ")";
                }
            }
        }
        else
        {
            qStr = QString("UPDATE %1 SET ")
                    .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName));
            for (int c=0; c < colnames.size(); ++c)
            {
                qStr += drv->escapeIdentifier(colnames.at(c), QSqlDriver::FieldName) + " = ?";
                if (c < colnames.size()-1)
                {
                    qStr += ", ";
                }
            }
        }

        qStr += QString(" WHERE %1 == ? ;").arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName));

        mRowUpdateQuery = QSqlQuery(db);
        if (!mRowUpdateQuery.prepare(qStr))
        {

            MosraLogError(<< "Failed to prepare update query: " << qStr.toStdString() << std::endl
                          << "ERROR: " << mRowUpdateQuery.lastError().text().toStdString());
            ret = false;
        }
        else
        {
            MosraLogDebug(<< "UpdateRow query: " << qStr.toStdString());
            ret = true;
        }
    }

    return ret;
}

bool
NMMosraDataSet::prepareRowGet(const QStringList &colnames)
{
    bool ret = false;
    if (mVtkDS != nullptr)
    {
        mRowGetColumns = colnames;
        ret = true;
    }
    else if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            std::vector<std::string> vcolnames;
            for (int i=0; i < colnames.size(); ++i)
            {
                vcolnames.push_back(colnames.at(i).toStdString());
            }
            QString wc = QString("WHERE \"%1\" == ?1").arg(sqltab->GetPrimaryKey().c_str());
            ret = sqltab->PrepareBulkGet(vcolnames, wc.toStdString());
        }
    }
    else if (mSqlMod != nullptr)
    {
        QSqlDatabase db = mSqlMod->database();
        QSqlDriver* drv = db.driver();

        QString colstr;
        for (int i=0; i < colnames.size(); ++i)
        {
            colstr += drv->escapeIdentifier(colnames.at(i), QSqlDriver::FieldName);
            if (i < colnames.size()-1)
            {
                colstr += ",";
            }
        }

        // just in case we got an empty list of colnames
        if (colstr.isEmpty())
        {
            return false;
        }

        QString qStr = QString("SELECT %1 from %2 WHERE %3 == ?")
                        .arg(colstr)
                        .arg(drv->escapeIdentifier(mTableName, QSqlDriver::TableName))
                        .arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName));

        mRowGetQuery = QSqlQuery(db);
        if (!mRowGetQuery.prepare(qStr))
        {
            MosraLogError(<< "Failed preparing 'get row' query: "
                          << qStr.toStdString() << std::endl
                          << "ERROR: " << mRowGetQuery.lastError().text().toStdString());
        }
        else
        {
            MosraLogDebug(<< "GetRow query: " << qStr.toStdString());
            ret = true;
        }
    }

    return ret;
}

bool
NMMosraDataSet::updateRow(const QVariantList &values, const int &row)
{
    bool ret = false;
    if (mVtkDS != nullptr)
    {
        ret = true;
        vtkDataSetAttributes* dsa = mVtkDS->GetAttributes(vtkDataSet::CELL);
        for (int var=0; var < values.size(); ++var)
        {
            const QVariant val = values.at(var);
            const QString colname = mRowUpdateColumns.at(var);

            switch(val.type())
            {
            case QVariant::Int:
                {
                    vtkIntArray* ia = vtkIntArray::SafeDownCast(dsa->GetAbstractArray(colname.toStdString().c_str()));
                    ia->SetValue(row, val.toInt());
                }
                break;

           case QVariant::Double:
                {
                    vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(dsa->GetAbstractArray(colname.toStdString().c_str()));
                    da->SetValue(row, val.toDouble());
                }
                break;

            case QVariant::String:
                 {
                     vtkStringArray* sa = vtkStringArray::SafeDownCast(dsa->GetAbstractArray(colname.toStdString().c_str()));
                     sa->SetValue(row, val.toString().toStdString());
                 }
                 break;

            default:
                ret = false;
                break;

            }
        }
    }
    else if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            ret = true;
            std::vector<otb::AttributeTable::ColumnValue> cvalues;
            for (int var=0; var < values.size(); ++var)
            {
                const QVariant val = values.at(var);
                switch(val.type())
                {
                case QVariant::Int:
                case QVariant::LongLong:
                    {
                      otb::AttributeTable::ColumnValue lv;
                      lv.type = otb::AttributeTable::ATTYPE_INT;
                      lv.ival = val.toLongLong();
                      cvalues.push_back(lv);
                    }
                    break;

                case QVariant::Double:
                    {
                      otb::AttributeTable::ColumnValue dv;
                      dv.type = otb::AttributeTable::ATTYPE_DOUBLE;
                      dv.dval = val.toDouble();
                      cvalues.push_back(dv);
                    }
                    break;

                case QVariant::String:
                    {
                      otb::AttributeTable::ColumnValue sv;
                      sv.type = otb::AttributeTable::ATTYPE_STRING;
                      sv.tval = const_cast<char*>(val.toString().toStdString().c_str());
                      cvalues.push_back(sv);
                    }
                    break;

                default:
                    ret = false;
                    break;
                }
            }
            ret = sqltab->DoBulkSet(cvalues, row);
        }
    }
    else if (mSqlMod != nullptr)
    {
        for (int var=0; var < values.size(); ++var)
        {
            mRowUpdateQuery.bindValue(var, values.at(var));
        }
        mRowUpdateQuery.bindValue(values.size(), QVariant::fromValue((int)row));

        if (!mRowUpdateQuery.exec())
        {
            NMDebugAI(<< "Failed updating table record: "
                           << mRowUpdateQuery.lastQuery().toStdString() << std::endl
                           << mRowUpdateQuery.lastError().text().toStdString());
            MosraLogError(<< "Failed updating table record: "
                          << mRowUpdateQuery.lastQuery().toStdString() << std::endl
                          << mRowUpdateQuery.lastError().text().toStdString());
            mRowUpdateQuery.finish();
            ret = false;
        }
        else
        {
            mRowUpdateQuery.finish();
            ret = true;
        }
    }

    return ret;
}

bool
NMMosraDataSet::getRowValues(QVariantList &values, const int &row)
{
    bool ret = false;
    if (mVtkDS != nullptr)
    {
        ret = true;
        vtkDataSetAttributes* dsa = mVtkDS->GetAttributes(vtkDataSet::CELL);
        if (dsa != nullptr)
        {
            for (int val=0; val < values.size(); ++val)
            {
                QVariant& varVal = values[val];
                const QVariant::Type vt = varVal.type();
                vtkAbstractArray* aa = dsa->GetAbstractArray(mRowGetColumns.at(val).toStdString().c_str());

                switch(vt)
                {
                case QVariant::Int:
                    {
                        const int aatype = aa->GetArrayType();
                        switch(aatype)
                        {
                        case VTK_LONG:
                            {
                                vtkLongArray* la = vtkLongArray::SafeDownCast(aa);
                                if (la != nullptr)
                                {
                                    varVal = QVariant::fromValue(la->GetValue(row));
                                }
                            }
                            break;

                        case VTK_INT:
                            {
                                vtkIntArray* ia = vtkIntArray::SafeDownCast(aa);
                                if (ia != nullptr)
                                {
                                    varVal = QVariant::fromValue(ia->GetValue(row));
                                }
                            }
                            break;

                        default:
                            break;
                        }
                    }
                    break;

                case QVariant::Double:
                    {
                        vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(dsa->GetAbstractArray(mRowGetColumns.at(val).toStdString().c_str()));
                        if (da == nullptr)
                        {
                            MosraLogError(<< "Failed fetching row value for column '" << mRowGetColumns.at(val).toStdString() << "'!");
                            return false;
                        }
                        varVal = QVariant::fromValue(da->GetValue(row));
                    }
                    break;

                case QVariant::String:
                    {
                        vtkStringArray* sa = vtkStringArray::SafeDownCast(dsa->GetAbstractArray(mRowGetColumns.at(val).toStdString().c_str()));
                        if (sa == nullptr)
                        {
                            MosraLogError(<< "Failed fetching row value for column '" << mRowGetColumns.at(val).toStdString() << "'!");
                            return false;
                        }
                        varVal = QVariant::fromValue(QString(sa->GetValue(row).c_str()));
                    }
                    break;

                default:
                    ret = false;
                    break;
                }
            }
        }
    }
    else if (mOtbTab.IsNotNull())
    {
        otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mOtbTab.GetPointer());
        if (sqltab != nullptr)
        {
            std::vector<otb::AttributeTable::ColumnValue> cvalues;
//            for (int var=0; var < values.size(); ++var)
//            {
//                const QVariant& val = values.at(var);
//                switch(val.type())
//                {
//                case QVariant::Int:
//                case QVariant::LongLong:
//                    {
//                      otb::AttributeTable::ColumnValue lv;
//                      lv.type = otb::AttributeTable::ATTYPE_INT;
//                      lv.ival = val.toLongLong();
//                      cvalues.push_back(lv);
//                    }
//                    break;

//                case QVariant::Double:
//                    {
//                      otb::AttributeTable::ColumnValue dv;
//                      dv.type = otb::AttributeTable::ATTYPE_DOUBLE;
//                      dv.dval = val.toDouble();
//                      cvalues.push_back(dv);
//                    }
//                    break;

//                case QVariant::String:
//                    {
//                      int strsize = ::atoi(val.toString().toStdString().c_str());
//                      otb::AttributeTable::ColumnValue sv;
//                      cvalues.push_back(sv);
//                      cvalues.back().type = otb::AttributeTable::ATTYPE_STRING;
//                      cvalues.back().tval = new char[strsize];//const_cast<char*>(val.toString().toStdString().c_str());
//                      //::sprintf(sv.tval, "\0");

//                    }
//                    break;

//                default:
//                    break;
//                }
//            }
            cvalues.resize(values.size());
            otb::AttributeTable::ColumnValue rowVal;
            rowVal.type = otb::AttributeTable::ATTYPE_INT;
            rowVal.ival = row;
            cvalues.push_back(rowVal);

            ret = sqltab->DoBulkGet(cvalues);


            for (int var=0; var < values.size(); ++var)
            {
                QVariant& val = values[var];
                switch(val.type())
                {
                case QVariant::Int:
                case QVariant::LongLong:
                    val = QVariant::fromValue(cvalues[var].ival);
                    break;

                case QVariant::Double:
                    val = QVariant::fromValue(cvalues[var].dval);
                    break;

                case QVariant::String:
                    {
                        const QString tstr = const_cast<char*>(cvalues[var].tval);
                        val = QVariant::fromValue(tstr);
                        //                        if (cvalues[var].tval != nullptr)
                        //                        {
                        //                            delete[] cvalues[var].tval;
                        //                        }
                        //                        cvalues[var].tval = nullptr;
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }
    else if (mSqlMod != nullptr)
    {
        mRowGetQuery.bindValue(0, QVariant::fromValue(row));

        if (!mRowGetQuery.exec() || !mRowGetQuery.next())
        {
            MosraLogError(<< "Failed getting row values: "
                          << mRowGetQuery.lastQuery().toStdString() << std::endl
                          << "ERROR: " << mRowGetQuery.lastError().text().toStdString());
            mRowGetQuery.finish();
            ret = false;
        }
        else
        {
            for (int val=0; val < values.size(); ++val)
            {
                values[val] = QVariant::fromValue(mRowGetQuery.value(val));
            }
            mRowGetQuery.finish();
            ret = true;
        }
    }

    return ret;
}


////////////////////////////////
/// NMMosra implementation
////////////////////////////////

// initialising static class members
const std::string NMMosra::ctxNMMosra = "NMMosra";
const QString NMMosra::msOPTIONS = QStringLiteral("OPTIONS");
const QString NMMosra::msSDU = QStringLiteral("SDU");
const QStringList NMMosra::mslLowDims = {NMMosra::msSDU, NMMosra::msOPTIONS};
const QMap<QChar, QChar> NMMosra::mReverseLeft    = NMMosra::rl();
const QMap<QChar, QChar> NMMosra::mReverseRight   = NMMosra::rr();
const QMap<QString, int> NMMosra::mParseOperators = NMMosra::po();
const QMap<QString, int> NMMosra::mAMPLOperators  = NMMosra::aov();
const QMap<QString, int> NMMosra::mAMPLFunctions  = NMMosra::amplfunc();
const QList<QChar> NMMosra::mWhitespaces = NMMosra::ws();
const QList<QChar> NMMosra::mOpCharList  = NMMosra::opchar();
const QStringList  NMMosra::mLoopNames   = NMMosra::lnames();
const QList<QChar> NMMosra::mNumCharList = NMMosra::numchar();
const QMap<QString, int> NMMosra::mmOpLevel = NMMosra::opLevel();

// max size of O_seg
//const size_t NMMosra::miOsegSizeLimit = 2136997888;
//using VarAdmin = NMMosra::VarAdmin;

NMMosra::NMMosra(QObject* parent) //: QObject(parent)
    : mProblemFilename(""), mProblemType(NM_MOSO_LP),
      mSolverType(NM_SOLVER_NONE)
{
    NMDebugCtx(ctxNMMosra, << "...");

    this->mDataSet = new NMMosraDataSet(this);

    this->mLogger = 0;
    this->mProcObj = 0;
    this->setParent(parent);
    this->mLp = new HLpHelper();
    this->reset();

    mScenarioName = "My Optimisation Scenario";
    mdMinAlloc = 1.0;
    mbEnableLUCControl = false;
    mbLockInLuGrps = false;
    miMaxOptAlloc = 3;
    mpNLFile = nullptr;

    msBkpLUCControlField = "LuCtrBkp";
    miNumRecLUCChange = 0;

    mdLuLbndFactor = 0.0;

    mbGAMultiThreading = true;
    mGAConstraintTolerance = 0.2;
    mGAPopulation = 50;
    mGAMaxGeneration = 100;

    // seed the random number generator
    srand(time(0));

    NMDebugCtx(ctxNMMosra, << "done!");
}

NMMosra::~NMMosra()
{
    NMDebugCtx(ctxNMMosra, << "...");

    if (this->mLp != 0)
    {
        this->mLp->DeleteLp();
        delete this->mLp;
    }

    NMDebugCtx(ctxNMMosra, << "done!");
}

void
NMMosra::setItkProcessObject(itk::ProcessObject *obj)
{
    mProcObj = obj;
    mDataSet->setProcObj(obj);
    mDataSet->setLogger(nullptr);
    mLogger = nullptr;
}

void
NMMosra::setLogger(NMLogger *logger)
{
    mLogger = logger;
    mDataSet->setLogger(logger);
    mDataSet->setProcObj(nullptr);
}

void
NMMosra::setDataSet(const vtkDataSet* dataset)
{
    if (dataset != 0)
        this->mDataSet->setDataSet(const_cast<vtkDataSet*>(dataset));
    else
    {
        MosraLogError( << "vtkDataSet data set is NULL!");
    }
}

void
NMMosra::setDataSet(otb::AttributeTable::Pointer otbtab)
{
    if (otbtab.IsNotNull())
        this->mDataSet->setDataSet(otbtab);
    else
    {
        MosraLogError( << "otb::AttributeTable is NULL!");
    }
}

void
NMMosra::setDataSet(const QSqlTableModel *sqlmod)
{
    if (sqlmod != nullptr)
    {
        this->mDataSet->setDataSet(const_cast<QSqlTableModel*>(sqlmod));
    }
    else
    {
        MosraLogError( << "QSqlTableModel is NULL!");
    }
}

void NMMosra::reset(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    this->mLp->DeleteLp();
    this->msLosFileName.clear();

    this->mlNumDVar = 0;
    this->mlNumOptFeat = 0;
    this->mlCriCons = 0;
    this->mlLpCols = 0;
    this->mdAreaSelected = 0;
    this->mdAreaTotal = 0;
    this->mdAreaScalingFactor = 1.0;

    this->muiTimeOut = 60;
    this->miNumOptions = 0;
    this->meDVType = NMMosra::NM_MOSO_REAL;
    this->meScalMeth = NMMosra::NM_MOSO_WSUM;

    this->mmslAreaCons.clear();
    this->mmslCriCons.clear();
    this->mmslCriteria.clear();
    this->mmslEvalFields.clear();
    this->mmslObjCons.clear();
    this->mmslObjectives.clear();
    this->mmslIncentives.clear();
    this->mIncNamePair.clear();

    this->mbBreakAtFirst = false;
    this->mbCanceled = false;

    this->msOptFeatures.clear();
    this->mDataSet->reset();
    msDataPath = std::getenv("HOME");
    mslPerturbItems.clear();
    mlflPerturbUncertainties.clear();
    mflUncertainties.clear();
    mlReps=1;

    mslPerfSumZones.clear();

    NMDebugCtx(ctxNMMosra, << "done!")
}

bool
NMMosra::doBatch()
{
    bool ret = false;

    if (   !msDataPath.isEmpty()
        && mslPerturbItems.size() > 0
        && mlflPerturbUncertainties.size() > 0
       )
    {
        ret = true;
    }

    return ret;
}


int NMMosra::loadSettings(QString fileName)
{
    NMDebugCtx(ctxNMMosra, << "...")

    QFile los(fileName);
    if (!los.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        MosraLogError( << "failed reading settings file!")
        NMDebugCtx(ctxNMMosra, << "done!")
        return 0;
    }
    QTextStream str(&los);

    int ret = parseStringSettings(str.readAll());

    los.close();

    this->msLosFileName = fileName;
    NMDebugCtx(ctxNMMosra, << "done!")
    return ret;
}

int NMMosra::parseStringSettings(QString strSettings)
{
    NMDebugCtx(ctxNMMosra, << "...")

    // reset this objects vars
    this->reset();

    QTextStream str;
    str.setString(&strSettings);

    QString rep;
    QTextStream sReport(&rep);
    QString sLine, sVarName, sValueStr;
    int length, sep;
    long lnumOpt=0;
    long numline = 0;

    enum ParSec {
        problem,
        criteria,
        parameters,
        variables,
        equations,
        incentives,
        objectives,
        linearcons,
        nonlinearcons,
        logiccons,
        arealcons,
        featcons,
        featsetcons,
        zonecons,
        cricons,
        objcons,
        batch,
        scaling,
        nosection
    };

    ParSec section = nosection;

    sReport << "Import Report" << Qt::endl << Qt::endl;
    MosraLogDebug( << "parsing settings file ..." << std::endl)
    while (!str.atEnd())
    {
        sLine = str.readLine();

        //increment the line number
        numline++;

        //eliminate leading and trailing blanks
        sLine = sLine.simplified();

        // skip empty lines and comments
        if (sLine.isEmpty() || !sLine.left(1).compare(tr("#")))
            continue;

        //detect processing mode
        if (!sLine.compare(tr("<PROBLEM>"), Qt::CaseInsensitive))
        {
            section = problem;
            continue;
        }
        else if (!sLine.compare(tr("<CRITERIA>"), Qt::CaseInsensitive))
        {
            section = criteria;
            continue;
        }
        else if (!sLine.compare(tr("<PARAMETERS>"), Qt::CaseInsensitive))
        {
            section = parameters;
            continue;
        }
        else if (!sLine.compare(tr("<VARIABLES>"), Qt::CaseInsensitive))
        {
            section = variables;
            continue;
        }
        else if (!sLine.compare(tr("<EQUATIONS>"), Qt::CaseInsensitive))
        {
            section = equations;
            continue;
        }
        else if (!sLine.compare(tr("<INCENTIVES>"), Qt::CaseInsensitive))
        {
            section = incentives;
            continue;
        }
        else if (!sLine.compare(tr("<OBJECTIVES>"), Qt::CaseInsensitive))
        {
            section = objectives;
            continue;
        }
        else if (!sLine.compare(tr("<ALGEBRAIC_LINEAR_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = linearcons;
            continue;
        }
        else if (!sLine.compare(tr("<ALGEBRAIC_NONLINEAR_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = nonlinearcons;
            continue;
        }
        else if (!sLine.compare(tr("<LOGIC_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = logiccons;
            continue;
        }
        else if (!sLine.compare(tr("<AREAL_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = arealcons;
            continue;
        }
        else if (!sLine.compare(tr("<FEATURE_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = featcons;
            continue;
        }
        else if (!sLine.compare(tr("<FEATSET_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = featsetcons;
            continue;
        }
        else if (!sLine.compare(tr("<ZONE_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = zonecons;
            continue;
        }
        else if (!sLine.compare(tr("<CRITERIA_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = cricons;
            continue;
        }
        else if (!sLine.compare(tr("<OBJECTIVE_CONSTRAINTS>"), Qt::CaseInsensitive))
        {
            section = objcons;
            continue;
        }
        else if (!sLine.compare(tr("<PARAMETER_SCALING>"), Qt::CaseInsensitive))
        {
            section = scaling;
            continue;
        }
        else if (!sLine.compare(tr("<BATCH_SETTINGS>"), Qt::CaseInsensitive))
        {
            section = batch;
            continue;
        }
        else if (!sLine.right(5).compare(tr("_END>"), Qt::CaseInsensitive))
        {
            section = nosection;
            continue;
        }

        //check, if we are dealing with a valid data section
        if (section == nosection)
        {
            sReport << "Line " << numline << " does not belong to a valid data section" << Qt::endl;
            continue;
        }

        //look for an equal sign to indicate data assignment
        sep = sLine.indexOf('=');

        //if there is no equal sign skip processing of this line
        if (sep == -1)
        {
            sReport << "Line " << numline << " contains invalid data" << Qt::endl;
            continue;
        }

        //split the line string into the variable name and the value string
        length = sLine.size();
        sVarName = sLine.left(sep);
        sValueStr = sLine.right(length-1-sep);


        //process the line depending on the current data section
        //----------------------------------------------PROBLEM-------------
        switch(section)
        {
        case problem:
        {
            if (sVarName.compare(tr("DVTYPE"), Qt::CaseInsensitive) == 0)
            {
                if (sValueStr.indexOf(tr("DVTYPE_BINARY"), Qt::CaseInsensitive) != -1)
                    this->meDVType = NMMosra::NM_MOSO_BINARY;
                else if (sValueStr.indexOf(tr("DVTYPE_INTEGER"), Qt::CaseInsensitive) != -1)
                    this->meDVType = NMMosra::NM_MOSO_INT;
                else // DVTYPE_CONTINUOUS
                    this->meDVType = NMMosra::NM_MOSO_REAL;

                MosraLogInfo( << "DEVTYPE: " << this->meDVType << std::endl)
            }
            else if (sVarName.compare(tr("CRITERION_LAYER"), Qt::CaseInsensitive) == 0)
            {
                this->msLayerName = sValueStr;
                MosraLogInfo( << "LayerName: " << this->msLayerName.toStdString() << std::endl)
            }
            else if (sVarName.compare(tr("LAND_USE_FIELD"), Qt::CaseInsensitive) == 0)
            {
                this->msLandUseField = sValueStr;
                MosraLogInfo( << "LandUseField: " << this->msLandUseField.toStdString() << std::endl)
            }
            else if (sVarName.compare(tr("LUC_CONTROL_FIELD"), Qt::CaseInsensitive) == 0)
            {
                this->msLUCControlField = sValueStr;
                MosraLogInfo( << "LUCControlField: " << this->msLUCControlField.toStdString() << std::endl)
            }
            else if (sVarName.compare(tr("ENABLE_LUC_CONTROL"), Qt::CaseInsensitive) == 0)
            {
                if (    sValueStr.compare(QStringLiteral("on"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0
                   )
                {
                    this->mbEnableLUCControl = true;
                }
                else
                {
                    this->mbEnableLUCControl = false;
                }

                MosraLogInfo( << "EnableLUCControl: " << (mbEnableLUCControl ? "on" : "off") << std::endl)
            }
            else if (sVarName.compare(tr("LOCK_IN_LU_GRPS"), Qt::CaseInsensitive) == 0)
            {
                if (    sValueStr.compare(QStringLiteral("on"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0
                   )
                {
                    this->mbLockInLuGrps = true;
                }
                else
                {
                    this->mbLockInLuGrps = false;
                }

                MosraLogInfo( << "LockInLuGrps: " << (mbLockInLuGrps ? "on" : "off") << std::endl)
            }
            else if (sVarName.compare(tr("GA_MULTITHREADING"), Qt::CaseInsensitive) == 0)
            {
                if (    sValueStr.compare(QStringLiteral("on"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
                     || sValueStr.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0
                   )
                {
                    this->mbGAMultiThreading = true;
                }
                else
                {
                    this->mbGAMultiThreading = false;
                }

                MosraLogInfo( << "GA Multithreading: " << (mbGAMultiThreading ? "on" : "off") << std::endl)
            }
            else if (sVarName.compare(tr("GA_CONS_TOLERANCE"), Qt::CaseInsensitive) == 0)
            {
                bool bConv = false;
                double consTol = sValueStr.toDouble(&bConv);
                if (bConv && consTol >= 0)
                {
                    this->mGAConstraintTolerance = consTol;
                }
                else
                {
                    MosraLogError(<< "Invalid value specified for 'GA_CONS_TOLERANCE'! "
                                  << "Please specify a real value >= 0 !" << std::endl);
                }

                MosraLogInfo( << "GA_CONS_TOLERANCE: " << this->mGAConstraintTolerance << std::endl)
            }
            else if (sVarName.compare(tr("GA_POPULATION"), Qt::CaseInsensitive) == 0)
            {
                bool bConv = false;
                double gaPop = sValueStr.toInt(&bConv);
                if (bConv && gaPop > 0)
                {
                    this->mGAPopulation = gaPop;
                }
                else
                {
                    MosraLogError(<< "Invalid value specified for 'GA_POPULATION'! "
                                  << "Please specify an integer value > 0 !" << std::endl);
                }

                MosraLogInfo( << "GA_POPULATION: " << this->mGAConstraintTolerance << std::endl)
            }
            else if (sVarName.compare(tr("GA_MAX_GENERATION"), Qt::CaseInsensitive) == 0)
            {
                bool bConv = false;
                double gaMaxGen = sValueStr.toInt(&bConv);
                if (bConv && gaMaxGen > 0)
                {
                    this->mGAMaxGeneration = gaMaxGen;
                }
                else
                {
                    MosraLogError(<< "Invalid value specified for 'GA_MAX_GENERATION'! "
                                  << "Please specify an int value > 0 !" << std::endl);
                }

                MosraLogInfo( << "GA_MAX_GENERATION: " << this->mGAConstraintTolerance << std::endl)
            }
            else if (sVarName.compare(tr("LU_LBND_FACTOR"), Qt::CaseInsensitive) == 0)
            {
                bool bLuConv = false;
                double dLowBndFact = sValueStr.toDouble(&bLuConv);

                if (bLuConv)
                {
                    // ensure value is in interval [0, 1]
                    bool bcorr = false;
                    if (dLowBndFact < 0)
                    {
                        bcorr = true;
                    }
                    if (dLowBndFact > 1)
                    {
                        bcorr = true;
                    }

                    if (bcorr)
                    {
                        dLowBndFact = std::min(1.0, std::max(0.0, dLowBndFact));
                        MosraLogInfo(<< "Invalid value specified for 'LU_BND_FACTOR' := " << sValueStr.toStdString()
                                      << "! As this values must be in the interval [0, 1], "
                                      << "LUMASS adjusted the value to := " << dLowBndFact << " .");
                    }
                    this->mdLuLbndFactor = dLowBndFact;
                }
                else if (!bLuConv)
                {
                    MosraLogError(<< "Invalid value specified for 'LU_BND_FACTOR'!"
                                  << "Please specify a value in the interval [0, 1]!" << std::endl);
                }

                MosraLogInfo( << "LU_LBND_FACTOR: " << this->mdLuLbndFactor << std::endl)
            }
            else if (sVarName.compare(tr("MAX_OPTION_ALLOC"), Qt::CaseInsensitive) == 0)
            {
                //this->msAreaField = sValueStr;
                bool bConv = false;
                int maxAlloc = sValueStr.toInt(&bConv);
                if (bConv && maxAlloc > 0)
                {
                    this->miMaxOptAlloc = maxAlloc;
                }
                else
                {
                    MosraLogError(<< "Invalid value specified for 'MAX_OPTION_ALLOC'! "
                                  << "Please specify an integer value >= 1 !" << std::endl);
                }

                MosraLogInfo( << "MaxOptionAllocation: " << this->miMaxOptAlloc << std::endl)
            }
            else if (sVarName.compare(tr("AREA_FIELD"), Qt::CaseInsensitive) == 0)
            {
                this->msAreaField = sValueStr;
                MosraLogInfo( << "AreaField: " << this->msAreaField.toStdString() << std::endl)
            }
            else if (sVarName.compare(tr("PERFORMANCE_SUM_ZONES"), Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->mslPerfSumZones = sValueStr.split(" ", Qt::SkipEmptyParts);
                }
                MosraLogInfo( << "PerformanceSumZones: " << mslPerfSumZones.join(" ").toStdString() << std::endl)
            }
            else if (sVarName.compare("TIMEOUT", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    if (sValueStr.compare("break_at_first", Qt::CaseInsensitive) == 0)
                    {
                        this->muiTimeOut = 0;
                        this->mbBreakAtFirst = true;
                        MosraLogInfo(<< "Solver timeout: break at first feasible solution!" << std::endl);
                    }
                    else
                    {
                        bool bok;
                        unsigned int timeout = sValueStr.toUInt(&bok);
                        if (bok)
                        {
                            this->mbBreakAtFirst = false;
                            this->muiTimeOut = timeout;
                            MosraLogInfo(<< "Solver timeout: " << timeout << std::endl);
                        }
                    }
                }
            }
            else if (sVarName.compare("DATAPATH", Qt::CaseInsensitive) == 0)
            {
                this->msDataPath = sValueStr;
                MosraLogInfo(<< "batch data path: " << this->msDataPath.toStdString() << std::endl);
            }
            else if (sVarName.compare("OPT_FEATURES", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->msOptFeatures = sValueStr;
                    MosraLogInfo(<< "OPT_FEATURES: " << this->msOptFeatures.toStdString() << std::endl);
                }
            }
            else if (sVarName.compare("SOLVER", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->msSolver = sValueStr;
                    MosraLogInfo(<< "SOLVER: " << this->msSolver.toStdString() << std::endl);

                    if (msSolver.compare(QStringLiteral("ipopt"), Qt::CaseInsensitive) == 0)
                    {
                        mSolverType = NM_SOLVER_IPOPT;
                    }
                    else if (   msSolver.compare(QStringLiteral("lpsolve"), Qt::CaseInsensitive) == 0
                             || msSolver.compare(QStringLiteral("lp_solve"), Qt::CaseInsensitive) == 0
                            )
                    {
                        mSolverType = NM_SOLVER_LPSOLVE;
                    }
                    else if (msSolver.compare(QStringLiteral("openGA"), Qt::CaseInsensitive) == 0)
                    {
                        mSolverType = NM_SOLVER_GA;
                        MosraLogWarn(<< "FYI, the GA solver is still under development! So, "
                                     << "don't be surpised if things are not working as expected, or "
                                     << "if nothing at all is working! We're working on it!");
                    }
                }
            }
            else if (sVarName.compare(QStringLiteral("MIN_ALLOC"), Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    bool bok = false;
                    const double minalloc = sValueStr.toDouble(&bok);
                    if (!bok)
                    {
                        MosraLogError(<< "Invalid number '" << sValueStr.toStdString()
                                      << "' specified for 'MIN_ALLOC'!" << std::endl);
                    }
                    else
                    {
                        mdMinAlloc = minalloc;
                        MosraLogInfo(<< "MIN_ALLOC: " << mdMinAlloc << std::endl);
                    }
                }
            }
        }
        break;
        //---------------------------------------------CRITERIA-----------
        case criteria:
        {
            if (sVarName.compare(tr("NUM_OPTIONS"), Qt::CaseInsensitive) == 0)
            {
                bool ok;
                long lo = sValueStr.toLong(&ok);
                if (ok)
                    this->miNumOptions = lo;
                else
                {
                    MosraLogError(<< "Line " << numline << " contains an invalid number" << std::endl;)
                    sReport << "Line " << numline << " contains an invalid number" << Qt::endl;
                }


                MosraLogInfo( << "number of resource options: " << this->miNumOptions << std::endl)
            }
            else if (sVarName.compare(msOPTIONS, Qt::CaseInsensitive) == 0)
            {
                QStringList tmpList = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (tmpList.size() == this->miNumOptions)
                {
                    this->mslOptions.clear();
                    this->mslOptions << tmpList;
                }
                else
                {
                    MosraLogError(<< "Line " << numline << " contains an invalid number of options" << std::endl)
                    sReport << "Line " << numline << " contains an invalid number of options" << Qt::endl;
                }

                MosraLogInfo( << "options: " << this->mslOptions.join(tr(" ")).toStdString() << std::endl)

            }
            else if (sVarName.indexOf(tr("OPT_GRP_"), Qt::CaseInsensitive) != -1)
            {
                QStringList grp = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (grp.size() > 0)
                {
                    mlslOptGrps << grp;
                }
            }
            else if (sVarName.indexOf(tr("CRI_"), Qt::CaseInsensitive) != -1)
            {
                QStringList criFieldNames = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (criFieldNames.size() == this->miNumOptions + 1)
                {
                    QString scri = criFieldNames.at(0);
                    criFieldNames.removeAt(0);
                    this->mmslCriteria.insert(scri, criFieldNames);

                    MosraLogDebug( << "criterion: " << scri.toStdString() << " " << this->mmslCriteria.find(scri).value().join(tr(" ")).toStdString() << std::endl);
                }
                else
                {
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criteria" << std::endl;)
                    sReport << "Line " << numline << " contains an invalid number of criteria" << Qt::endl;
                }
            }
            else if (sVarName.indexOf(tr("EVAL_"), Qt::CaseInsensitive) != -1)
            {
                QStringList evalFieldNames = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (evalFieldNames.size() == this->miNumOptions + 1)
                {
                    QString scri = evalFieldNames.at(0);
                    evalFieldNames.removeAt(0);
                    this->mmslEvalFields.insert(scri, evalFieldNames);

                    MosraLogDebug( << "criterion evaluation fields: " << scri.toStdString() << " " << this->mmslEvalFields.find(scri).value().join(tr(" ")).toStdString() << std::endl);
                }
                else
                {
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criterion evaluation fields" << std::endl;)
                    sReport << "Line " << numline << " contains an invalid number of criterion evaluation fields" << Qt::endl;
                }
            }

        }
        break;
        //---------------------------------------------Parameters-----------
        case parameters:
        {
            if (sVarName.indexOf(tr("PAR_"), Qt::CaseInsensitive) != -1)
            {
                QStringList parDef = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (parDef.size() != 0)
                {
                    QStringList parSpec = parDef.takeAt(0).split(":");
                    mmslParameters.insert(parSpec[0], parDef);
                    if (parSpec.size() == 2)
                    {
                        mslDistinctParameters << parSpec[0];
                    }
                }
            }
        }
        break;
        //---------------------------------------------Variables-----------
        case variables:
        {
            if (sVarName.indexOf(tr("VAR_"), Qt::CaseInsensitive) != -1)
            {
                const QStringList types = {"c", "i", "b"};
                QStringList varbnds;
                QStringList bndsep = sValueStr.split(tr(":"), Qt::SkipEmptyParts);

                QStringList varDef = bndsep.at(0).split(tr(" "), Qt::SkipEmptyParts);
                QString varName;
                if (varDef.size() != 0)
                {
                    varName = varDef.takeAt(0);
                }
                else
                {
                    MosraLogError(<< "No variable name defined for '" << sVarName.toStdString() << "'!" << std::endl);
                    return false;
                }

                if (bndsep.size() == 2)
                {
                    QStringList bnds = bndsep.at(1).split(tr(" "), Qt::SkipEmptyParts);
                    if (    bnds.size() == 3
                         && types.contains(bnds.at(0))
                       )
                    {
                        varbnds << bnds;
                    }
                    else
                    {
                        MosraLogWarn(<< "Invalid Domain specification for variable '"
                                     << varName.toStdString() << "'! We're gonna use "
                                     << " default values: c 0 " << std::numeric_limits<double>::max()
                                     << std::endl);
                        varbnds << QStringLiteral("c");
                        varbnds << QStringLiteral("0");
                        varbnds << QString("%1").arg(std::numeric_limits<double>::max());
                    }

                    /*
                    if (bnds.size() == 2)
                    {
                        for (int b=0; b < 2; ++b)
                        {
                            bConv = false;
                            double minmax = bnds[b].toDouble(&bConv);
                            if (bConv)
                            {
                                varbnds.push_back(minmax);
                            }
                            else
                            {
                                if (b == 0)
                                {
                                    varbnds.push_back(0);
                                }
                                else
                                {
                                    varbnds.push_back(std::numeric_limits<double>::max());
                                }
                            }
                        }
                    }
                    else if (bnds.size() == 1)
                    {
                        bConv = false;
                        double lower = bnds.at(0).toDouble(&bConv);
                        if (bConv)
                        {
                            varbnds.push_back(lower);
                        }
                        else
                        {
                            varbnds.push_back(0);
                        }

                        varbnds.push_back(std::numeric_limits<double>::max());
                    }*/
                }
                // we just fill the var domain specification with default values
                else
                {
                    MosraLogWarn(<< "Domain of variable '" << varName.toStdString() << "' not specified, "
                                 << "so we're setting default values: c 0 " << std::numeric_limits<double>::max()
                                 << std::endl);
                    varbnds << QStringLiteral("c");
                    varbnds << QStringLiteral("0");
                    varbnds << QString("%1").arg(std::numeric_limits<double>::max());
                }

                mmslVariables.insert(varName, varDef);
                mmslVarBoundsMap.insert(varName, varbnds);

            }
        }
        break;
        //---------------------------------------------Equations-----------
        case equations:
        {
            if (sVarName.indexOf(tr("EQN_"), Qt::CaseInsensitive) != -1)
            {
                QStringList eqnDef = sValueStr.split(tr(":"), Qt::SkipEmptyParts);
                if (eqnDef.size() == 2)
                {
                    QString eqnName = eqnDef.at(0).simplified();
                    mmsEquations.insert(eqnName, eqnDef.at(1).simplified());
                }
            }
        }
        break;
        //----------------------------------------INCENTIVES----------
        case incentives:
        {
            if (sVarName.indexOf(tr("INC_"), Qt::CaseInsensitive) != -1)
            {
                QStringList incfields = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (!incfields.isEmpty())
                {
                    QString incName = incfields.takeFirst();
                    QStringList namePair = incName.split("_", Qt::SkipEmptyParts);
                    if (namePair.size() == 2)
                    {
                        mmslIncentives.insert(incName, incfields);
                        mIncNamePair.insert(incName, namePair);
                    }
                    else
                    {
                        std::stringstream errmsg;
                        errmsg << "Line " << numline << " contains an invalid incentives name! Make sure any incentive name "
                                      << "is comprised of two criteria names concatenated by and underscore \'_\'. Thereby, the "
                                      << "leading name represents the objective and the second name represents the criterion "
                                      << "whose per unit reduction is incentivised by the values specified in the column given "
                                      << "whose name is given after the incentive name." << std::endl;
                        sReport << QString(errmsg.str().c_str());
                    }
                }
            }
        }
        break;
        //----------------------------------------OBJECTIVES----------
        case objectives:
        {
            QString sAggrMeth;
            if (sVarName.compare(tr("AGGR_METHOD"), Qt::CaseInsensitive) == 0)
            {
                if (sValueStr.compare(tr("WSUM")) == 0)
                {
                    this->meScalMeth = NMMosra::NM_MOSO_WSUM;
                    sAggrMeth = tr("Weighted Sum");
                }
                else
                {
                    this->meScalMeth = NMMosra::NM_MOSO_INTERACTIVE;
                    sAggrMeth = tr("Interactive");
                }
                MosraLogInfo( << "Scalarisation method is '" << sAggrMeth.toStdString() << "'" << std::endl)
            }
            else if (sVarName.indexOf(tr("OBJ_"), Qt::CaseInsensitive) != -1)
            {
                 QStringList objs = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                 if (objs.size() >= 2)
                 {
                     QString obj = objs.takeAt(1);
                     this->mmslObjectives.insert(obj, objs);
                     MosraLogInfo( << "obj: " << obj.toStdString() << ": "
                             << this->mmslObjectives.find(obj).value().join(tr(" ")).toStdString() << std::endl)
                 }
            }
        }
        break;
        //------------------------------------ALGEBRAIC_LINEAR_CONS------------
        case linearcons:
        {
            if (sVarName.indexOf(tr("LIN_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList operators;
                //             1       2      3
                operators << "<=" << ">=" << "=";

                QString theop;
                QStringList linCons;
                for (int nlc=0; nlc < operators.size(); ++nlc)
                {
                    linCons = sValueStr.split(operators.at(nlc), Qt::SkipEmptyParts);
                    if (linCons.size() == 2)
                    {
                        theop = operators.at(nlc);
                        break;
                    }
                }

                if (linCons.size() < 2)
                {
                    std::stringstream errmsg;
                    errmsg << "Line " << numline << " contains a malformed algebraic linear constraint. "
                           << "Please proivde an expression of the form " << std::endl
                           << "\tLIN_CONS_< x >=< eqn name > < <= | >= | = > < eqn name | number >" << std::endl;
                    sReport << QString(errmsg.str().c_str());
                    MosraLogError(<< errmsg.str());
                }
                else
                {
                    QStringList opRhs;
                    opRhs << theop << linCons[1].simplified();
                    mmslLinearConstraints.insert(linCons[0].simplified(), opRhs);
                }
            }
        }
        break;
        //------------------------------------ALGEBRAIC_NONLINEAR_CONS------------
        case nonlinearcons:
        {
            if (sVarName.indexOf(tr("NONLIN_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList operators;
                //             1       2      3
                operators << "<=" << ">=" << "=";

                QString theop;
                QStringList nlinCons;
                for (int nlc=0; nlc < operators.size(); ++nlc)
                {
                    nlinCons = sValueStr.split(operators.at(nlc), Qt::SkipEmptyParts);
                    if (nlinCons.size() == 2)
                    {
                        theop = operators.at(nlc);
                        break;
                    }
                }

                if (nlinCons.size() < 2)
                {
                    std::stringstream errmsg;
                    errmsg << "Line " << numline << " contains a malformed non-linear constraint. "
                           << "Please proivde an expression of the form " << std::endl
                           << "\tNLIN_CONS_< x >=< eqn name > < <= | >= | = > < eqn name | number >" << std::endl;
                    sReport << QString(errmsg.str().c_str());
                    MosraLogError(<< errmsg.str());
                }
                else
                {
                    QStringList opRhs;
                    opRhs << theop << nlinCons[1].simplified();
                    mmslNonLinearConstraints.insert(nlinCons[0].simplified(), opRhs);
                }
            }
        }
        break;
        //------------------------------------LOGIC_CONS------------
        case logiccons:
        {
            if (sVarName.indexOf(tr("LOGIC_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList operators;
                //             1       2      3
                operators << "<=" << ">=" << "=";

                QString theop;
                QStringList logicCons;
                for (int logc=0; logc < operators.size(); ++logc)
                {
                    logicCons = sValueStr.split(operators.at(logc), Qt::SkipEmptyParts);
                    if (logicCons.size() == 2)
                    {
                        theop = operators.at(logc);
                        break;
                    }
                }

                if (logicCons.size() < 2)
                {
                    std::stringstream errmsg;
                    errmsg << "Line " << numline << " contains a malformed logic constraint. "
                           << "Please proivde an expression of the form " << std::endl
                           << "\tLOGIC_CONS_< x >=< eqn name > < <= | >= | = > < eqn name | number >" << std::endl;
                    sReport << QString(errmsg.str().c_str());
                    MosraLogError(<< errmsg.str());
                }
                else
                {
                    QStringList opRhs;
                    opRhs << theop << logicCons[1].simplified();
                    mmslLogicConstraints.insert(logicCons[0].simplified(), opRhs);
                }
            }
        }
        break;
        //-----------------------------------------AREAL_CONS------------
        case arealcons:
        {
            if (sVarName.indexOf(tr("AREAL_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList arCons = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (arCons.size() != 0)
                {
                    this->mmslAreaCons.insert(sVarName, arCons);
                    MosraLogDebug( << "areal cons: " << sVarName.toStdString() << ": "
                            << this->mmslAreaCons.find(sVarName).value().join(tr(" ")).toStdString() << std::endl);

                    // check, whether we've got a zoning constraint here and if so, initialise the
                    // zones area with 0
                    QString luopt = arCons.at(0);
                    if (luopt.contains(':', Qt::CaseInsensitive))
                    {
                        QStringList luoptlist = luopt.split(tr(":"), Qt::SkipEmptyParts);
                        QString zonefield = luoptlist.at(1);
                        // allow for multiple land use options being specified separated by comata
                        // !without whitespace!
                        QStringList options = luoptlist.at(0).split("+", Qt::SkipEmptyParts);

                        // check, whether we've got already a map created for this zonefield
                        QMap<QString, QMap<QString, double> >::iterator zonesIt;
                        QMap<QString, QMap<QString, int> >::iterator lengthIt;
                        zonesIt = this->mmslZoneAreas.find(zonefield);
                        lengthIt = this->mmslZoneLength.find(zonefield);
                        if (zonesIt == this->mmslZoneAreas.end())
                        {
                            QMap<QString, double> zoneArea;
                            QMap<QString, int> zoneLength;
                            foreach(const QString& opt, options)
                            {
                                zoneArea.insert(opt, 0);
                                zoneLength.insert(opt, 0);
                            }
                            this->mmslZoneAreas.insert(zonefield, zoneArea);
                            this->mmslZoneLength.insert(zonefield, zoneLength);
                        }
                        else
                        {
                            foreach(const QString& opt, options)
                            {
                                zonesIt.value().insert(opt, 0);
                                lengthIt.value().insert(opt, 0);
                            }
                            this->mmslZoneAreas.insert(zonefield, zonesIt.value());
                            this->mmslZoneLength.insert(zonefield, lengthIt.value());
                        }
                    }
                }
            }
        }
        break;
        //----------------------------------------------FEAT_CONS--------
        case featcons:
        {
            if (sVarName.indexOf(tr("FEAT_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList featCons = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (featCons.size() != 0)
                {
                    this->mmslFeatCons.insert(sVarName, featCons);
                    MosraLogDebug( << "feature cons: " << sVarName.toStdString() << ": "
                            << this->mmslFeatCons.find(sVarName).value().join(tr(" ")).toStdString() << std::endl);
                }
            }
        }
        break;
        //----------------------------------------------FEATSET_CONS--------
        case featsetcons:
        {
            if (sVarName.indexOf(tr("FEATSET_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList featsetCons = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (featsetCons.size() != 0)
                {
                    QString keyPair = featsetCons.at(0);
                    QStringList keyPairList = keyPair.split(":", Qt::SkipEmptyParts);
                    QStringList consValueSpec = featsetCons.mid(1, -1);
                    // just checking whether the length of the keypair list is alright,
                    // no point to look for valid values as they could LUMASS expressions
                    // that still need to be evaluated!
                    if (keyPairList.size() == 2)
                    {
                         this->msFeatureSetConsLabel.insert(keyPair, sVarName);
                         this->mslFeatSetCons.insert(keyPair, consValueSpec);
                         MosraLogDebug( << "featset_cons: " << sVarName.toStdString() << ": "
                                          << featsetCons.join(" ").toStdString() << std::endl);
                    }
                }
            }
        }
        break;
        //----------------------------------------------ZONE_CONST-------
        case zonecons:
        {
            if (sVarName.indexOf(tr("ZONE_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList fullZoneCons = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (fullZoneCons.size() == 3)
                {
                    QString label = sVarName;
                    QStringList zonespec = fullZoneCons.at(0).split(":", Qt::SkipEmptyParts);
                    QString zoneOp = fullZoneCons.at(1);
                    QString valueCol = fullZoneCons.at(2);

                    QStringList zonecon;
                    if (zonespec.size() != 3)
                    {
                        MosraLogError (<< "Malfomred ZONE_CONSTRAINT on line " << numline);
                    }

                    // zonecon
                    //  0: zone id column
                    //  1: resource list column | total
                    //  2: criterion label
                    //  3: operator
                    //  4: constraint value (rhs value)


                    zonecon << zonespec << zoneOp << valueCol;
                    this->mslZoneConstraints.insert(label, zonecon);

                    std::stringstream logs;
                    logs << label.toStdString() << ": ";
                    for (int r=0; r < zonecon.size(); ++r)
                    {
                        logs << zonecon.at(r).toStdString();
                        if (r < zonecon.size()-2)
                        {
                            logs << " | ";
                        }
                    }
                    MosraLogInfo(<< logs.str());

                }
                else
                {
                    MosraLogError (<< "Malfomred ZONE_CONSTRAINT on line " << numline);
                }
            }
        }
        break;
        //----------------------------------------------ATTR_CONS--------
        case cricons:
        {
            if (sVarName.indexOf(tr("CRI_CONS_"), Qt::CaseInsensitive) != -1)
            {
                MosraLogDebug(<< "\tgonna split raw list: " << sValueStr.toStdString() << std::endl);
                QStringList outerList = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (outerList.size() != 0)
                {
                    QString criLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tcriLabel is '" << criLabel.toStdString() << "'" << std::endl);
                    QString luLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tland use is '" << luLabel.toStdString() << "'" << std::endl);

                    QMap<QString, QStringList> innerMap;
                    innerMap.insert(luLabel, outerList);

                    this->mmslCriCons.insert(criLabel, innerMap);

                    MosraLogDebug( << "cri cons: " << criLabel.toStdString() << ": "
                            << luLabel.toStdString() << ": "
                            << outerList.join(tr(" ")).toStdString() << std::endl);
                }
            }
        }
        break;
        //------------------------------------------------OBJ_CONS-------
        case objcons:
        {
            if (sVarName.indexOf(tr("OBJ_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList objCons = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (objCons.size() > 0)
                {
                    QString objkey = sVarName + QString(tr("_%1")).arg(objCons.value(0));
                    this->mmslObjCons.insert(objkey, objCons);
                    MosraLogInfo( << objkey.toStdString() << ": "
                            << this->mmslObjCons.find(objkey).value().join(tr(" ")).toStdString() << std::endl);
                }
            }
        }
        break;
        //------------------------------------------------PARAMETER_SCALING-------
        case scaling:
        {
            if (sVarName.indexOf(tr("SCALE_"), Qt::CaseInsensitive) != -1)
            {
                QStringList scalePara = sValueStr.split(tr(" "), Qt::SkipEmptyParts);
                if (scalePara.size() >= 2)
                {
                    const QString paraName = scalePara.at(0);
                    bool bConv = false;
                    const double scaleFactor = scalePara.at(1).toDouble(&bConv);
                    if (!bConv)
                    {
                        MosraLogError(<< "scaling: scaling factor for '" << paraName.toStdString()
                                      << "' is invalid! Please provide a numerical scaling factor!" << std::endl);
                    }

                    //if (    paraName.compare(QStringLiteral("AreaHa"), Qt::CaseSensitive) != 0
                    //     && scalePara.size() < 3
                    //   )
                    //{
                    //    MosraLogError( << "scaling: No equation names provied!" << std::endl);
                    //}

                    QStringList affectedEqns;
                    for (int s=2; s < scalePara.size(); ++s)
                    {
                        affectedEqns << scalePara.at(s);
                    }

                    std::pair<double, QStringList> scaleInfo(scaleFactor, affectedEqns);
                    mmParameterScaling.insert(paraName, scaleInfo);

                    MosraLogInfo( << "scaling: " << paraName.toStdString()
                                  << " * " << scaleFactor << " affecting: " << affectedEqns.join(" ").toStdString()
                                  << std::endl);
                }
            }
        }
        break;
        //------------------------------------------------BATCH_SETTINGS-------
        case batch:
        {
            if (sVarName.compare("DATAPATH", Qt::CaseInsensitive) == 0)
            {
                this->msDataPath = sValueStr;
                MosraLogInfo(<< "batch data path: " << this->msDataPath.toStdString() << std::endl);
            }
            else if (sVarName.compare("PERTURB", Qt::CaseInsensitive) == 0)
            {
                this->mslPerturbItems = sValueStr.split(" ", Qt::SkipEmptyParts);
                if (mslPerturbItems.size() > 0)
                {
                    MosraLogInfo(<< "Criteria/constraints to be perturbed: ");
                    foreach(const QString& pc, this->mslPerturbItems)
                    {
                        NMDebug(<< pc.toStdString() << " ");
                    }
                    NMDebug(<< std::endl);
                }
                else
                {
                    MosraLogInfo(<< "No perturbation criteria provided!" << std::endl);
                }
            }
            else if (sVarName.compare("UNCERTAINTIES", Qt::CaseInsensitive) == 0)
            {
                QStringList lunsure = sValueStr.split(" ", Qt::SkipEmptyParts);
                bool bok;
                float val;
                foreach(const QString& vstr, lunsure)
                {
                    QStringList levels = vstr.split(",");
                    QList<float> lstUncertainties;
                    foreach(const QString& l, levels)
                    {
                        val = l.toFloat(&bok);
                        if (bok)
                        {
                            lstUncertainties.push_back(val);
                        }
                    }
                    if (lstUncertainties.size() > 0)
                    {
                        this->mlflPerturbUncertainties.push_back(lstUncertainties);
                    }
                }

                if (this->mlflPerturbUncertainties.size() > 0)
                {
                    std::stringstream logstr;
                    logstr << "Perturbation levels: ";
                    foreach(const QList<float>& lf, mlflPerturbUncertainties)
                    {
                        foreach(const float& f, lf)
                        {
                            logstr << f << " ";
                        }
                        logstr << " | ";
                    }
                    logstr << std::endl;
                    MosraLogDebug(<< logstr.str())
                }
                else
                {
                    MosraLogInfo(<< "No uncertainty levels for perturbation provided!" << std::endl);
                }
            }
            else if (sVarName.compare("REPETITIONS", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    bool bok;
                    long reps = sValueStr.toLong(&bok);
                    if (bok)
                    {
                        this->mlReps = reps;
                        MosraLogInfo(<< "Number of perturbations: " << reps << std::endl);
                    }
                }
            }
            else if (sVarName.compare("TIMEOUT", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    if (sValueStr.compare("break_at_first", Qt::CaseInsensitive) == 0)
                    {
                        this->muiTimeOut = 0;
                        this->mbBreakAtFirst = true;
                        MosraLogInfo(<< "Solver timeout: break at first feasible solution!" << std::endl);
                    }
                    else
                    {
                        bool bok;
                        unsigned int timeout = sValueStr.toUInt(&bok);
                        if (bok)
                        {
                            this->mbBreakAtFirst = false;
                            this->muiTimeOut = timeout;
                            MosraLogInfo(<< "Solver timeout: " << timeout << std::endl);
                        }
                    }
                }
            }
        }
        break;
        default:
            break;
        }
    }


    NMDebug(<< std::endl << "Report..." << std::endl << sReport.readAll().toStdString() << std::endl);

    NMDebugCtx(ctxNMMosra, << "done!");

    return 1;
}

// this function is useful when the optimisation settings
// were read from a file and hence only the layer name
// was set; the owner of the object then can read the
// layer name and make sure, that the appropriate dataset
// is set afterwards
QString NMMosra::getLayerName(void)
{
    return this->msLayerName;
}

int NMMosra::configureProblem(void)
{
    NMDebugCtx(ctxNMMosra, << "...");
    // check, whether all settings are ok
    if (!this->checkSettings())
    {
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }
    MosraLogInfo(<< "checking optimisation settings - OK")

    // calc baseline
    this->calcBaseline();
    MosraLogInfo(<< "calculdated baseline alright!");

    this->makeLp();

    if (!this->addObjFn())
    {
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }
    MosraLogInfo(<< "adding objective function - OK")

    if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
            this->mmslObjCons.size() > 0)
    {
        this->addObjCons();
        MosraLogInfo(<< "adding objective constraints - OK")
    }

    // doe we have any additional constraints?
    if (this->mmslAreaCons.size() > 0)
    {
        if (!this->addExplicitAreaCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding allocation constraints - OK")
    }

    if (this->mmslCriCons.size() > 0)
    {
        if (!this->addCriCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding performance constraints - OK")
    }

    if (this->mmslIncentives.size() > 0)
    {
        if (!this->addIncentCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding incentives 'constraints' - OK")
    }

    if (this->mslZoneConstraints.size() > 0)
    {
        if (!this->addZoneCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding zone constraints - OK")
    }


    if (this->mmslFeatCons.size() > 0)
    {
        if (!this->addFeatureCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding feature constraints - OK");
    }
    else
    {
        if (!this->addImplicitAreaCons())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding internal areal (consistency) constraints - OK")
    }

    if (this->mslFeatSetCons.size() > 0)
    {
        if (!this->addFeatureSetConsDb())
        {
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        MosraLogInfo(<< "adding feature-set constraints - OK")
    }

    if (this->mbBreakAtFirst)
    {
        this->mLp->SetBreakAtFirst(true);

        MosraLogInfo(<< "solver stops at first feasible solution!" << std::endl);
    }
    else
    {
        this->mLp->SetTimeout(this->muiTimeOut);

        MosraLogInfo(<< "solver times out after " << this->muiTimeOut
                << " seconds!" << std::endl);
    }
    this->mbCanceled = false;
    this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);
    this->mLp->SetLogFunc((void*)this, NMMosra::lpLogCallback);

    // if the user wishes, we write out the problem we've just created ...
    if (!this->mProblemFilename.isEmpty())
    {
        if (this->mProblemType == NM_MOSO_LP)
        {
            this->mLp->WriteLp(this->mProblemFilename.toStdString());
        }
        else
        {
            this->mLp->WriteMps(this->mProblemFilename.toStdString());
        }
    }

    this->mLp->SetPresolve(PRESOLVE_COLS |
                           PRESOLVE_ROWS |
                           PRESOLVE_IMPLIEDFREE |
                           PRESOLVE_REDUCEGCD |
                           PRESOLVE_MERGEROWS |
                           PRESOLVE_ROWDOMINATE |
                           PRESOLVE_COLDOMINATE |
                           PRESOLVE_KNAPSACK |
                           PRESOLVE_PROBEFIX);

    this->mLp->SetScaling(SCALE_GEOMETRIC |
                          SCALE_DYNUPDATE);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

bool NMMosra::solveProblem(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    if (mSolverType == NM_SOLVER_LPSOLVE)
    {
        this->mLp->Solve();

        if (this->calcOptPerformanceDb() == 0)
        {
            MosraLogWarn(<< "Hit trouble calculating the optimal performance!"
                         << " Please double check results and notification messages!")
        }

        this->createReport();

        MosraLogInfo(<< "Optimisation Report ... \n" << this->getReport().toStdString() << std::endl);

    }
    else if (mSolverType == NM_SOLVER_IPOPT)
    {
// this must go at one point as it should always run
#ifndef LUMASS_DEBUG
        if (!this->checkSettings())
        {
            MosraLogError(<< "Didn't pass the settings check - better go and double check the *.los file!" << std::endl);
            return false;
        }
#endif

        this->clearInternalNLDataStructures();

        if (!this->infixToPolishPrefix())
        {
            MosraLogError(<< "Optimisation aborted: ERROR while processing input equations!" << std::endl);
            return false;
        }

        if (!this->makeNL2())
        {
            MosraLogError(<< "Failed writing the *.nl file! Did you get any other error messages that could help?" << std::endl);
            return false;
        }
    }
    else if (mSolverType == NM_SOLVER_GA)
    {
        // this must go at one point as it should always run
        #ifndef LUMASS_DEBUG
                if (!this->checkSettings())
                {
                    MosraLogError(<< "Didn't pass the settings check - better go and double check the *.los file!" << std::endl);
                    return false;
                }
        #endif

        if (!this->solveOpenGA())
        {
            MosraLogError(<< "OpenGA failed!" << std::endl);
        }
    }

    return true;

    NMDebugCtx(ctxNMMosra, << "done!");
}

int NMMosra::solveOpenGA(void)
{
    int ret = 1;

    NMOpenGA gawrap;
    gawrap.mProcObj = this->mProcObj;
    GA_Type ga_obj;
    ga_obj.mProcObj = this->mProcObj;

    // ===============================================================
    //  START high jacking some intel gathered for the NL processing

    this->clearInternalNLDataStructures();

    if (!this->infixToPolishPrefix())
    {
        MosraLogError(<< "Optimisation aborted: ERROR while processing input equations!" << std::endl);
        return false;
    }


    QString b_seg, x_seg;
    long n_vars;
    if (!processVariables(b_seg, x_seg, &n_vars, &gawrap))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Just got it wrong! :-( " << std::endl);
        return 0;
    }

    // just throw this in for good measure - can't remember whether that's
    // required for any of the subsequent routines to determine which
    // columns to be fetched from the database
    if (!processLoopDimensions())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Just got it wrong! :-( " << std::endl);
        return 0;
    }

    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< "ERROR - DataSet is NULL!" << endl);
        return false;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogError(<< "ERROR: DataSet is NULL!" << endl);
        return false;
    }

    // ========================================================
    //          PREPARE LOOPING && EQUATION POPULATION
    // ========================================================
    // create dimension lookup by eqn
    // and
    // create pos lookup by parameter/variable
    // i.e. pos in getnames and getvalues respectively
    // NOTE: OPTIONS dimension is ignored as values
    // are either known already for each option and don't
    // have to be retrieved or they just reference
    // a column in the db

    // how often does a defined variable appear
    // in objectives and constraints (count)
    QMap<QString, int> procVarInObjConsMap;

    // <operand or dimension parameter name>,
    //      < index of associated column name/value in
    //        getnames/getvalues vector >
    QMap<QString, std::vector<size_t> > nameValPosMap;
    std::vector<std::string> getnames;
    std::vector<otb::AttributeTable::ColumnValue> getvalues;

    // track actual columns added to getnames / getvalues
    mmLookupColsFstParam.clear();

    // list of names of equations that need to be populated
    // and written into NL file
    QStringList eqnNames;
    eqnNames << mmslObjectives.keys();
    //eqnNames << mslProcessVars;
    bool bOK;
    double val;
    QString rhs;
    eqnNames << mmslLinearConstraints.keys();
    auto linConRhs = mmslLinearConstraints.cbegin();
    for(;  linConRhs != mmslLinearConstraints.cend(); ++linConRhs)
    {
        rhs = linConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << linConRhs.value().at(1);
        }
    }

    eqnNames << mmslNonLinearConstraints.keys();
    auto nlConRhs = mmslNonLinearConstraints.cbegin();
    for(;  nlConRhs!= mmslNonLinearConstraints.cend(); ++nlConRhs)
    {
        rhs = nlConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << nlConRhs.value().at(1);
        }
    }

    eqnNames << mmslLogicConstraints.keys();
    auto logConRhs = mmslLogicConstraints.cbegin();
    for(;  logConRhs!= mmslLogicConstraints.cend(); ++logConRhs)
    {
        rhs = logConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << logConRhs.value().at(1);
        }
    }

    // <dimension parameter name>,
    //      < set of (distinct) equation names
    //        the parameter appears in >
    //QMap<QString, QSet<QString> > mmDimEqnMap;

    // populate the above maps
    foreach(const QString& name, eqnNames)
    {
        identifyHighDimLoopEquations(name, name);

        if (!createEqnElemAdminMap(
                    mmDimEqnMap,
                    nameValPosMap,
                    procVarInObjConsMap,
                    getnames,
                    getvalues,
                    name)
           )
        {
            return false;
        }
    }

    //make sure we've got the LUCControlField included
    if (    !nameValPosMap.contains(msLUCControlField)
         && !msLUCControlField.isEmpty()
       )
    {
        std::vector<size_t> posvec = {getnames.size()};
        nameValPosMap.insert(msLUCControlField, posvec);

        getnames.push_back(msLUCControlField.toStdString());
        otb::AttributeTable::ColumnValue luccval;
        getvalues.push_back(luccval);
    }


    // make sure we've got the AreaHa field in there
    if (    !nameValPosMap.contains(msAreaField)
         && !msAreaField.isEmpty()
       )
    {
        std::vector<size_t> posvec = {getnames.size()};
        nameValPosMap.insert(msAreaField, posvec);

        getnames.push_back(msAreaField.toStdString());
        otb::AttributeTable::ColumnValue areaval;
        getvalues.push_back(areaval);
    }



    std::vector<std::string> minFields =
        {"dai_min_fmusw", "snb_min_fmusw", "hap_min_fmusw",
         "hav_min_fmusw", "hbl_min_fmusw", "hch_min_fmusw",
         "hkgr_min_fmusw", "hkgd_min_fmusw", "hgp_min_fmusw",
         "hgs_min_fmusw", "von_min_fmusw", "vpt_min_fmusw",
         "amz_min_fmusw", "ape_min_fmusw", "awt_min_fmusw",
         "fst_min_fmusw", "flg_min_fmusw", "nat_min_fmusw"};

    for (int f=0; f < minFields.size(); ++f)
    {
        QString fname = minFields[f].c_str();
        if (!nameValPosMap.contains(fname))
        {
            std::vector<size_t> posvec = {getnames.size()};
            nameValPosMap.insert(fname, posvec);

            getnames.push_back(fname.toStdString());
            otb::AttributeTable::ColumnValue propval;
            getvalues.push_back(propval);
        }
    }

    // make sure we've got the spatial domain total areas
    // hard-wired at the moment (for testing)
    // -> i.e. we're using real column names here as these
    //    attributes are not used in the EQUATIONS
    //    section at the moment

    std::vector<std::string> zoneHaList = {"FMU_SW_ha"};
    for (auto zn : zoneHaList)
    {
        QString znq = zn.c_str();
        if (!nameValPosMap.contains(znq))
        {
            std::vector<size_t> posvec = {getnames.size()};
            nameValPosMap.insert(znq, posvec);

            getnames.push_back(znq.toStdString());
            otb::AttributeTable::ColumnValue propval;
            getvalues.push_back(propval);
        }
    }



    //gawrap.mDimensionAttrNames = {"FMU_SW_ha"};

    std::map<size_t, double> emptyMap;
    gawrap.mDimensionArea.insert(
           std::pair<std::string, std::map<size_t, double> >("FMU_SW", emptyMap));

    for (auto dimArea : gawrap.mDimensionArea)
    {
        QString zone_name = dimArea.first.c_str();
        if (!nameValPosMap.contains(zone_name))
        {
            std::vector<size_t> posvec = {getnames.size()};
            nameValPosMap.insert(zone_name, posvec);

            getnames.push_back(zone_name.toStdString());
            otb::AttributeTable::ColumnValue propval;
            getvalues.push_back(propval);
        }
    }

    // ------------------------------------------------------
    //          BL PERFORMANCE DATA

    // below we're just summarising over zones of a given dimension;
    // in the general case, there might more than just one zone

    // using real column names here as this aspect is not coded
    // as equation in the LOS file
    // also using coding for a SINGLE DIMENSION here, i.e FMU_SW

    // TODO: Develop representation in LOS FILE
    gawrap.mBLSumDimensionNames = {"N_BL", "P_BL", "SW_BL", "GW_BL"};
    std::stringstream blquery;

    blquery << "select \"FMU_SW\", ";
    std::vector<otb::AttributeTable::TableColumnType> bltypes;
    otb::AttributeTable::TableColumnType zoneIdType = otb::AttributeTable::ATTYPE_INT;
    bltypes.push_back(zoneIdType);

    for (int bd=0; bd < gawrap.mBLSumDimensionNames.size(); ++bd)
    {
        const std::string bdName = gawrap.mBLSumDimensionNames[bd];
        const int blcidx = sqltab->ColumnExists(bdName);
        if (blcidx >=0)
        {
            otb::AttributeTable::TableColumnType blctype = sqltab->GetColumnType(blcidx);
            bltypes.push_back(blctype);
        }
        else
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Couldn't find column '" << bdName << "' in database! "
                          << "SQLite's last message: " << sqltab->getLastLogMsg() << std::endl
                          );
            return 0;
        }

        blquery << "sum(\"" << bdName << "\") as \"" << bdName << "\"";
        if (bd < gawrap.mBLSumDimensionNames.size()-1)
        {
            blquery << ", ";
        }
    }
    blquery << " from \"" << sqltab->GetTableName() << "\""
            << " where \"" << this->msOptFeatures.toStdString() << "\" == 1 "
            << " group by \"FMU_SW\""
               ;

    const std::string blQuery = blquery.str();
    MosraLogDebug(<< "GA - BL Dim performance TableDataFetch query:\n" << blQuery << std::endl);

    std::vector<std::vector<otb::AttributeTable::ColumnValue> > blDimData;
    if (!sqltab->TableDataFetch(blDimData, bltypes, blquery.str()))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << sqltab->getLastLogMsg() << std::endl);
        return 0;
    }

    // adding zone-attribute-map for FMU_SW
    std::map<size_t, std::vector<double> > fmuswZoneMap;
    gawrap.mDimZoneTotalAttr.insert(
           std::pair<std::string, std::map<size_t, std::vector<double> > >("FMU_SW", fmuswZoneMap));

    for (int dr=0; dr < blDimData.size(); ++dr)
    {
        std::vector<double> zoneAttrVals;
        const size_t zoneVal = gawrap.getIntColValue(blDimData[dr][0]);

        // start with dc=1 since we've got FMU_SW in the result records at pos 0
        for (int dc=1; dc <= gawrap.mBLSumDimensionNames.size(); ++dc)
        {
            zoneAttrVals.push_back(gawrap.getDblColValue(blDimData[dr][dc]));
        }
        gawrap.mDimZoneTotalAttr["FMU_SW"].insert(std::pair<size_t, std::vector<double> >(zoneVal, zoneAttrVals));
    }
    blDimData.clear();

    //  DONE WITH high jacking some intel gathered for the NL processing
    // ===============================================================

    // ===============================================================
    //          FETCH DATA FROM THE DATABASE

    // -----------------------------------------------------
    //          EVAL_SOLUTION DATA

    // construct table fetch query and get col types
    std::stringstream tfqss;
    tfqss << "select ";

    std::vector<otb::AttributeTable::TableColumnType> gettypes;
    for(int s=0; s < getnames.size(); ++s)
    {
        const std::string cname = getnames[s];
        const int colidx = sqltab->ColumnExists(cname);
        if (colidx >= 0)
        {
            otb::AttributeTable::TableColumnType ctype = sqltab->GetColumnType(colidx);
            gettypes.push_back(ctype);
        }
        else
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Couldn't find column '" << cname << "' in database! "
                          << "SQLite's last message: " << sqltab->getLastLogMsg() << std::endl
                          );
            return 0;
        }

        tfqss << cname;

        if (s < getnames.size()-1)
        {
            tfqss << ", ";
        }
    }

    tfqss << " from \"" << sqltab->GetTableName() << "\""
          << " where \"" << this->msOptFeatures.toStdString() << "\" == 1;";

    const std::string theQuery = tfqss.str();
    MosraLogDebug(<< "GA - TableDataFetch query:\n" << theQuery << std::endl);

    //std::vector<std::vector<otb::AttributeTable::ColumnValue> > tabData;
    if (!sqltab->TableDataFetch(gawrap.mData, gettypes, tfqss.str()))
    {
        //sqltab->EndTransaction();
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << sqltab->getLastLogMsg() << std::endl);
        return 0;
    }

    // =====================================================================
    //          INIT SOME DATA STRUCTURES IN NMOpenGA

    if (gawrap.mData.size() == 0)
    {
        return 0;
    };

    // copy over the varadmin structures
    gawrap.mVarAdminMap  = mmVarAdminMap;
    gawrap.mMinAreaFields = minFields;

    // copy name val pos map
    auto nvpit = nameValPosMap.cbegin();
    for (; nvpit != nameValPosMap.cend(); ++nvpit)
    {
        gawrap.nameValPosMap.insert(std::pair<std::string, std::vector<size_t> >(nvpit.key().toStdString(), nvpit.value()));
    }

    // -----------------------------------------------------------
    // init mLucCtrl, the land-use control data structure
    // calc unique dimension totals

    // LAND USE CONTROL
    // ... add a row vector for each land-use option
    // ... the rows are going to be added below
    for (int opt=0; opt < mslOptions.size(); ++opt)
    {
        std::vector<unsigned char> luvec;
        gawrap.mLucCtrl.push_back(luvec);
    }


    const unsigned long lucCtrIdx = nameValPosMap[this->msLUCControlField][0];
    const unsigned long numLu     = this->mslOptions.size();


    const size_t fmuswId = nameValPosMap["FMU_SW"][0];
    const size_t fmuswAreaId = nameValPosMap["FMU_SW_ha"][0];

    for (int row=0; row < gawrap.mData.size(); ++ row)
    {
        QString ctrlstr = gawrap.mData[row][lucCtrIdx].tval;
        QStringList lulist = ctrlstr.split(" ", Qt::SkipEmptyParts);

        for (int col=0; col < numLu; ++col)
        {
            // if a land-use option is mentioned in the LUCControl field,
            // it means it is NOT ALLOWED to allocated to that SDU!!
            if (lulist.contains(mslOptions.at(col)))
            {
                gawrap.mLucCtrl[col].push_back(static_cast<unsigned char>(0));
            }
            else
            {
                gawrap.mLucCtrl[col].push_back(static_cast<unsigned char>(1));
            }
        }

        // add fmu_sw total area information

        const size_t dimValue = gawrap.getIntColValue(gawrap.mData[row][fmuswId]);
        const double dimArea = gawrap.getDblColValue(gawrap.mData[row][fmuswAreaId]);

        if (gawrap.mDimensionArea["FMU_SW"].find(dimValue) == gawrap.mDimensionArea["FMU_SW"].end())
        {
            gawrap.mDimensionArea["FMU_SW"].insert(std::pair<size_t, double>(dimValue, dimArea));
        }
    }


    // =====================================================================
    //          RUN THE GA OPTIMISATION

    // log file
    QString ga_logfilename = QString("%1/%2_%3.log")
            .arg(this->msDataPath)
            .arg(this->mScenarioName)
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate));

    std::string galogfn = ga_logfilename.toStdString();
    gawrap.setOutputFileName(galogfn);
    gawrap.setConstraintTolerance(mGAConstraintTolerance);

    EA::Chronometer timer;
    timer.tic();

    using std::bind;
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;


    ga_obj.problem_mode=EA::GA_MODE::SOGA;
    ga_obj.multi_threading=mbGAMultiThreading;
    ga_obj.idle_delay_us=10; // switch between threads quickly
    ga_obj.dynamic_threading=true;
    ga_obj.verbose=false;
    ga_obj.population=mGAPopulation;//50;
    ga_obj.generation_max=mGAMaxGeneration;//100;
    ga_obj.calculate_SO_total_fitness= bind(&NMOpenGA::calculate_SO_total_fitness, &gawrap, _1);
    ga_obj.init_genes= bind(&NMOpenGA::init_genes, &gawrap, _1, _2);
    ga_obj.eval_solution= bind(&NMOpenGA::eval_solution, &gawrap, _1, _2);
    ga_obj.mutate= bind(&NMOpenGA::mutate, &gawrap, _1, _2, _3);
    ga_obj.crossover= bind(&NMOpenGA::crossover, &gawrap, _1, _2, _3);

    ga_obj.SO_report_generation=bind(&NMOpenGA::SO_report_generation, &gawrap, _1, _2, _3);
    ga_obj.crossover_fraction=0.7;
    ga_obj.mutation_rate=0.2;
    ga_obj.best_stall_max=10;
    ga_obj.elite_count=10;
    ga_obj.solve();

    cout<<"The problem was optimized in "<<timer.toc()<<" seconds."<<endl;

    std::vector<std::string> optionsvec;
    for (int b=0; b < this->mslOptions.size(); ++b)
    {
        optionsvec.push_back(mslOptions.at(b).toStdString());
    }

    if (!gawrap.mapGASolutions(ga_obj,
                               sqltab,
                               optionsvec,
                               this->msOptFeatures.toStdString(),
                               this->mLogger,
                               this->mmslParameters))
    {
        // somethin' happend
        // oh oh
    }

    return ret;
}

int NMMosra::solveLp(void)
{
    int ret = 1;

    if (this->msSolver.compare("lp_solve", Qt::CaseInsensitive) == 0)
    {
        if (!this->configureProblem())
        {
            return 0;
        }

        this->solveProblem();
    }
    else
    {
        MosraLogError(<< "Please configure: 'SOLVER=lp_solve' to run this command!");
        ret = 0;
    }

    return ret;
}

void NMMosra::createReport(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // let's put out some info about the solution, if any
    QString streamstring;
    QTextStream sRes(&streamstring);
    sRes.setRealNumberNotation(QTextStream::SmartNotation);
    sRes.setRealNumberPrecision(15);

    sRes <<  Qt::endl <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes << "\t" << mScenarioName << " -- " << QDateTime::currentDateTime().toString(Qt::ISODate) <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes <<  Qt::endl <<  Qt::endl;
    sRes << tr("\tProblem setting details") <<  Qt::endl;
    sRes << tr("\t-----------------------") <<  Qt::endl <<  Qt::endl;
    sRes << this->msSettingsReport <<  Qt::endl;

    sRes << tr("\n\n\tResults from lp_solve 5.5\n"
               "\t-------------------------\n\n"
               "Please read the lp_solve documentation for\n"
               "further information about optimisation results!\n\n");

    //get return value from the solve function
    int ret = this->mLp->GetLastReturnFromSolve();

    sRes << tr("Solver returned: ");
    switch (ret)
    {
    case -2: sRes << tr("NOMEMORY\n"); break;
    case 0: sRes << tr("OPTIMAL\n"); break;
    case 1: sRes << tr("SUBOPTIMAL\n"); break;
    case 2: sRes << tr("INFEASIBLE\n"); break;
    case 3: sRes << tr("UNBOUNDED\n"); break;
    case 4: sRes << tr("DEGENERATE\n"); break;
    case 5: sRes << tr("NUMFAILURE\n"); break;
    case 6: sRes << tr("USERABORT\n"); break;
    case 7: sRes << tr("TIMEOUT\n"); break;
    case 10: sRes << tr("PROCFAIL\n"); break;
    case 11: sRes << tr("PROCBREAK\n"); break;
    case 12: sRes << tr("FEASFOUND\n"); break;
    case 13: sRes << tr("NOFEASFOUND\n"); break;
    }

    if (ret != 0 && ret != 1 && ret != 12)
    {
        sRes << "There was neither a sub-optimal\n"
                "nor an optimal solution found by\n"
                "lp_solve 5.5!\n"
                "RETURN CODE = " << ret << " - see lp_solve doc!\n" <<  Qt::endl;
    }

    //log the number of solutions
    sRes << "Number of solutions: " << this->mLp->GetSolutionCount() <<  Qt::endl <<  Qt::endl;

    //log the objective function
    sRes << "Objective function result = " << this->mLp->GetObjective() <<  Qt::endl <<  Qt::endl;

    //get the values of the constraints
    int iNumCons = this->mLp->GetNRows();
    double *pdCons;
    this->mLp->GetPtrConstraints(&pdCons);

    //log the objective constraints
    if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
            this->mmslObjCons.size() > 0)
    {
        sRes << tr("OBJECTIVE CONSTRAINTS\n");
        QMap<QString, QStringList>::const_iterator oit = this->mmslObjCons.constBegin();
        for (; oit != this->mmslObjCons.constEnd(); oit++)
        {
            int index = this->mLp->GetNameIndex(oit.key().toStdString(), true);
            if (index >= 0)
                sRes << oit.key() << " = " << pdCons[index-1] << " ( "
                     << oit.value().at(1) << " " << oit.value().at(2) << " )" <<  Qt::endl;
        }
        sRes <<  Qt::endl;
    }

    //log the explicit areal constraints
    if (this->mmslAreaCons.size() > 0)
    {
        sRes << tr("USER DEFINED AREAL CONSTRAINTS\n");
        QMap<QString, QStringList>::const_iterator ait =
                this->mmslAreaCons.constBegin();
        int totalcount = 0;
        for (int r=0; ait != this->mmslAreaCons.constEnd(); ++ait, ++r)
        {
            QString sACL;
            QStringList zonespec;
            int index;
            double dval;
            bool ok;
            int nit = 1;
            QStringList options;

            if (ait.value().at(0).contains(":", Qt::CaseInsensitive))
            {
                zonespec = ait.value().at(0).split(tr(":"), Qt::SkipEmptyParts);
                options = zonespec.at(0).split(tr("+"), Qt::SkipEmptyParts);
                //nit = 2;
            }
            else
            {
                options << ait.value().at(0).split(tr("+"), Qt::SkipEmptyParts);
            }

            for (int q=0; q < nit; ++q)
            {
                sACL = this->mmslAreaConsLabel.at(totalcount);
                index = this->mLp->GetNameIndex(sACL.toStdString() , true);
                if (index >= 0)
                {
                    double maxval = 0;
                    foreach(const QString& opt, options)
                    {
                        QStringList ozpec;
                        if (zonespec.size() > 1)
                        {
                            ozpec << opt << zonespec.at(1);
                        }
                        dval = ait.value().at(2).toDouble(&ok);
                        dval = this->convertAreaUnits(dval, ait.value().at(3), ozpec);
                        maxval = dval > maxval ? dval : maxval;
                    }
                    if (q == 1)
                        maxval = 0;

                    sRes << sACL << " = " << pdCons[index-1] << " ( "
                         << ait.value().at(1) << " " << maxval << " )" <<  Qt::endl;
                }
                ++totalcount;
            }
        }
        sRes <<  Qt::endl;
    }

    //log the attributive constraints
    if (this->mmslCriCons.size() > 0)
    {
        sRes << tr("CRITERIA CONSTRAINTS\n");

        QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit =
                this->mmslCriCons.constBegin();
        QMap<QString, QStringList>::const_iterator criit;
        for (; crilabit != this->mmslCriCons.constEnd(); ++crilabit)
        {
            for (criit = crilabit.value().constBegin(); criit != crilabit.value().constEnd(); ++criit)
            {
                QString compOp = criit.value().at(criit.value().size()-2);
                QString compTypeLabel;
                if (compOp.compare(tr("<=")) == 0)
                    compTypeLabel = "upper";
                else if (compOp.compare(tr(">=")) == 0)
                    compTypeLabel = "lower";
                else if (compOp.compare(tr("=")) == 0)
                    compTypeLabel = "equals";

                QString sCL = crilabit.key() + QString(tr("_%1_%2")).arg(criit.key()).arg(compTypeLabel);
                int index = this->mLp->GetNameIndex(sCL.toStdString().c_str(), true);

                if (index >= 0)
                {
                    sRes << sCL << " " << compOp << " " << pdCons[index -1] <<  Qt::endl;
                }
                else
                {
                    QString tmplab = crilabit.key() + QString(tr("_%1")).arg(criit.key());
                    sRes << "error reading cirterion constraint for '"
                         << tmplab << "'!" <<  Qt::endl;
                }
            }
        }
        sRes <<  Qt::endl;
    }

    // DEBUG - we just dump all constraints values here
    //    MosraLogDebug( <<  Qt::endl << "just dumping all constraint values .... " <<  Qt::endl);
    //	int nrows = this->mLp->GetNRows();
    //	for (int q=1; q < nrows; ++q)
    //	{
    //		QString name(this->mLp->GetRowName(q).c_str());
    //        if (name.contains("Feature") || name.contains("BaselineCons"))
    //			continue;
    //		int op = this->mLp->GetConstraintType(q-1);
    //		double cv = pdCons[q-1];
    //		char fv[256];
    //		::sprintf(fv, "%g", cv);
    //        MosraLogDebug(<< name.toStdString() << " " << op << " " << fv <<  Qt::endl);
    //	}
    //NMDebug(<<  Qt::endl);


    sRes <<  Qt::endl;

    QString platz = tr("\t\t");
    sRes << "SENSITIVITY - CONSTRAINTS\n\n";
    sRes << "ROW" << platz << tr("\t") << "DUAL VALUE" << platz << "DUAL FROM" << platz << "DUAL TILL\n";
    //get some info about sensitivity
    double *pDuals, *pDualsFrom, *pDualsTill;
    if (this->mLp->GetPtrSensitivityRHS(&pDuals, &pDualsFrom, &pDualsTill))
    {
        // avoid dumping feature and baseline constraints here, so we're looking for the
        // first occurence (row number) of either of them
        //const int nRowsFeat = this->mLp->GetNameIndex("Feature_0a", true);//this->mLp->GetNRows();
        //const int nRowsBase = this->mLp->GetNameIndex("Baseline_0_1", true);
        const int nRows = this->mLp->GetNRows(); //std::max(std::min(nRowsFeat, nRowsBase), -1);

        for (int r=1; r < nRows; r++)
        {
            const QString consname = this->mLp->GetRowName(r).c_str();
            if (    !consname.startsWith("Feature")
                    && !consname.startsWith("Baseline")
                    && !consname.startsWith("Reduc")
                    )
            {
                sRes << consname << platz << tr("\t") <<
                        pDuals[r] << platz << pDualsFrom[r] << platz << pDualsTill[r] <<  Qt::endl;
            }
        }
    }


    this->msReport.clear();
    this->msReport = sRes.readAll();

    NMDebugCtx(ctxNMMosra, << "done!");

}

void
NMMosra::writeSTECReport(QString fileName, std::map<int, std::map<long long, double> >& valstore)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // let's put out some info about the solution, if any
    QString streamstring;
    QTextStream sRes(&streamstring);
    sRes.setRealNumberNotation(QTextStream::SmartNotation);
    sRes.setRealNumberPrecision(15);

    sRes <<  Qt::endl <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes << "\t" << mScenarioName << " -- " << QDateTime::currentDateTime().toString(Qt::ISODate) <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes <<  Qt::endl <<  Qt::endl;
    //sRes << tr("\tProblem setting details") <<  Qt::endl;
    //sRes << tr("\t-----------------------") <<  Qt::endl <<  Qt::endl;
    //sRes << this->msSettingsReport <<  Qt::endl;

    sRes << tr("\n\n\tResults from lp_solve 5.5\n"
               "\t-------------------------\n\n"
               "Please read the lp_solve documentation for\n"
               "further information about optimisation results!\n\n");

    //get return value from the solve function
    int ret = this->mLp->GetLastReturnFromSolve();

    sRes << tr("Solver returned: ");
    switch (ret)
    {
    case -2: sRes << tr("NOMEMORY\n"); break;
    case 0: sRes << tr("OPTIMAL\n"); break;
    case 1: sRes << tr("SUBOPTIMAL\n"); break;
    case 2: sRes << tr("INFEASIBLE\n"); break;
    case 3: sRes << tr("UNBOUNDED\n"); break;
    case 4: sRes << tr("DEGENERATE\n"); break;
    case 5: sRes << tr("NUMFAILURE\n"); break;
    case 6: sRes << tr("USERABORT\n"); break;
    case 7: sRes << tr("TIMEOUT\n"); break;
    case 10: sRes << tr("PROCFAIL\n"); break;
    case 11: sRes << tr("PROCBREAK\n"); break;
    case 12: sRes << tr("FEASFOUND\n"); break;
    case 13: sRes << tr("NOFEASFOUND\n"); break;
    }

    if (ret != 0 && ret != 1 && ret != 12)
    {
        sRes << "There was neither a sub-optimal\n"
                "nor an optimal solution found by\n"
                "lp_solve 5.5!\n"
                "RETURN CODE = " << ret << " - see lp_solve doc!\n" << Qt::endl;
    }

    //log the number of solutions
    sRes << "Number of solutions: " << this->mLp->GetSolutionCount() << Qt::endl << Qt::endl;

    //log the objective function
    sRes << "Objective function result = " << this->mLp->GetObjective() << Qt::endl << Qt::endl;

    //get the values of the decision variables
    sRes << "Decision variables ... " << Qt::endl << Qt::endl;

    // **************** VARIABLES **************************
    double* pdVars;
    this->mLp->GetPtrVariables(&pdVars);
    int ncols = this->mLp->GetNColumns();
    int nrows = this->mLp->GetNRows();


    // ---------------- COH & TAN_PHI -----------------------
    int coidx = this->mLp->GetNameIndex("coh", false);
    int tanphiidx = this->mLp->GetNameIndex("tan_phi", false);
    const double coh      = pdVars[coidx];
    const double tanphi   = pdVars[tanphiidx];

    //sRes << "SedNetNZ's total landslide load: " << sn_totls << Qt::endl;
    sRes << "cohesion = " << coh << Qt::endl;
    sRes << "tan(phi) = " << tanphi << Qt::endl << Qt::endl;


    // ---------------- FAIL SWITCHES -----------------------
    long long lNumCells = this->mDataSet->getNumRecs();
    std::stringstream colname;
    int yearCount = 1;

    sRes << "fail switches: ..." << Qt::endl;
    for (int c=1; c <= lNumCells; ++c)
    {
        colname << "b_" << valstore[1][c] << "_" << yearCount;
        const int nidx = this->mLp->GetNameIndex(colname.str(), false);
        sRes << colname.str().c_str() << " = " << pdVars[nidx] << Qt::endl;
        colname.str("");
        if (yearCount == this->mlNumArealDVar)
        {
            ++yearCount;
        }
    }
    sRes << Qt::endl << Qt::endl;


    // **************** CONSTRAINTS **************************
    sRes << "constraints: ..." << Qt::endl;
    double* pdConstrs;
    this->mLp->GetPtrConstraints(&pdConstrs);

    // ---------------- OBJECTIVE CONSTRAINT -----------------------
    int objcidx = this->mLp->GetNameIndex("Obj cons #1 sn_ls >= +sum", true);
    double objcval = pdConstrs[objcidx];
    sRes << "Obj cons #1 sn_ls >= +sum:  = " << objcval << Qt::endl;

    int cohupidx = this->mLp->GetNameIndex("cohesion_upper", true);
    double cohupval = pdConstrs[cohupidx];
    sRes << "cohesion_upper" << cohupval << Qt::endl;

    int tanloidx = this->mLp->GetNameIndex("tan(phi)_lower", true);
    double tanloval = pdConstrs[tanloidx];
    sRes << "tan(phi)_lower" << tanloval << Qt::endl;

    int tanupidx = this->mLp->GetNameIndex("tan(phi)_upper", true);
    double tanupval = pdConstrs[tanupidx];
    sRes << "tan(phi)_upper" << tanupval << Qt::endl;


    // ---------------- FAIL SWITCH CONSTRAINTS -----------------------
    std::stringstream consname;
    yearCount = 1;

    for (int r=1; r <= lNumCells; ++r)
    {
        consname << "b_" << valstore[1][r] << "_" << yearCount << "_cons";
        const int ncidx = this->mLp->GetNameIndex(consname.str(), true);
        sRes << consname.str().c_str() << " = " << pdConstrs[ncidx] << Qt::endl;
        consname.str("");
        if (yearCount == this->mlNumArealDVar)
        {
            ++yearCount;
        }
    }
    sRes << Qt::endl << Qt::endl;

    this->msReport.clear();
    this->msReport = sRes.readAll();

    this->writeReport(fileName);


    NMDebugCtx(ctxNMMosra, << "done!");

}

void
NMMosra::writeSTECReport2(QString fileName, std::map<int, std::map<long long, double> >& valstore)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // let's put out some info about the solution, if any
    QString streamstring;
    QTextStream sRes(&streamstring);
    sRes.setRealNumberNotation(QTextStream::SmartNotation);
    sRes.setRealNumberPrecision(15);

    sRes <<  Qt::endl <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes << "\t" << mScenarioName << " -- " << QDateTime::currentDateTime().toString(Qt::ISODate) <<  Qt::endl;
    sRes << "====================================================" <<  Qt::endl;
    sRes <<  Qt::endl <<  Qt::endl;
    //sRes << tr("\tProblem setting details") <<  Qt::endl;
    //sRes << tr("\t-----------------------") <<  Qt::endl <<  Qt::endl;
    //sRes << this->msSettingsReport <<  Qt::endl;

    sRes << tr("\n\n\tResults from lp_solve 5.5\n"
               "\t-------------------------\n\n"
               "Please read the lp_solve documentation for\n"
               "further information about optimisation results!\n\n");

    //get return value from the solve function
    int ret = this->mLp->GetLastReturnFromSolve();

    sRes << tr("Solver returned: ");
    switch (ret)
    {
    case -2: sRes << tr("NOMEMORY\n"); break;
    case 0: sRes << tr("OPTIMAL\n"); break;
    case 1: sRes << tr("SUBOPTIMAL\n"); break;
    case 2: sRes << tr("INFEASIBLE\n"); break;
    case 3: sRes << tr("UNBOUNDED\n"); break;
    case 4: sRes << tr("DEGENERATE\n"); break;
    case 5: sRes << tr("NUMFAILURE\n"); break;
    case 6: sRes << tr("USERABORT\n"); break;
    case 7: sRes << tr("TIMEOUT\n"); break;
    case 10: sRes << tr("PROCFAIL\n"); break;
    case 11: sRes << tr("PROCBREAK\n"); break;
    case 12: sRes << tr("FEASFOUND\n"); break;
    case 13: sRes << tr("NOFEASFOUND\n"); break;
    }

    if (ret != 0 && ret != 1 && ret != 12)
    {
        sRes << "There was neither a sub-optimal\n"
                "nor an optimal solution found by\n"
                "lp_solve 5.5!\n"
                "RETURN CODE = " << ret << " - see lp_solve doc!\n" <<  Qt::endl;
    }

    //log the number of solutions
    sRes << "Number of solutions: " << this->mLp->GetSolutionCount() <<  Qt::endl <<  Qt::endl;

    //log the objective function
    sRes << "Objective function result = " << this->mLp->GetObjective() <<  Qt::endl <<  Qt::endl;

    //get the values of the decision variables
    sRes << "Decision variables ... " <<  Qt::endl <<  Qt::endl;

    // **************** VARIABLES **************************
    double* pdVars;
    this->mLp->GetPtrVariables(&pdVars);
    int ncols = this->mLp->GetNColumns();
    int nrows = this->mLp->GetNRows();


    // ---------------- COH & TAN_PHI -----------------------
    std::stringstream dbg;
    std::string dbstr;
    std::stringstream coh_name, tan_name;
    const int maxColIdx = 1 * this->mlNumArealDVar;
    for (int ct=1; ct <= maxColIdx; ++ct)
    {
        //coh_name << "coh_" << valstore[1][ct];
        //tan_name << "tan_" << valstore[1][ct];
        //
        //const int coidx = this->mLp->GetNameIndex(coh_name.str(), false);
        //const int tanphiidx = this->mLp->GetNameIndex(tan_name.str(), false);
        //
        //const double cohVal = pdVars[coidx];
        //const double tanVal = pdVars[tanphiidx];
        //
        //sRes << coh_name.str().c_str() << " = " << cohVal <<  Qt::endl;
        //sRes << tan_name.str().c_str() << " = " << tanVal <<  Qt::endl;
        //
        //coh_name.str("");
        //tan_name.str("");

        const std::string colname = this->mLp->GetColName(ct);
        sRes << colname.c_str() << " = " << pdVars[ct-1] <<  Qt::endl;
    }
    sRes <<  Qt::endl;

    // ---------------- FAIL SWITCHES -----------------------
    long long lNumCells = this->mDataSet->getNumRecs();
    std::stringstream fail_name;
    int yearCount = 1;

    const int startCol = maxColIdx + 1;


    sRes << "fail switches: ..." <<  Qt::endl;
    for (int c=startCol; c <= lNumCells; ++c)
    {
        //fail_name << "b_" << valstore[1][c] << "_" << yearCount;
        //const int nidx = this->mLp->GetNameIndex(fail_name.str(), false);
        //sRes << fail_name.str().c_str() << " = " << pdVars[nidx] <<  Qt::endl;
        //fail_name.str("");
        //if (yearCount == this->mlNumArealDVar)
        //{
        //    ++yearCount;
        //}

        const std::string switchname = this->mLp->GetColName(c);
        sRes << switchname.c_str() << " = " << pdVars[c-1] <<  Qt::endl;
    }
    sRes <<  Qt::endl <<  Qt::endl;


    // **************** CONSTRAINTS **************************
    sRes << "constraints: ..." <<  Qt::endl;
    double* pdConstrs;
    this->mLp->GetPtrConstraints(&pdConstrs);

    // ---------------- OBJECTIVE CONSTRAINT -----------------------
    int objcidx = this->mLp->GetNameIndex("Obj cons #1 sn_ls >= +sum", true);
    const double objcval = pdConstrs[objcidx];
    sRes << "Obj cons #1 sn_ls >= +sum:  = " << objcval <<  Qt::endl;

    // ---------------- COH & TAN_PHI CONSTRAINTS -------------------
    std::stringstream coh_upper, tan_lower, tan_upper;
    for (int a=1; a < this->mlNumArealDVar; ++a)
    {
        const int svid = valstore[1][a];

        coh_upper << "coh_" << svid << "_upper_cons";
        tan_lower << "tan_" << svid << "_lower_cons";
        tan_upper << "tan_" << svid << "_upper_cons";

        //const int cohupidx = this->mLp->GetNameIndex(coh_upper.str(), true);
        //const double cohupval = pdConstrs[cohupidx];
        //sRes << coh_upper.str().c_str() << " = " << cohupval <<  Qt::endl;

        const int tanloidx = this->mLp->GetNameIndex(tan_lower.str(), true);
        const double tanloval = pdConstrs[tanloidx];
        sRes << tan_lower.str().c_str() << " = " << tanloval <<  Qt::endl;

        const int tanupidx = this->mLp->GetNameIndex(tan_upper.str(), true);
        const double tanupval = pdConstrs[tanupidx];
        sRes << tan_upper.str().c_str() << " = " << tanupval <<  Qt::endl;

        coh_upper.str("");
        tan_lower.str("");
        tan_upper.str("");
    }
    sRes <<  Qt::endl;

    // ---------------- FAIL SWITCH CONSTRAINTS -----------------------
    fail_name.str("");
    yearCount = 1;

    for (int r=1; r <= lNumCells; ++r)
    {
        fail_name << "b_" << valstore[1][r] << "_" << yearCount << "_cons";
        const int ncidx = this->mLp->GetNameIndex(fail_name.str(), true);
        sRes << fail_name.str().c_str() << " = " << pdConstrs[ncidx] <<  Qt::endl;
        fail_name.str("");
        if (yearCount == this->mlNumArealDVar)
        {

            ++yearCount;
        }
    }
    sRes <<  Qt::endl <<  Qt::endl;

    this->msReport.clear();
    this->msReport = sRes.readAll();

    this->writeReport(fileName);


    NMDebugCtx(ctxNMMosra, << "done!");

}


void
NMMosra::writeBaselineReductions(QString filename)
{
    // DEBUG if we've got incentives, let's get 'em and see 'em!
    const int lNumRecs = this->mDataSet->getNumRecs();
    const int iNumIncs = this->mmslIncentives.size();
    const bool optFeat = this->mDataSet->hasColumn(this->msOptFeatures);
    if (iNumIncs > 0)
    {
        QFile incFile(filename);
        if (incFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream incstr(&incFile);

            incstr <<  Qt::endl <<  Qt::endl;
            incstr << "Let's get a feeling for those reduction variables (r_i_r_q) ...." <<  Qt::endl;
            incstr << "     with: i: feature index, r: land use index, q: incentive index" <<  Qt::endl <<  Qt::endl;

            double* pdVars = nullptr;
            this->mLp->GetPtrVariables(&pdVars);
            if (pdVars == nullptr)
            {
                NMLogError(<< "NMMosra::writeBaselineReductions() - failed to fetch the decision variables!")
                return;
            }

            for (int rec=0; rec < lNumRecs; ++rec)
            {
                if (optFeat && this->mDataSet->getIntValue(this->msOptFeatures, rec) == 0)
                {
                    continue;
                }

                for (int inc=0; inc < iNumIncs; ++inc)
                {
                    // print parcel reduction - in case of land-use retirement
                    const QString predName = QString("R_%1_0_%2").arg(rec).arg(inc+1);
                    const int predIdx = this->mLp->GetNameIndex(predName.toStdString(), false)-1;
                    if (predIdx >= 0 && predIdx < this->mLp->GetNColumns())
                    {
                        incstr << predName << "=" << pdVars[predIdx] << " ";
                    }

                    // print reduction associated with individual land-uses
                    for (int lu=0; lu < miNumOptions; ++lu)
                    {
                        const QString incName = QString("R_%1_%2_%3").arg(rec).arg(lu+1).arg(inc+1);
                        const int ivalIdx = this->mLp->GetNameIndex(incName.toStdString(), false)-1;
                        if (ivalIdx >=0 && ivalIdx < this->mLp->GetNColumns())
                        {
                            incstr << incName << "=" << pdVars[ivalIdx] << " ";
                        }
                    }
                }
                incstr <<  Qt::endl;
            }
            incstr <<  Qt::endl <<  Qt::endl;
            incFile.close();
        }
        else
        {
            NMLogError(<< "Cannot write baseline reductions values to '"
                       << filename.toStdString() << "'!");
        }
    }

}

QString NMMosra::getReport(void)
{
    return this->msReport;
}

void NMMosra::writeProblem(QString filename, NMMosoExportType type)
{
    QFileInfo fifo(filename);
    QFileInfo difo(fifo.absolutePath());
    if (difo.isWritable())
    {
        QString path = fifo.absolutePath();
        QString baseName = fifo.completeBaseName();
        if (type == NM_MOSO_LP)
        {
            this->mProblemFilename = QString("%1/%2.lp").arg(path).arg(baseName);
            if (this->mLp->CheckLp())
            {
                this->mLp->WriteLp(mProblemFilename.toStdString());
            }
        }
        else
        {
            this->mProblemFilename = QString("%1/%2.mps").arg(path).arg(baseName);
            if (this->mLp->CheckLp())
            {
                this->mLp->WriteMps(mProblemFilename.toStdString());
            }
        }

        this->mProblemType = type;
    }
    else
    {
        this->mProblemFilename.clear();
    }
}

void NMMosra::writeReport(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        MosraLogError( << "failed writing file '" << fileName.toStdString() << "'!");
        return;
    }

    QByteArray bar(this->msReport.toStdString().c_str());
    file.write(bar);
    file.close();


    // DEBUG WRITE BASELINE REDUCTION VALUES
//#ifdef LUMASS_DEBUG
//    QFileInfo fifo(fileName);
//    QString brfn = QString("%1/%2_%3.txt").arg(fifo.path()).arg(fifo.baseName()).arg("BL_reduction");
//    this->writeBaselineReductions(brfn);
//#endif
}

int NMMosra::checkSettings(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    QString settrep;
    QTextStream sstr(&settrep);

    sstr << "type of DV (0=REAL | 1=INT | 2=BINARY): " << this->meDVType << Qt::endl;

    // get the attributes of the layer
    MosraLogInfo(<< "Optimisation - Checking settings ...")

    //  get the total area of the layer (summing the provided field's data)
    if (this->msAreaField.isEmpty())
    {
        MosraLogError( << "no area field specified!");
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }

    // if we've got option groups, make sure the options in each group are also in
    // the overall options list!
    int ogcnt = 1;
    for(int g=0; g < mlslOptGrps.size(); ++g)
    {
        foreach(const QString& opt, mlslOptGrps[g])
        {
            if (!mslOptions.contains(opt))
            {
                MosraLogError(<< "Option '" << opt.toStdString()
                              << " (Grp #" << ogcnt << ") is not "
                              << "defined in 'OPTIONS' list!");
                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }
            mmOptGrpMap.insert(opt, g);
        }
        ++ogcnt;
    }

    bool optfeatures = false;
    if (this->msOptFeatures.isEmpty())
    {
        MosraLogInfo(<< "no OPT_FEATURES field specified - all features in this dataset"
                     << " are available as 'spatial alternatives' in this scenario!");
    }
    else
    {
        if (!mDataSet->hasColumn(this->msOptFeatures))
        {
            MosraLogError( << "the specified OPT_FEATURES field could not be found!");
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }
        optfeatures = true;
    }

    if (!mDataSet->hasColumn(this->msAreaField.toStdString().c_str()))
    {
        MosraLogError( << "specified area field could not be found!");
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }
    MosraLogInfo(<< "area field OK" << endl);

    // --------------------------------------------------------------------------------------------------------
    MosraLogDebug(<< "calculating area and counting features ..." << endl);
    bool nm_hole = mDataSet->hasColumn("nm_hole");
    int numTuples = mDataSet->getNumRecs();
    int numFeat = 0;

    QMap<QString, QMap<QString, double> >::iterator zonesIt;
    QMap<QString, QMap<QString, int> >::iterator zonesLenIt;
    QMap<QString, double>::iterator optIt;
    QMap<QString, int>::iterator optLenIt;

    double tmpVal;
    int tmpLen;
    bool arealCriValid = true;
    this->mdAreaTotal = 0;

    for (int cs=0; cs < numTuples; cs++)
    {
        if (nm_hole && mDataSet->getIntValue("nm_hole", cs) == 1)
        {
            continue;
        }
        if (optfeatures && mDataSet->getIntValue(this->msOptFeatures, cs) == 0)
        {
            continue;
        }

        this->mdAreaTotal += mDataSet->getDblValue(this->msAreaField, cs);
        numFeat++;

        // iterate over the initialised zones and calc areas
        zonesIt = this->mmslZoneAreas.begin();
        zonesLenIt = this->mmslZoneLength.begin();
        for (; zonesIt != this->mmslZoneAreas.end(); ++zonesIt, ++zonesLenIt)
        {
            bool zoneAr = mDataSet->hasColumn(zonesIt.key());

            if (zoneAr == false)
            {
                arealCriValid = false;
                MosraLogError( << "specified zone field '" << zonesIt.key().toStdString()
                        << "' does not exist in the data base!");
                continue;
            }

            optIt = zonesIt.value().begin();
            optLenIt = zonesLenIt.value().begin();
            for (; optIt != zonesIt.value().end(); ++optIt, ++optLenIt)
            {
                const QString zoneVal = mDataSet->getStrValue(zonesIt.key(), cs);
                const QStringList zoneValList = zoneVal.split(" ", Qt::SkipEmptyParts);
                if (zoneValList.contains(optIt.key()))
                {
                    tmpVal = optIt.value() + mDataSet->getDblValue(this->msAreaField, cs);
                    tmpLen = optLenIt.value() + 1;
                    zonesIt.value().insert(optIt.key(), tmpVal);
                    zonesLenIt.value().insert(optLenIt.key(), tmpLen);
                }
            }
        }
    }

    // report total area to the user
    this->mlNumOptFeat = numFeat;
    sstr << "total area from " << this->msAreaField
            << "(" << numTuples << " | " << numFeat << ")" << " is " << this->mdAreaTotal <<  Qt::endl;


    // iterate over the initialised zones and report areas
    zonesIt = this->mmslZoneAreas.begin();
    zonesLenIt = this->mmslZoneLength.begin();
    for (; zonesIt != this->mmslZoneAreas.end(); ++zonesIt, ++zonesLenIt)
    {
        bool zoneAr = mDataSet->hasColumn(zonesIt.key());

        if (zoneAr == false)
        {
            continue;
        }

        optIt = zonesIt.value().begin();
        optLenIt = zonesLenIt.value().begin();
        for (; optIt != zonesIt.value().end(); ++optIt, ++optLenIt)
        {
            sstr << "total area for option '" << optIt.key() << "' with respect to zone field '" << zonesIt.key() <<
                    "' = " << optIt.value() <<  Qt::endl;
            sstr << "no of features for option '" << optLenIt.key() << "' with respect to zone field '"
                    << zonesLenIt.key() << "' = " << optLenIt.value() <<  Qt::endl;
        }
    }

    // -----------------------------------------------------------------------------------------------------------------
    // now actually check the areas
    bool bConv = true;
    QMap<QString, QStringList>::const_iterator acIt = this->mmslAreaCons.constBegin();
    double totalConsArea = 0;
    for (; acIt != this->mmslAreaCons.constEnd(); ++acIt)
    {
        // get the user specified area constraint value and add it to the total of all area constraints
        if (acIt.value().size() < 4)
        {
            MosraLogError( << "areal constraint '" << acIt.key().toStdString()
                    << ": " << acIt.value().join(" ").toStdString() << "' is invalid! Check number of parameters!");
            arealCriValid = false;
            continue;
        }

        QString unittype = acIt.value().at(3);
        QStringList zonespec;
        double val = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, zonespec);
        totalConsArea += val;

        // now check, whether we've got an issue with one of the zonal values
        QString OptZone = acIt.value().at(0);
        if (OptZone.contains(":", Qt::CaseInsensitive))
        {
            QStringList ozlist = OptZone.split(tr(":"), Qt::SkipEmptyParts);
            QStringList options = ozlist.at(0).split(tr("+"), Qt::SkipEmptyParts);
            QString zone = ozlist.at(1);

            double zval = 0;
            foreach(const QString& opt, options)
            {
                if (!this->mslOptions.contains(opt))
                {
                    MosraLogError( << "specified option '" << opt.toStdString()
                          << "' is not a valid resource option!");
                    arealCriValid = false;
                    continue;
                }

                QStringList ozspec;
                ozspec << opt << zone;
                double oval = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, ozspec);
                zval = oval > zval ? oval : zval;
            }


            foreach(const QString& opt, options)
            {
                QStringList ozspec;
                ozspec << opt << zone;
                double oval = this->convertAreaUnits(acIt.value().at(2).toDouble(&bConv), unittype, ozspec);

                if (oval > zval)
                {
                    MosraLogWarn( << "area constraint for option '" << opt.toStdString() << "' with respect to zone field '"
                            << zone.toStdString() << "' exceeds the available area for that option in that zone!");
                    //	arealCriValid = false;
                }
            }
        }

    }
    if (arealCriValid)
    {
        MosraLogInfo(<< "areal constraints OK!");
    }


    // now check on the total area
//	if (totalConsArea > this->mdAreaTotal)
//	{
//		MosraLogError( << "the specified area constraints exceed the total available area!");
//		arealCriValid = false;
//	}

    // we just don't bother checking it as above, which allows for having constraints like
    // a >= x
    // a <= y with y>=x

    // well we don't bother with the above but if we're missing the unittype, we have to pull out!
    //arealCriValid = true;


    // selection
    // TODO: add some meaningful code for selections
//	if (this->mLayer->getLayerType() == NMLayer::NM_VECTOR_LAYER)
//	{
//		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this->mLayer);
//		this->mlNumOptFeat = vl->getNumberOfFeatures();
//
//		MosraLogInfo( << "layer features: " << this->mlNumOptFeat << endl);
//	}
//	else // NM_IMAGE_LAYER
//	{
//		; //TODO: in case of an image layer return the number of
//		  //pixels
//	}

    // cell id field = vtkPolyData cellId

    // --------------------------------------------------------------------------------------------------------
    //sMosraLogInfo(<< "checking performance indicator fields for optimisation criteria ..." << endl);
    // performance indicator fields
    QMap<QString, QStringList>::const_iterator criit =
            this->mmslCriteria.constBegin();

    bool criValid = true;
    for (; criit != this->mmslCriteria.constEnd(); ++criit)
    {
        // check each given performance indicator field
        QStringList fields = criit.value();
        for (int f=0; f < fields.size(); ++f)
        {
            if (!mDataSet->hasColumn(fields.at(f)))
            {
                criValid = false;
                MosraLogError( << "couldn't find performance indicator '"
                        << fields.at(f).toStdString() << "'!");
            }
        }
    }
    if (criValid)
    {
        MosraLogInfo( << "optimisation criteria: performance indicator fields OK");
    }

    //    MosraLogInfo(<< "checking fields for evaluating criteria performance ..." << endl);
    //	// performance indicator fields
    criit = this->mmslEvalFields.constBegin();

    bool evalValid = true;
    for (; criit != this->mmslEvalFields.constEnd(); ++criit)
    {
        // check each given performance indicator field
        QStringList fields = criit.value();
        for (int f=0; f < fields.size(); ++f)
        {
            if (!mDataSet->hasColumn(fields.at(f)))
            {
                evalValid = false;
                MosraLogError( << "couldn't find performance evaluation indicator '"
                        << fields.at(f).toStdString() << "'!");
            }
        }
    }
    if (evalValid)
    {
        MosraLogInfo(<< "optimisation criteria: performance evaluation fields OK")
    }

    // check the performance indicator fields specified with
    // criteria constraints
    bool criConsValid = true;
    if (this->mmslCriCons.size() > 0)
    {
        //MosraLogInfo(<< "checking performance indicator fields for attributive constraints ..." << endl);
        QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit =
                this->mmslCriCons.constBegin();

        for (; crilabit != this->mmslCriCons.end(); ++crilabit)
        {
            QMap<QString, QStringList> luFieldList = crilabit.value();
            criit = luFieldList.begin();
            for (; criit != luFieldList.end(); ++criit)
            {
                QStringList zonespec;
                QString landuse;
                QString zone = "";
                if (criit.key().contains(tr(":"), Qt::CaseInsensitive))
                {
                    zonespec = criit.key().split(tr(":"), Qt::SkipEmptyParts);
                    landuse = zonespec.at(0);
                    zone = zonespec.at(1);

                    // check for zone field
                    if (!mDataSet->hasColumn(zone))
                    {
                        criConsValid = false;
                        MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find zone field '"
                                << zone.toStdString() << "'!");
                    }
                }
                else
                    landuse = criit.key();

                QStringList fieldList = criit.value();
                if (landuse.compare(tr("total"), Qt::CaseInsensitive) == 0)
                {
                    // look through m_iNumOptions fields
                    for (int f=0; f < this->miNumOptions; ++f)
                    {
                        if (!mDataSet->hasColumn(fieldList.at(f)))
                        {
                            criConsValid = false;
                            MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
                                    << fieldList.at(f).toStdString() << "'!");
                        }
                    }
                }
                else
                {
                    // check whether the given land use matches one of the specified ones
                    if (!this->mslOptions.contains(landuse))
                    {
                        criConsValid = false;
                        MosraLogError( << "CRITERIA_CONSTRAINTS: specified resource '"
                                << landuse.toStdString() << "' does not match any of "
                                "the previously specified resources!");
                        continue;
                    }

                    // look for the specified performance field of the given land use
                    if (!mDataSet->hasColumn(fieldList.at(0)))
                    {
                        criConsValid = false;
                        MosraLogError( << "CRITERIA_CONSTRAINTS: couldn't find performance indicator '"
                                << fieldList.at(0).toStdString() << "'!");
                    }
                }
            }
        }
    }
    if (criConsValid)
    {
        MosraLogInfo(<< "performance constraints OK")
    }

    // -------------------------------------------------------------------------
    bool parametersValid = true;

    auto pit = mmslParameters.cbegin();
    for (; pit != mmslParameters.cend(); ++pit)
    {
        foreach(const QString& parCol, pit.value())
        {
            if (!mDataSet->hasColumn(parCol))
            {
                parametersValid = false;
                MosraLogError(<< "Parameter '" << pit.key().toStdString()
                              << "': couldn't find '" << parCol.toStdString()
                              << "' column in the dataset!");
            }
        }
    }


    // -------------------------------------------------------------------------
    // check availability of specified field names in zone constraints
    bool zoneConsValid = true;
    QMap<QString, QStringList>::ConstIterator zconsIt = this->mslZoneConstraints.cbegin();
    while (zconsIt != mslZoneConstraints.cend())
    {
        if (!mDataSet->hasColumn(zconsIt.value().at(0)))
        {
            zoneConsValid = false;
            MosraLogError(<< zconsIt.key().toStdString() << ": "
                          << "couldn't find '" << zconsIt.value().at(0).toStdString()
                          << "' column in the dataset!");
        }

        // check resource specification
        if (    zconsIt.value().at(1).compare("total") != 0
             && !mDataSet->hasColumn(zconsIt.value().at(1))
            )
        {
            zoneConsValid = false;
            MosraLogError(<< zconsIt.key().toStdString() << ": "
                          << "couldn't find '" << zconsIt.value().at(1).toStdString()
                          << "' column in the dataset!");
        }

        if (!mDataSet->hasColumn(zconsIt.value().at(4)))
        {
            zoneConsValid = false;
            MosraLogError(<< zconsIt.key().toStdString() << ": "
                          << "couldn't find '" << zconsIt.value().at(4).toStdString()
                          << "' column in the dataset!");
        }

        QMap<QString, QStringList>::ConstIterator lablIt = mmslCriteria.find(zconsIt.value().at(2));
        if (lablIt == mmslCriteria.cend())
        {
            zoneConsValid = false;
            MosraLogError(<< zconsIt.key().toStdString() << ": "
                          << "criterion '" << zconsIt.value().at(2).toStdString()
                          << "' doesn't seem to be defined in the optimisation settings!");
        }

        QString opStr = zconsIt.value().at(3);
        if (    opStr.compare("<=") != 0
             && opStr.compare(">=") != 0
             && opStr.compare("=")  != 0
           )
        {
            zoneConsValid = false;
            MosraLogError(<< zconsIt.key().toStdString() << ": "
                          << "invalid operator '" << zconsIt.value().at(3).toStdString()
                          << "'!");
        }

        ++zconsIt;
    }
    if (zoneConsValid)
    {
        MosraLogInfo(<< "zone constraints OK")
    }

    // --------------------------------------------------------------------------------------------------------
    // checking the incentives specification
    bool incentivesValid = true;

    // check whether the criteria have been specified
    QMap<QString, QStringList>::ConstIterator incCriPair = this->mIncNamePair.constBegin();
    while (incCriPair != this->mIncNamePair.constEnd())
    {
        const QStringList& criteria = this->mmslCriteria.keys();
        foreach (const QString& cri, incCriPair.value())
        {
            if (!criteria.contains(cri))
            {
                incentivesValid = false;
                MosraLogError(<< "Incentive criterion '" << cri.toStdString() << "' "
                              << "is not a valid criterion of the configured "
                              << "spatial optimisation problem!");
            }
        }
        ++incCriPair;
    }

    // checking the provided fieldnames for incentive scores
    QMap<QString, QStringList>::ConstIterator incFieldIt = this->mmslIncentives.constBegin();
    while (incFieldIt != this->mmslIncentives.constEnd())
    {
        const QString& scoreField = incFieldIt.value().at(0);
        if (!this->mDataSet->hasColumn(scoreField))
        {
            incentivesValid = false;
            MosraLogError(<< "The specified incentive score '" << scoreField.toStdString() << "' "
                          << "could not be found in the dataset!");
        }
        ++incFieldIt;
    }
    if (incentivesValid)
    {
        MosraLogInfo(<< "incentives spec OK")
    }

    // --------------------------------------------------------------------------------------------------------
    // checking the feature-set constraints
    bool featsetconsValid = true;

    NMMosraDataSet::NMMosraDataSetType dstype = this->mDataSet->getDataSetType();
    if (dstype == NMMosraDataSet::NM_MOSRA_DS_OTBTAB)
    {
        if (this->mDataSet->getOtbAttributeTable()->GetTableType() != otb::AttributeTable::ATTABLE_TYPE_SQLITE)
        {
            featsetconsValid = false;
            MosraLogError(<< "Feature-set constraints are currently only supported "
                          << "by the lumassengine in conjunction with SQLite-based "
                          << "attribute tables (*.ldb)!");
        }
    }

    QMap<QString, QStringList>::ConstIterator fsconsIt = this->mslFeatSetCons.constBegin();
    while(fsconsIt != this->mslFeatSetCons.constEnd())
    {
        QStringList keyPair = fsconsIt.key().split(":", Qt::SkipEmptyParts);
        if (keyPair.size() == 2)
        {
            if (    keyPair.at(0).compare(QStringLiteral("total"), Qt::CaseInsensitive) != 0
                 && !this->mslOptions.contains(keyPair.at(0))
               )
            {
                QStringList uses = keyPair.at(0).split("+", Qt::SkipEmptyParts);
                if (this->mslOptions.size() < uses.size())
                {
                    featsetconsValid = false;
                    MosraLogError(<< "The nubmer of specified options ("
                                  << uses.size() << ") exceeds the number ("
                                  << this->mslOptions.size() << ") "
                                  << "defined options!");
                }

                foreach (const QString use, uses)
                {
                    if (!this->mslOptions.contains(use))
                    {
                        featsetconsValid = false;
                        MosraLogError(<< "The specified option '"
                                      << use.toStdString() << "' "
                                      << "is invalid! Please correct your settings!");
                    }
                }

                if (!this->mDataSet->hasColumn(keyPair.at(1)))
                {
                    featsetconsValid = false;
                    MosraLogError(<< "The specified column of feature-set identifiers '"
                                  << keyPair.at(1).toStdString() << "' "
                                  << "could not be found in the specified dataset!");
                }
            }
        }

        const QStringList& valueSpec = fsconsIt.value();
        if (valueSpec.size() < 3)
        {
            featsetconsValid = false;
            MosraLogError(<< "The specified criterion and value specification '"
                          << valueSpec.join(" ").toStdString() << "' "
                          << "is incomplete!");
        }

        if (this->mmslCriteria.constFind(valueSpec.at(0)) == this->mmslCriteria.constEnd())
        {
            featsetconsValid = false;
            MosraLogError(<< "The specified criterion '"
                          << valueSpec.at(0).toStdString() << "' "
                          << "is invalid!");
        }

        QStringList compOps;
        compOps << "<=" << "=" << ">=";
        if (!compOps.contains(valueSpec.at(1)))
        {
            featsetconsValid = false;
            MosraLogError(<< "The specified comparison operator '"
                          << valueSpec.at(1).toStdString() << "' "
                          << "is invalid!");

        }

        if (!this->mDataSet->hasColumn(valueSpec.at(2)))
        {
            featsetconsValid = false;
            MosraLogError(<< "The specified column of RHS vlaues '"
                          << valueSpec.at(2).toStdString() << "' "
                          << "could not be found in the specified dataset!");
        }


        ++fsconsIt;
    }

    // the rest is only relevant for ipsolve (at this stage)
    if (mSolverType != NM_SOLVER_LPSOLVE)
    {
        this->msSettingsReport = sstr.readAll();

        MosraLogDebug(<< "Optimisation Settings Report ...\n"
                   << this->msSettingsReport.toStdString() << endl);

        NMDebugCtx(ctxNMMosra, << "done!");
        if (    !parametersValid || !criValid || !criConsValid
             || !evalValid || !arealCriValid || !incentivesValid
             || !featsetconsValid
           )
        {
            MosraLogError( << "The configuration file contains invalid settings!");
            return 0;
        }
        else
            return 1;
    }

    // --------------------------------------------------------------------------------------------------------
    //MosraLogInfo(<< "calculating size of the optimsation matrix ..." << endl);
    /*
     * structure of the lp matrix  assuming that
     * we've got two options (land uses)
     *
     * i : feature index  (0-based)
     * r : land use index (1-based) note: 0 represents the whole parcel (s. incentives below)
     *
     * X_i_r: area share of land-use r allocated to feature i
     * NOT USED! b_i  : binary conditional variable we need to model that
     *           a feature has to be either completely covered by
     *           any combination of the land use options or not at all
     *
     * note: row-0 contains the objective function and cell 0,0 contains
     *       the objective function value; row-1 and beyond contain the
     *       constraints of the model
     *
     *colindex:     0     1       2       3       4       5
     *row-0           | X_0_1 | X_0_2 | X_1_1 | X_1_2 | X_2_1 ...
     *row-1
     *
     * .....................................................................
     * if incentives are specified, the matrix structure becomes:
     *
     * q : incentive index
     *
     * R_i_r_q   note: r includes '0' value here, representing the whole parcel!
     *
     *colindex:     0     1       2        3         4         5         6         7         8        9       10       11        12
     *row-0           | X_0_1 | X_0_2 | R_0_0_1 | R_0_1_1 | R_0_2_1 | R_0_0_2 | R_0_1_2 | R_0_2_2 | X_1_1 | X_1_2 | R_1_0_1 | R_1_1_1 | R_1_2_1 | R_1_1_2 | R_1_2_2 | ...
     *row-1                                ^                                       ^
     *                                     |                                       |
     *                             reduction due to                        reduction through
     *                             retirement of parcel                  LU 1 w.r.t criterion 2
     *                             w.r.t criterion 1
     *                             (not used at the moment)
     * ......................................................................
     * DEPRECATED
     *colindex:     0     1       2      3      4       5
     *row-0           | X_0_1 | X_0_2 | b_0 | X_1_1 | X_1_2 | b_1 | ...
     *row-1 (constraint #1: i.e. coefficients for decision variables)
     *row-2 (constraint #2)
     *row-3  etc.
     *etc.                               ^
     *                                   |
     *                               deprecated
     */

    this->mlNumArealDVar = this->miNumOptions * this->mlNumOptFeat;
    sstr << "mlNumArealDVar = miNumOptions * mlNumOptFeat = "
            << this->mlNumArealDVar << " = " << this->miNumOptions << " * "
            << this->mlNumOptFeat <<  Qt::endl;

    this->mlNumDVar =  this->mlNumArealDVar;// + this->mlNumOptFeat;

    QString plusRedVar = "";
    QString addNumVar = "\n";
    if (incentivesValid)
    {
        const int incDVars = this->mlNumOptFeat *
                            (this->miNumOptions * this->mmslIncentives.size() + this->mmslIncentives.size());
        this->mlNumDVar += incDVars;
        addNumVar = QString(" + %1 * (%2 * %3+%2)\n")
                .arg(this->mlNumOptFeat)
                .arg(this->mmslIncentives.size())
                .arg(this->miNumOptions);
        plusRedVar = QString(" + mlNumOptFeat * (iNumIncentives * miNumOptions + iNumIncentives)");
    }

    sstr << "mlNumDvar = mlNumArealDVar" << plusRedVar
         << " = " << this->mlNumDVar << " = " << this-> mlNumArealDVar
         << addNumVar;
            //<< " + " << this->mlNumOptFeat <<  Qt::endl;

    // number of columns of the decision matrix
    this->mlLpCols = this->mlNumDVar + 1;
    sstr << "mlLpCols = mlNumDvar + 1  = " << this->mlLpCols <<  Qt::endl;

    // Scalarisation method
    QString sMeth;
    if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
        sMeth = tr("Weighted Sum");
    else
        sMeth = tr("Interactive");
    sstr << "Scalarisation Method: " << sMeth <<  Qt::endl <<  Qt::endl;

    this->msSettingsReport = sstr.readAll();

    MosraLogDebug(<< "Optimisation Settings Report ...\n"
               << this->msSettingsReport.toStdString() << endl);

    NMDebugCtx(ctxNMMosra, << "done!");
    if (!criValid || !criConsValid || !evalValid || !arealCriValid || !incentivesValid || !featsetconsValid)
    {
        MosraLogError( << "The configuration file contains invalid settings!");
        return 0;
    }
    else
        return 1;
}

int NMMosra::mapLp(void)
{
    // check return value from lp_solve
    int ret = this->mLp->GetLastReturnFromSolve();

    int resval = 0;
    if (ret != 0 && ret != 1 && ret != 12)
    {
        MosraLogDebug( << "unfortunately no feasible solution!" << endl);
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }

    if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_OTBTAB)
    {
        otb::AttributeTable::Pointer otbtab = mDataSet->getOtbAttributeTable();
        if (otbtab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
        {
            resval = mapLpDb();
        }
    }
    else// if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_VTKDS)
    {
        resval = mapLpTab();
    }
//    else if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_QTSQL)
//    {
//        resval = mapLpQtSql();
//    }

    return resval;
}

int NMMosra::backupLUCControl(void)
{
    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                     << "Dataset is undefined!");
        return 0;
    }

    if (this->msLUCControlField.isEmpty())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                     << "'Land use control field' is not defined!");

        return 0;
    }

    bkprestoreLUCControl(true);

    return 1;

}

int NMMosra::restoreLUCControl(void)
{
    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                     << "Dataset is undefined!");
        return 0;
    }

    if (this->msLUCControlField.isEmpty())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                     << "'Land use control field' is not defined!");

        return 0;
    }

    bkprestoreLUCControl(false);

    return 1;
}

bool NMMosra::bkprestoreLUCControl(bool bBackup)
{
    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.GetPointer() == nullptr)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Failed accessing the database!");
        return false;
    }

    if (sqltab->ColumnExists(this->msLUCControlField.toStdString()) < 0)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << this->msLUCControlField.toStdString()
                      << " does not exist in the database!");
        return false;
    }

    if (sqltab->ColumnExists(this->msBkpLUCControlField.toStdString()) < 0)
    {
        if (!sqltab->AddColumn(this->msBkpLUCControlField.toStdString(), otb::AttributeTable::ATTYPE_STRING))
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Failed adding '" << this->msBkpLUCControlField.toStdString()
                          << "' to the database!");
            return false;
        }
    }

    std::string toField;
    std::string fromField;
    if (bBackup)
    {
        toField = this->msBkpLUCControlField.toStdString();
        fromField = this->msLUCControlField.toStdString();
    }
    else // restore
    {
        fromField = this->msBkpLUCControlField.toStdString();
        toField = this->msLUCControlField.toStdString();
    }

    std::stringstream querystr;
    querystr << "update \"" << sqltab->GetTableName() << "\" "
             << "set \"" << toField << "\" = \"" << fromField << "\" ";

    if (!this->msOptFeatures.isEmpty())
    {
        querystr << "where \"" << this->msOptFeatures.toStdString() << "\" = 1";
    }

    if (!sqltab->SqlExec(querystr.str()))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                     << sqltab->getLastLogMsg());

        return false;
    }

    return true;
}


int NMMosra::mapNL(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // double check whether we've got intel on decision variables yet ...
    // ... in case we just want to map a previously prepared solutions
    //     (i.e. not during the lifetime of this object)
    if (mmVarAdminMap.size() == 0)
    {
        // parse equations
        infixToPolishPrefix();

        // create variable admin structures
        long nvars = 0;
        processVariables(mb_seg, mx_seg, &nvars);
    }

    double dBackScaling = 1.0;
    auto psit = mmParameterScaling.constBegin();
    for (; psit != mmParameterScaling.constEnd(); ++psit)
    {
        if (psit.key().compare(QStringLiteral("AreaHa"), Qt::CaseSensitive) == 0)
        {
            mdAreaScalingFactor = psit.value().first;
            dBackScaling = 1.0/mdAreaScalingFactor;
            break;
        }
    }


    // ---------------------------------------------------------------------------
    // look for the 'solutions' file

    QFile solFile(this->msSolFileName);
    if (!solFile.exists())
    {
        NMDebugAI(<< "ERROR: " << "Couldn't find Ipopt Solutions file '"
                  << this->msSolFileName.toStdString() << "'!" << std::endl)
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Couldn't find Ipopt Solutions file '"
                      << this->msSolFileName.toStdString() << "'!");
        return 0;
    }


    if (!solFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        NMDebugAI(<< "ERROR: Couldn't open Ipopt Solutions file '"
                      << this->msSolFileName.toStdString() << "'!" << std::endl);

        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Couldn't open Ipopt Solutions file '"
                      << this->msSolFileName.toStdString() << "'!");
        return 0;
    }


    // =====================================================================
    // prep the database
    // =====================================================================
    NMDebugAI(<< "Adding output columns to the database ... ");

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.GetPointer() == nullptr)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Houston, we've got a problem: we don't have a rocket, "
                      << "i.e. our database is NULL!'"
                      << this->msSolFileName.toStdString() << "'!");
        return 0;
    }

    //const long long lNumCells = sqltab->GetNumRows();
    //const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    QMap<QString, QVector<int> > colValuePos;
    std::vector< std::string > colnames;
    std::vector< otb::AttributeTable::ColumnValue > colvalues;


    QMap<QString, QVector<int> > getValuePos;
    std::vector< std::string > getnames;
    std::vector< otb::AttributeTable::ColumnValue > getvalues;

    // ..........................
    // add the SDU dim
    QVector<int> vrid;
    vrid.push_back(getnames.size());

    if (mmslParameters.constFind(msSDU) == mmslParameters.cend())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Compulsory '" << msSDU.toStdString()
                      << "' dimension parameter is not defined!");
        return 0;
    }

    getnames.push_back(mmslParameters[msSDU].at(0).toStdString());
    getValuePos.insert(msSDU, vrid);

    // ........................
    // add LUCControlField, if LUCControl is enabled
    if (mbEnableLUCControl)
    {
        QVector<int> lucid;
        lucid.push_back(getnames.size());

        if (sqltab->ColumnExists(this->msLUCControlField.toStdString()) < 0)
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "The specified land use change control field '"
                          << this->msLUCControlField.toStdString() << "' "
                          << "could not be found in the database!");
            return 0;
        }

        getnames.push_back(this->msLUCControlField.toStdString());
        getValuePos.insert(msLUCControlField, lucid);
    }

    // -------------------------------------------------------------------------
    // add the optimisation result columns to the table

    sqltab->BeginTransaction();
    sqltab->AddColumn("OPT_STR", otb::AttributeTable::ATTYPE_STRING);

    QVector<int> ospos;
    ospos.push_back(colnames.size());
    colnames.push_back("OPT_STR");
    colValuePos.insert("OPT_STR", ospos);

    otb::AttributeTable::ColumnValue osval;
    osval.type = otb::AttributeTable::ATTYPE_STRING;
    colvalues.push_back(osval);

    // the land-use variable columns
    QVector<int> lupos;
    QString sOptStr;
    for (int option=0; option < this->miNumOptions; ++option)
    {
        QString optx_val = QString(tr("OPT%1_VAL")).arg(option+1);
        lupos.push_back(colnames.size());
        colnames.push_back(optx_val.toStdString());

        otb::AttributeTable::ColumnValue oval;
        oval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        oval.dval = 0;
        colvalues.push_back(oval);

        sqltab->AddColumn(optx_val.toStdString(), otb::AttributeTable::ATTYPE_DOUBLE);

        // create longest possible version of sOptStr, i.e. for all options
        sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
    }
    colValuePos.insert("OPT_VAL", lupos);

    // ...........................................

    // add LUCControl field to the setter vectors for
    // active land use change control
    if (mbEnableLUCControl)
    {
        QVector<int> setlucid;
        setlucid.push_back(colnames.size());
        colnames.push_back(this->msLUCControlField.toStdString());
        colValuePos.insert(this->msLUCControlField, setlucid);

        otb::AttributeTable::ColumnValue lucval;
        lucval.type = otb::AttributeTable::ATTYPE_STRING;
        colvalues.push_back(lucval);
    }

    // ...........................................

    // add all variables dimensions to the 'gettypes and getnames' vectors
    // and all non-land-use variables to the 'setter vectors'
    QStringList vardims;
    auto varit = mmVarAdminMap.cbegin();
    for (; varit != mmVarAdminMap.cend(); ++varit)
    {
        if (varit.key().compare(QStringLiteral("lu"), Qt::CaseInsensitive) == 0)
        {
            continue;
        }

        // adding dim parameter columns of the input table
        bool bHasOptions = false;
        foreach(const QString& dim, varit.value().dimensions)
        {
            if (dim.compare(msOPTIONS, Qt::CaseInsensitive) == 0)
            {
                bHasOptions = true;
                continue;
            }

            if (mmslParameters.constFind(dim) == mmslParameters.cend())
            {
                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Dimension parameter '" << dim.toStdString() << "' is not defined!");
                return 0;
            }

            if (vardims.contains(dim))
            {
                continue;
            }

            QVector<int> vgetpos;
            vgetpos.push_back(getnames.size());
            getnames.push_back(mmslParameters[dim].at(0).toStdString());
            getValuePos.insert(dim, vgetpos);

            vardims << dim;
        }

        int numpar = 1;
        if (bHasOptions)
        {
            numpar = miNumOptions;
        }

        QVector<int> colpos;
        for (int i=0; i < numpar; ++i)
        {
            QString colName;
            if (bHasOptions)
            {
                colName = QString("%1_%2").arg(varit.key()).arg(mslOptions.at(i));
            }
            else
            {
                colName = QString("%1").arg(varit.key());
            }

            colpos.push_back(colnames.size());
            colnames.push_back(colName.toStdString());

            otb::AttributeTable::ColumnValue cv;
            if (    msBinaryVars.contains(varit.key())
                 || msIntVars.contains(varit.key())
               )
            {
                cv.type = otb::AttributeTable::ATTYPE_INT;
                cv.ival = 0;
            }
            else
            {
                cv.type = otb::AttributeTable::ATTYPE_DOUBLE;
                cv.dval = 0;
            }
            colvalues.push_back(cv);
            sqltab->AddColumn(colName.toStdString(), cv.type);
        }
        colValuePos.insert(varit.key(), colpos);
    }
    sqltab->EndTransaction();
    NMDebug(<< "done!" << std::endl);


    // ---------------------------------------------------------------------
    // prepare get and set statements

    std::stringstream wclstr;
    // where clause or not?
    if (!msOptFeatures.isEmpty())
    {
        wclstr << "where \"" << msOptFeatures.toStdString() << "\" == 1";
    }

    if (!sqltab->PrepareBulkGet(getnames, wclstr.str()))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Failed preparing dim column read query: "
                      << sqltab->getLastLogMsg());
        return 0;
    }

    if (!sqltab->PrepareBulkSet(colnames, false))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Failed preparing dvar write column query: "
                      << sqltab->getLastLogMsg());
        return 0;
    }


    // =====================================================================
    // read and cache solution by offset value
    // =====================================================================

    QTextStream sol(&solFile);
    long long solPos = 0;
    long long dvarCounter = 0;
    bool bok = false;
    bool bDoReadVar = false;

    // <dvar offset>, <dvar value>
    // offsets are 0-based
    QMap<long long, double> dvarCache;

    // ---------------------------------------------
    // Find the optimal solution inside the file
    NMDebugAI(<< "Looking for the optimal solution ... ");
    while (!sol.atEnd())// && solPos == 0)
    {
        QString anyLine = sol.readLine();

        if (!bDoReadVar)
        {
            // --------------------------------------------------
            // looking for the right position
            if (anyLine.contains("Optimal Solution Found"))
            {
                MosraLogInfo(<< anyLine.toStdString() << endl);
            }
            else if (anyLine.contains("Problem may be infeasible"))
            {
                MosraLogWarn(<< anyLine.toStdString() << endl);
            }
            else if (anyLine.contains("_svar[1]"))
            {
                solPos = sol.pos();
                MosraLogInfo(<< "Found decision variables at pos :" << solPos << endl);
                bDoReadVar = true;
            }
        }

        if (bDoReadVar)
        {
            const QStringList elems = anyLine.split(" ", Qt::SkipEmptyParts);
            if (elems.size() == 2)
            {
                const QString dVarVal = elems.at(1);
                double dval = dVarVal.toDouble(&bok);
                /// CANNOT DO THIS HERE AS WE DON'T KNOW WHETHER THIS IS AN AREA VAR!!!!!
                //dval *= dBackScaling;
                if (!bok)
                {
                    NMDebugAI(<< "ERROR: Failed parsing number of variables to read!" << std::endl);
                    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Failed parsing value of decision _svar[" << dvarCounter+1 << "]!");
                    solFile.close();
                    return 0;
                }
                dvarCache.insert(dvarCounter, dval);
                ++dvarCounter;
            }
        }
    }
    solFile.close();


    // =====================================================================
    //   go through database and write cached values
    // =====================================================================
    getvalues.resize(getnames.size());


    // expected number of dvars
    //numVars = this->mlNumDVar;

    // allocate a char* that can hold the longest possible sOptStr
    char* optstr = new char[sOptStr.length()+1];

    // allocate a new LUCControl string
    char* lucstr = new char[sOptStr.length()+1];

    //double dVal = 0;
    //varCounter = 0;

    NMDebugAI(<< "Writing variable values into DB ... " << std::endl)
    //bool bError = false;

    sqltab->BeginTransaction();
    bool brow = sqltab->DoBulkGet(getvalues);
    if (!brow)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Failed fetching even the first row of the database: "
                      << sqltab->getLastLogMsg());
        sqltab->EndTransaction();
        return 0;
    }

    this->miNumRecLUCChange = 0;
    long lowAllocCounter = 0;
    long allVarCounter = 0;
    do
    {
        const long long rid = getvalues[getValuePos[msSDU].at(0)].ival;

        QString curLUCControlStr;
        if (mbEnableLUCControl)
        {
            curLUCControlStr = getvalues[getValuePos[msLUCControlField].at(0)].tval;
        }
        QMap<double, QString> luAllocMap;

        varit = mmVarAdminMap.cbegin();
        QStringList optstrList;
        for (; varit != mmVarAdminMap.cend(); ++varit)
        {
            bool blu = false;
            if (varit.key().compare(QStringLiteral("lu"), Qt::CaseInsensitive) == 0)
            {
                blu = true;
            }

            bool bOptions = false;
            if (varit.value().dimensions.contains(msOPTIONS))
            {
                bOptions = true;
            }

            // get all other but the OPTIONS dimension
            QVector<long long> vdimVals;
            foreach (const QString& vdim, varit.value().dimensions)
            {
                if (vdim.compare(msOPTIONS, Qt::CaseInsensitive) == 0)
                {
                    continue;
                }
                const otb::AttributeTable::TableColumnType tct = getvalues.at(getValuePos[vdim].at(0)).type;
                switch(tct)
                {
                case otb::AttributeTable::ATTYPE_INT:
                    vdimVals.push_back(getvalues.at(getValuePos[vdim].at(0)).ival);
                    break;
                case otb::AttributeTable::ATTYPE_DOUBLE:
                    vdimVals.push_back(getvalues.at(getValuePos[vdim].at(0)).dval);
                    break;
                default:
                    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Got an unsupported data type for variable '" << varit.key().toStdString()
                                  << "'!");
                    sqltab->EndTransaction();
                    return 0;
                    ;
                }

            }

            int nopts = bOptions ? miNumOptions : 1;
            for (int v=0; v < nopts; ++v)
            {
                QVector<long long> vTempDims = vdimVals;
                if (bOptions)
                {
                    vTempDims.push_back(v);
                }

                ++allVarCounter;
                const long long voffset = varit.value().dimOffsetMap[vTempDims];
                double dval = 0;

                QString vname = varit.key();

                if (blu)
                {
                    if (dvarCache[voffset] >= mdMinAlloc)
                    {
                        dval = dvarCache[voffset];
                        /// NOW, WE 'BACK-SCALE' area values, if applicable
                        dval *= dBackScaling;
                    }
                    else
                    {
                        ++lowAllocCounter;
                    }

                    vname = QStringLiteral("OPT_VAL");
                    if (dval > 0)
                    {
                        optstrList << mslOptions.at(v);
                    }

                    if (mbEnableLUCControl && dval > 0)
                    {
                        luAllocMap.insert(dval, mslOptions.at(v));
                    }
                }

                const otb::AttributeTable::TableColumnType dt = colvalues[colValuePos[vname].at(v)].type;
                switch(dt)
                {
                case otb::AttributeTable::ATTYPE_INT:
                    colvalues[colValuePos[vname].at(v)].ival = static_cast<long long>(dval);
                    break;
                case otb::AttributeTable::ATTYPE_DOUBLE:
                    colvalues[colValuePos[vname].at(v)].dval = dval;
                    break;
                default:
                    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Got an unsupported data type for variable '" << vname.toStdString()
                                  << "'!");
                    sqltab->EndTransaction();
                    return 0;
                }
            }
        }

        optstr[0] = '\0';
        ::sprintf(optstr, "%s", optstrList.join(" ").toStdString().c_str());
        colvalues[colValuePos["OPT_STR"].at(0)].tval = optstr;

        // do luc control, if enabled
        if (mbEnableLUCControl)
        {
            QString newLUCControlString = curLUCControlStr;
            // only add lu to LUCControl field for this SDU when
            // - we've got more lus allocated than we want
            // - and allocated lus are spread across more than
            //   one lu group
            if (luAllocMap.size() > miMaxOptAlloc)
            {
                bool bAllInOne = true;
                if (mmOptGrpMap.size() > 0)
                {
                    // check whether lus belong to the same group
                    auto amit = luAllocMap.cbegin();
                    const int grp = mmOptGrpMap[amit.value()];
                    amit++;

                    while(amit != luAllocMap.cend())
                    {
                        if (mmOptGrpMap[amit.value()] != grp)
                        {
                            bAllInOne = false;
                            break;
                        }
                    }
                }
                else
                {
                    bAllInOne = false;
                }

                // ... and if they don't belong to the same group
                // apply land-use control
                if (!bAllInOne)
                {
                    QStringList passList;
                    // only add unwanted lus to LUCControl string
                    auto luit = luAllocMap.constBegin();
                    int ditch = luAllocMap.size() - miMaxOptAlloc;
                    for (int r=0; luit != luAllocMap.cend(); ++luit, ++r)
                    {
                        if (r < ditch)
                        {
                            continue;
                        }
                        passList << luit.value();
                    }

                    // add all non passes to the LUCControl string if not already on it
                    bool bcountedRec = false;
                    for (int flu=0; flu < mslOptions.size(); ++flu)
                    {
                        if (   !passList.contains(mslOptions[flu])
                            && !curLUCControlStr.contains(mslOptions[flu])
                           )
                        {
                            newLUCControlString += QString(" %1").arg(mslOptions[flu]);
                            if (!bcountedRec)
                            {
                                this->miNumRecLUCChange++;
                                bcountedRec = true;
                            }
                        }
                    }
                }
            }
            // lock in the group we've got to minimise the number of iterations!
            else if (this->mbLockInLuGrps)
            {
                QString curOpt = optstr;
                for (int lus=0; lus < mslOptions.size(); ++lus)
                {
                    if (    curOpt.compare(mslOptions[lus], Qt::CaseInsensitive) != 0
                         && !curLUCControlStr.contains(mslOptions[lus])
                         && mmOptGrpMap[curOpt] != mmOptGrpMap[mslOptions[lus]]
                       )
                    {
                        newLUCControlString += QString(" %1").arg(mslOptions[lus]);
                    }
                }
            }

            lucstr[0] = '\0';
            ::sprintf(lucstr, "%s", newLUCControlString.toStdString().c_str());
            colvalues[colValuePos[msLUCControlField].at(0)].tval = lucstr;
        }

        if (!sqltab->DoBulkSet(colvalues, rid))
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Failed writing solution for record #" << rid << ": "
                          << sqltab->getLastLogMsg());
            sqltab->EndTransaction();
            return 0;
        }

        brow = sqltab->DoBulkGet(getvalues);
    } while (brow);

    sqltab->EndTransaction();

    MosraLogInfo(<< "Ignored " << lowAllocCounter << " of " << allVarCounter
                 << " variable values that were smaller than " << mdMinAlloc << "!" << endl);

    /*
    for (long f=0; f < lNumCells; ++f)
    {
        if (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
        {
            continue;
        }

        // -------------------------------------------------
        // processing variables per record / feature at a time
        if (sol.atEnd() && varCounter < numVars)
        {
            NMDebugAI(<< "Got only " << varCounter << " of " << numVars << " variables. Solution '" << this->msSolFileName.toStdString() << "' does not contain "
                          << "all variable values we expected!" << std::endl);
            MosraLogError(<< "Got only " << varCounter << " of " << numVars << " variables. Solution '" << this->msSolFileName.toStdString() << "' does not contain "
                          << "all variable values we expected!" << std::endl);
            break;
        }

        sOptStr.clear();
        for (int option=0; option < this->miNumOptions && !sol.atEnd(); ++option)
        {
            QString line = sol.readLine();
            QStringList ll = line.split(" ", Qt::SkipEmptyParts);
            if (ll.size() == 2)
            {
                dVal = ll.at(1).toDouble(&bok);
                if (!bok)
                {
                    MosraLogError(<< "failed parsing variable _svar[" << (varCounter + 1) << "]!");
                    bError = true;
                    break;
                }

//////////////////////////////////////////////////////////////////
/// ToDo: need to read in Ipopt options file to set the precision!
//////////////////////////////////////////////////////////////////
                if (dVal <= 1e-8)
                {
                    dVal = 0;
                }

                colValues[option+1].type = otb::AttributeTable::ATTYPE_DOUBLE;
                colValues[option+1].dval = dVal;

                // format sOptStr
                if (dVal > 0)
                {
                    sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
                }
            }
            else
            {
                MosraLogError(<< "failed parsing variable _svar[" << (varCounter + 1) << "]!");
                bError = true;
                break;
            }

            ++varCounter;
        }

        if (bError)
        {
            break;
        }

        // ----------------------------------------------------
        // writing feature / record variable into DB

        // trim sOptStr and write into array
        sOptStr = sOptStr.simplified();
        colValues[0].type = otb::AttributeTable::ATTYPE_STRING;
        optstr[0] = '\0';
        ::sprintf(optstr, "%s", sOptStr.toStdString().c_str());
        colValues[0].tval = optstr;

        sqltab->DoBulkSet(colValues, f);

        if (f % 100 == 0)
            NMDebug(<< ".");
    }*/


    //sqltab->EndTransaction();
    NMDebug(<< std::endl);

    //solFile.close();
    delete[] optstr;
    delete[] lucstr;

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::mapLpDb(void)
{
    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(this->mDataSet->getOtbAttributeTable().GetPointer());

    const long long lNumCells = sqltab->GetNumRows();
    const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    std::vector< std::string > colnames;
    std::vector< otb::AttributeTable::TableColumnType > coltypes;

    // add the optimisation result columns to the table
    sqltab->BeginTransaction();
    sqltab->AddColumn("OPT_STR", otb::AttributeTable::ATTYPE_STRING);
    colnames.push_back("OPT_STR");
    coltypes.push_back(otb::AttributeTable::ATTYPE_STRING);

    QString sOptStr;
    for (int option=0; option < this->miNumOptions; ++option)
    {
        QString optx_val = QString(tr("OPT%1_VAL")).arg(option+1);
        colnames.push_back(optx_val.toStdString());
        coltypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
        sqltab->AddColumn(optx_val.toStdString(), otb::AttributeTable::ATTYPE_DOUBLE);

        // create longest possible version of sOptStr, i.e. for all options
        sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
    }
    sqltab->EndTransaction();


    // write opt results in table
    std::vector< otb::AttributeTable::ColumnValue > colValues;
    colValues.resize(this->miNumOptions+1);
    sqltab->PrepareBulkSet(colnames, false);
    sqltab->BeginTransaction();


    // get the decision vars
    double * pdVars;
    this->mLp->GetPtrVariables(&pdVars);

    // some more vars
    QString sColName;
    double dVal;
    int iValIndex;

    // allocate a char* that can hold the longest possible sOptStr
    char* optstr = new char[sOptStr.length()+1];

    // interate over the table and write optimisation results into the table
    for (long long f=0; f < lNumCells; ++f)
    {
        if (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
        {
            continue;
        }

        sOptStr.clear();
        for (int option=0; option < this->miNumOptions; ++option)
        {
            // format the column name (i.e. name of the decision variable
            // we are dealing with int this iteration)
            sColName = QString(tr("X_%1_%2")).arg(f).arg(option+1);

            //determine index for the pdVars array holding the values of the decision
            //variables (i.e. the appropriate column index - 1)
            iValIndex = this->mLp->GetNameIndex(sColName.toStdString(), false)-1;

            if (iValIndex >=0)
            {
                // get the actual value and put it into the right OPTx_VAL array (attribute)
                dVal = pdVars[iValIndex];
                //mDataSet->setDblValue(vValAr.at(option), f, dVal);
                colValues[option+1].type = otb::AttributeTable::ATTYPE_DOUBLE;
                colValues[option+1].dval = dVal;

                // format sOptStr
                if (dVal > 0)
                    sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
            }
            else
            {
                MosraLogError( << "failed fetching value of decision variable '"
                               << sColName.toStdString() << "'!");
            }
        }

        // trim sOptStr and write into array
        sOptStr = sOptStr.simplified();
        colValues[0].type = otb::AttributeTable::ATTYPE_STRING;
        optstr[0] = '\0';
        ::sprintf(optstr, "%s", sOptStr.toStdString().c_str());
        colValues[0].tval = optstr;

        sqltab->DoBulkSet(colValues, f);

        if (f % 100 == 0)
            NMDebug(<< ".");
    }
    sqltab->EndTransaction();

    // clear memory for optstr
    delete[] optstr;

    return 1;
}

int
NMMosra::mapLpQtSql(void)
{
    //add mapping columns
//    this->mDataSet->addColumn("OPT_STR", NMMosraDataSet::NM_MOSRA_DATATYPE_STRING);
//    for (int option=0; option < miNumOptions; ++option)
//    {
//        QString optx_val = QString("OPT%1_VAL").arg(option+1);
//        mDataSet->addColumn(optx_val, NMMosraDataSet::NM_MOSRA_DATATYPE_DOUBLE);
//    }

//    QSqlTableModel* sqlMod = mDataSet->getQSqlTableModel();
//    QSqlDatabase db = sqlMod->database();
//    QSqlDriver* drv = db.driver();

    return 0;
}

int NMMosra::mapLpTab(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // get the attributes of the layer
    const bool hole = mDataSet->hasColumn("nm_hole");
    const bool optFeat = mDataSet->hasColumn(this->msOptFeatures);
    const long lNumCells = mDataSet->getNumRecs();

    // prepare list of update columns
    QStringList colnames;
    QVariantList valList;

    // create a new result field for the resource option
    mDataSet->beginTransaction();
    mDataSet->addColumn("OPT_STR", NMMosraDataSet::NM_MOSRA_DATATYPE_STRING);
    colnames.push_back("OPT_STR");
    valList.push_back(QVariant(QVariant::String));
    MosraLogDebug( << "created attribute 'OPT_STR'" << endl);

    // add scoring attributes
    QStringList vValAr;
    for (int option=0; option < this->miNumOptions; option++)
    {
        QString optx_val = QString(tr("OPT%1_VAL")).arg(option+1);
        mDataSet->addColumn(optx_val, NMMosraDataSet::NM_MOSRA_DATATYPE_DOUBLE);
        colnames.push_back(optx_val);
        valList.push_back(QVariant(0.0));
        vValAr.push_back(optx_val);
        MosraLogDebug( << "created scoring attribute '" << optx_val.toStdString() << "'" << endl);
    }
    mDataSet->endTransaction();

    // get the decision vars
    double * pdVars;
    this->mLp->GetPtrVariables(&pdVars);

    // some more vars
    QString sColName, sOptStr;
    double dVal;
    int iValIndex;
    const int iOffset = this->miNumOptions;
    long lNonHoleCounter = 0;

    QVariantList getValues;
    QStringList getNames;
    if (hole)
    {
        getValues << QVariant::Int;
        getNames << "nm_hole";
    }
    if (optFeat)
    {
        getValues << QVariant::Int;
        getNames << this->msOptFeatures;
    }

    mDataSet->prepareRowGet(getNames);
    mDataSet->prepareRowUpdate(colnames, false);
    mDataSet->beginTransaction();


    //MosraLogDebug( << "extracting results ..." << endl);
    // -------------------------------------------------------------------for each feature
    for (int f=0; f < lNumCells; f++)
    {
        if (    (hole || optFeat)
            &&  !mDataSet->getRowValues(getValues, f)
           )
        {
            MosraLogError( << "mapLp(): Failed fetching row values!");
            mDataSet->endTransaction();
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }

        // special treatment for polygon holes or no-opt features
        if (    (hole && getValues[0] == 1)
             || (optFeat && getValues[getValues.size()-1] == 0)
           )
        {
            //mDataSet->setStrValue("OPT_STR", f, "");
            valList[0] = QVariant(QVariant::String);

            for (int option=0; option < this->miNumOptions; option++)
            {
                //mDataSet->setDblValue(vValAr.at(option), f, 0.0);
                valList[option+1] = QVariant(0.0);
            }

            // leap frog
            //continue;
        }
        else
        {
            // -----------------------------------------------------------------for each option
            sOptStr.clear();
            for (int option=0; option < this->miNumOptions; option++)
            {
                // format the column name (i.e. name of the decision variable
                // we are dealing with int this iteration)
                sColName = QString(tr("X_%1_%2")).arg(lNonHoleCounter).arg(option+1);

                //determine index for the pdVars array holding the values of the decision
                //variables (i.e. the appropriate column index - 1)
                iValIndex = this->mLp->GetNameIndex(sColName.toStdString(), false)-1;

                // get the actual value and put it into the right OPTx_VAL array (attribute)
                dVal = pdVars[iValIndex];
                valList[option+1] = QVariant(dVal);

                // format sOptStr
                if (dVal > 0)
                    sOptStr += QString(this->mslOptions.at(option)) + tr(" ");
            }

            // trim sOptStr and write into array
            sOptStr = sOptStr.simplified();
            valList[0] = sOptStr;
        }

        if (!mDataSet->updateRow(valList, f))
        {
            mDataSet->rollBack();
            mDataSet->endTransaction();
            NMDebugAI(<< "ERROR: failed updating row #" << f << "!" << endl);
            NMDebugCtx(ctxNMMosra, << "done!");
            return 0;
        }

        if (f % 100 == 0)
            NMDebug(<< ".");

        // increment the valid feature counter
        lNonHoleCounter++;
    }

    mDataSet->endTransaction();

    MosraLogDebug(<< "extracted results!" << endl);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

bool NMMosra::infixToPolishPrefix()
{
    /// This implementation is based on
    /// - Dijkstra EW, 1961: Algol 60 translation: An Algol 60 translator for the X1 and making a translator for Algol 60. Stichting Mathematisch Centrum rekenafdeling. Stichting Mathematisch Centrum. https://ir.cwi.nl/pub/9251 (accessed 2023-05-03)
    /// - Gay DM, 2005: Writing .nl Files. https://ampl.github.io/nlwrite.pdf (accessed 2023-05-02)
    /// - Borchia D, 2022: Polish Notation Converter. https://www.omnicalculator.com/math/polish-notation#examples-of-conversion-from-infix-to-the-polish-notations-and-vice-versa (accessed 2023-05-02)


    // -----------------------------------------
    // parse equations
    // -----------------------------------------
    MosraLogInfo(<< "parsing Equations ...");
    auto eqnit = mmsEquations.cbegin();
    while (eqnit != mmsEquations.cend())
    {
        EquationAdmin ea;
        ea.name = eqnit.key();

        if (mmEquationAdmins.constFind(ea.name) != mmEquationAdmins.cend())
        {
            MosraLogError(<< "NMMosra::infixToPolishPrefix() - ERROR: "
                          << "Equation '" << ea.name.toStdString() << "' "
                          << "has already been defined! Please use unique "
                          << "equation names!" << endl);
            return false;
        }

        MosraLogDebug(<< "  '" << ea.name.toStdString() << "' ... ");
        // logging the end positions of equation elments as
        // we use this EquationAdmin for reversing the
        // equations in prep for polishing ...
        if (!parseEquation(eqnit.value().simplified(), ea, true))
        {
            MosraLogError(<< "NMMosra::infixToPolishPrefix() - ERROR: "
                          << "Equation parsing failed!");
            return false;
        }

        mmEquationAdmins.insert(ea.name, ea);
        ++eqnit;
    }

    //auto cit = mmsLinearConstraints.cbegin();
    //while (cit != mmsLinearConstraints.cend())
    //{
    //    EquationAdmin outEa;
    //    outEa.name = cit.key();
    //
    //    MosraLogInfo(<< "  '" << outEa.name.toStdString() << "' ... ");
    //    if (!parseEquation(cit.value().simplified(), outEa, true))
    //    {
    //        MosraLogError(<< "NMMosra::infixToPolishPrefix() - ERROR: "
    //                      << "Equation parsing failed!");
    //        return false;
    //    }
    //
    //    mmEquationAdmins.insert(outEa.name, outEa);
    //    ++cit;
    //}


    // -----------------------------------------
    // convert infix into polish prefix notation
    // -----------------------------------------
    MosraLogDebug(<< "transforming Equations ...");

    auto eqnadminit = mmEquationAdmins.begin();
    while (eqnadminit != mmEquationAdmins.end())
    {
        EquationAdmin pa;
        pa.name = eqnadminit.key();

        QStringList polishPrefix;

        // ----------------------------------------------------------
        // DEBUG DEBUG DEBUG
        MosraLogDebug(<< pa.name.toStdString() << ": ...");
        QString pEqn;
        if (mmsEquations.constFind(pa.name) != mmsEquations.cend())
        {
            pEqn = mmsEquations.value(pa.name);
            MosraLogDebug(<< pEqn.toStdString());
        }
        //else if (mmsLinearConstraints.constFind(pa.name) != mmsLinearConstraints.cend())
        //{
        //    pEqn = mmsLinearConstraints.value(pa.name);
        //    MosraLogInfo(<< pEqn.toStdString());
        //}
        // DEBUG DEBUG DEBUG
        // ----------------------------------------------------------

        if (!polishingEqn(eqnadminit.value(), pa, polishPrefix))
        {
            MosraLogError(<< "NMMosra::infixToPolishPrefix() - ERROR: "
                          << "Polishing equation failed!");
            return false;
        }

        mmPrefixEquations.insert(
                    pa.name,
                    std::pair<EquationAdmin, QStringList>(
                    pa,
                    polishPrefix
                    )
                );

        ++eqnadminit;
    }

    return true;
}

bool NMMosra::parseEquation(const QString eqn,
        EquationAdmin &ea, bool blogEndPos)
{
    bool success = true;

    QStack<std::pair<EqnElement,int>> procstack;
    QStack<std::pair<EqnElement,int>> nostack;

    //QList<Loop> loopList;
    QString token;
    QString noToken;

    QString cleanStr = eqn.simplified() + QChar(' ');

    //MosraLogInfo(<< "NMMosra::parseEquation(): simplified input equation: \n\n"
    //             << cleanStr.toStdString() << endl << endl);
    int logpos = 0;
    for(int p=0; p < cleanStr.size(); ++p)
    {
        const QChar ch = cleanStr[p];

        // -----------------------------------------------------
        //              OPERATORS & NUMBERS
        // -----------------------------------------------------

        if (    mWhitespaces.contains(ch)
             || mOpCharList.contains(ch)
           )
        {
            // FINALISE PARAMETER
            if (    (    mOpCharList.contains(ch)
                      || p == cleanStr.size()-1
                    )
                 && procstack.size() > 0
                 && procstack.last().first == EQN_PARAMETER
               )
            {
                Param& param = ea.paramList[procstack.last().second];
                logpos = blogEndPos ? param.paramEnd : param.paramStart;
                ea.elemMap.insert(
                            logpos,
                            std::pair<EqnElement, int>(
                                EQN_PARAMETER,
                                procstack.last().second
                                )
                            );
                procstack.pop();
            }

            // FINALISE NUMBER
            if (   nostack.size() > 0
                     && nostack.last().first == EQN_NUMBER
                    )
            {
                ea.numList[nostack.last().second].numEnd = p-1;
                bool bConv;
                double val = noToken.toDouble(&bConv);
                if (bConv)
                {
                    ea.numList[nostack.last().second].value = val;
                }
                else
                {
                    ea.numList.takeAt(nostack.last().second);
                    MosraLogError(<< "ERROR in Equation '"
                                  << ea.name.toStdString() << "' at position " << p-1-noToken.size()
                                  << "' - Failed converting '" << noToken.toStdString()
                                  << "' to double!");
                    success = false;
                }

                logpos = blogEndPos ? p-1 : ea.numList[nostack.last().second].numStart;
                ea.elemMap.insert(
                            logpos,
                            std::pair<EqnElement, int>(
                                EQN_NUMBER,
                                nostack.last().second
                                )
                            );

                nostack.pop();
                noToken.clear();
            }
            // LOG EQUATION
            else if (   nostack.size() == 0
                     && noToken.isEmpty()
                     && !token.isEmpty()
                    )
            {
                if (   (   ea.name.compare(token, Qt::CaseSensitive) != 0
                        && (   mmsEquations.keys().contains(token)
                            //|| mmslLinearConstraints.keys().contains(token)
                           )
                       )
                    //|| (   pRevLoopMap != nullptr
                    //    && pRevLoopMap->keys().contains(token)
                    //   )
                   )
                {
                    Equation eqn = {
                        .eqn      = token,
                        .eqnStart = p-(token.size()-1)-1,
                        .eqnEnd   = p-1
                    };

                    logpos = blogEndPos ? eqn.eqnEnd : eqn.eqnStart;
                    ea.elemMap.insert(
                                logpos,
                                std::pair<EqnElement, int>(
                                    EQN_EQUATION,
                                    ea.eqnList.size()
                                    )
                                );

                    ea.eqnList.push_back(eqn);
                }
                else if (   ea.name.compare(token, Qt::CaseSensitive) != 0
                         && mmslVariables.keys().contains(token)
                        )
                {
                    // 0-dimension variable
                    if (mmslVariables[token].size() == 0)
                    {
                        Param par;
                        par.name = token;
                        par.paramStart = p-1-token.size();
                        par.paramEnd = p-1;

                        logpos = blogEndPos ? par.paramEnd : par.paramStart;
                        ea.elemMap.insert(
                                    logpos,
                                    std::pair<EqnElement, int>(
                                        EQN_PARAMETER,
                                        ea.paramList.size()
                                        )
                                    );

                        ea.paramList.push_back(par);
                    }
                    else
                    {
                        MosraLogError(<< "ERROR in Equation '"
                                      << ea.name.toStdString() << "' at position " << p-token.size()
                                      << "' - No dimension(s) specified for variable '"
                                      << token.toStdString() << "'!");
                        success = false;
                    }
                }
                // log 'naked' (i.e. dimensionless) parameter specification
                else if (   procstack.size() == 0
                         && nostack.size() == 0
                         && mmslParameters.constFind(token) != mmslParameters.cend()
                        )
                {
                    Param param = {
                        .name       = token,
                        .paramStart = p-(token.size()-1)-1,
                        .paramEnd   = p-1
                    };
                    logpos = blogEndPos ? param.paramEnd : param.paramStart;
                    ea.elemMap.insert(
                                logpos,
                                std::pair<EqnElement, int>(
                                    EQN_PARAMETER,
                                    ea.paramList.size()
                                    )
                                );
                    ea.paramList.push_back(param);
                }
                else
                {
                    bool bisNum;
                    token.toDouble(&bisNum);

                    if (    !bisNum
                         && !mAMPLOperators.contains(token)
                         && !mParseOperators.contains(token)
                       )
                    {
                        MosraLogError(<< "ERROR in Equation '"
                                      << ea.name.toStdString() << "' at position " << p-token.size()
                                      << "' - Unidentified or incomplete equation, variable, or parameter: '"
                                      << token.toStdString()
                                      << "'!");
                        success = false;
                    }
                }
                token.clear();
            }

            // OPERATOR
            if (mOpCharList.contains(ch))
            {
                if (   noToken.isEmpty()
                    && nostack.size() == 0
                   )
                {
                    Operator op = {
                        .opStart = p
                    };

                    nostack.push(std::pair<EqnElement,int>(EQN_OPERATOR, ea.opList.size()));
                    ea.opList.push_back(op);

                    noToken += ch;
                }
                else if (   nostack.size() > 0
                         && nostack.last().first == EQN_OPERATOR
                        )
                {
                    noToken += ch;
                }

            }

            // FINALISE OPERATOR
            if (mWhitespaces.contains(ch))
            {
                if (   nostack.size() > 0
                    && nostack.last().first == EQN_OPERATOR
                    && (   mParseOperators.contains(noToken)
                        || mAMPLOperators.contains(noToken)
                       )
                   )
                {
                    ea.opList[nostack.last().second].op = noToken;
                    ea.opList[nostack.last().second].opEnd = p-1;

                    logpos = blogEndPos ? p-1 : ea.opList[nostack.last().second].opStart;
                    ea.elemMap.insert(
                                logpos,
                                std::pair<EqnElement, int>(
                                    EQN_OPERATOR,
                                    nostack.last().second
                                    )
                                );

                    nostack.pop();
                    noToken.clear();
                }
            }

            if (mOpCharList.contains(ch))
            {
                token.clear();
            }
            continue;
        }
        else
        {
            if (   nostack.size() > 0
                && nostack.last().first == EQN_OPERATOR
                && (   mParseOperators.contains(noToken)
                    || mAMPLOperators.contains(noToken)
                   )
               )
            {
                ea.opList[nostack.last().second].op = noToken;
                ea.opList[nostack.last().second].opEnd = p-1;

                logpos = blogEndPos ? p-1 : ea.opList[nostack.last().second].opStart;
                ea.elemMap.insert(
                            logpos,
                            std::pair<EqnElement, int>(
                                EQN_OPERATOR,
                                nostack.last().second
                                )
                            );

                nostack.pop();
                noToken.clear();
            }
        }


        // -----------------------------------------------------
        // LOOPS, FUNCTIONS, PARAMETERS/VARIABLES, EQUATIONS, BRACES
        // -----------------------------------------------------

        // LOOP DIMENSIONS
        if (ch == QChar('{') || ch == QChar('}'))
        {
            // LOOP DIMENSION START
            if (ch == QChar('{'))
            {
                if (mLoopNames.contains(token))
                {
                    int lvl = 0;
                    for (int s=0; s < procstack.size(); ++s)
                    {
                        if (procstack[s].first == EQN_LOOP)
                        {
                            ++lvl;
                        }
                    }

                    Loop loop = {
                        .name = token,
                        .level = lvl,
                        .bodyStart = -1,
                        .bodyEnd = -1
                    };

                    std::pair<EqnElement,int> procitem(EQN_LOOP, ea.loopList.size());
                    ea.loopList.push_back(loop);
                    procstack.push(procitem);
                }
                else
                {
                    // 01234567
                    // sum{dim}
                    MosraLogError(<< "ERROR in Equation '"
                                  << ea.name.toStdString() << "' at position " << p-token.size()
                                  << ": Unknwon loop type '" << token.toStdString() << "'!");
                    success = false;
                }
            }
            // LOOP DIMENSION END
            else // ch == QChar('}')
            {
                if (procstack.size() > 0 && procstack.last().first == EQN_LOOP)
                {
                    ea.loopList[procstack.last().second].dim = token;
                }
            }
            token.clear();
        }
        // FUNCTION BODIES, LOOP BODIES, OR ALGEBRAIC PRECEDENCE CONTROL
        else if (ch == QChar('(') || ch == QChar(')'))
        {
            if (ch == QChar('('))
            {
                // FUNCTION BODY
                if (mAMPLFunctions.keys().contains(token))
                {
                    int flvl = 0;
                    for (int s=0; s < procstack.size(); ++s)
                    {
                        if (procstack[s].first == EQN_FUNCTION)
                        {
                            ++flvl;
                        }
                    }

                    Func func = {
                        .name = token,
                        .level = flvl,
                        .bodyStart = p,
                        .bodyEnd = -1
                    };
                    std::pair<EqnElement,int> funitem(EQN_FUNCTION, ea.funcList.size());
                    procstack.push_back(funitem);
                    ea.funcList.push_back(func);
                }
                // LOOP BODY
                else if (   procstack.size() > 0
                         && procstack.last().first == EQN_LOOP
                         && ea.loopList[procstack.last().second].bodyStart == -1
                        )
                {
                    ea.loopList[procstack.last().second].bodyStart = p;
                }
                else
                {
                    procstack.push(std::pair<EqnElement,int>(EQN_LBRACE, -1));
                }
            }
            else // ch == QChar(')')
            {
                // FINALISE PARAMETER
                if (procstack.last().first == EQN_PARAMETER)
                {
                    Param& param = ea.paramList[procstack.last().second];
                    logpos = blogEndPos ? param.paramEnd : param.paramStart;
                    ea.elemMap.insert(
                                logpos,
                                std::pair<EqnElement, int>(
                                    EQN_PARAMETER,
                                    procstack.last().second
                                    )
                                );
                    procstack.pop();
                }

                // LOOP BODY
                if (procstack.size() > 0 && procstack.last().first == EQN_LBRACE)
                {
                    procstack.pop();
                }
                else if (procstack.size() > 0 && procstack.last().first == EQN_LOOP)
                {
                    if (ea.loopList[procstack.last().second].bodyEnd == -1)
                    {
                        ea.loopList[procstack.last().second].bodyEnd = p;

                        logpos = blogEndPos ? p : ea.loopList[procstack.last().second].bodyStart;
                        ea.elemMap.insert(
                                    logpos,
                                    std::pair<EqnElement, int>(
                                        EQN_LOOP,
                                        procstack.last().second
                                        )
                                    );
                    }
                    procstack.pop();
                }
                // FUNCTION BODY
                else if (procstack.size() > 0 && procstack.last().first == EQN_FUNCTION)
                {
                    if (ea.funcList[procstack.last().second].bodyEnd = -1)
                    {
                        ea.funcList[procstack.last().second].bodyEnd = p;


                        logpos = blogEndPos ? p : ea.funcList[procstack.last().second].bodyStart;
                        ea.elemMap.insert(
                                    logpos,
                                    std::pair<EqnElement, int>(
                                        EQN_FUNCTION,
                                        procstack.last().second
                                        )
                                    );
                    }
                    procstack.pop();
                }

                if (   ea.name.compare(token, Qt::CaseSensitive) != 0
                    && (   mmsEquations.keys().contains(token)
                        //|| mmslLinearConstraints.keys().contains(token)
                       )
                   )
                {
                    Equation eqn = {
                        .eqn      = token,
                        .eqnStart = p-token.size(),
                        .eqnEnd   = p-1
                    };

                    logpos = blogEndPos ? eqn.eqnEnd : eqn.eqnStart;
                    ea.elemMap.insert(
                                logpos,
                                std::pair<EqnElement, int>(
                                    EQN_EQUATION,
                                    ea.eqnList.size()
                                    )
                                );

                    ea.eqnList.push_back(eqn);
                    token.clear();
                }
            }
            token.clear();
        }
        // PARAMETER OR VARIABLE
        else if (ch == QChar('[') || ch == QChar(']'))
        {
            if (ch == QChar('['))
            {
                if (    mmslParameters.keys().contains(token)
                     || mmslVariables.keys().contains(token)
                   )
                {
                    Param param;
                    param.name = token;
                    param.paramStart = p-token.size();
                    std::pair<EqnElement,int> paritem(EQN_PARAMETER, ea.paramList.size());
                    procstack.push_back(paritem);
                    ea.paramList.push_back(param);
                }
                else if (    procstack.size() == 0
                         ||  procstack.last().first != EQN_PARAMETER
                        )
                {
                    MosraLogError(<< "ERROR in Equation '"
                                  << ea.name.toStdString() << "' at position "
                                  << p-token.size()
                                  << ": Unrecognised parameter or variable name: "
                                  << token.toStdString());
                    success = false;
                }
            }
            else // ch == QChar(']')
            {
                if (procstack.size() > 0 && procstack.last().first == EQN_PARAMETER)
                {
                    Param& param = ea.paramList[procstack.last().second];
                    param.dimensions << token;
                    param.paramEnd = p;

                    // note: we finalise parameters (i.e. update paramEnd parameter
                    // and remove the procstack entry once we hit an operator because
                    // we're never quite sure about the number dimensons specified for
                    // the parameter! This is different for variables as they need to
                    // have dimensions defined for them.

                    // DEPRECATED
                    //if (mmslParameters.keys().contains(param.name))
                    //{
                    //    if (mmslParameters[param.name].size() == param.dimensions.size())
                    //    {
                    //        procstack.pop();
                    //    }
                    //    else if (mmslParameters[param.name].size() == param.dimensions.size())
                    //    {
                    //        procstack.pop();
                    //    }
                    //}
                    //else

                    // FINALIZE VARIABLE (PARAMETER)
                    if (mmslVariables.keys().contains(param.name))
                    {
                        if (mmslVariables[param.name].size() == param.dimensions.size())
                        {
                            logpos = blogEndPos ? param.paramEnd : param.paramStart;
                            ea.elemMap.insert(
                                        logpos,
                                        std::pair<EqnElement, int>(
                                            EQN_PARAMETER,
                                            procstack.last().second
                                            )
                                        );

                            procstack.pop();
                        }
                    }
                }
            }
            token.clear();
        }
        // FUNCTION ARGUMENT
        else if (ch == QChar(','))
        {
            if (procstack.size() > 0 && procstack.last().first == EQN_FUNCTION)
            {
                ea.funcList[procstack.last().second].sep.push_back(p);
            }
            token.clear();
        }
        else
        {
            token += ch;
        }

        // -----------------------------------------
        //  Operators & Numbers
        // -----------------------------------------

        // FINALISE OPERATOR
        if (   !mOpCharList.contains(ch)
            && nostack.size() > 0
            && nostack.last().first == EQN_OPERATOR
            && (   mParseOperators.contains(noToken)
                || mAMPLOperators.contains(noToken)
               )
           )
        {
            ea.opList[nostack.last().second].op = noToken;
            ea.opList[nostack.last().second].opEnd = p-1;

            logpos = blogEndPos ? p-1 : ea.opList[nostack.last().second].opStart;
            ea.elemMap.insert(
                        logpos,
                        std::pair<EqnElement, int>(
                            EQN_OPERATOR,
                            nostack.last().second
                            )
                        );

            nostack.pop();
            noToken.clear();
        }

        // INIT NUMBER
        if (   token.size() == 1
            && mNumCharList.contains(ch)
           )
        {
            Number num = {
                .numStart = p
            };

            noToken.clear();
            noToken += ch;

            nostack.push(std::pair<EqnElement,int>(EQN_NUMBER, ea.numList.size()));
            ea.numList.push_back(num);
        }
        // FINALISE NUMBER
        else if (   token.isEmpty()
                 && nostack.size() > 0
                 && nostack.last().first == EQN_NUMBER
                )
        {
            ea.numList[nostack.last().second].numEnd = p-1;
            bool bConv;
            double val = noToken.toDouble(&bConv);
            if (bConv)
            {
                ea.numList[nostack.last().second].value = val;
            }
            else
            {
                MosraLogError(<< "Failed converting '" << noToken.toStdString()
                              << "' to double!");
                success = false;
            }

            logpos = blogEndPos ? p-1 : ea.numList[nostack.last().second].numStart;
            ea.elemMap.insert(
                        logpos,
                        std::pair<EqnElement, int>(
                            EQN_NUMBER,
                            nostack.last().second
                            )
                        );

            nostack.pop();
            noToken.clear();
        }
        else if (   mNumCharList.contains(ch)
                 && nostack.size() > 0
                 && nostack.last().first == EQN_NUMBER
                )
        {
            noToken += ch;
        }
    }

    return success;
}

bool NMMosra::polishingEqn(EquationAdmin &ea, EquationAdmin &pa,
                           QStringList &polishPrefix)
{
    NMDebugCtx(ctxNMMosra, << "...");
    /*
    highest loop level is 0; the level increases by 1
    each time a loop is nested inside another loop
    e.g.
          sum{SDU}            // level 0
          (
              sum{Farm}       // level 1
              (

              )

              sum{...}        // level 1
              (
                 sum{Rec}     // level 2
                 (

                 )
              )
          )
    */

    // =====================================================
    //          REVERSING LOOPS (and all that's in it)
    // =====================================================

    // bodyStart, <Loop, ReversedEquation>
    QString eqn;
    QString revEqn;

    if (mmsEquations.constFind(ea.name) != mmsEquations.cend())
    {
        eqn = mmsEquations.value(ea.name);
    }
    //else if (mmslLinearConstraints.constFind(ea.name) != mmslLinearConstraints.cend())
    //{
    //    eqn = mmslLinearConstraints.value(ea.name);
    //}

    if (eqn.isEmpty())
    {
        MosraLogError(<< "polishingEqn() - ERROR: "
                      << "got an empty equation!" << endl);
        return false;
    }

    revEqn = reverseEqn(eqn, ea, 0, eqn.size()-1);
    MosraLogDebug(<< revEqn.toStdString());
    MosraLogDebug(<< " ");

    // =====================================================
    //   Parse reversed Equations
    // =====================================================
    EquationAdmin eaRev;
    eaRev.name = ea.name;

    if (!parseEquation(revEqn, eaRev, false))
    {
        MosraLogError(<< "Mosra - ERROR: parsing reversed equation failed!");
        return false;
    }

    // =====================================================
    //   SHUNTY YARD ALGORITHM : infix -> polish prefix
    // =====================================================
    if (!shuntyYard(revEqn, polishPrefix, eaRev, pa))
    {
        MosraLogError(<< "Mosra - ERROR: Shunty Yard algorithm!");
        return false;
    }

    NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

bool NMMosra::shuntyYard(const QString &revInEqn,
                         QStringList &polishPrefixList,
                         EquationAdmin &ea,
                         EquationAdmin &pa)
{
    QStringList revEqnList;

    EquationAdmin revEa;
    revEa.name = ea.name;
    revEqnList = shuntyCoreRev(revInEqn, ea, revEa, 0, 0);

    // DEBUG DEBUG DEBUG
    MosraLogDebug(<< "revPrefix(" << revEa.name.toStdString() << ")\n "
                 << revEqnList.join("\n").toStdString() << endl << endl);

    PrintElemMap(revEa);

    polishPrefixList = shuntyCorePref(revEqnList, revEa, pa);

    // DEBUG DEBUG DEBUG
    MosraLogDebug(<< "polishPrefix(" << pa.name.toStdString() << ")\n "
                 << polishPrefixList.join("\n").toStdString() << endl << endl);

#ifdef LUMASS_DEBUG
    PrintElemMap(pa);
#endif
    return true;
}

void NMMosra::PrintElemMap(const EquationAdmin &admin)
{
    // DEBUG =================================================
    MosraLogDebug(<< admin.name.toStdString() << " elemMap positions: ..." << endl);
    QStringList output;
    auto it = admin.elemMap.cbegin();
    while (it != admin.elemMap.cend())
    {
        QString es;
        QString ename;
        const EqnElement ee = it.value().first;
        const int vpos = it.value().second;
        switch (ee)
        {
        case EQN_LOOP: es = "Loop"; ename = QString("%1{%2}").arg(admin.loopList[vpos].name).arg(admin.loopList[vpos].dim); break;
        case EQN_FUNCTION: es = "Func"; ename = admin.funcList[vpos].name; break;
        case EQN_PARAMETER: es = "Param"; ename = QString("%1[%2]").arg(admin.paramList[vpos].name).arg(admin.paramList[vpos].dimensions.join(",")); break;
        case EQN_EQUATION: es = "Eqn"; ename = admin.eqnList[vpos].eqn; break;
        default: ;
        }

        QString out = QString("[%1]: %2 - %3")
                      .arg(it.key())
                      .arg(es)
                      .arg(ename);
        output << out;
        ++it;
    }
    MosraLogDebug(<< endl << output.join("\n").toStdString() << endl << endl);
    // DEBUG =================================================
}

QStringList NMMosra::shuntyCorePref(const QStringList &revList,
                                    EquationAdmin &inEa,
                                    EquationAdmin &outEa)
{
    QStringList prefList;
    for (int p=revList.size()-1; p >= 0; --p)
    {
        const QString token = revList[p];

        if (inEa.elemMap.constFind(p) != inEa.elemMap.constEnd())
        {
            const EqnElement inElem = inEa.elemMap[p].first;
            const int inPos = inEa.elemMap[p].second;

            switch (inElem)
            {
            case EQN_EQUATION:
                {
                    Equation outEqn = inEa.eqnList[inPos];
                    outEqn.eqnStart = prefList.size();
                    outEqn.eqnEnd = prefList.size();
                    outEa.elemMap.insert(
                                prefList.size(),
                                std::pair<EqnElement, int>(
                                    EQN_EQUATION,
                                    outEa.eqnList.size()
                                    )
                                );
                    outEa.eqnList.push_back(outEqn);
                }
                break;

            case EQN_PARAMETER:
                {
                    Param outPar = inEa.paramList[inPos];
                    outPar.paramStart = prefList.size();
                    outPar.paramEnd = prefList.size();
                    outEa.elemMap.insert(
                                prefList.size(),
                                std::pair<EqnElement, int>(
                                    EQN_PARAMETER,
                                    outEa.paramList.size()
                                    )
                                );
                    outEa.paramList.push_back(outPar);
                }
                break;

            case EQN_LOOP:
                {
                    Loop outLoop = inEa.loopList[inPos];
                    // note these positions are from a backwards
                    // passed list, i.e. bodyStart > bodyEnd
                    const int loopLen = outLoop.bodyStart - outLoop.bodyEnd;
                    outLoop.bodyStart = prefList.size();
                    outLoop.bodyEnd = outLoop.bodyStart + loopLen;
                    outEa.elemMap.insert(
                                prefList.size(),
                                std::pair<EqnElement, int>(
                                    EQN_LOOP,
                                    outEa.loopList.size()
                                    )
                                );
                    outEa.loopList.push_back(outLoop);
                }
                break;

            default:
                ;
            }
        }
        prefList << token;
    }

    return prefList;
}

QStringList NMMosra::shuntyCoreRev(const QString &revEqn,
               EquationAdmin &ea, EquationAdmin &pa, int offset, int outlistoffset)
{
    // create 'reversed' infix string
    // variables
    //  vi  with 0 <= i < n_var e.g. v0, v1, v2, v...
    // constants
    //  n<float>  e.g. 'n1'  or 'n-3.2e-7'

    /* SHUNTY YARD ALGORITHM
     for each char in expression:
        if operand -> output
        if operator (a)
            if stack has operator (b)
                while (b)'s precedence on stack is greater than a's
                    -> pop operator (b) from stack onto output

                -> push operator (a) on `stack`
        if '(' -> push it on the stack
        if ')'
            pop operators from stack onto output until hitting '('
            pop '(' (into nothingness)
    */

    QStringList revoutList;
    QStack<QString> stack;

    // < <loop end pos in rev Eqn>, <loop pos in pa.loopList> >
    for (int p=0; p < revEqn.size(); ++p)
    {
        QChar ch = revEqn.at(p);
        bool bCharProcessed = false;


        if (ea.elemMap.constFind(p+offset) != ea.elemMap.cend())
        {
            const std::pair<EqnElement, int> elemPair = ea.elemMap[p+offset];
            const EqnElement elem = elemPair.first;
            const int epos = elemPair.second;

            switch(elem)
            {
            case EQN_LOOP:
                if (ea.loopList[epos].bodyStart - offset == p)
                {
                    Loop& myLoop = ea.loopList[epos];

                    int bstart = myLoop.bodyStart+1 - offset;
                    int bend   = myLoop.bodyEnd-1 - offset;
                    // 345678
                    // (1234)
                    int blen   = bend - bstart + 1;

                    QString revBody = revEqn.mid(bstart, blen);
                    QStringList revBodyList = shuntyCoreRev(revBody, ea, pa, myLoop.bodyStart+1, revoutList.size());

                    int rbend = revoutList.size();
                    revoutList << revBodyList;
                    int rbstart = revoutList.size();
                    revoutList << QString("%1{%2}").arg(myLoop.name).arg(myLoop.dim);

                    pa.elemMap.insert(outlistoffset + revoutList.size() - 1,
                                      std::pair<EqnElement, int>(
                                          EQN_LOOP,
                                          pa.loopList.size()
                                          )
                                );

                    Loop outLoop = {
                        .name = myLoop.name,
                        .dim = myLoop.dim,
                        .level = myLoop.level,
                        .bodyStart = rbstart,
                        .bodyEnd = rbend
                    };
                    pa.loopList.push_back(outLoop);

                    p = ea.loopList[epos].bodyEnd - offset;
                    bCharProcessed = true;
                }
                break;


            case EQN_FUNCTION:
                // 345678901234567
                // myfun(par1, p2)
                if (ea.funcList[epos].bodyStart - offset == p)
                {
                    Func& myFun = ea.funcList[epos];

                    QString revArg;
                    QStringList revArgList;

                    // ------------------------------------------------
                    //parse arguments from back to front

                    int aend = myFun.bodyEnd - offset - 1;
                    int astart;
                    for (int s=myFun.sep.size()-1; s >= -1; --s)
                    {
                        if (s == -1)
                        {
                            astart = myFun.bodyStart - offset + 1;
                        }
                        else
                        {
                            astart = myFun.sep[s] - offset + 1;
                        }

                        revArg = revEqn.mid(astart, aend-astart+1);

                        revArgList = shuntyCoreRev(revArg, ea, pa,
                                                   astart + offset,
                                                   outlistoffset + revoutList.size());
                        aend = astart - 2;
                        revoutList << revArgList;
                    }
                    revoutList << QString("o%1").arg(mAMPLFunctions[myFun.name]);

                    p = myFun.bodyEnd - offset;
                    bCharProcessed = true;
                }
                break;

            case EQN_PARAMETER:
            // Param, Var
                if (ea.paramList[epos].paramStart - offset == p)
                {
                    Param param = ea.paramList.at(epos);
                    int param_end = param.paramEnd;
                    param.paramEnd = -1;
                    param.paramStart = -1;

                    pa.elemMap.insert(
                                outlistoffset + revoutList.size(),
                                std::pair<EqnElement, int>(
                                    EQN_PARAMETER,
                                    pa.paramList.size()
                                    )
                                );
                    pa.paramList.push_back(param);

                    QString pout = param.name;
                    for (int d=0; d < param.dimensions.size(); ++d)
                    {
                        pout += QString("[%1]").arg(param.dimensions[d]);
                    }
                    revoutList << pout;

                    // 34567890123
                    // pa[d1][d2]
                    p = param_end - offset;
                    bCharProcessed = true;
                }
                break;

            case EQN_EQUATION:
                if (ea.eqnList[epos].eqnStart - offset == p)
                {
                    Equation eqn = ea.eqnList.at(epos);
                    int eqn_end = eqn.eqnEnd;
                    eqn.eqnEnd = -1;
                    eqn.eqnStart = -1;

                    pa.elemMap.insert(
                                outlistoffset + revoutList.size(),
                                std::pair<EqnElement, int>(
                                    EQN_EQUATION,
                                    pa.eqnList.size()
                                    )
                                );
                    pa.eqnList.push_back(eqn);
                    revoutList << eqn.eqn;

                    p = eqn_end - offset;
                    bCharProcessed = true;
                }
                break;

            case EQN_OPERATOR:
                if (ea.opList[epos].opStart - offset == p)
                {
                    const Operator& op = ea.opList.at(epos);

                    if (stack.size() > 0)
                    {
                        if (mmOpLevel.keys().contains(op.op))
                        {
                            bool bPopedOp = false;
                            while (stack.size() > 0 &&  mmOpLevel[stack.last()] > mmOpLevel[op.op])
                            {
                                revoutList << QString("o%1").arg(mParseOperators[stack.pop()]);
                                bPopedOp = true;
                            }
                        }
                        else
                        {
                            MosraLogError(<< "Mosra::shuntyCore() - ERROR: unknown operator '"
                                          << stack.last().toStdString() << "' at pos " << p
                                          << " of '" << revEqn.toStdString() << "'!");
                            return revoutList;
                        }

                        // push operator onto stack
                        stack.push(op.op);
                    }
                    else
                    {
                        stack.push(op.op);
                    }
                    p = op.opEnd - offset;
                    bCharProcessed = true;
                }
                break;

            case EQN_NUMBER:
                if (ea.numList[epos].numStart - offset == p)
                {
                    //revout += QString("n%1\n").arg(ea.numList[n].value);
                    revoutList << QString("n%1").arg(ea.numList[epos].value);

                    p = ea.numList[epos].numEnd - offset;

                    bCharProcessed = true;
                }

            default:
                ;
            }

        }

        // algebraic '(' or ')'
        if (!bCharProcessed)
        {
            if  (ch == QChar('('))
            {
                stack.push(QString(ch));
                bCharProcessed = true;
            }
            else if (ch == QChar(')'))
            {
                while (stack.size() > 0 && stack.last().compare("(") != 0)
                {
                    QString sop = stack.pop();
                    if (mParseOperators.keys().contains(sop))
                    {
                        revoutList << QString("o%1").arg(mParseOperators[sop]);
                    }
                }

                if (stack.size() > 0 && stack.last().compare("(") == 0)
                {
                    stack.pop();
                }
                bCharProcessed = true;
            }
        }
    }

    // pop the rest of the operators
    while (stack.size() > 0)
    {
        QString op = stack.pop();
        if (mParseOperators.keys().contains(op))
        {
            revoutList << QString("o%1").arg(mParseOperators[op]);
        }
    }

    return revoutList;

}

QString NMMosra::reverseEqn(const QString &eqn, EquationAdmin &ea,
                            int offset, int end)
{
    QString revEqn;
    int orgpos = end;

    // 123456789
    //   1234

    for (int p=eqn.size()-1; p >= 0; --p, --orgpos)
    {
        QChar ch = eqn.at(p);
        bool bCharProcessed = false;

        if (ea.elemMap.constFind(orgpos) != ea.elemMap.cend())
        {
            const std::pair<EqnElement, int> elemPair = ea.elemMap[orgpos];
            const EqnElement elem = elemPair.first;
            const int epos = elemPair.second;

            switch(elem)
            {
            case EQN_LOOP:
                if (ea.loopList[epos].bodyEnd == orgpos)
                {
                    Loop& myLoop = ea.loopList[epos];

                    const int llen = orgpos - (myLoop.bodyStart + 1);
                    const int localStart = p-llen;
                    const int globalStart = localStart + offset;
                    const int globalEnd   = orgpos-1;

                    QString preamble = QString("%1{%2}(")
                            .arg(ea.loopList[epos].name)
                            .arg(ea.loopList[epos].dim);
                    revEqn += preamble;

                    QString loopBody = eqn.mid(localStart, llen);
                    QString revBody = reverseEqn(loopBody, ea, globalStart, globalEnd);

                    revEqn += revBody;
                    revEqn += QChar(')');

                    p -= (myLoop.bodyEnd - myLoop.bodyStart) + preamble.size()-1;
                    orgpos -= (myLoop.bodyEnd - myLoop.bodyStart) + preamble.size()-1;

                    bCharProcessed = true;
                    break;
                }
                break;

            case EQN_FUNCTION:
                if (ea.funcList[epos].bodyEnd == orgpos)
                {
                    Func& myFun = ea.funcList[epos];

                    // local fun coordinates
                    // 0123456789012345
                    //    01234567890
                    //       012345
                    // pow( 10, 3)
                    //
                    const int flen = orgpos - (myFun.bodyStart + 1);
                    const int localStart = p-flen;

                    // add function name
                    revEqn += ea.funcList[epos].name + '(';

                    // add arguments
                    int argStart = localStart;
                    QString argN;
                    QString revArg;
                    for (int sp=0; sp < myFun.sep.size(); ++sp)
                    {
                        const int seppos    = myFun.sep[sp] - offset;       //local
                        const int argOffset = argStart+offset;              //global
                        const int argEnd    = (seppos-1) + offset;          //global
                        const int argLen    = seppos-argStart;              //local

                        argN = eqn.mid(argStart, argLen);
                        revArg = reverseEqn(argN, ea, argOffset, argEnd);
                        revEqn += revArg;

                        if (sp < myFun.sep.size()-1)
                        {
                            revEqn += QString(", ");
                        }

                        argStart = seppos + 1;
                    }

                    // add comma, if we've got more than one argument
                    if (myFun.sep.size() > 0)
                    {
                        revEqn += QString(", ");
                    }

                    // add last argument

                    argN = eqn.mid(argStart, p-argStart);
                    revArg = reverseEqn(argN, ea, argStart+offset, orgpos-1);
                    revEqn += revArg;
                    revEqn += QChar(')');

                    // 123456
                    // tan(4)
                    p -= (myFun.bodyEnd - myFun.bodyStart) + myFun.name.size();
                    orgpos -= (myFun.bodyEnd - myFun.bodyStart) + myFun.name.size();
                    bCharProcessed = true;
                    break;
                }
                break;

            case EQN_PARAMETER:
                if (ea.paramList[epos].paramEnd == orgpos)
                {
                    Param& myPar = ea.paramList[epos];

                    revEqn += myPar.name;
                    QString revDim;
                    // 1234567890123456
                    // name[dim1][dim2]
                    int dStart = myPar.paramStart +
                            myPar.name.size() + 1;
                    for (int d=0; d < myPar.dimensions.size(); ++d)
                    {
                        revEqn += QChar('[');
                        bool allLetters = true;
                        for (int z=0; z < myPar.dimensions[d].size(); ++z)
                        {
                            const QChar tc = myPar.dimensions[d].at(z);
                            if (!tc.isLetter())
                            {
                                allLetters = false;
                            }
                        }

                        if (allLetters)
                        {
                            revEqn += myPar.dimensions[d];
                        }
                        else
                        {
                            //revDim = reverseEqn(myPar.dimensions[d], ea, revLoopMap,
                            //         dStart, dStart+myPar.dimensions[d].size()-1);
                            revDim = reverseEqn(myPar.dimensions[d], ea,
                                                dStart, dStart+myPar.dimensions[d].size()-1);
                            revEqn += revDim;
                        }

                        revEqn += QChar(']');
                        dStart = dStart+myPar.dimensions[d].size()+2;
                    }

                    p -= (myPar.paramEnd - myPar.paramStart);
                    orgpos -= (myPar.paramEnd - myPar.paramStart);
                    bCharProcessed = true;
                }
                break;

            case EQN_OPERATOR:
                if (ea.opList[epos].opEnd == orgpos)
                {
                    revEqn += ea.opList[epos].op;

                    // 1 12 123
                    // + && and
                    p -= (ea.opList[epos].op.size()) - 1;
                    orgpos -= (ea.opList[epos].op.size()) - 1;
                    bCharProcessed = true;
                }
                break;

            case EQN_NUMBER:
                if (ea.numList[epos].numEnd == orgpos)
                {
                    Number& num = ea.numList[epos];
                    // 1234567
                    // 1.34e-7


                    revEqn += eqn.mid(num.numStart-offset, (num.numEnd - num.numStart) + 1);
                    p -= (num.numEnd - num.numStart);
                    orgpos -= (num.numEnd - num.numStart);
                    bCharProcessed = true;
                }
                break;

            case EQN_EQUATION:
                if (ea.eqnList[epos].eqnEnd == orgpos)
                {
                    Equation& esub = ea.eqnList[epos];
                    QString esubStr = eqn.mid(esub.eqnStart-offset, esub.eqn.size());
                    revEqn += esubStr;

                    p -= (esub.eqnEnd - esub.eqnStart);
                    orgpos -= (esub.eqnEnd - esub.eqnStart);
                    bCharProcessed = true;
                }
                break;

            default:
                ;
            }
        }
        // algebraic ')', function ')' or param ']
        else if (ch == QChar('(') || ch == QChar(')') && !bCharProcessed)
        {
            if (mReverseRight.contains(ch))
            {
                revEqn += mReverseRight[ch];
            }
            else
            {
                revEqn += mReverseLeft[ch];
            }
            bCharProcessed = true;
        }

        // ... otherwise
        if (!bCharProcessed)
        {
            revEqn += ch;
        }
     }

    return revEqn;
}

void NMMosra::identifyHighDimLoopEquations(
        const QString &rootEqnName,
        const QString &subEqnName
        )
{
    auto eait = mmEquationAdmins.constFind(subEqnName);
    if (eait == mmEquationAdmins.cend())
    {
        return;
    }

    const EquationAdmin& ea = eait.value();

    if (    !mmslObjectives.keys().contains(rootEqnName)
         && !mmslLinearConstraints.keys().contains(rootEqnName)
         && !mmslNonLinearConstraints.keys().contains(rootEqnName)
         && !mmslLogicConstraints.keys().contains(rootEqnName)
       )
    {
        return;
    }

    // check all referenced 'sub' equations, if not done already
    for (int eqn=0; eqn < ea.eqnList.size(); ++eqn)
    {
        const QString& ename = ea.eqnList[eqn].eqn;
        if (mmslHighDimLoopEquations.constFind(ename) == mmslHighDimLoopEquations.cend())
        {
            identifyHighDimLoopEquations(rootEqnName, ename);
        }
    }


    // need to traverse eqn's loops twice:
    // 1. determine whether this is a high dim loop equation at all
    // 2. add dimensions of all nested loops to it (.first dim list)
    // 3. keep a record of ALL its dimensions

    // ... do we have a high dim loop equation?
    for (int loop=0; loop < ea.loopList.size(); ++loop)
    {
        int llvl = ea.loopList[loop].level;

        if (    llvl == 0
             && ea.loopList[loop].dim.compare(msOPTIONS) != 0
             && ea.loopList[loop].dim.compare(QStringLiteral("SDU")) != 0
           )
        {
            if (mmslHighDimLoopEquations.constFind(rootEqnName) == mmslHighDimLoopEquations.cend())
            {
                std::pair<QStringList, QStringList> dimtracker;
                dimtracker.first << ea.loopList[loop].dim;
                mmslHighDimLoopEquations.insert(rootEqnName, dimtracker);
            }
            else if (!mmslHighDimLoopEquations[rootEqnName].first.contains(ea.loopList[loop].dim))
            {
                mmslHighDimLoopEquations[rootEqnName].first << ea.loopList[loop].dim;
            }

            // add dims for nested loops (except for OPTIONS)
            for (int nl=loop+1; nl < ea.loopList.size(); ++nl)
            {
                if (    ea.loopList[nl].bodyStart > ea.loopList[loop].bodyStart
                     && ea.loopList[nl].bodyEnd   < ea.loopList[loop].bodyEnd
                     && ea.loopList[nl].level > llvl
                     && ea.loopList[nl].dim.compare(msOPTIONS) != 0
                     && ea.loopList[nl].dim.compare(msSDU) != 0
                   )
                {
                    mmslHighDimLoopEquations[rootEqnName].first << ea.loopList[nl].dim;
                    ++llvl;
                }
            }
        }
    }

    // ... now note all dimensions if this equation qualifies
    if (mmslHighDimLoopEquations.constFind(rootEqnName) != mmslHighDimLoopEquations.cend())
    {
        for (int loop=0; loop < ea.loopList.size(); ++loop)
        {
            if (!mmslHighDimLoopEquations[rootEqnName].second.contains(ea.loopList[loop].dim))
            {
                mmslHighDimLoopEquations[rootEqnName].second << ea.loopList[loop].dim;
            }
        }

        // ... now add parameter and variable dimensions
        for (int par=0; par < ea.paramList.size(); ++par)
        {
            foreach (const QString& pdim, ea.paramList[par].dimensions)
            {
                if (!mmslHighDimLoopEquations[rootEqnName].second.contains(pdim))
                {
                    mmslHighDimLoopEquations[rootEqnName].second << pdim;
                }
            }
        }
    }

    return;
}

bool NMMosra::processLoopDimensions(void)
{

    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "DataSet is NULL!" << endl);
        return false;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "DataSet is NULL!" << endl);
        return false;
    }

    QSet<QString> dimensionSet;
    mmslHighDimLoopEquations.clear();

    // split variables and fill first part of orderedVars
    // with decision variable names
    auto eait = mmEquationAdmins.cbegin();
    for(; eait != mmEquationAdmins.cend(); ++eait)
    {
        const QList<Loop>& loopList = eait.value().loopList;
        for (int el=0; el < loopList.size(); ++el)
        {
            dimensionSet << loopList.at(el).dim;
        }
    }

    sqltab->BeginTransaction();
    QStringList dimSetList = dimensionSet.values();
    for (int di=0; di < dimSetList.size(); ++di)
    {
        const QString dimname = dimSetList.at(di);
        if (dimname.compare(msOPTIONS) == 0)
        {
            mmDimLengthMap.insert(dimname, miNumOptions);
            continue;
        }

        std::vector<otb::AttributeTable::TableColumnType> colTypes = {
                        otb::AttributeTable::ATTYPE_INT
        };
        std::vector<std::vector<otb::AttributeTable::ColumnValue> > colValues;


        // fetch actual column values
        std::string dimColName;
        if (mmslParameters.constFind(dimname) != mmslParameters.cend())
        {
            if (mmslParameters[dimname].size() > 0)
            {
                dimColName = mmslParameters[dimname].at(0).toStdString();
            }
            else
            {
                sqltab->EndTransaction();
                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Parameter '" << dimname.toStdString() << "' has no "
                              << "database columns assigned to it!"
                              << std::endl);
                return false;
            }
        }

        std::stringstream query;
        query << "with C as (select distinct \"" << dimColName << "\" "
              << "from \"" << sqltab->GetTableName() << "\" "
              << "where \"" << msOptFeatures.toStdString() << "\" == 1) "
                           << "select count(*) from C;";

        if (!sqltab->TableDataFetch(
                    colValues,
                    colTypes,
                    query.str()))
        {
            sqltab->EndTransaction();
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << sqltab->getLastLogMsg() << std::endl);
            return false;
        }

        long long dimLen = -1;
        if (colValues.size() > 0)
        {
            if (colValues.at(0).size() > 0)
            {
                dimLen = colValues.at(0).at(0).ival;
            }
        }

        mmDimLengthMap.insert(dimname, static_cast<long>(dimLen));
    }
    sqltab->EndTransaction();


    return true;
}

bool NMMosra::processVariables(QString& b_seg, QString &x_seg, long* n_vars, NMOpenGA *oga)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // for good performance, this relies on the 'optFeat' column to be indexed!

    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< "::processVariables() - ERROR: DataSet is NULL!" << endl);
        return false;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogError(<< "NMMosra::processVariables() - ERROR: DataSet is NULL!" << endl);
        return false;
    }

    // DEEPRECATED:
    // ----------------------------------------------------------
    // split vars in to decision vars and process (defined) vars
    // ----------------------------------------------------------
    // we need to process all decision variables first as the NL file
    // requires variable offsets of decision variables to be
    //      0 <= dv_offset < n_var   with n_var: number of decision variables
    //
    // Defined or process variables have variable offset values
    //      pv_offset >= n_var

    QStringList orderedVars;

    // while analysing variables, generate set of dimensions
    // note: assumption is that any dimension appearing in any of the
    // equations (<EQUATION>) needs to be assigned to at least one
    // of the variables
    QSet<QString> dimensionSet;

    // DEPRECATED:
    // split variables into decision and process vars and fill
    // first part of orderedVars with decision variables

    // create list of binary and integer variables as they will
    // appear in that order at the end of the NL 'b' segement,
    // i.e. variable boundaries definition
    //QStringList msBinaryVars;
    //QStringList msIntVars;

    auto allit = mmslVariables.cbegin();
    while (allit != mmslVariables.cend())
    {
        // if process variables
        if (mmsEquations.constFind(allit.key()) != mmsEquations.cend())
        {
            mslProcessVars << allit.key();
        }
        // decision variables
        else
        {
            const QString type = mmslVarBoundsMap[allit.key()].at(0).toStdString().c_str();
            const QString vn = allit.key();

            if (type.compare(QStringLiteral("c"), Qt::CaseInsensitive) == 0)
            {
                mslDecisionVars << vn;
            }
            else if (type.compare(QStringLiteral("i"), Qt::CaseInsensitive) == 0)
            {
                msIntVars << vn;
            }
            else if (type.compare(QStringLiteral("b"), Qt::CaseInsensitive) == 0)
            {
                msBinaryVars << vn;
            }
            else
            {
                MosraLogError(<< "Couldn't find a valid variable type "
                              << "for '" << vn.toStdString() << "'!"
                              << std::endl);
                return false;
            }
        }

        // fill set of dimension
        foreach(const QString& vdim, allit.value())
        {
            dimensionSet << vdim;
        }

        ++allit;
    }

    // now add the binary and then integer variables
    // to the list of continuous decision variables
    mslDecisionVars << msBinaryVars << msIntVars;

    // --------------------------------------------
    // keep track of dimensions


    // construct orderedVars starting with DecisionVars to ensure that
    // variable offset i meets
    //                        0 <= i < mslDecisionVars.size()   for decision variables and
    //   mslDecisionVars.size() <= i < mslProcessVars.size()    for process variables
    //
    orderedVars << mslDecisionVars;
    orderedVars << mslProcessVars;

    // double check we've got something to work with
    if (orderedVars.size() == 0)
    {
        MosraLogInfo(<< "NMMosra::processVariables() - no variables !?" << endl);
        return false;
    }


    // ---------------------------------------------------------------------------------
    // process variables
    // ---------------------------------------------------------------------------------
    MosraLogInfo(<< "Processing Optimisation Variables ..." << endl);
    NMDebugAI(<< "Processing Optimisation Variables ..." << endl);

    // variable offset counter: starts at 0;
    // tracks global offset for *.nl file in IpOpt mode
    long varoff = 0;
    long xcount = 0;


    const QChar sp = QChar(' ');
    const QChar eol = QChar('\n');
    const QChar zero = QChar('0');

    QTextStream xstr(&x_seg);
    QTextStream bstr(&b_seg);
    bstr << QStringLiteral("b\n");

    // add variable dimension parameters to the list of values to
    // be retrieved from the database
    for (int v=0; v < orderedVars.size(); ++v)
    {
        auto vit = mmslVariables.constFind(orderedVars[v]);

        NMDebugAI(<< vit.key().toStdString() << "[" << vit.value().join(",").toStdString() << "] ..." << endl);

        std::vector<std::string> getnames;
        std::vector<otb::AttributeTable::ColumnValue> getvalues;
        bool bOptions = false;

        foreach(const QString& dim, vit.value())
        {
            if (mmslParameters.constFind(dim) != mmslParameters.cend())
            {
                const std::string dimpar = mmslParameters.value(dim).at(0).toStdString();
                const int colid = sqltab->ColumnExists(dimpar);

                if (colid == -1)
                {
                    MosraLogError(<< "NMMosra::processVariables() - ERROR: Could not "
                                  << "find dimension parameter '" << dimpar
                                  << "' in the database!" << endl);
                    return false;
                }

                otb::AttributeTable::TableColumnType type = sqltab->GetColumnType(colid);
                if (type == otb::AttributeTable::ATTYPE_DOUBLE)
                {
                    MosraLogWarn( << "NMMosra::processVariables() - WARNING: Dimension parameter '"
                                   << dimpar << "' is of type REAL and will be converted "
                                   << "to INTEGER. This may result in data loss and hard to "
                                   << " detect ERRORS in the results! Please ensure all dimension "
                                   << " parameters are of type INTEGER!"
                                   << endl);
                }
                else if (type == otb::AttributeTable::ATTYPE_STRING)
                {
                    MosraLogError( << "NMMosra::processVariables() - ERROR: Unsupported data type "
                                   << " STRING of dimension parameter '"
                                   << dimpar << "' detected! Please ensure all dimension "
                                   << " parameters are of type INTEGER!"
                                   << endl);
                    return false;

                }

                otb::AttributeTable::ColumnValue colval;
                colval.type = type;

                getnames.push_back(mmslParameters.value(dim).at(0).toStdString());
                getvalues.push_back(colval);
            }
            else if (dim.compare(msOPTIONS) == 0)
            {
                bOptions = true;
            }
            else
            {
                MosraLogError(<< "::processVariables() - ERROR: Dimension field '"
                              << dim.toStdString() << "' needs to be defined in the "
                              << "PARMETERS section of the *.los file!" << endl);
                return false;
            }
        }

        const int numDimValues = getnames.size();

        // ------------------------------------------------------------
        //      VARIABLE BOUNDS

        // we add the bounds information or not
        // outer vector: one inner vec each for lower & upper bnds
        // inner vector:
        //  [0]: if 1 then [1] == bound value
        //       if 0 then [1][2...n-1] == offsetValues into getnames & getvalues
        std::vector<std::vector<double> > bndsVec;

        // keep track of scaling requirements of variable bounds
        std::vector<double> vBndScalingFactors;

        auto vbm = mmslVarBoundsMap.constFind(orderedVars[v]);
        if (vbm != mmslVarBoundsMap.cend())
        {
            // we'll skip the first entry in the bounds list
            // as it is actually the variable type, i.e.
            // continuous, integer, or binary
            for (int b=1; b < vbm.value().size(); ++b)
            {
                const QString& vbnd = vbm.value().at(b);

                std::vector<double> innerVec;
                bool bConv;
                double bval = vbnd.toDouble(&bConv);

                // bnd info can be converted to number
                if (bConv)
                {
                    innerVec.push_back(1);
                    innerVec.push_back(bval);
                    bndsVec.push_back(innerVec);
                }
                // bnd info refers to parameter
                else
                {
                    auto vbpar = mmslParameters.constFind(vbnd);
                    if (vbpar != mmslParameters.cend())
                    {
                        // check whether the boundary parameter
                        // is scaled ...
                        double scalingFactor = 1.0;
                        auto spit = mmParameterScaling.constFind(vbnd);
                        if (spit != mmParameterScaling.cend())
                        {
                            scalingFactor = spit.value().first;
                        }

                        innerVec.push_back(0);
                        for (int pp=0; pp < vbpar.value().size(); ++pp)
                        {
                            innerVec.push_back(getnames.size());
                            getnames.push_back(vbpar.value().at(pp).toStdString());
                            otb::AttributeTable::ColumnValue bcv;
                            getvalues.push_back(bcv);

                            // store a scaling factor for each parameter
                            vBndScalingFactors.push_back(scalingFactor);
                        }
                        bndsVec.push_back(innerVec);
                    }
                    else
                    {
                        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "()- ERROR: "
                                      << "Variable's bounds parameter '" << vbnd.toStdString()
                                      << "' needs to be defined in the "
                                      << "PARAMETERS section of the *.los file!" << endl);
                        return false;
                    }
                }
            }
        }

        // note whether we're processing 'lu' (OPTIONS) variables
        //  and if so, add the LAND_USE_FIELD to the db values we want to fetch
        bool bLU = false;
        if (vit.key().compare(QStringLiteral("lu")) == 0)
        {
            bLU = true;

            getnames.push_back(msLandUseField.toStdString());
            otb::AttributeTable::ColumnValue lucv;
            getvalues.push_back(lucv);
        }

        // ---------------------------------------------------------------
        //          VARIABLE ADMIN STRUCTURES

        VarAdmin vad;
        vad.name = vit.key();
        vad.dimensions = vit.value();
        QMap<QVector<long long>, long long>& doffMap = vad.dimOffsetMap;
        //QMap<long long, std::vector<long long> >& dimMap = vad.offsetDimMap;

        QString whereClause;
        if (!msOptFeatures.isEmpty())
        {
            whereClause = QString("where %1 == 1").arg(msOptFeatures);
        }

        // prepare distinct value retrieval
        if (!sqltab->PrepareBulkGet(getnames, whereClause.toStdString(), true))
        {
            MosraLogError(<< "NMMosra::processVariables() - ERROR: Failed fetching data from "
                          << "the database: " << sqltab->getLastLogMsg() << ".");
            sqltab->EndTransaction();
            return false;
        }
        sqltab->BeginTransaction();
        bool brow = sqltab->DoBulkGet(getvalues);
        if (!brow)
        {
            MosraLogError(<< "NMMosra::processVariables() - ERROR: Failed fetching data from "
                          << "the database: " << sqltab->getLastLogMsg() << ". "
                          << "Double check your OPT_FEATURES setting and table values!");
            sqltab->EndTransaction();
            return false;
        }

        // --------------------------------------------------------------
        //          DB VALUE PROCESSING
        // iterate over distinct dim value tuples and store

        // tracks local variable offset (for OpenGA mode)
        long dbiter = 0;
        do
        {
            // ..................................................
            //          get dimension parameter values
            QVector<long long> vids;
            for (int d=0; d < numDimValues; ++d)
            {
                if (getvalues[d].type == otb::AttributeTable::ATTYPE_INT)
                {
                    vids.push_back(getvalues[d].ival);
                }
                else if (getvalues[d].type == otb::AttributeTable::ATTYPE_DOUBLE)
                {
                    const double dval = getvalues[d].dval;
                    const int ival = static_cast<int>(dval);
                    vids.push_back(ival);
                }
                else
                {
                    MosraLogError( << "NMMosra::processVariables() - ERROR: Unsupported data type "
                                   << "detected for dimension parameter '"
                                   << getnames[d] << "'! "
                                   << "All dimension parameters must refer to INTEGER fields!"
                                   << endl);
                    sqltab->EndTransaction();
                    return false;
                }
            }

            // prep var bounds writing
            QString boundRow;
            QTextStream rowStr(&boundRow);

            // ..................................................................
            //  - generate dimension parameter values for OPTIONS, if applicable
            //  - store dimension parameter values
            //
            //  - fetch variable bounds from db, if applicable
            //  - write variable bound to b_seg string and txt file
            //
            //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
            //  primal initial guess based on current land-use
            //  - bLU == true
            //
            QString curLU;
            if (bLU)
            {
                curLU = getvalues[getvalues.size()-1].tval;
            }

            // format 'alternative' in case we're dealing with a 'lu' variable here
            // and need to account for a potential LuLbndFactor >= 0 !
            QString luBndRow;
            QTextStream luBndRowStr(&luBndRow);
            std::vector<double> luBndValVec{0.0, 0.0};
            bool bApplyLuLbndFactor = false;

            // only if we haven't added the index info yet
            if (oga != nullptr)
            {
                if (oga->varIdxMap.find(vad.name.toStdString()) == oga->varIdxMap.cend())
                {
                    oga->varIdxMap.insert(std::pair<string, size_t >(vad.name.toStdString(), oga->varBnds.size()));
                    std::vector<std::vector<std::pair<float, float> > > bv;
                    oga->varBnds.push_back(bv);

                    // regardless of whether this variable is defined over OPTIONS, we need at least one vector
                    // for each variable that stores the boundary values for each distinct 'location' for
                    // which this variable is defined
                    std::vector<std::pair<float, float> > optVarBndVec;
                    oga->varBnds[oga->varIdxMap[vad.name.toStdString()]].push_back(optVarBndVec);
                }
            }

            // OPTIONS has been specified as a parameter / variable dimension
            if (bOptions)
            {
                for (int opt=0; opt < mslOptions.size(); ++opt)
                {
                    QVector<long long> dimids = vids;
                    dimids.push_back(opt);


                    if (oga != nullptr)
                    {
                        // track 'local' variable offset
                        // in NMSolution.dvars and NMOpenGA.varBnds
                        doffMap.insert(dimids, dbiter);

                        // make sure we've got a boundary vector for this variable
                        // for each land-use option
                        if (oga->varBnds[oga->varIdxMap[vad.name.toStdString()]].size() < opt+1)
                        {
                            std::vector<std::pair<float, float> > optVarBndVec;
                            oga->varBnds[oga->varIdxMap[vad.name.toStdString()]].push_back(optVarBndVec);
                        }
                    }
                    else
                    {
                        // track global variable offset for *.nl file
                        doffMap.insert(dimids, varoff);
                    }

                    rowStr << zero << sp;

                    // process lower (b=0) and upper (b=1) bounds
                    double boundVal;
                    for (int b=0; b < 2; ++b)
                    {
                        if (bndsVec[b][0] == 1)
                        {
                            boundVal = bndsVec[b][1];
                        }
                        else
                        {
                            // lookup could be just one column or one for each land use
                            int lookup = bndsVec[b].size() == 2 ? bndsVec[b][1] : bndsVec[b][opt+1];

                            // do we have to scale area values ?
                            auto apnit = mmslParameters.constFind(QStringLiteral("AreaHa"));
                            if (apnit == mmslParameters.cend())
                            {
                                MosraLogError(<< "'AreaHa' parameter not defined!");
                                sqltab->EndTransaction();
                                return false;
                            }

                            const otb::AttributeTable::ColumnValue lcv = getvalues[lookup];
                            switch(lcv.type)
                            {
                            case otb::AttributeTable::ATTYPE_INT:
                                boundVal = static_cast<double>(lcv.ival);
                                break;
                            case otb::AttributeTable::ATTYPE_DOUBLE:
                                boundVal = static_cast<double>(lcv.dval);
                                break;
                            default: ;
                            }

                            // scaling
                            assert(lookup >= 0);
                            boundVal *= vBndScalingFactors[lookup-1];
                        }
                        luBndValVec[b] = boundVal;
                        rowStr << QString("%1").arg(boundVal);


                        // .....................................................
                        // mostly for DEBUG ...
                        // - append comment with variable name + dimension info

                        if (b==0)
                        {
                            rowStr << sp;
                        }
                        else
                        {
                            rowStr << "     #" << vbm.key();
                            rowStr << ": ";
                            for (int di=0; di < vids.size(); di++)
                            {
                                rowStr << QString("%1").arg(vids[di]) << sp;
                            }
                            rowStr << QString("%1").arg(opt) << eol;

                            // accounting for LU lower bound factor and
                            // processing of x segment, i.e. primal guess processing
                            if (bLU)
                            {
                                if (luBndValVec[0] == 0 && mdLuLbndFactor > 0)
                                {
                                    bApplyLuLbndFactor = true;

                                    luBndRowStr << zero << sp
                                                << (luBndValVec[1] * mdLuLbndFactor)
                                                << sp
                                                << luBndValVec[1];

                                    luBndRowStr << "     #" << vbm.key();
                                    luBndRowStr << ": ";
                                    for (int di=0; di < vids.size(); di++)
                                    {
                                        luBndRowStr << QString("%1").arg(vids[di]) << sp;
                                    }
                                    luBndRowStr << QString("%1").arg(opt) << eol;
                                }

                                if (curLU.compare(mslOptions[opt]) == 0)
                                {
                                    xstr << varoff << sp << boundVal << eol;
                                    ++xcount;
                                }
                            }
                        }
                        // .....................................................
                    }

                    if (oga != nullptr)
                    {
                        std::pair<float, float> ogaBndVals;
                        ogaBndVals.first = luBndValVec[0];
                        ogaBndVals.second = luBndValVec[1];
                        oga->varBnds[oga->varIdxMap[vad.name.toStdString()]][opt].push_back(ogaBndVals);
                    }

                    mNonZeroJColCount.insert(varoff, 0);
                    ++varoff;
                }
            }
            else
            {
                if (oga != nullptr)
                {
                    doffMap.insert(vids, dbiter);
                }
                else
                {
                    doffMap.insert(vids, varoff);
                }


                // write variable boundaries
                // var boundaries
                rowStr << zero << sp;

                // if we're running in GA mode, we fill this pair
                std::pair<float, float> ogaBndVals;
                double boundVal;
                for (int b=0; b < 2; ++b)
                {
                    if (bndsVec[b][0] == 1)
                    {
                        boundVal = bndsVec[b][1];
                    }
                    else
                    {
                        const otb::AttributeTable::ColumnValue lcv = getvalues[bndsVec[b][1]];
                        switch(lcv.type)
                        {
                        case otb::AttributeTable::ATTYPE_INT:
                            boundVal = static_cast<double>(lcv.ival);
                            break;
                        case otb::AttributeTable::ATTYPE_DOUBLE:
                            boundVal = static_cast<double>(lcv.dval);
                            break;
                        default: ;
                        }

                        // scaling
                        boundVal *= vBndScalingFactors[bndsVec[b][1]];
                    }
                    rowStr << QString("%1").arg(boundVal);
                    if (b == 0) rowStr << sp;

                    // fill GA structure with bound values
                    if (b == 0)
                    {
                        ogaBndVals.first = boundVal;
                    }
                    else
                    {
                        ogaBndVals.second = boundVal;
                    }
                }
                rowStr << "     #" << vbm.key();
                rowStr << ": ";
                for (int di=0; di < vids.size(); di++)
                {
                    rowStr << QString("%1").arg(vids[di]) << sp;
                }
                rowStr << eol;

                if (oga != nullptr)
                {
                    oga->varBnds[oga->varIdxMap[vad.name.toStdString()]][0].push_back(ogaBndVals);
                }

                mNonZeroJColCount.insert(varoff, 0);
                ++varoff;

            }
            // add row to
            if (bApplyLuLbndFactor)
            {
                bstr << luBndRow;
            }
            else
            {
                bstr << boundRow;
            }

            // get the next row
            brow = sqltab->DoBulkGet(getvalues);

            // count rows in 'NMOpenGA::NMSolution::mData' structure
            ++dbiter;

        } while (brow);

        sqltab->EndTransaction();

        // add VarAdmin to map
        mmVarAdminMap.insert(vad.name, vad);

        // DEBUG DEBUG
        NMDebugAI(<< "... distinct variables added: " << vad.dimOffsetMap.size() << endl);

        MosraLogInfo(<< vit.key().toStdString()
                     << "[" << vit.value().join(",").toStdString()
                     << "] -> " << vad.dimOffsetMap.size()
                     << " variables added" << endl);
    }
    *n_vars = varoff;

    // finalize x_segement
    xstr.flush();
    const QString xhead = QString("x%1 \n").arg(xcount);
    mx_seg.prepend(xhead);


    NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

/*
bool NMMosra::processVariableBounds(QStringList &b_seg)
{
    // ========================================================
    //          GENERATE VARIABLE BOUNDS SECTION
    // ========================================================

    QMap<QString, std::vector<int> > varBndOffsetMap;
    QMap<QString, std::vector<double> > varNumericBoundsMap;
    std::vector<std::string> varnames;
    std::vector<otb::AttributeTable::ColumnValue> varbounds;

    foreach(const QString& vname, mslDecisionVars)
    {
        auto vbm = mmslVarBoundsMap.constFind(vname);
        if (vbm != mmslVarBoundsMap.cend())
        {
            bool bConv;
            double lbound = 0;
            double ubound = std::numeric_limits<double>::max();
            std::vector<double> realBnds;
            std::vector<int>    offsetMap;
            if (vbm.value().size() == 0)
            {
                realBnds.push_back(lbound);
                realBnds.push_back(ubound);
            }
            else if (vbm.value().size() == 1)
            {
                const QString lbndName = vbm.value().at(0);
                lbound = lbndName.toDouble(&bConv);
                if (!bConv)
                {
                    realBnds.push_back(-9);

                    if (mmslParameters.constFind(lbndName) != mmslParameters.cend())
                    {
                        foreach(const QString& lbcol, mmslParameters[lbndName])
                        {
                            offsetMap.push_back(varnames.size());
                            varnames.push_back(lbcol.toStdString());
                            otb::AttributeTable::ColumnValue bv;
                            varbounds.push_back(bv);
                        }
                    }
                    else
                    {
                        MosraLogError(<< "NMMosra::processVariableBounds() - ERROR: "
                                      << "Couldn't find parameter '" << lbndName.toStdString()
                                      << "' in optimisation dataset!" << endl);
                        return false;
                    }
                }
                else
                {
                    realBnds.push_back(lbound);
                }
                realBnds.push_back(ubound);
            }
            else if (vbm.value().size() == 2)
            {
                lbound = vbm.value().at(0).toDouble(&bConv);
                if (!bConv)
                {
                    realBnds.push_back(-9);
                    offsetMap.push_back(varnames.size());
                    varnames.push_back(vbm.value().at(0).toStdString());
                    otb::AttributeTable::ColumnValue bv;
                    varbounds.push_back(bv);
                }
                else
                {
                    realBnds.push_back(lbound);
                }

                ubound = vbm.value().at(1).toDouble(&bConv);
                if (!bConv)
                {
                    realBnds.push_back(-9);
                    offsetMap.push_back(varnames.size());
                    varnames.push_back(vbm.value().at(1).toStdString());
                    otb::AttributeTable::ColumnValue bv;
                    varbounds.push_back(bv);
                }
                else
                {
                    realBnds.push_back(ubound);
                }
            }

            // go over the var and fetch/set bounds







            // if offsetMap.size() > 0 then retrieve value(s) from DB
            // if offsetMap.size() == 0 then set 'realBnds'
            //varBndOffsetMap.insert(vname, offsetMap);
            //varNumericBoundsMap.insert(vname, realBnds);
        }
        // variable not specified in the *.los file
        else
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__
                          << "(): Variable is not defined in "
                          << "*.los file!" << endl);
            return false;
        }
    }




    return true;
}
*/

bool NMMosra::createSegmentFile(QFile*& file, const NLSegment segType, const QString& abbr)
{
    bool ret = true;

    QString suffix = "";
    if (!abbr.isEmpty())
    {
        suffix = QString("%1_").arg(abbr);
    }

    QString fn;
    switch(segType)
    {
    case NL_SEG_C:
        fn = QString("%1/C_seg_%2%3.txt").arg(msDataPath).arg(suffix).arg(mConsCounter);
        break;
    case NL_SEG_L:
        fn = QString("%1/L_seg_%2%3.txt").arg(msDataPath).arg(suffix).arg(mLogicConsCounter);
        break;
    case NL_SEG_O:
        fn = QString("%1/O_seg_%2%3.txt").arg(msDataPath).arg(suffix).arg(mObjCounter);
        break;
    case NL_SEG_J:
        fn = QString("%1/J_seg_%2%3.txt").arg(msDataPath).arg(suffix).arg(mConsCounter);
        break;
    case NL_SEG_G:
        fn = QString("%1/G_seg_%2%3.txt").arg(msDataPath).arg(suffix).arg(mObjCounter);
        break;
    default:
        ret = false;
    }

    if (file == nullptr)
    {
        file = new QFile(fn, this);
    }
    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Failed creating file '" << fn.toStdString()
                      << "'!" << std::endl);
        ret = false;
    }

    return ret;
}

void NMMosra::removeNLFiles(void)
{
    QFileInfo fifo(this->getLosFileName());
    QString nlFileName = QString("%1/%2.nl").arg(this->msDataPath).arg(fifo.baseName());
    QFileInfo nlfifo(nlFileName);
    if (nlfifo.isWritable())
    {
        QFile::remove(nlFileName);
    }

    QStringList filterList = {"C_seg*.txt", "L_seg*.txt", "J_seg*.txt", "G_seg*.txt", "O_seg*.txt"};
    QDirIterator dit(msDataPath, filterList, QDir::Files);
    while (dit.hasNext())
    {
        QFile(dit.next()).remove();
    }

    QFile::remove(QString("%1/header_seg.txt").arg(msDataPath));
    QFile::remove(QString("%1/x_seg.txt").arg(msDataPath));
    QFile::remove(QString("%1/r_seg.txt").arg(msDataPath));
    QFile::remove(QString("%1/k_seg.txt").arg(msDataPath));
    QFile::remove(QString("%1/b_seg.txt").arg(msDataPath));

}

void NMMosra::clearInternalNLDataStructures(void)
{
    mvNonLinearConsVarCounter.clear();
    mvNonLinearObjVarCounter.clear();
    mslDecisionVars.clear();
    mslProcessVars.clear();
    msBinaryVars.clear();
    msIntVars.clear();
    mmLookupColsFstParam.clear();
    mmVarAdminMap.clear();
    mmDimLengthMap.clear();
    mmDimEqnMap.clear();
    mmHighDimEqnMap.clear();
    mmslHighDimValComboTracker.clear();
    mmvHighDimLoopIterLength.clear();
    mvvFSCUniqueIDCounterMap.clear();
    mvvFSCUniqueIDSegmentRHSMap.clear();

    //mvC_seg.clear();
    //mvL_seg.clear();
    //mvO_seg.clear();
    //mvJ_seg.clear();
    //mvG_seg.clear();

    if (mpNLFile != nullptr)
    {
        mpNLFile->close();
        delete mpNLFile;
        mpNLFile = nullptr;
    }

    // remove the segment files
    for(int fi=0; fi < mvfC_seg.size(); ++fi)
    {
        mvfC_seg[fi]->close();
        //mvfC_seg[fi]->remove();
        delete mvfC_seg[fi];
    }
    for(int fi=0; fi < mvfL_seg.size(); ++fi)
    {
        mvfL_seg[fi]->close();
        //mvfL_seg[fi]->remove();
        delete mvfL_seg[fi];
    }
    for(int fi=0; fi < mvfO_seg.size(); ++fi)
    {
        mvfO_seg[fi]->close();
        //mvfO_seg[fi]->remove();
        delete mvfO_seg[fi];
    }
    for(int fi=0; fi < mvfJ_seg.size(); ++fi)
    {
        mvfJ_seg[fi]->close();
        //mvfJ_seg[fi]->remove();
        delete mvfJ_seg[fi];
    }
    for(int fi=0; fi < mvfG_seg.size(); ++fi)
    {
        mvfG_seg[fi]->close();
        //mvfG_seg[fi]->remove();
        delete mvfG_seg[fi];
    }
    mvfC_seg.clear();
    mvfL_seg.clear();
    mvfO_seg.clear();
    mvfJ_seg.clear();
    mvfG_seg.clear();
    mx_seg.clear();
    mb_seg.clear();
    mr_seg.clear();
    mk_seg.clear();

    mvExplicitAreaConsVarOffsets.clear();
    mvvImplicitAreaConsSegments.clear();
    mmslHighDimLoopEquations.clear();

    mNonZeroJColCount.clear();

    mmEquationAdmins.clear();
    mmPrefixEquations.clear();
}

int NMMosra::makeNL2(void)
{
    // delete old files
    removeNLFiles();

    // ---------------------------------------
    // initiate header

    QString nlheader;
    QTextStream nlstr(&nlheader);

    QString sp = " ";
    QString eol = "\n";

    // first line never changes
    nlstr << "g3 1 1 0" << eol;

    // num variables
    long cnt_nvars = 0;

    // set the area scaling factor
    auto psit = mmParameterScaling.constBegin();
    for (; psit != mmParameterScaling.constEnd(); ++psit)
    {
        if (psit.key().compare(QStringLiteral("AreaHa"), Qt::CaseSensitive) == 0)
        {
            mdAreaScalingFactor = psit.value().first;
            break;
        }
    }

    // ----------------------------------------------------
    //          b segment

    if (!processVariables(mb_seg, mx_seg, &cnt_nvars))
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Just got it wrong! :-( " << std::endl);
        return 0;
    }

    if (!processLoopDimensions())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Just got it wrong! :-( " << std::endl);
        return 0;
    }


    MosraLogInfo(<< "Number of decision variables for '" << mScenarioName.toStdString()
                 << "' : " << std::to_string(cnt_nvars) << std::endl);

    // DEBUG / TESTING OUTPUT ONLY
    //std::stringstream ndvstr;
    //long calc_nvars = 0;
    //for (int dv=0; dv < mslDecisionVars.size(); ++dv)
    //{
    //    auto vait = mmVarAdminMap.constFind(mslDecisionVars[dv]);
    //    if (vait != mmVarAdminMap.cend())
    //    {
    //        ndvstr << "   " << mslDecisionVars.at(dv).toStdString()
    //               << ": "  << vait.value().dimOffsetMap.size()
    //               << std::endl;
    //        calc_nvars += vait.value().dimOffsetMap.size();
    //    }
    //}
    //MosraLogInfo(<< std::endl << ndvstr.str() << std::endl
    //             << "   Total number of decision variables: "
    //             << std::to_string(calc_nvars) << std::endl);

    this->mlNumDVar = cnt_nvars;

    // add number of variables to the header
    nlstr << cnt_nvars << sp;


    // -------------------------------------------------------
    //      NON-LINEAR C, L & O; LINEAR J, k, and r segments

    if (!processNLSegmentData2())
    {
        MosraLogError(<< "NMMosra::makeNL2() - ERROR: "
                      << "Oh no! Something didn't work! "
                      << "You better check your *.los file "
                      << "- I don't have a clue what went wrong!"
                      << std::endl);
        return 0;
    }

    // add number of contraints & objectives to the header
    //const int allConsRanges = mConsCounter = mLogicConsCounter;
    nlstr << mConsCounter << sp << mObjCounter << sp << mConsCounter << sp << 0 << sp << mLogicConsCounter << eol;

    // add non-linear constraints & objectives
    //    for now: all non- and linear constraints explicitly specified are treated as non-linear
    //             so we reduce the amount of work involved with identifying linear and non-linear
    //             parts thereof
    const int nnlcons = mmslLinearConstraints.size() + mmslNonLinearConstraints.size() + mmslLogicConstraints.size();
    nlstr << nnlcons << sp << mvfO_seg.size() << eol;

    // network constraints: non-linear, linear
    nlstr << 0 << sp << 0 << eol;

    // non-linear vars in objectives, constraints, both
    int commonVar = 0;
    for (auto it = mvNonLinearConsVarCounter.constBegin(); it != mvNonLinearConsVarCounter.cend(); ++it)
    {
        if (mvNonLinearObjVarCounter.constFind(it.key()) != mvNonLinearObjVarCounter.cend())
        {
            ++commonVar;
        }
    }

    nlstr << mvNonLinearConsVarCounter.size() << sp << mvNonLinearObjVarCounter.size() << sp << commonVar << eol;

    // -----------------------------------------------------
    //    OTHER INFO ON FEATURES NOT SUPPORTED BY LUMASS

    // linear network vars, functions, arith, flags
    nlstr << 0 << sp << 0 << sp << 0 << sp << 0 << eol;


    // calc number of binary and integer vars
    int nbvar = 0, nivar = 0;
    foreach(const QString& bvn, msBinaryVars)
    {
        nbvar += mmVarAdminMap[bvn].dimOffsetMap.size();
    }
    foreach(const QString& ivn, msIntVars)
    {
        nivar += mmVarAdminMap[ivn].dimOffsetMap.size();
    }

    // discrete variables : binary, integer, nonlinear (b, c, o)
    nlstr << nbvar << sp << nivar << sp;

    for (int t=0; t < 3; ++t)
    {
        nlstr << 0 << sp;
    }
    nlstr << eol;

    // non-zeros in Jacobian, gradients
    long nonzeroJacobian = 0;
    const long klen = mNonZeroJColCount.size();
    auto jit = mNonZeroJColCount.cbegin();
    for (int k=0; k < klen && jit != mNonZeroJColCount.cend(); ++k, ++jit)
    {
        nonzeroJacobian += jit.value();
    }

    // num vars in objectives
    nlstr << nonzeroJacobian << sp << 0 << eol;
           //mvNonLinearObjVarCounter.size() << eol;

    // max name length: constraints, variables
    nlstr << 0 << sp << 0 << eol;

    // common expr. b,c,o,c1,o1
    for (int ce=0; ce < 5; ++ce)
    {
        nlstr << 0 << sp;
    }
    nlstr << eol;

    // ---------------------------------------------------------
    //              NL FILE
    QFile* dF = nullptr;
    QString dS;

    writeNLSegment(nlheader, dF, NL_SEG_NL, 0);

    // NL file segments
    for (int cs=0; cs < mvfC_seg.size(); ++cs)
    {
        writeNLSegment(dS, mvfC_seg[cs], NL_SEG_C, 1);
    }

    for (int ls=0; ls < mvfL_seg.size(); ++ls)
    {
        writeNLSegment(dS, mvfL_seg[ls], NL_SEG_L, 1);
    }

    for (int os=0; os < mvfO_seg.size(); ++os)
    {
        writeNLSegment(dS, mvfO_seg[os], NL_SEG_O, 1);
    }

    writeNLSegment(mx_seg, dF, NL_SEG_x, 0);
    writeNLSegment(mr_seg, dF, NL_SEG_r, 0);
    writeNLSegment(mb_seg, dF, NL_SEG_b, 0);
    writeNLSegment(mk_seg, dF, NL_SEG_k, 0);

    for (int js=0; js < mvfJ_seg.size(); ++js)
    {
        writeNLSegment(dS, mvfJ_seg[js], NL_SEG_J, 1);
    }

    for (int gs=0; gs < mvfG_seg.size(); ++gs)
    {
        if (mvfG_seg[gs] != nullptr && mvfG_seg[gs]->size() > 0)
        {
            writeNLSegment(dS, mvfG_seg[gs], NL_SEG_G, 1);
        }
    }

    mpNLFile->close();

    clearInternalNLDataStructures();


    return 1;
}


bool NMMosra::populateHDLE(
        const QString &hdeqn,
        otb::SQLiteTable::Pointer &sqltab,
        QMap<QString, std::vector<size_t> >& nameValPosMap,
        std::vector<std::string>& getnames,
        std::vector<otb::AttributeTable::ColumnValue>& getvalues
        )
{


    return true;
}

bool NMMosra::populateHighDimLoopEquations(const QString &hdeqn,
        otb::SQLiteTable::Pointer &sqltab,
        QMap<QString, std::vector<size_t> > &nameValPosMap,
        std::vector<std::string> &getnames,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues
        )
{
    foreach(const QString eqn, mmslHighDimLoopEquations.keys())
    {

        // create queries for determining the size of the
        // individeual sum dimensions, i.e. the number of
        // iterations for each dimension

        QStringList hdims = mmslHighDimLoopEquations[eqn].first;
        if (hdims.last().compare(QStringLiteral("SDU")) != 0)
        {
            hdims << QStringLiteral("SDU");
        }

        QStringList dimCols;
        foreach(const QString& hd, hdims)
        {
            if (mmslParameters.constFind(hd) != mmslParameters.cend())
            {
                dimCols << mmslParameters[hd].at(0);
            }
            else
            {
                MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: Failed "
                              << "fetching table column for dimension parameter '"
                              << hd.toStdString() << "'!");
                return false;
            }
        }

        /*
        const int numHighDims = dimCols.size();
        for (int dims=0; dims < numHighDims; ++dims)
        {
            std::stringstream sqlstr, gbcl;
            sqlstr << "select ";
            gbcl << "group by ";

            std::vector<std::vector<otb::AttributeTable::ColumnValue> > tab;
            std::vector<otb::AttributeTable::TableColumnType> coltypes;

            otb::AttributeTable::TableColumnType tct = otb::AttributeTable::ATTYPE_INT;

            // numHighDims - dims = 3, 2, 1
            const int ncols = numHighDims - dims;
            for (int len=0; len < ncols ; ++len)
            {
                if (ncols == 1)
                {
                    sqlstr << "count(distinct \""
                           << dimCols.at(len).toStdString()
                           << "\")";
                    coltypes.push_back(tct);
                }
                else if (len == ncols-1)
                {
                    sqlstr << "count(\"" << dimCols.at(len).toStdString()
                           << "\")";
                    coltypes.push_back(tct);
                }
                else
                {
                    sqlstr << "\"" << dimCols.at(len).toStdString()
                           << "\"";
                    coltypes.push_back(tct);
                }

                // fmu, rowidx
                if (ncols > 1 && len < ncols-1)
                {
                    gbcl << "\"" << dimCols.at(len).toStdString()
                         << "\"";
                }

                if (len < ncols-1)
                {
                    sqlstr << ", ";
                }

                //   0      1    2      3
                // region, fmu, farm, rowidx
                if (ncols > 2 && len < ncols-2)
                {
                    gbcl   << ", ";
                }
            }

            // complete the query string
            sqlstr << " from \"" << sqltab->GetTableName() << "\" "
                   << " where " << msOptFeatures.toStdString() << " == 1 ";
            if (ncols > 1)
            {
                sqlstr << gbcl.str();
            }

            std::string thequery = sqlstr.str();

            if (!sqltab->TableDataFetch(tab, coltypes, thequery))
            {
                MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Round #" << dims << " populating HighDimLoopIterLength failed: "
                              << thequery);
                return false;
            }


            for (int row=0; row < tab.size(); ++row)
            {
                QVector<long long> key;
                for (int col=0; col < tab[0].size()-1; ++col)
                {
                    key.push_back(tab[row][col].ival);
                }
                mmvHighDimLoopIterLength.insert(key, tab[row][tab[0].size()-1].ival);
            }

            NMDebug(<< "--------------- \n")
            NMDebug(<< "mmvHiDimLoopIterLengths ...\n");
            auto dimit = mmvHighDimLoopIterLength.cbegin();
            for (; dimit != mmvHighDimLoopIterLength.cend(); ++dimit)
            {
                for (int c=0; c < dimit.key().size(); ++c)
                {
                    NMDebug(<< dimit.key().at(c) << " => ");
                }
                NMDebug(<< endl);
            }
            NMDebug(<< "END ---------\n\n");
        }
        */

        if (!determineHighDimLoopIterLength(eqn, sqltab))
        {
            MosraLogError(<< "Unable to determine the size of high dimensions sets!");
            sqltab->EndTransaction();
            return false;
        }

        // prepare the whereClause for this equation to ensure that a sequantial
        // traversal of the records enables proper loop processing
        // ... i.e. sequential summing of dimensions starting with most deeply
        //     nested dimension to the top most dimension
        std::stringstream whereClause;
        whereClause << "where \"" << msOptFeatures.toStdString() << "\" == 1 ";

        if (dimCols.size() > 0)
        {
            whereClause << "order by ";
            for (int dim=0; dim < dimCols.size(); dim++)
            {
                whereClause << "\"" << dimCols.at(dim).toStdString() << "\"";

                if (dim < dimCols.size()-1)
                {
                    whereClause << ", ";
                }
            }
        }
        else
        {
            continue;
        }

        const std::string wcstr = whereClause.str();
        MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - DEBUG: Table prep for "
                      << eqn.toStdString() << ": " << wcstr);

        bool brow = sqltab->PrepareBulkGet(getnames, wcstr);
        if (!brow)
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Populating equations failed: "
                          << sqltab->getLastLogMsg());
            sqltab->EndTransaction();
            return false;
        }

        sqltab->BeginTransaction();
        brow = sqltab->DoBulkGet(getvalues);

        if (!brow)
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Populating equations failed: "
                          << sqltab->getLastLogMsg());
            sqltab->EndTransaction();
            return false;
        }

        // -----------------------------------------------------------
        // create lookup for last value of a dimension
        // dimValueMap: NOTE: OPTIONS values for
        // variable offset look up are stored 0-based!
        QMap<QString, double> dimValueMap;
        QMap<QString, bool> dimChangeMap;

        const QStringList& dimensions = mmslHighDimLoopEquations[eqn].second;
        foreach(const QString& td, dimensions)
        {
            dimValueMap[td] = -1;
            dimChangeMap[td] = false;
        }


        // this map is empty to begin with, if an inner
        // loop is iterated over multiple times, the
        // after the last iteration, the loop counter
        // is reset to '0'

        //<eqn name>, <loop dim, counter value 1-based!> >
        QMap<QString, QMap<QString, int> > eqnLoopCounterMap;
        QMap<QString, QStack<std::vector<int> > > eqnActiveSegmentMap;


        long recCounter = 0;
        do
        {
            // -----------------------------------------------------
            //          HAVE DIMENSION VALUES CHANGED?
            // get dimension values
            foreach (const QString& dim, dimensions)
            {
                // as OPTIONS differ even at the smallest spatial
                // dimension, i.e. at SDU level, we trigger processing
                // each time it appears in an equation
                if (dim.compare(msOPTIONS) == 0)
                {
                    dimValueMap[dim] = -9999;
                    dimChangeMap[dim] = true;
                    continue;
                }

                double dimValue = getRecordValue(
                            nameValPosMap,
                            getvalues,
                            dim
                            );

                if (dimValue != dimValueMap[dim])
                {
                    dimValueMap[dim] = dimValue;
                    dimChangeMap[dim] = true;
                }
                else
                {
                    dimChangeMap[dim] = false;
                }
            }

            // -----------------------------------------------------
            //          populate eqns




            // if the equation is only evaluated on 'higher' (i.e. above SDU)
            // dimensions, we double check whether we have actually already
            // processed this specific combination of dimension values,
            // in which case we'd have found a record for the given combination
            // of higher dimension values ...
            if (    mmHighDimEqnMap.contains(eqn)
                    && (   mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                           || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                           || mmslObjectives.constFind(eqn) != mmslObjectives.cend()
                           )
                    )
            {
                QVector<long long> highDimValues;
                foreach(const QString& hdim, mmHighDimEqnMap[eqn])
                {
                    highDimValues.push_back(dimValueMap[hdim]);
                }

                if (    mmslHighDimValComboTracker[eqn].constFind(highDimValues)
                        != mmslHighDimValComboTracker[eqn].cend()
                        )
                {
                    continue;
                }
                else
                {
                    mmslHighDimValComboTracker[eqn].insert(highDimValues);
                }
            }

            // ---------------------------------------------
            //  C    (Non-linear) algebraic constraints
            // ---------------------------------------------
            if (    mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                    || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                    //|| mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend()
                    )
            {
                /*
                        if (mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend())
                        {
                            if (!populateEquations(
                                        eqn,
                                        eqnActiveSegmentMap,
                                        0,
                                        NL_SEG_L,
                                        mLogicConsCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                               )
                            {
                                MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                sqltab->EndTransaction();
                                return false;
                            }
                        }
                        else
                        */

                // ATTENTION:
                // - if we've got a constraint defined over OPTIONS (for at least one par,
                //   var, or loop) and
                // - we don't have a loop at the beginning of the constraint,
                // - and we can find the OPTIONS dimension parameter somewhere in the eqn
                //   that indicates that this eqn does not just apply to a single element
                //   of OPTIONS
                // ==> we assume that this constraint is meant to be defined
                //     individually for each <other dims> + OPTIONS combination ...

                if (mmDimEqnMap[msOPTIONS].constFind(eqn) != mmDimEqnMap[msOPTIONS].cend())
                {
                    if (    mmEquationAdmins[eqn].elemMap[0].first != EQN_LOOP
                            && mmsEquations[eqn].contains(msOPTIONS)
                            )
                    {
                        for (int opt=0; opt < mslOptions.size(); ++opt)
                        {
                            dimValueMap[msOPTIONS] = opt;

                            if (!populateEquations(
                                        eqn,
                                        eqnActiveSegmentMap,
                                        0,
                                        NL_SEG_C,
                                        mConsCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                                    )
                            {
                                MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                sqltab->EndTransaction();
                                return false;
                            }
                        }

                        dimValueMap[msOPTIONS] = -9999;
                    }
                }
                else
                {
                    if (!populateEquations(
                                eqn,
                                eqnActiveSegmentMap,
                                0,
                                NL_SEG_C,
                                mConsCounter,
                                eqnLoopCounterMap,
                                dimValueMap,
                                nameValPosMap,
                                getvalues
                                )
                            )
                    {
                        MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                        sqltab->EndTransaction();
                        return false;
                    }

                }
            }

            // --------------------------------------------------
            //  O    Non-linear algebraic OBJECTIVE segment
            // --------------------------------------------------
            if (mmslObjectives.constFind(eqn) != mmslObjectives.cend())
            {
                if (!populateEquations(
                            eqn,
                            eqnActiveSegmentMap,
                            0,
                            NL_SEG_O,
                            mObjCounter,
                            eqnLoopCounterMap,
                            dimValueMap,
                            nameValPosMap,
                            getvalues
                            )
                        )
                {
                    MosraLogError(<< "ERROR in objctive eqn!" << std::endl);
                    sqltab->EndTransaction();
                    return false;
                }
            }


            // get new record with values
            brow = sqltab->DoBulkGet(getvalues);
            ++recCounter;

        } while (brow);

        sqltab->EndTransaction();

        MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - INFO: "
                      << "processed " << recCounter << " records while "
                      << "poplulating the equations!" << endl);

    }



    return true;
}

bool NMMosra::populateLowDimAndOtherEquations(
        otb::SQLiteTable::Pointer &sqltab,
        QMap<QString, std::vector<size_t> > &nameValPosMap,
        std::vector<std::string> &getnames,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues
        )
{
    // --------------------------------------------------------
    //      prepare linear explict area constraints
    // --------------------------------------------------------
    QChar eol('\n');
    QChar sp(' ');

    std::vector<QString> vsConsLabel;
    std::vector<int> vnZoneLength;
    std::vector<QString> vsZoneField;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    if (!processExplicitArealCons(vsConsLabel, vnZoneLength, vsZoneField,
                             vvnOptionIndex, vnConsType, vdRHS))
    {
        return false;
    }

    std::vector<QString> vsFSCZoneField;
    std::vector<std::vector<unsigned int> > vvnFSCOptionIndex;
    if (!processFeatureSetConstraints(vsFSCZoneField, vvnFSCOptionIndex))
    {
        return false;
    }


    // add zone fields to getnames & getvalues
    for (int z=0; z < vsZoneField.size(); ++z)
    {
        if (    !vsZoneField.at(z).isEmpty()
             && nameValPosMap.constFind(vsZoneField[z]) == nameValPosMap.cend()
           )
        {
            std::vector<size_t> zfoff = {getnames.size()};
            nameValPosMap.insert(vsZoneField[z], zfoff);

            getnames.push_back(vsZoneField[z].toStdString());
            otb::AttributeTable::ColumnValue zcv;
            getvalues.push_back(zcv);
        }
    }

    // add zone fields for featureset constraints, if not already present
    for (int fscf=0; fscf < vsFSCZoneField.size(); ++fscf)
    {
        if (    !vsFSCZoneField.at(fscf).isEmpty()
             && nameValPosMap.constFind(vsFSCZoneField.at(fscf)) == nameValPosMap.cend()
           )
        {
            std::vector<size_t> vfsczone = {getnames.size()};
            nameValPosMap.insert(vsFSCZoneField.at(fscf), vfsczone);

            getnames.push_back(vsFSCZoneField.at(fscf).toStdString());
            otb::AttributeTable::ColumnValue fscfv;
            getvalues.push_back(fscfv);
        }
    }

    // add criterion column for each FS constraint
    auto fsit = this->mslFeatSetCons.constBegin();
    for (; fsit != this->mslFeatSetCons.constEnd(); ++fsit)
    {
        QString criName = fsit.value().at(0);
        if (nameValPosMap.constFind(criName) == nameValPosMap.cend())
        {
            std::vector<size_t> ccpos;

            QStringList criColNames = mmslCriteria.value(criName);
            foreach(const QString& cname, criColNames)
            {
                ccpos.push_back(getnames.size());
                getnames.push_back(cname.toStdString());
                otb::AttributeTable::ColumnValue ccval;
                getvalues.push_back(ccval);
            }
            nameValPosMap.insert(criName, ccpos);
        }
    }

    // make sure we've got the SDU field (rowidx) in the namValPosMap
    if (nameValPosMap.constFind(msSDU) == nameValPosMap.cend())
    {
        if (mmslParameters.constFind(msSDU) == mmslParameters.cend())
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Compulsory parameter 'SDU' is not defined!");
            return false;

        }

        std::vector<size_t> rowoff = {getnames.size()};
        nameValPosMap.insert(msSDU, rowoff);

        getnames.push_back(mmslParameters.value(msSDU).at(0).toStdString());
        otb::AttributeTable::ColumnValue rowval;
        getvalues.push_back(rowval);
    }

    // add the AreaHa field to the parameters to be fetched
    const int areaValueIdx = getnames.size();
    getnames.push_back(msAreaField.toStdString());
    otb::AttributeTable::ColumnValue areaColVal;
    getvalues.push_back(areaColVal);


    // fetch the first record of the optimisation table/dataset
    std::stringstream whereClause;
    whereClause << "where \"" << msOptFeatures.toStdString() << "\" == 1";
    bool brow = sqltab->PrepareBulkGet(getnames, whereClause.str());
    if (!brow)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Populating equations failed: "
                      << sqltab->getLastLogMsg());
        sqltab->EndTransaction();
        return false;
    }

    sqltab->BeginTransaction();
    brow = sqltab->DoBulkGet(getvalues);

    if (!brow)
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Populating equations failed: "
                      << sqltab->getLastLogMsg());
        sqltab->EndTransaction();
        return false;
    }



    // -----------------------------------------------------------
    // create lookup for last value of a dimension
    // dimValueMap: NOTE: OPTIONS values for
    // variable offset look up are stored 0-based!
    QMap<QString, double> dimValueMap;
    QMap<QString, bool> dimChangeMap;

    const QStringList dimensions = mmDimEqnMap.keys();
    foreach(const QString& td, dimensions)
    {
        dimValueMap[td] = -1;
        dimChangeMap[td] = false;
    }

    // this map is empty to begin with, if an inner
    // loop is iterated over multiple times, the
    // after the last iteration, the loop counter
    // is reset to '0'

    //<eqn name>, <loop dim, counter value 1-based!> >
    QMap<QString, QMap<QString, int> > eqnLoopCounterMap;
    QMap<QString, QStack<std::vector<int> > > eqnActiveSegmentMap;
    long recCounter = 0;
    do
    {
        // -----------------------------------------------------
        //          HAVE DIMENSION VALUES CHANGED?
        // get dimension values
        foreach (const QString& dim, dimensions)
        {
            // as OPTIONS differ even at the smallest spatial
            // dimension, i.e. at SDU level, we trigger processing
            // each time it appears in an equation
            if (dim.compare(msOPTIONS) == 0)
            {
                dimValueMap[dim] = -9999;
                dimChangeMap[dim] = true;
                continue;
            }

            double dimValue = getRecordValue(
                        nameValPosMap,
                        getvalues,
                        dim
                        );

            if (dimValue != dimValueMap[dim])
            {
                dimValueMap[dim] = dimValue;
                dimChangeMap[dim] = true;
            }
            else
            {
                dimChangeMap[dim] = false;
            }
        }

        // -----------------------------------------------------
        //          populate eqns

        QStringList processedEqns;
        auto dimChMapIt = dimChangeMap.cbegin();
        for(; dimChMapIt != dimChangeMap.cend(); ++dimChMapIt)
        {
            if (dimChMapIt.value() == true)
            {
                auto dimEqnIt = mmDimEqnMap.constFind(dimChMapIt.key());

                // keep moving, if we can't find the equation
                if (dimEqnIt == mmDimEqnMap.cend())
                {
                    continue;
                }

                foreach(const QString& eqn, dimEqnIt.value())
                {
                    // keep moving if we've processed this eqn already
                    if (    processedEqns.contains(eqn)
                         || mmslHighDimLoopEquations.contains(eqn)
                       )
                    {
                        continue;
                    }

                    // if the equation is only evaluated on 'higher' (i.e. above SDU)
                    // dimensions, we double check whether we have actually already
                    // processed this specific combination of dimension values,
                    // in which case we'd have found a record for the given combination
                    // of higher dimension values ...
                    if (    mmHighDimEqnMap.contains(eqn)
                         && (   mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                             || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                             || mmslObjectives.constFind(eqn) != mmslObjectives.cend()
                            )
                       )
                    {
                        QVector<long long> highDimValues;
                        foreach(const QString& hdim, mmHighDimEqnMap[eqn])
                        {
                            highDimValues.push_back(dimValueMap[hdim]);
                        }

                        if (    mmslHighDimValComboTracker[eqn].constFind(highDimValues)
                             != mmslHighDimValComboTracker[eqn].cend()
                           )
                        {
                            continue;
                        }
                        else
                        {
                            mmslHighDimValComboTracker[eqn].insert(highDimValues);
                        }
                    }

                    // ---------------------------------------------
                    //  C    (Non-linear) algebraic constraints
                    // ---------------------------------------------
                    if (    mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                         || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                         //|| mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend()
                       )
                    {
                        /*
                        if (mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend())
                        {
                            if (!populateEquations(
                                        eqn,
                                        eqnActiveSegmentMap,
                                        0,
                                        NL_SEG_L,
                                        mLogicConsCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                               )
                            {
                                MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                sqltab->EndTransaction();
                                return false;
                            }
                        }
                        else
                        */

                        // ATTENTION:
                        // - if we've got a constraint defined over OPTIONS (for at least one par,
                        //   var, or loop) and
                        // - we don't have a loop at the beginning of the constraint,
                        // - and we can find the OPTIONS dimension parameter somewhere in the eqn
                        //   that indicates that this eqn does not just apply to a single element
                        //   of OPTIONS
                        // ==> we assume that this constraint is meant to be defined
                        //     individually for each <other dims> + OPTIONS combination ...

                        if (mmDimEqnMap[msOPTIONS].constFind(eqn) != mmDimEqnMap[msOPTIONS].cend())
                        {
                            if (    mmEquationAdmins[eqn].elemMap[0].first != EQN_LOOP
                                 && mmsEquations[eqn].contains(msOPTIONS)
                               )
                            {
                                for (int opt=0; opt < mslOptions.size(); ++opt)
                                {
                                    dimValueMap[msOPTIONS] = opt;

                                    if (!populateEquations(
                                                eqn,
                                                eqnActiveSegmentMap,
                                                0,
                                                NL_SEG_C,
                                                mConsCounter,
                                                eqnLoopCounterMap,
                                                dimValueMap,
                                                nameValPosMap,
                                                getvalues
                                                )
                                       )
                                    {
                                        MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                        sqltab->EndTransaction();
                                        return false;
                                    }
                                }

                                dimValueMap[msOPTIONS] = -9999;
                            }
                        }
                        else
                        {
                            if (!populateEquations(
                                        eqn,
                                        eqnActiveSegmentMap,
                                        0,
                                        NL_SEG_C,
                                        mConsCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                               )
                            {
                                MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                sqltab->EndTransaction();
                                return false;
                            }

                        }


                        // book keeping
                        processedEqns << eqn;
                    }

                    // --------------------------------------------------
                    //  O    Non-linear algebraic OBJECTIVE segment
                    // --------------------------------------------------
                    if (mmslObjectives.constFind(eqn) != mmslObjectives.cend())
                    {
                        if (!populateEquations(
                                    eqn,
                                    eqnActiveSegmentMap,
                                    0,
                                    NL_SEG_O,
                                    mObjCounter,
                                    eqnLoopCounterMap,
                                    dimValueMap,
                                    nameValPosMap,
                                    getvalues
                                    )
                           )
                        {
                            MosraLogError(<< "ERROR in objctive eqn!" << std::endl);
                            sqltab->EndTransaction();
                            return false;
                        }
                        processedEqns << eqn;
                    }
                }
            }
        }

        // --------------------------------------------------
        //  process values for J linear explicit area constraints
        // --------------------------------------------------
        QMap<QString, QStringList>::const_iterator it = this->mmslAreaCons.constBegin();
        for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r)
        {
            QStringList luOptions = mslOptions;
            if (!vsZoneField.at(r).isEmpty())
            {
                const QString zoneStr = getvalues[nameValPosMap[vsZoneField[r]][0]].tval;
                luOptions = zoneStr.split(" ", Qt::SkipEmptyParts);
            }

            std::vector<size_t> vLuOff;
            for (int opt=0; opt < vvnOptionIndex.at(r).size(); ++opt)
            {
                const int luIdx = vvnOptionIndex.at(r).at(opt);
                const QVector<long long> luDims = {
                            static_cast<long long>(dimValueMap[msSDU]),
                            static_cast<long long>(luIdx)
                            };

                const long long luOffset = mmVarAdminMap[QStringLiteral("lu")].dimOffsetMap[luDims];
                if (mNonZeroJColCount.constFind(luOffset) == mNonZeroJColCount.cend())
                {
                    assert(0);
                }

                if (luOptions.contains(mslOptions.at(luIdx)))
                {
                    mvExplicitAreaConsVarOffsets[r].push_back(luOffset);
                    mNonZeroJColCount[luOffset] += 1;
                }
            }
        }

        // --------------------------------------------------
        //  process values for J linear implicit area constraints
        // --------------------------------------------------
        // stores three strings per SDU
        // 0: SDU (rowidx) dim value
        // 1: J segment body
        // 2: rhs segement body
        QString sduVal;
        QString jseg  ;
        QString rhsseg;
        for (int opt=0; opt < mslOptions.size(); ++opt)
        {
            const QVector<long long> iacDim = {
                        static_cast<long long>(dimValueMap[msSDU]),
                        static_cast<long long>(opt)
                        };

            const long long iacOffset = mmVarAdminMap[QStringLiteral("lu")].dimOffsetMap[iacDim];

            double areahaval = 0;
            const otb::AttributeTable::TableColumnType tct = getvalues[areaValueIdx].type;
            switch(tct)
            {
            case otb::AttributeTable::ATTYPE_INT:
                areahaval = getvalues[areaValueIdx].ival;
                break;

            case otb::AttributeTable::ATTYPE_DOUBLE:
                areahaval = getvalues[areaValueIdx].dval;
                break;
            default: ;
            }

            // account for area scaling (if no scaling is specified, factor is 1)
            areahaval *= mdAreaScalingFactor;

            if (opt == 0)
            {
                sduVal.append(QString("%1").arg(iacDim[0]));
                rhsseg.append(QString("1 %1    #iac SDU[%2]\n")
                              .arg(areahaval)
                              .arg(iacDim[0])
                              );
            }
            jseg.append(QString("%1 1\n").arg(iacOffset));

            if (mNonZeroJColCount.constFind(iacOffset) == mNonZeroJColCount.cend())
            {
                assert(0);
            }
            mNonZeroJColCount[iacOffset] += 1;
        }
        const std::vector<QString> iacinfo = {sduVal, jseg, rhsseg};
        mvvImplicitAreaConsSegments.push_back(iacinfo);
        // -------------------------------------------------------------------------------


        // --------------------------------------------------
        //  process values for J linear FeatureSet Constraints
        // --------------------------------------------------
        QMap<QString, QStringList>::ConstIterator fscIt = this->mslFeatSetCons.constBegin();
        for(size_t fsCnt=0; fscIt != this->mslFeatSetCons.constEnd(); ++fscIt, ++fsCnt)
        {
            MosraLogDebug(<< "processing " << fscIt.key().toStdString());

            QString criName = fscIt.value().at(0);

            QString fsIdField = vsFSCZoneField.at(fsCnt);
            if (nameValPosMap.constFind(fsIdField) == nameValPosMap.cend())
            {
                MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "ZoneField '" << fsIdField.toStdString() << "' "
                              << "not found while poplulating FeatureSet constraints!" << endl);
                sqltab->EndTransaction();
                return false;
            }
            size_t fsId = getvalues.at(nameValPosMap[fsIdField].at(0)).ival;

            if (mvvFSCUniqueIDCounterMap.at(fsCnt).constFind(fsId) != mvvFSCUniqueIDCounterMap.at(fsCnt).cend())
            {
                // get offset for each lu variable
                // associate the criterion value
                // append entries for current id and options
                // keep track of number of offsets by inrcrementing the counter

                // increment Non-zero Col Pos
                for (int fso=0; fso < vvnFSCOptionIndex.at(fsCnt).size(); ++fso)
                {
                    const int fso_id = vvnFSCOptionIndex.at(fsCnt).at(fso);

                    // get options offset
                    const QVector<long long> fsoDim = {
                                static_cast<long long>(dimValueMap[msSDU]),
                                static_cast<long long>(fso_id)
                                };
                    const long long fsoOffset = mmVarAdminMap[QStringLiteral("lu")].dimOffsetMap[fsoDim];

                    // get criterion coefficient
                    if (nameValPosMap.constFind(criName) != nameValPosMap.cend())
                    {
                        const size_t criidx = nameValPosMap.value(criName).at(vvnFSCOptionIndex.at(fsCnt).at(fso));
                        const double criVal = getvalues.at(criidx).dval;

                        QString segstr = QString("%1 %2\n").arg(fsoOffset).arg(criVal);
                        mvvFSCUniqueIDSegmentRHSMap.at(fsCnt)[fsId][0].append(segstr);

                        mvvFSCUniqueIDCounterMap.at(fsCnt)[fsId] += 1;
                        mNonZeroJColCount[fsoOffset] += 1;
                    }
                }
            }
        }

        // -------------------------------------------------
        // get new record with values
        brow = sqltab->DoBulkGet(getvalues);
        ++recCounter;

    } while (brow);

    sqltab->EndTransaction();

    MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - INFO: "
                  << "processed " << recCounter << " records while "
                  << "poplulating the equations!" << endl);


    // ---------------------------------------------------------
    //      LINEAR J section

    QTextStream rhsStr(&mr_seg);
    rhsStr.setRealNumberNotation(QTextStream::SmartNotation);
    if (mConsCounter == 0)
    {
        rhsStr << QStringLiteral("r \n");
    }

    // ..............................................
    // implicit area constraints
    // just one file for the C / J implict are constraints
    // as they're short and otherwise create an error
    QFile* iacfj_seg = nullptr;
    if (!createSegmentFile(iacfj_seg, NMMosra::NL_SEG_J, "iac"))
    {
        return false;
    }
    QTextStream iacJStr(iacfj_seg);
    mvfJ_seg.push_back(iacfj_seg);

    QFile* iacfc_seg = nullptr;
    if (!createSegmentFile(iacfc_seg, NMMosra::NL_SEG_C, "iac"))
    {
        return false;
    }
    QTextStream iacCStr(iacfc_seg);
    mvfC_seg.push_back(iacfc_seg);

    for (int iac=0; iac < mvvImplicitAreaConsSegments.size(); ++iac, ++mConsCounter)
    {
        // constraint body
        QString sj_seg = mvvImplicitAreaConsSegments.at(iac).at(1);
        sj_seg.prepend(QString("J%1 %2   # iac SDU[%3]\n")
                        .arg(mConsCounter)
                        .arg(mslOptions.size())
                        .arg(mvvImplicitAreaConsSegments[iac][0])
                      );
        iacJStr << sj_seg;

        // rhs
        rhsStr << mvvImplicitAreaConsSegments[iac][2];

        // non-linear pendant
        iacCStr << QString("C%1    # iac SDU[%3]\n")
                            .arg(mConsCounter)
                            .arg(mvvImplicitAreaConsSegments[iac][0]);
        iacCStr << QString("n0\n");
    }
    iacJStr.flush();
    iacCStr.flush();
    ++mJAccessCounter;
    ++mCAccessCounter;


    // ..............................................
    // explicit area constraints
    QFile* eacfc_seg = nullptr;
    if (!createSegmentFile(eacfc_seg, NMMosra::NL_SEG_C, "eac"))
    {
        return false;
    }
    mvfC_seg.push_back(eacfc_seg);
    QTextStream eaccStr(mvfC_seg[mCAccessCounter]);

    QFile* eacfj_seg = nullptr;
    if (!createSegmentFile(eacfj_seg, NMMosra::NL_SEG_J, "eac"))
    {
        return false;
    }
    mvfJ_seg.push_back(eacfj_seg);
    QTextStream eacjStr(mvfJ_seg[mJAccessCounter]);

    QMap<QString, QStringList>::const_iterator it = this->mmslAreaCons.constBegin();
    for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r, ++mConsCounter)
         //, ++mCAccessCounter, ++mJAccessCounter)
    {

        const QString consLabel = QString("    #%1").arg(it.key());

        eaccStr << QString("C%1    # %2\n").arg(mConsCounter).arg(it.key());
        eaccStr << QString("n0\n");


        // body <= rhs
        if (vnConsType.at(r) == 1)
        {
            rhsStr << 1 << sp << vdRHS.at(r) << consLabel << eol;
        }
        // body >= rhs
        else if (vnConsType.at(r) == 2)
        {
            rhsStr << 2 << sp << vdRHS.at(r) << consLabel << eol;
        }
        // body = rhs
        else if (vnConsType.at(r) == 3)
        {
            rhsStr << 4 << sp << vdRHS.at(r) << consLabel << eol;
        }
        // no constraints on body
        else
        {
            rhsStr << 3 << eol;
        }


        // .................... J seg ....................

        std::vector<size_t> luOff = mvExplicitAreaConsVarOffsets.at(r);
        eacjStr << QString("J%1 %2    # %3\n").arg(mConsCounter).arg(luOff.size()).arg(it.key());

        // sort offsets in ascending order
        std::sort(luOff.begin(), luOff.end());

        for (int lu=0; lu < luOff.size(); ++lu)
        {
            eacjStr << luOff[lu] << sp << 1 << eol;
        }
    }
    eaccStr.flush();
    eacjStr.flush();
    ++mCAccessCounter;
    ++mJAccessCounter;

    // ....................................................
    // FeatureSet Constraints
    QFile* fscc_seg = nullptr;
    if (!createSegmentFile(fscc_seg, NMMosra::NL_SEG_C, "fsc"))
    {
        return false;
    }
    mvfC_seg.push_back(fscc_seg);
    QTextStream fsccStr(mvfC_seg[mCAccessCounter]);

    QFile* fscj_seg = nullptr;
    if (!createSegmentFile(fscj_seg, NMMosra::NL_SEG_J, "fsc"))
    {
        return false;
    }
    mvfJ_seg.push_back(fscj_seg);
    QTextStream fscjStr(mvfJ_seg[mJAccessCounter]);


    QMap<QString, QStringList>::ConstIterator fsIt = this->mslFeatSetCons.constBegin();
    for (int f=0; fsIt != this->mslFeatSetCons.constEnd(); ++fsIt, ++f)
    {
        QStringList optColPair = fsIt.key().split(":", Qt::SkipEmptyParts);

        QMap<size_t, QStringList>::ConstIterator fsidIt = mvvFSCUniqueIDSegmentRHSMap.at(f).constBegin();
        for (int fid=0; fsidIt != mvvFSCUniqueIDSegmentRHSMap.at(f).constEnd();
                 ++fid, ++fsidIt, ++mConsCounter
             //, ++mCAccessCounter, ++mJAccessCounter
             )
        {
            const QString consLabel = QString("    #%1 FSC_%2_%3_%4").arg(f)
                                                                  .arg(vsFSCZoneField.at(f))
                                                                  .arg(fsidIt.key())
                                                                  .arg(optColPair.at(0));

            fsccStr << QString("C%1    # %2\n").arg(mConsCounter).arg(consLabel);
            fsccStr << QString("n0\n");

            QString idrhs = fsidIt.value().at(1);
            rhsStr << idrhs << consLabel << eol;

            // .................... J seg ....................

            fscjStr << QString("J%1 %2    # %3\n").arg(mConsCounter)
                                               .arg(mvvFSCUniqueIDCounterMap.at(f)[fsidIt.key()])
                                               .arg(consLabel);

            QString idjstr = fsidIt.value().at(0);
            fscjStr << idjstr;
        }
    }
    fsccStr.flush();
    fscjStr.flush();
    ++mCAccessCounter;
    ++mJAccessCounter;

    // --------------------------------------------------------------------------------

    return true;
}

bool NMMosra::determineHighDimLoopIterLength(
        const QString &eqn,
        otb::SQLiteTable::Pointer &sqltab
        )
{
    if (!mmslHighDimLoopEquations.keys().contains(eqn))
    {
        MosraLogError(<< "No registered high dim loop equation specified!" << std::endl);
        return false;
    }

    QStringList hdims = mmslHighDimLoopEquations[eqn].first;
    if (hdims.last().compare(QStringLiteral("SDU")) != 0)
    {
        hdims << QStringLiteral("SDU");
    }

    QStringList dimCols;
    foreach(const QString& hd, hdims)
    {
        if (mmslParameters.constFind(hd) != mmslParameters.cend())
        {
            dimCols << mmslParameters[hd].at(0);
        }
        else
        {
            MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: Failed "
                          << "fetching table column for dimension parameter '"
                          << hd.toStdString() << "'!");
            return false;
        }
    }


    const int numHighDims = dimCols.size();
    for (int dims=0; dims < numHighDims; ++dims)
    {
        std::stringstream sqlstr, gbcl;
        sqlstr << "select ";
        gbcl << "group by ";

        std::vector<std::vector<otb::AttributeTable::ColumnValue> > tab;
        std::vector<otb::AttributeTable::TableColumnType> coltypes;

        otb::AttributeTable::TableColumnType tct = otb::AttributeTable::ATTYPE_INT;

        // numHighDims - dims = 3, 2, 1
        const int ncols = numHighDims - dims;
        for (int len=0; len < ncols ; ++len)
        {
            if (ncols == 1)
            {
                sqlstr << "count(distinct \""
                       << dimCols.at(len).toStdString()
                       << "\")";
                coltypes.push_back(tct);
            }
            else if (len == ncols-1)
            {
                sqlstr << "count(\"" << dimCols.at(len).toStdString()
                       << "\")";
                coltypes.push_back(tct);
            }
            else
            {
                sqlstr << "\"" << dimCols.at(len).toStdString()
                       << "\"";
                coltypes.push_back(tct);
            }

            // fmu, rowidx
            if (ncols > 1 && len < ncols-1)
            {
                gbcl << "\"" << dimCols.at(len).toStdString()
                     << "\"";
            }

            if (len < ncols-1)
            {
                sqlstr << ", ";
            }

            //   0      1    2      3
            // region, fmu, farm, rowidx
            if (ncols > 2 && len < ncols-2)
            {
                gbcl   << ", ";
            }
        }

        // complete the query string
        sqlstr << " from \"" << sqltab->GetTableName() << "\" "
               << " where " << msOptFeatures.toStdString() << " == 1 ";
        if (ncols > 1)
        {
            sqlstr << gbcl.str();
        }

        std::string thequery = sqlstr.str();

        if (!sqltab->TableDataFetch(tab, coltypes, thequery))
        {
            MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Round #" << dims << " populating HighDimLoopIterLength failed: "
                          << thequery);
            return false;
        }

        for (int row=0; row < tab.size(); ++row)
        {
            QVector<long long> key;
            for (int col=0; col < tab[0].size()-1; ++col)
            {
                key.push_back(tab[row][col].ival);
            }
            // skip empty keys (might be some null values around ...
            if (key.size() > 0)
            {
                mmvHighDimLoopIterLength.insert(key, tab[row][tab[0].size()-1].ival);
            }
        }

        //NMDebug(<< "--------------- \n")
        //NMDebug(<< "mmvHiDimLoopIterLengths ...\n");
        //auto dimit = mmvHighDimLoopIterLength.cbegin();
        //for (; dimit != mmvHighDimLoopIterLength.cend(); ++dimit)
        //{
        //    for (int c=0; c < dimit.key().size(); ++c)
        //    {
        //        NMDebug(<< dimit.key().at(c) << " => ");
        //    }
        //    NMDebug(<< endl);
        //}
        //NMDebug(<< "END ---------\n\n");
    }

    return true;
}

bool NMMosra::processNLSegmentData2()
{

    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< "ERROR - DataSet is NULL!" << endl);
        return false;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogError(<< "ERROR: DataSet is NULL!" << endl);
        return false;
    }

    // ========================================================
    //          PREPARE LOOPING && EQUATION POPULATION
    // ========================================================
    // create dimension lookup by eqn
    // and
    // create pos lookup by parameter/variable
    // i.e. pos in getnames and getvalues respectively
    // NOTE: OPTIONS dimension is ignored as values
    // are either known already for each option and don't
    // have to be retrieved or they just reference
    // a column in the db

    // how often does a defined variable appear
    // in objectives and constraints (count)
    QMap<QString, int> procVarInObjConsMap;

    // <operand or dimension parameter name>,
    //      < index of associated column name/value in
    //        getnames/getvalues vector >
    QMap<QString, std::vector<size_t> > nameValPosMap;
    std::vector<std::string> getnames;
    std::vector<otb::AttributeTable::ColumnValue> getvalues;

    // track actual columns added to getnames / getvalues
    mmLookupColsFstParam.clear();

    // list of names of equations that need to be populated
    // and written into NL file
    QStringList eqnNames;
    eqnNames << mmslObjectives.keys();
    //eqnNames << mslProcessVars;
    bool bOK;
    double val;
    QString rhs;
    eqnNames << mmslLinearConstraints.keys();
    auto linConRhs = mmslLinearConstraints.cbegin();
    for(;  linConRhs != mmslLinearConstraints.cend(); ++linConRhs)
    {
        rhs = linConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << linConRhs.value().at(1);
        }
    }

    eqnNames << mmslNonLinearConstraints.keys();
    auto nlConRhs = mmslNonLinearConstraints.cbegin();
    for(;  nlConRhs!= mmslNonLinearConstraints.cend(); ++nlConRhs)
    {
        rhs = nlConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << nlConRhs.value().at(1);
        }
    }

    eqnNames << mmslLogicConstraints.keys();
    auto logConRhs = mmslLogicConstraints.cbegin();
    for(;  logConRhs!= mmslLogicConstraints.cend(); ++logConRhs)
    {
        rhs = logConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << logConRhs.value().at(1);
        }
    }

    // <dimension parameter name>,
    //      < set of (distinct) equation names
    //        the parameter appears in >
    //QMap<QString, QSet<QString> > mmDimEqnMap;

    // populate the above maps
    foreach(const QString& name, eqnNames)
    {
        identifyHighDimLoopEquations(name, name);

        if (!createEqnElemAdminMap(
                    mmDimEqnMap,
                    nameValPosMap,
                    procVarInObjConsMap,
                    getnames,
                    getvalues,
                    name)
           )
        {
            return false;
        }
    }


    // ========================================================
    //          POPULATE EQUATIONS
    // ========================================================

    mCAccessCounter = 0;
    mJAccessCounter = 0;
    mConsCounter = 0;
    mObjCounter = 0;
    mLogicConsCounter = 0;

    // ---------------------------------------------------------
    //          LOW-DIM EQUATIONS
    // ---------------------------------------------------------

    populateLowDimAndOtherEquations(
                sqltab,
                nameValPosMap,
                getnames,
                getvalues
                );

    // ---------------------------------------------------------
    //          HIGH-DIM EQUATIONS
    // ---------------------------------------------------------
    foreach(const QString& eqn, mmslHighDimLoopEquations.keys())
    {
        if (mmPrefixEquations.constFind(eqn) == mmPrefixEquations.cend())
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Couldn't find HighDim Equation '"
                          << eqn.toStdString()
                          << "' in EquationAdmins, so we'll skip it!" << endl);
            continue;
        }

        EquationAdmin& pea = mmPrefixEquations[eqn].first;

        // check whether we've got a non-nested for loop
        if (    pea.loopList.size() > 0
             && pea.loopList.at(0).name.compare(QStringLiteral("for"), Qt::CaseInsensitive) == 0
             && pea.loopList.at(0).level == 0
            )
        {
            // bail out if the loop does not encompass the whole equation
            // 0123456789012
            // for{FMU_QL}( )  --> loop preamble = 5 + dim.length
            if (    pea.loopList.at(0).bodyStart > 0
                 || mmslNonLinearConstraints.constFind(eqn) == mmslNonLinearConstraints.cend()
               )
            {
                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - LUMASS PANIC: "
                              << "Bear with me, please, but I've got no clue what you want me to do, really! "
                              << " I can only handle "
                              << " for-loops in constraints and only if they apply to the whole "
                              << " equation!");
                return false;
            }

            // --------------------------------------------------------------------
            // determine number of iterations for each dimension

            if (!determineHighDimLoopIterLength(eqn, sqltab))
            {
                MosraLogError(<< "Uable to determine high-dim loop length!");
                return false;
            }

            /*
            // pepare table fetch of distinct values of the for-loop dimension
            std::vector< std::vector<otb::AttributeTable::ColumnValue> > fortab;
            std::vector<otb::AttributeTable::TableColumnType> fortypes;
            fortypes.push_back(otb::AttributeTable::ATTYPE_INT);

            std::stringstream forsql;
            forsql << "select distinct \"" << pea.loopList.at(0).dim.toStdString() << "\" "
                 << "from \"" << sqltab->GetTableName() << "\" "
                 << "where \"" << msOptFeatures.toStdString() << "\" == 1";

            if (!sqltab->TableDataFetch(fortab, fortypes, forsql.str()))
            {
                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Unable to fetch distinct for-loop dimension values: "
                              << sqltab->getLastLogMsg());
                return false;
            }
            */

            mmLookupColsFstParam.clear();
            QMap<QString, QSet<QString> > hdimEqnMap;
            QMap<QString, std::vector<size_t> > hdValPosMap;
            QMap<QString, int> pvicm;
            std::vector<std::string> hdgetnames;
            std::vector<otb::AttributeTable::ColumnValue> hdgetvalues;

            if (!createEqnElemAdminMap(
                        hdimEqnMap,
                        hdValPosMap,
                        pvicm,
                        hdgetnames,
                        hdgetvalues,
                        eqn))
            {
                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Failed creating EqnElemAdminMap!");
                return false;
            }


            // ---------------------------------------------------------------
            // ------------- DISTINCT VALUE ITERATION ------------------------

            // number of iterations ( == unique values in for-set)
            //const int numForElems = fortab.size() > 0 ? fortab.at(0).size() : 0;

            // iterate over the distinct values in set <for-dim>
            //for (int dfv=0; dfv < numForElems; ++dfv)

            QStringList dims = this->mmslHighDimLoopEquations[eqn].first;

            auto hditer = mmvHighDimLoopIterLength.cbegin();
            for (; hditer != mmvHighDimLoopIterLength.cend(); ++hditer)
            {
                // select set of SDUs for the given distinct for-value/element
                std::stringstream vwclStr;
                vwclStr << "where \"" << msOptFeatures.toStdString() << "\" == 1 ";
                for (int d=0; d < dims.size(); ++d)
                {
                    //if (d < dims.size()-1)
                    //{
                    //    vwclStr << " and ";
                    //}

                    vwclStr << " and \"" << dims.at(d).toStdString() << "\" == " << hditer.key().at(d);
                }

                std::string wcstr = vwclStr.str();

                bool brow = sqltab->PrepareBulkGet(hdgetnames, vwclStr.str());
                if (!brow)
                {
                    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Populating for-loop equation '" << eqn.toStdString() << "' "
                                  << "failed: " << sqltab->getLastLogMsg());
                    sqltab->EndTransaction();
                    return false;
                }

                sqltab->BeginTransaction();
                brow = sqltab->DoBulkGet(hdgetvalues);
                if (!brow)
                {
                    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Populating for-loop equation '" << eqn.toStdString() << "' "
                                  << "failed: " << sqltab->getLastLogMsg());
                    sqltab->EndTransaction();
                    return false;
                }

/// TODO!! - the following piece of code is a modified copy from ::processNLSegmentData() which also
/// appears in this or a slightly modified form in ::populateLowDimAndOtherEquations!
/// very bad practice, low maintainability, error prone --> nees to be rectified with more time

                // -----------------------------------------------------------

                // clear HighDimValueComboTracker for this equation
                mmslHighDimValComboTracker[eqn].clear();

                // create lookup for last value of a dimension
                // dimValueMap: NOTE: OPTIONS values for
                // variable offset look up are stored 0-based!
                QMap<QString, double> dimValueMap;
                QMap<QString, bool> dimChangeMap;

                const QStringList dimensions = hdimEqnMap.keys();
                foreach(const QString& td, dimensions)
                {
                    dimValueMap[td] = -1;
                    dimChangeMap[td] = false;
                }

                // this map is empty to begin with, if an inner
                // loop is iterated over multiple times, the
                // after the last iteration, the loop counter
                // is reset to '0'

                //<eqn name>, <loop dim, counter value 1-based!> >
                QMap<QString, QMap<QString, int> > eqnLoopCounterMap;

                // set the eqn's active segment map to account for the leading for loop
                QMap<QString, QStack<std::vector<int> > > eqnActiveSegmentMap;
                QStack<std::vector<int> > segStack;
                std::vector<int> aseg = {pea.loopList.at(0).bodyStart+1, mmPrefixEquations[eqn].second.size()-1};
                segStack.push_back(aseg);
                eqnActiveSegmentMap.insert(eqn, segStack);

                long recCounter = 0;
                do
                {
                    // -----------------------------------------------------
                    //          HAVE DIMENSION VALUES CHANGED?
                    // get dimension values
                    foreach (const QString& dim, dimensions)
                    {
                        // as OPTIONS differ even at the smallest spatial
                        // dimension, i.e. at SDU level, we trigger processing
                        // each time it appears in an equation
                        if (dim.compare(msOPTIONS) == 0)
                        {
                            dimValueMap[dim] = -9999;
                            dimChangeMap[dim] = true;
                            continue;
                        }

                        double dimValue = getRecordValue(
                                    hdValPosMap,
                                    hdgetvalues,
                                    dim
                                    );

                        if (dimValue != dimValueMap[dim])
                        {
                            dimValueMap[dim] = dimValue;
                            dimChangeMap[dim] = true;
                        }
                        else
                        {
                            dimChangeMap[dim] = false;
                        }
                    }

                    // --------------------------------------------------
                    //          populate equation

                    //auto dimChMapIt = dimChangeMap.cbegin();
                    //for(; dimChMapIt != dimChangeMap.cend(); ++dimChMapIt)
                    {
                        //if (dimChMapIt.value() == true)
                        {

                            // track high dim combinations
                            //QVector<long long> highDimValues;
                            ////foreach(const QString& hdim, mmHighDimEqnMap[eqn])
                            //foreach(const QString& hdim, mmslHighDimLoopEquations[eqn].first)
                            //{
                            //    highDimValues.push_back(dimValueMap[hdim]);
                            //}

                            //if (    mmslHighDimValComboTracker[eqn].constFind(highDimValues)
                            //     != mmslHighDimValComboTracker[eqn].cend()
                            //   )
                            //{
                            //    continue;
                            //}
                            //else
                            //{
                            //    mmslHighDimValComboTracker[eqn].insert(highDimValues);
                            //}

                            // ---------------------------------------------
                            //  C    (Non-linear) algebraic constraints
                            // ---------------------------------------------
                            if (    mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                                 || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                               )
                            {
                                // ATTENTION:
                                // - if we've got a constraint defined over OPTIONS (for at least one parameter,
                                //   variable, or loop) and
                                // - we don't have a loop at the beginning of the constraint,
                                // - and we can find the OPTIONS dimension parameter somewhere in the eqn
                                //   that indicates that this eqn does not just apply to a single element
                                //   of OPTIONS
                                // ==> we assume that this constraint is meant to be defined
                                //     individually for each <other dims> + OPTIONS combination ...

                                // set the eqn's active segment map


                                if (mmDimEqnMap[msOPTIONS].constFind(eqn) != mmDimEqnMap[msOPTIONS].cend())
                                {
                                    if (    mmEquationAdmins[eqn].elemMap[0].first != EQN_LOOP
                                         && mmsEquations[eqn].contains(msOPTIONS)
                                       )
                                    {
                                        for (int opt=0; opt < mslOptions.size(); ++opt)
                                        {
                                            dimValueMap[msOPTIONS] = opt;

                                            if (!populateEquations(
                                                        eqn,
                                                        eqnActiveSegmentMap,
                                                        0,
                                                        NL_SEG_C,
                                                        mConsCounter,
                                                        eqnLoopCounterMap,
                                                        dimValueMap,
                                                        hdValPosMap,
                                                        hdgetvalues
                                                        )
                                               )
                                            {
                                                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                                              << "Populating for-loop equation '" << eqn.toStdString() << "' "
                                                              << "failed: " << sqltab->getLastLogMsg());
                                                sqltab->EndTransaction();
                                                return false;
                                            }
                                        }

                                        dimValueMap[msOPTIONS] = -9999;
                                    }
                                }
                                else
                                {
                                    if (!populateEquations(
                                                eqn,
                                                eqnActiveSegmentMap,
                                                0,
                                                NL_SEG_C,
                                                mConsCounter,
                                                eqnLoopCounterMap,
                                                dimValueMap,
                                                hdValPosMap,
                                                hdgetvalues
                                                )
                                       )
                                    {
                                        MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                        sqltab->EndTransaction();
                                        return false;
                                    }
                                }
                            }
                        }
                    }



                    // get new record with values
                    brow = sqltab->DoBulkGet(hdgetvalues);
                    ++recCounter;

                } while (brow);

                sqltab->EndTransaction();

                MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - INFO: "
                              << "processed " << recCounter << " records while "
                              << "poplulating the equations!" << endl);







                // for each set create a new constraint
                   //-> - process equation with sums and all

            }

        }
        // HIGH DIM SUM EQUATION?
        else
        {
            MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - INFO: "
                          << "Asked to process eqn '" << pea.name.toStdString() << "' but don't a clue how!"
                          << " We'll skip the action on this one and see how it goes - shall we?");
        }
    }

    // ========================================================


    /*
    // ..............................................
    // implicit area constraints
    for (int iac=0; iac < mvvImplicitAreaConsSegments.size(); ++iac, ++mConsCounter)
    {

        // constraint body
        QString j_seg = mvvImplicitAreaConsSegments.at(iac).at(1);
        j_seg.prepend(QString("J%1 %2   # iac SDU[%3]\n")
                        .arg(mConsCounter)
                        .arg(mslOptions.size())
                        .arg(mvvImplicitAreaConsSegments[iac][0])
                      );
        mvJ_seg.push_back(j_seg);

        // rhs
        rhsStr << mvvImplicitAreaConsSegments[iac][2];

        // corresponding non-linear C part
        QString c_seg;
        mvC_seg.push_back(c_seg);
        QTextStream cStr(&mvC_seg[mConsCounter]);

        cStr << QString("C%1    # iac SDU[%3]\n")
                            .arg(mConsCounter)
                            .arg(mvvImplicitAreaConsSegments[iac][0]);
        cStr << QString("n0\n");
    }
    */


    // ---------------------------------------------------------
    //      k Segment - Jacobian column count

    QTextStream kStr(&mk_seg);

    const int klen = mNonZeroJColCount.size()-1;
    kStr << QString("k%1    # cumulative non-zeros in Jacobian\n").arg(klen);
    long cumVar = 0;
    auto jcolit = mNonZeroJColCount.constBegin();
    for (int k=0; k < klen; ++k, ++jcolit)
    {
        cumVar += jcolit.value();
        kStr << cumVar << Qt::endl;
    }

    return true;
}

/*
bool NMMosra::processNLSegmentData()
{

    QChar eol('\n');
    QChar sp(' ');

    if (this->mDataSet == nullptr)
    {
        MosraLogError(<< "ERROR - DataSet is NULL!" << endl);
        return false;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogError(<< "ERROR: DataSet is NULL!" << endl);
        return false;
    }

    // ========================================================
    //          PREPARE LOOPING && EQUATION POPULATION
    // ========================================================
    // create dimension lookup by eqn
    // and
    // create pos lookup by parameter/variable
    // i.e. pos in getnames and getvalues respectively
    // NOTE: OPTIONS dimension is ignored as values
    // are either known already for each option and don't
    // have to be retrieved or they just reference
    // a column in the db

    // how often does a defined variable appear
    // in objectives and constraints (count)
    QMap<QString, int> procVarInObjConsMap;

    // <operand or dimension parameter name>,
    //      < index of associated column name/value in
    //        getnames/getvalues vector >
    QMap<QString, std::vector<size_t> > nameValPosMap;
    std::vector<std::string> getnames;
    std::vector<otb::AttributeTable::ColumnValue> getvalues;

    // track actual columns added to getnames / getvalues
    mmLookupColsFstParam.clear();

    // list of names of equations that need to be populated
    // and written into NL file
    QStringList eqnNames;
    eqnNames << mmslObjectives.keys();
    //eqnNames << mslProcessVars;
    bool bOK;
    double val;
    QString rhs;
    eqnNames << mmslLinearConstraints.keys();
    auto linConRhs = mmslLinearConstraints.cbegin();
    for(;  linConRhs != mmslLinearConstraints.cend(); ++linConRhs)
    {
        rhs = linConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << linConRhs.value().at(1);
        }
    }

    eqnNames << mmslNonLinearConstraints.keys();
    auto nlConRhs = mmslNonLinearConstraints.cbegin();
    for(;  nlConRhs!= mmslNonLinearConstraints.cend(); ++nlConRhs)
    {
        rhs = nlConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << nlConRhs.value().at(1);
        }
    }

    eqnNames << mmslLogicConstraints.keys();
    auto logConRhs = mmslLogicConstraints.cbegin();
    for(;  logConRhs!= mmslLogicConstraints.cend(); ++logConRhs)
    {
        rhs = logConRhs.value().at(1);
        val = rhs.toDouble(&bOK);
        if (!bOK)
        {
            eqnNames << logConRhs.value().at(1);
        }
    }

    // <dimension parameter name>,
    //      < set of (distinct) equation names
    //        the parameter appears in >
    //QMap<QString, QSet<QString> > mmDimEqnMap;

    // populate the above maps
    foreach(const QString& name, eqnNames)
    {
        if (!createEqnElemAdminMap(
                    mmDimEqnMap,
                    nameValPosMap,
                    procVarInObjConsMap,
                    getnames,
                    getvalues,
                    name)
           )
        {
            return false;
        }
    }

    // ========================================================
    // create data structure for higher equation dim combos
    // ========================================================
    //const QStringList lowDims = {msSDU, msOPTIONS};
    const QStringList dimensions = mmDimEqnMap.keys();


    //std::vector<std::string> highDimNames;
    //std::vector<otb::AttributeTable::ColumnValue> highDimValues;

    //std::vector<otb::AttributeTable::TableColumnType> colTypes;
    //std::vector<std::vector<otb::AttributeTable::ColumnValue> > colValues;


    //otb::SQLiteTable::Pointer uctab = otb::SQLiteTable::New();
    //const QString ucfn = QString("%1/uctab_%2.ldb").arg(this->msDataPath).arg(uctab->GetRandomString(8).c_str());
    //uctab->SetTableName("uctab");
    //if (uctab->CreateTable(ucfn.toStdString()) != otb::SQLiteTable::ATCREATE_CREATED)
    //{
    //    sqltab->EndTransaction();
    //    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
    //                  << "Couldn't create in-memory helper DB!" << std::endl);
    //    return false;
    //}

    //uctab->BeginTransaction();
    //std::stringstream ucstr;
    //ucstr << "select count(*) from " << uctab->GetTableName()
    //      << " where ";

    //int highdimcnt = 0;
    //foreach(const QString& adim, dimensions)
    //{
    //    if (!lowDims.contains(adim))
    //    {
    //        if (!uctab->AddColumn(adim.toStdString(), otb::AttributeTable::ATTYPE_INT))
    //        {
    //            uctab->EndTransaction();
    //            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
    //                          << "Couldn't add column '" << adim.toStdString()
    //                          << "' to in-memory helper DB!" << std::endl);
    //            return false;
    //        }

    //        ucstr << adim.toStdString() << " == ? ";
    //        if (highdimcnt < dimensions.size()-1 - lowDims.size())
    //        {
    //            ucstr << "and ";
    //        }

    //        highDimNames.push_back(adim.toStdString());
    //        otb::AttributeTable::ColumnValue hdval;
    //        hdval.type = otb::AttributeTable::ATTYPE_INT;
    //        highDimValues.push_back(hdval);

    //        otb::AttributeTable::TableColumnType tcol = otb::AttributeTable::ATTYPE_INT;
    //        colTypes.push_back(tcol);


    //        ++highdimcnt;
    //    }
    //}
    //uctab->EndTransaction();
    //const std::string rowCountWhereClause = ucstr.str();

    // --------------------------------------------------------
    //      prepare linear explict area constraints
    // --------------------------------------------------------

    std::vector<QString> vsConsLabel;
    std::vector<int> vnZoneLength;
    std::vector<QString> vsZoneField;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    if (!processExplicitArealCons(vsConsLabel, vnZoneLength, vsZoneField,
                             vvnOptionIndex, vnConsType, vdRHS))
    {
        return false;
    }

    // add zone fields to getnames & getvalues
    for (int z=0; z < vsZoneField.size(); ++z)
    {
        if (    !vsZoneField.at(z).isEmpty()
             && nameValPosMap.constFind(vsZoneField[z]) == nameValPosMap.cend()
           )
        {
            std::vector<size_t> zfoff = {getnames.size()};
            nameValPosMap.insert(vsZoneField[z], zfoff);

            getnames.push_back(vsZoneField[z].toStdString());
            otb::AttributeTable::ColumnValue zcv;
            getvalues.push_back(zcv);
        }
    }
    // make sure we've got the SDU field (rowidx) in the namValPosMap
    if (nameValPosMap.constFind(msSDU) == nameValPosMap.cend())
    {
        std::vector<size_t> rowoff = {getnames.size()};
        nameValPosMap.insert(msSDU, rowoff);

        getnames.push_back(msSDU.toStdString());
        otb::AttributeTable::ColumnValue rowval;
        getvalues.push_back(rowval);
    }

    // add the AreaHa field to the parameters to be fetched
    const int areaValueIdx = getnames.size();
    getnames.push_back(msAreaField.toStdString());
    otb::AttributeTable::ColumnValue areaColVal;
    getvalues.push_back(areaColVal);

    // -----------------------------------------------------------
    // create lookup for last value of a dimension

    // dimValueMap: NOTE: OPTIONS values for
    // variable offset look up are stored 0-based!
    QMap<QString, double> dimValueMap;
    QMap<QString, bool> dimChangeMap;

    // const QStringList dimensions = mmDimEqnMap.keys();
    //      defined above
    foreach(const QString& td, dimensions)
    {
        dimValueMap[td] = -1;
        dimChangeMap[td] = false;
    }

    // fetch the first record of the optimisation table/dataset
    std::stringstream whereClause;
    whereClause << "where \"" << msOptFeatures.toStdString() << "\" == 1";
    bool brow = sqltab->PrepareBulkGet(getnames, whereClause.str());
    if (!brow)
    {
        MosraLogError(<< "NMMosra::processNLSegementData() - ERROR: "
                      << "Populating equations failed: "
                      << sqltab->getLastLogMsg());
        sqltab->EndTransaction();
        return false;
    }

    sqltab->BeginTransaction();
    brow = sqltab->DoBulkGet(getvalues);

    if (!brow)
    {
        MosraLogError(<< "NMMosra::processNLSegmentData() - ERROR: "
                      << "Populating equations failed: "
                      << sqltab->getLastLogMsg());
        sqltab->EndTransaction();
        return false;
    }


    // start transaction on mem db for fast retrieval & insert
    //uctab->BeginTransaction();
    //uctab->PrepareRowCount(rowCountWhereClause);
    //uctab->PrepareBulkSet(highDimNames, true);

    // ========================================================
    //          POPULATE EQUATIONS RECORD by RECORD
    // ========================================================
    mConsCounter = 0;
    mObjCounter = 0;
    mLogicConsCounter = 0;

    // this map is empty to begin with, if an inner
    // loop is iterated over multiple times, the
    // after the last iteration, the loop counter
    // is reset to '0'

    //<eqn name>, <loop dim, counter value 1-based!> >
    QMap<QString, QMap<QString, int> > eqnLoopCounterMap;
    QMap<QString, QStack<std::vector<int> > > eqnActiveSegmentMap;
    long recCounter = 0;
    do
    {
        // -----------------------------------------------------
        //          HAVE DIMENSION VALUES CHANGED?
        // get dimension values
        long dimcnt = 0;
        foreach (const QString& dim, dimensions)
        {
            // as OPTIONS differ even at the smallest spatial
            // dimension, i.e. at SDU level, we trigger processing
            // each time it appears in an equation
            if (dim.compare(msOPTIONS) == 0)
            {
                dimValueMap[dim] = -9999;
                dimChangeMap[dim] = true;
                continue;
            }

            double dimValue = getRecordValue(
                        nameValPosMap,
                        getvalues,
                        dim
                        );

            if (dimValue != dimValueMap[dim])
            {
                dimValueMap[dim] = dimValue;
                dimChangeMap[dim] = true;
            }
            else
            {
                dimChangeMap[dim] = false;
            }
        }


        //QString queryStr = QString("%1").arg(ucstr.str().c_str());
        //std::string ucquery = ucstr.str();

        //// check for higher dims
        //long long highDimRowCount = -1;
        ////if (!uctab->DoRowCount(highDimValues, highDimRowCount))
        ////{
        ////    sqltab->EndTransaction();
        ////    //uctab->EndTransaction();
        ////    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
        ////                  << "Querying in-memory helper DB for higher dimension values failed!" << std::endl);
        ////    return false;
        ////
        ////}
        //if (!uctab->TableDataFetch(colValues, colTypes, queryStr.toStdString()))
        //{
        //    MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
        //                  << "Querying in-memory helper DB for higher dimension values failed: "
        //                  << uctab->getLastLogMsg() << std::endl);
        //    sqltab->EndTransaction();
        //    //uctab->EndTransaction();

        //    return false;
        //}


        //const long ucnr = uctab->GetNumRows();
        //MosraLogDebug(<< "uctab #recs = " << ucnr << std::endl);

        //if (colValues.size() > 0)
        //{
        //    if (colValues.at(0).size() > 0)
        //    {
        //        highDimRowCount = colValues.at(0).at(0).ival;
        //    }
        //}


        // -----------------------------------------------------
        //          populate eqns

        QStringList processedEqns;
        auto dimChMapIt = dimChangeMap.cbegin();
        for(; dimChMapIt != dimChangeMap.cend(); ++dimChMapIt)
        {
            if (dimChMapIt.value() == true)
            {
                auto dimEqnIt = mmDimEqnMap.constFind(dimChMapIt.key());

                // keep moving, if we can't find the equation
                if (dimEqnIt == mmDimEqnMap.cend())
                {
                    continue;
                }

                foreach(const QString& eqn, dimEqnIt.value())
                {
                    // keep moving if we've processed this eqn already
                    if (processedEqns.contains(eqn))
                    {
                        continue;
                    }

                    // if the equation is only evaluated on 'higher' (i.e. above SDU)
                    // dimensions, we double check whether we have actually already
                    // processed this specific combination of dimension values,
                    // in which case we'd have found a record for the given combination
                    // of higher dimension values ...
                    if (    mmHighDimEqnMap.contains(eqn)
                         && (   mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                             || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                             || mmslObjectives.constFind(eqn) != mmslObjectives.cend()
                            )
                       )
                    {
                        QVector<long long> highDimValues;
                        foreach(const QString& hdim, mmHighDimEqnMap[eqn])
                        {
                            highDimValues.push_back(dimValueMap[hdim]);
                        }

                        if (    mmslHighDimValComboTracker[eqn].constFind(highDimValues)
                             != mmslHighDimValComboTracker[eqn].cend()
                           )
                        {
                            continue;
                        }
                        else
                        {
                            mmslHighDimValComboTracker[eqn].insert(highDimValues);
                        }
                    }

                    // ---------------------------------------------
                    //  C    (Non-linear) algebraic constraints
                    // ---------------------------------------------
                    if (    mmslNonLinearConstraints.constFind(eqn) != mmslNonLinearConstraints.cend()
                         || mmslLinearConstraints.constFind(eqn) != mmslLinearConstraints.cend()
                         //|| mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend()
                       )
                    {

                        //if (mmslLogicConstraints.constFind(eqn) != mmslLogicConstraints.cend())
                        //{
                        //    if (!populateEquations(
                        //                eqn,
                        //                eqnActiveSegmentMap,
                        //                0,
                        //                NL_SEG_L,
                        //                mLogicConsCounter,
                        //                eqnLoopCounterMap,
                        //                dimValueMap,
                        //                nameValPosMap,
                        //                getvalues
                        //                )
                        //       )
                        //    {
                        //        MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                        //        sqltab->EndTransaction();
                        //        return false;
                        //    }
                        //}
                        //else


                        // ATTENTION:
                        // - if we've got a constraint defined over OPTIONS (for at least one par,
                        //   var, or loop) and
                        // - we don't have a loop at thAre you available this Thursday or Friday? e beginning of the constraint,
                        // - and we can find the OPTIONS dimension parameter somewhere in the eqn
                        //   that indicates that this eqn does not just apply to a single element
                        //   of OPTIONS
                        // ==> we assume that this constraint is meant to be defined
                        //     individually for each <other dims> + OPTIONS combination ...

                        if (mmDimEqnMap[msOPTIONS].constFind(eqn) != mmDimEqnMap[msOPTIONS].cend())
                        {
                            if (    mmEquationAdmins[eqn].elemMap[0].first != EQN_LOOP
                                 && mmsEquations[eqn].contains(msOPTIONS)
                               )
                            {
                                for (int opt=0; opt < mslOptions.size(); ++opt)
                                {
                                    dimValueMap[msOPTIONS] = opt;
// do we need to add an opt counter to the eqnLoopCounterMap? would that overlap with a potential other options counter?
                                    if (!populateEquations(
                                                eqn,
                                                eqnActiveSegmentMap,
                                                0,
                                                NL_SEG_C,
                                                mConsCounter,
                                                eqnLoopCounterMap,
                                                dimValueMap,
                                                nameValPosMap,
                                                getvalues
                                                )
                                       )
                                    {
                                        MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                        sqltab->EndTransaction();
                                        return false;
                                    }
                                }

                                dimValueMap[msOPTIONS] = -9999;
                            }
                        }
                        else
                        {
                            if (!populateEquations(
                                        eqn,
                                        eqnActiveSegmentMap,
                                        0,
                                        NL_SEG_C,
                                        mConsCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                               )
                            {
                                MosraLogError(<< "ERROR in constraint eqn!" << std::endl);
                                sqltab->EndTransaction();
                                return false;
                            }

                        }


                        // book keeping
                        processedEqns << eqn;
                    }

                    // --------------------------------------------------
                    //  O    Non-linear algebraic OBJECTIVE segment
                    // --------------------------------------------------
                    if (mmslObjectives.constFind(eqn) != mmslObjectives.cend())
                    {
                        if (!populateEquations(
                                    eqn,
                                    eqnActiveSegmentMap,
                                    0,
                                    NL_SEG_O,
                                    mObjCounter,
                                    eqnLoopCounterMap,
                                    dimValueMap,
                                    nameValPosMap,
                                    getvalues
                                    )
                           )
                        {
                            MosraLogError(<< "ERROR in objctive eqn!" << std::endl);
                            sqltab->EndTransaction();
                            return false;
                        }
                        processedEqns << eqn;
                    }
                }
            }
        }

        // --------------------------------------------------
        //  process values for J linear explicit area constraints
        // --------------------------------------------------
        QMap<QString, QStringList>::const_iterator it = this->mmslAreaCons.constBegin();
        for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r)
        {
            QStringList luOptions = mslOptions;
            if (!vsZoneField.at(r).isEmpty())
            {
                const QString zoneStr = getvalues[nameValPosMap[vsZoneField[r]][0]].tval;
                luOptions = zoneStr.split(" ", Qt::SkipEmptyParts);
            }

            std::vector<size_t> vLuOff;
            for (int opt=0; opt < vvnOptionIndex.at(r).size(); ++opt)
            {
                const int luIdx = vvnOptionIndex.at(r).at(opt);
                const QVector<long long> luDims = {
                            static_cast<long long>(dimValueMap[msSDU]),
                            static_cast<long long>(luIdx)
                            };

                const long long luOffset = mmVarAdminMap[QStringLiteral("lu")].dimOffsetMap[luDims];
                if (mNonZeroJColCount.constFind(luOffset) == mNonZeroJColCount.cend())
                {
                    assert(0);
                }

                if (luOptions.contains(mslOptions.at(luIdx)))
                {
                    mvExplicitAreaConsVarOffsets[r].push_back(luOffset);
                    mNonZeroJColCount[luOffset] += 1;
                }
            }
        }


        //// --------------------------------------------------
        ////  process values for J linear implicit area constraints
        //// --------------------------------------------------
        //// stores three strings per SDU
        //// 0: SDU (rowidx) dim value
        //// 1: J segment body
        //// 2: rhs segement body
        //QString sduVal;
        //QString jseg  ;
        //QString rhsseg;
        //for (int opt=0; opt < mslOptions.size(); ++opt)
        //{
        //    const QVector<long long> iacDim = {
        //                static_cast<long long>(dimValueMap[msSDU]),
        //                static_cast<long long>(opt)
        //                };

        //    const long long iacOffset = mmVarAdminMap[QStringLiteral("lu")].dimOffsetMap[iacDim];

        //    double areahaval = 0;
        //    const otb::AttributeTable::TableColumnType tct = getvalues[areaValueIdx].type;
        //    switch(tct)
        //    {
        //    case otb::AttributeTable::ATTYPE_INT:
        //        areahaval = getvalues[areaValueIdx].ival;
        //        break;

        //    case otb::AttributeTable::ATTYPE_DOUBLE:
        //        areahaval = getvalues[areaValueIdx].dval;
        //        break;
        //    default: ;
        //    }

        //    if (opt == 0)
        //    {
        //        sduVal.append(QString("%1").arg(iacDim[0]));
        //        rhsseg.append(QString("1 %1    # SDU[%2]\n")
        //                      .arg(areahaval)
        //                      .arg(iacDim[0])
        //                      );
        //    }
        //    jseg.append(QString("%1 1\n").arg(iacOffset));

        //    if (mNonZeroJColCount.constFind(iacOffset) == mNonZeroJColCount.cend())
        //    {
        //        assert(0);
        //    }
        //    mNonZeroJColCount[iacOffset] += 1;
        //}
        //const std::vector<QString> iacinfo = {sduVal, jseg, rhsseg};
        //mvvImplicitAreaConsSegments.push_back(iacinfo);


        // get new record with values
        brow = sqltab->DoBulkGet(getvalues);
        ++recCounter;

    } while (brow);

    sqltab->EndTransaction();

    MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - INFO: "
                  << "processed " << recCounter << " records while "
                  << "poplulating the equations!" << endl);



    // ---------------------------------------------------------
    //      LINEAR J section

    // ..............................................
    // explicit area constraints

    QTextStream rhsStr(&mr_seg);
    rhsStr.setRealNumberNotation(QTextStream::SmartNotation);
    if (mConsCounter == 0)
    {
        rhsStr << QStringLiteral("r \n");
    }
    QMap<QString, QStringList>::const_iterator it = this->mmslAreaCons.constBegin();
    for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r, ++mConsCounter)
    {
        QString c_seg;
        mvC_seg.push_back(c_seg);
        QTextStream cStr(&mvC_seg[mConsCounter]);

        QString j_seg;
        mvJ_seg.push_back(j_seg);
        QTextStream jStr(&mvJ_seg[mConsCounter]);

        cStr << QString("C%1    # %2\n").arg(mConsCounter).arg(it.key());
        cStr << QString("n0\n");


        // body <= rhs
        if (vnConsType.at(r) == 1)
        {
            rhsStr << 1 << sp << vdRHS.at(r) << eol;
        }
        // body >= rhs
        else if (vnConsType.at(r) == 2)
        {
            rhsStr << 2 << sp << vdRHS.at(r) << eol;
        }
        // body = rhs
        else if (vnConsType.at(r) == 3)
        {
            rhsStr << 4 << sp << vdRHS.at(r) << eol;
        }
        // no constraints on body
        else
        {
            rhsStr << 3 << eol;
        }


        // .................... J seg ....................

        std::vector<size_t> luOff = mvExplicitAreaConsVarOffsets.at(r);
        jStr << QString("J%1 %2    # %3\n").arg(mConsCounter).arg(luOff.size()).arg(it.key());

        // sort offsets in ascending order
        std::sort(luOff.begin(), luOff.end());

        for (int lu=0; lu < luOff.size(); ++lu)
        {
            jStr << luOff[lu] << sp << 1 << eol;
        }
    }



    //// ..............................................
    //// implicit area constraints
    //for (int iac=0; iac < mvvImplicitAreaConsSegments.size(); ++iac, ++mConsCounter)
    //{

    //    // constraint body
    //    QString j_seg = mvvImplicitAreaConsSegments.at(iac).at(1);
    //    j_seg.prepend(QString("J%1 %2   # iac SDU[%3]\n")
    //                    .arg(mConsCounter)
    //                    .arg(mslOptions.size())
    //                    .arg(mvvImplicitAreaConsSegments[iac][0])
    //                  );
    //    mvJ_seg.push_back(j_seg);

    //    // rhs
    //    rhsStr << mvvImplicitAreaConsSegments[iac][2];

    //    // corresponding non-linear C part
    //    QString c_seg;
    //    mvC_seg.push_back(c_seg);
    //    QTextStream cStr(&mvC_seg[mConsCounter]);

    //    cStr << QString("C%1    # iac SDU[%3]\n")
    //                        .arg(mConsCounter)
    //                        .arg(mvvImplicitAreaConsSegments[iac][0]);
    //    cStr << QString("n0\n");
    //}



    // ---------------------------------------------------------
    //      k Segment - Jacobian column count

    QTextStream kStr(&mk_seg);

    const int klen = mNonZeroJColCount.size()-1;
    kStr << QString("k%1    # cumulative non-zeros in Jacobian\n").arg(klen);
    long cumVar = 0;
    auto jcolit = mNonZeroJColCount.constBegin();
    for (int k=0; k < klen; ++k, ++jcolit)
    {
        cumVar += jcolit.value();
        kStr << cumVar << eol;
    }

    return true;
}
*/

bool NMMosra::writeNLSegment(const QString& sseg, QFile* &fseg, const NLSegment &segType, int writeFile)
{
    if (    (writeFile == 1 && (fseg == nullptr || !fseg->isOpen()))
         || (writeFile == 0 && sseg.isEmpty())
         || (writeFile == 0 && segType == NL_SEG_UNKNOWN)
       )
    {
         MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "(): "
                       << "NL segment file/string is not open or empty!" << std::endl);
         return false;
    }

    QString ofn;
    if (writeFile == 0)
    {
        switch(segType)
        {
        case NL_SEG_x: ofn = QString("%1/x_seg.txt").arg(msDataPath); break;
        case NL_SEG_r: ofn = QString("%1/r_seg.txt").arg(msDataPath); break;
        case NL_SEG_b: ofn = QString("%1/b_seg.txt").arg(msDataPath); break;
        case NL_SEG_k: ofn = QString("%1/k_seg.txt").arg(msDataPath); break;
        case NL_SEG_NL: ofn = QString("%1/header_seg.txt").arg(msDataPath); break;
        default:
            ;
        }

        QFile ofile(ofn);
        if (!ofile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Failed writing NL-segement file!" << std::endl);
            return false;
        }

        QTextStream sstr(&ofile);
        sstr << sseg;
        ofile.close();
    }


    if (mpNLFile == nullptr)
    {
        QFileInfo fifo(this->getLosFileName());
        this->msNlFileName = QString("%1/%2.nl").arg(this->msDataPath).arg(fifo.baseName());
        this->msSolFileName = QString("%1/%2.dvars").arg(this->msDataPath).arg(fifo.baseName());

        mpNLFile = new QFile(this->msNlFileName, this);
        if (!mpNLFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)
           )
        {
                MosraLogError(<< "failed creating '" << msNlFileName.toStdString()
                              << "'!");
                return false;
        }
    }
    QTextStream writeNL(mpNLFile);

    if (writeFile)
    {
        if (mpNLFile == nullptr || !mpNLFile->isWritable())
        {
            MosraLogError(<< "mmh, nl file not writable! ...");
            return false;
        }

        if (!fseg->isReadable())
        {
            fseg->close();
            if (!fseg->open(QIODevice::ReadOnly | QIODevice::Text))
            {
                MosraLogError(<< "failed opening '" << fseg->fileName().toStdString()
                              << "' for reading!");
                return false;
            }
        }

        QByteArray fbar;
        for (long long bw=0; bw < fseg->size(); bw += fbar.size())
        {
            fbar = fseg->read(4096);
            if (fbar.size() > 0)
            {
                writeNL << fbar;
            }
            else
            {
                MosraLogError(<< "NL writing ... but there's nothing there! ");
                return false;
            }
        }
        //mpNLFile->flush();
        fseg->close();
    }
    else
    {
        writeNL << sseg;
        //mpNLFile->flush();
    }

    return true;
}

bool NMMosra::populateEquations(
        const QString& eqnName,
        QMap<QString, QStack<std::vector<int> > > &activeSegment,
        int nestingLevel,
        const NLSegment& nlSeg,
        long& segCounter,
        QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
        QMap<QString, double>& dimValueMap,
        QMap<QString, std::vector<size_t> > &nameValPosMap,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues
        )
{
    if (mmPrefixEquations.constFind(eqnName) == mmPrefixEquations.cend())
    {
        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                      << "Mmmh, did we overlook this equation? It is not "
                      << "in our list of (pre-)fixed ones ;-)!" << std::endl);
        return false;
    }

    const QChar eol('\n');
    const QChar sp(' ');

    QTextStream nlStr;
    nlStr.setRealNumberNotation(QTextStream::SmartNotation);
    QTextStream linStr;
    linStr.setRealNumberNotation(QTextStream::SmartNotation);
    QTextStream rhsStr;
    rhsStr.setRealNumberNotation(QTextStream::SmartNotation);

    QString segmentName;
    switch(nlSeg)
    {
    case NL_SEG_C:
        segmentName = QStringLiteral("NL Constraint");
        if (mvfC_seg.size() > 0)
        {
            nlStr.setDevice(mvfC_seg[mCAccessCounter - 1]);
            linStr.setDevice(mvfJ_seg[mJAccessCounter - 1]);
            rhsStr.setString(&mr_seg);
        }
        break;

    case NL_SEG_L:
        segmentName = QStringLiteral("NL Constraint");
        if (mvfL_seg.size() > 0)
        {
            nlStr.setDevice(  mvfL_seg[segCounter - 1]);
            linStr.setDevice( mvfJ_seg[mJAccessCounter - 1]);
            rhsStr.setString(&mr_seg);
        }
        break;

    case NL_SEG_O:
        segmentName = QStringLiteral("NL Objective");
        if (mvfO_seg.size() > 0)
        {
            nlStr.setDevice(  mvfO_seg[segCounter - 1]);
            linStr.setDevice( mvfG_seg[segCounter - 1]);
        }
        break;

    default:
        {
            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                          << "Ooops, how did this segment end up in here? "
                          << "We actually don't support type #" << nlSeg << " !" << std::endl);
            return false;
        }
        break;
    }

    // get the prefix equation element list and
    // associated EquationAdmin
    const EquationAdmin oea = mmPrefixEquations[eqnName].first;
    const QStringList oeqn = mmPrefixEquations[eqnName].second;

    // copy the range as it won't change until processed
    std::vector<int> segrange;
    if (activeSegment.constFind(eqnName) == activeSegment.cend())
    {
        segrange.push_back(0);
        segrange.push_back(mmPrefixEquations[eqnName].second.size()-1);
        QStack<std::vector<int>> rangeStack;
        rangeStack.push_back(segrange);
        activeSegment.insert(eqnName, rangeStack);
    }
    else
    {
        segrange = activeSegment[eqnName].last();
    }


    QStringList removeDimCounter;

    int expectedNewSegStartValue = 0;
    if (    mmPrefixEquations[eqnName].first.loopList.size() > 0
         && mmPrefixEquations[eqnName].first.loopList.at(0).name.compare(QStringLiteral("for"), Qt::CaseInsensitive) == 0
       )
    {
        // 012345678901
        // for{FMU_QL}(
        expectedNewSegStartValue = mmPrefixEquations[eqnName].first.loopList.at(0).bodyStart+1;
    }

    // for given range of eqn elements ...
    for (int oep=segrange[0]; oep <= segrange[1]; ++oep)
    {
        // check whether we need to feed in some
        // data from the db

        // ------------------------------------------------------
        //          NEW NL SEGMENT

        if (    nestingLevel == 0
             && oep == expectedNewSegStartValue
             && eqnLoopCounterMap.constFind(eqnName) == eqnLoopCounterMap.cend()
           )
        {
            //QString nl_seg;
            //QString lin_seg;
            QFile* nl_seg = nullptr;
            QFile* lin_seg = nullptr;

            if (nlSeg == NL_SEG_O || nlSeg == NL_SEG_L)
            {
                if (nlSeg == NL_SEG_L)
                {
                    if (createSegmentFile(nl_seg, nlSeg))
                    {
                        return false;
                    }
                    mvfL_seg.push_back(nl_seg);
                    nlStr.setDevice(mvfL_seg[segCounter]);
                    nlStr << QString("L%1 ").arg(segCounter);
                }
                else
                {
                    if (!createSegmentFile(nl_seg, nlSeg))
                    {
                        return false;
                    }
                    nlStr.setDevice(nl_seg);
                    mvfO_seg.push_back(nl_seg);

                    if (!createSegmentFile(lin_seg, nlSeg))
                    {
                        return false;
                    }
                    //nlStr.setDevice(lin_seg);
                    mvfG_seg.push_back(lin_seg);

                    nlStr << QString("O%1 ").arg(segCounter);
                    //linStr << QString("G%1 1 ").arg(segCounter);

                    // min/max?
                    auto objIt = mmslObjectives.constFind(eqnName);
                    if (objIt != mmslObjectives.cend())
                    {
                        const QString minmaxStr = objIt.value().at(0).left(3);
                        int minmax = 0;
                        if (minmaxStr.contains("max"))
                        {
                            minmax = 1;
                        }
                        nlStr << QString("%1 ").arg(minmax);
                    }
                    else
                    {
                        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                      << "That's not good! We were pretty sure we had and objective here ..."
                                      << " Sorry about that - we got it wrong!" << std::endl);
                        return false;
                    }
                }
            }
            // NL_SEG_C
            else if (nlSeg == NL_SEG_C) //|| nlSeg == NL_SEG_L)
            {
                // -----------------------------------------
                //    CONSTRAINT HEADER AND RANGE BODY

                if (nlSeg == NL_SEG_C)
                {
                    if (!createSegmentFile(nl_seg, nlSeg))
                    {
                        return false;
                    }
                    mvfC_seg.push_back(nl_seg);
                    nlStr.setDevice(mvfC_seg[mCAccessCounter]);
                    nlStr << QString("C%1 ").arg(segCounter);
                    ++mCAccessCounter;
                }
                //else // nlSeg == NL_SEG_L
                //{
                //    mvL_seg.push_back(nl_seg);
                //    nlStr.setString(&mvL_seg[segCounter]);
                //    nlStr << QString("L%1 ").arg(segCounter);
                //}

                //if (!createSegmentFile(lin_seg, nlSeg))
                //{
                //    return false;
                //}
                //mvfJ_seg.push_back(lin_seg);
                //++mJAccessCounter;
                //linStr.setString(&mvJ_seg[segCounter]);
                //linStr << QString("J%1 1 ").arg(segCounter);

                // add constaints' right hand sides (thresholds)
                rhsStr.setString(&mr_seg);
                if (segCounter == 0)
                {
                    rhsStr << QString("r ") << eol;
                }

                QStringList oprhs;
                if (mmslLinearConstraints.contains(eqnName))
                {
                    oprhs = mmslLinearConstraints[eqnName];
                }
                else if (mmslNonLinearConstraints.contains(eqnName))
                {
                    oprhs = mmslNonLinearConstraints[eqnName];
                }
                else if (mmslLogicConstraints.contains(eqnName))
                {
                    oprhs = mmslLogicConstraints[eqnName];
                }

                // -------------------------------------------------
                //     CONSTRAINT THRESHOLDS (RIGHT-HAND SIDES)

                // try converting to double first ...
                bool btd = false;
                double rhsVal = oprhs[1].toDouble(&btd);
                // ... lookup parameter value, if conversion failed
                if (!btd)
                {
                    if (mmPrefixEquations.constFind(oprhs[1]) != mmPrefixEquations.cend())
                    {
                        const EquationAdmin& pea = mmPrefixEquations[oprhs[1]].first;
                        if (pea.paramList.size() == 1)
                        {
                            if (!getParameterValue(
                                        rhsVal,
                                        eqnName,
                                        pea.paramList[0],
                                        getvalues,
                                        nameValPosMap,
                                        eqnLoopCounterMap,
                                        dimValueMap
                                        )
                               )
                            {
                                MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                              << "Lookup of parameter value for '" << pea.paramList[0].name.toStdString()
                                              << "' failed in " << segmentName.toStdString()
                                              << " #" << segCounter << std::endl );
                                return false;
                            }
                        }
                        else
                        {
                            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                          << "Invalid constraint threshold definition '"
                                          << oprhs[1].toStdString()
                                          << "' for contstraint '"
                                          << oprhs[0].toStdString() << " ("
                                          << segmentName.toStdString()
                                          << " #" << segCounter << ")! "
                                          << "Only a single parameter may be specified "
                                          << " as constraint threshold!" << std::endl );
                            return false;

                        }
                    }
                    else
                    {
                        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                      << "Lookup of constraint threshold parameter definition '"
                                      << oprhs[1].toStdString()
                                      << "' failed for contstraint '"
                                      << oprhs[0].toStdString() << " ("
                                      << segmentName.toStdString()
                                      << " #" << segCounter << ")" << std::endl );
                        return false;
                    }
                }


                if (oprhs.size() == 2)
                {
                    if (oprhs[0].compare(QStringLiteral("<=")) == 0)
                    {
                        rhsStr << 1 << sp << rhsVal << "     # " << eqnName << eol;
                    }
                    // body >= rhs
                    else if (oprhs[0].compare(QStringLiteral(">=")) == 0)
                    {
                        rhsStr << 2 << sp << rhsVal << "     # " << eqnName << eol;
                    }
                    // body = rhs
                    else if (oprhs[0].compare(QStringLiteral("=")) == 0)
                    {
                        rhsStr << 4 << sp << rhsVal << "     # " << eqnName << eol;
                    }
                    // no constraints on body
                    else
                    {
                        rhsStr << 3 << "     # " << eqnName << eol;
                    }

                    rhsStr.flush();
                }
            }
            nlStr << "     # " << eqnName;

            if (mmslHighDimLoopEquations.constFind(oea.name) != mmslHighDimLoopEquations.cend())
            {
                auto dvmit = dimValueMap.begin();
                for (; dvmit != dimValueMap.cend(); ++dvmit)
                {
                    nlStr << " " << QString("%1(%2)")
                                        .arg(dvmit.key())
                                        .arg(dvmit.value());
                }
            }
            nlStr << eol;
            nlStr.flush();

            //if (nlSeg != NL_SEG_L)
            //{
            //    linStr << "     # " << eqnName << eol;
            //    linStr << QStringLiteral("0 0") << eol;
            //}

            ++segCounter;
        }

        // -------------------------------------------------------------
        //          PROCESS EQUATION ELEMENTS

        auto oeait = oea.elemMap.constFind(oep);
        if (oeait != oea.elemMap.cend())
        {
            const EqnElement oee = oeait.value().first;
            switch(oee)
            {
            case EQN_LOOP:
                {
                    // get the loop
                    const Loop& ol = oea.loopList[oeait.value().second];
                    long numiter = 0;
                    if (mmslHighDimLoopEquations.constFind(oea.name) != mmslHighDimLoopEquations.cend())
                    {
                        QVector<long long> hdVals;
                        foreach(const QString& hd, mmslHighDimLoopEquations[oea.name].first)
                        {
                            if (dimValueMap.constFind(hd) != dimValueMap.cend())
                            {
                                hdVals.push_back(dimValueMap[hd]);
                            }
                        }

                        if (    hdVals.size() == mmslHighDimLoopEquations[oea.name].first.size()
                             && mmvHighDimLoopIterLength.constFind(hdVals) != mmvHighDimLoopIterLength.cend()
                           )
                        {
                            numiter = mmvHighDimLoopIterLength[hdVals];
                        }

                        // as nested loops in high dim equations are 're-entry' loops, we add the active segment
                        // again, if the loop has not finished yet (and hence is still in the countermap)
                        // NOTE: we need to do this as re-entry loops' active segments are removed after each iteration
                        if (eqnLoopCounterMap.constFind(eqnName) != eqnLoopCounterMap.cend())
                        {
                            if (eqnLoopCounterMap[eqnName].constFind(ol.dim) != eqnLoopCounterMap[eqnName].cend())
                            {
                                std::vector<int> loopseg = {ol.bodyStart, ol.bodyEnd};
                                activeSegment[eqnName].push(loopseg);
                            }
                        }
                    }
                    else if (mmDimLengthMap.constFind(ol.dim) != mmDimLengthMap.cend())
                    {
                        numiter = mmDimLengthMap[ol.dim];
                    }

                    // have we entered this loop already? (yes, if we've tracked a loop with that dimension already ...
                    if (    eqnLoopCounterMap.constFind(eqnName) != eqnLoopCounterMap.cend()
                         && eqnLoopCounterMap.value(eqnName).constFind(ol.dim) != eqnLoopCounterMap.value(eqnName).cend())
                    {
                        eqnLoopCounterMap[eqnName][ol.dim] += 1;

                        if (ol.dim.compare(msOPTIONS) == 0)
                        {
                            dimValueMap[msOPTIONS] += 1;
                        }

                        // check whether we've reached the end of the loop
                        // and reset if applicable
                        if(eqnLoopCounterMap[eqnName][ol.dim] == numiter)
                        {
                            removeDimCounter << ol.dim;
                        }

                        //NMDebug(<< ol.dim.toStdString() << " #" << eqnLoopCounterMap[eqnName][ol.dim] << std::endl);
                    }
                    // ... nope, first time we come across a loop with this dimension
                    // ... however, don't want OPTIONS or HighDim as we count those elsewhere ...
                    else if (    ol.dim.compare(msOPTIONS) != 0
                              && !(    mmslHighDimLoopEquations.constFind(oea.name) != mmslHighDimLoopEquations.cend()
                                    && mmslHighDimLoopEquations[oea.name].first.contains(ol.dim)
                                  )
                            )
                    {
                        if (eqnLoopCounterMap.constFind(eqnName) != eqnLoopCounterMap.cend())
                        {
                            eqnLoopCounterMap[eqnName].insert(ol.dim, 1);
                        }
                        else
                        {
                            QMap<QString, int> imDimCnt;
                            imDimCnt.insert(ol.dim, 1);
                            eqnLoopCounterMap.insert(eqnName, imDimCnt);
                        }

                        // need to adjust the operator depending on the number of operands
                        if (numiter >= 3)
                        {
                            nlStr << QString("o%1\n").arg(mAMPLFunctions["sum"])
                                  << QString("%1\n").arg(numiter);
                        }
                        else if (numiter == 2)
                        {
                            nlStr << QString("o%1\n").arg(mParseOperators["+"]);
                        }
                        nlStr.flush();

                        // limit the active segment of this equation to the loop
                        // until we're done with it
                        std::vector<int> loopseg = {ol.bodyStart, ol.bodyEnd};
                        activeSegment[eqnName].push(loopseg);
                    }
                    // locally loop over OPTIONS section
                    else if (ol.dim.compare(msOPTIONS) == 0)
                    {
                        // need to adjust the operator depending on the number of operands
                        if (numiter >= 3)
                        {
                            nlStr << QString("o%1\n").arg(mAMPLFunctions["sum"])
                                  << QString("%1\n").arg(numiter);
                        }
                        else if (numiter == 2)
                        {
                            nlStr << QString("o%1\n").arg(mParseOperators["+"]);
                        }
                        nlStr.flush();

                        std::vector<int> segrange = {ol.bodyStart+1, ol.bodyEnd};
                        activeSegment[eqnName].push(segrange);

                        for (int opt=0; opt < mslOptions.size(); ++opt)
                        {
                            eqnLoopCounterMap[eqnName][msOPTIONS] = opt+1;
                            dimValueMap[msOPTIONS] = opt;

                            if (!populateEquations(
                                        eqnName,
                                        activeSegment,
                                        nestingLevel+1,
                                        nlSeg,
                                        segCounter,
                                        eqnLoopCounterMap,
                                        dimValueMap,
                                        nameValPosMap,
                                        getvalues
                                        )
                               )
                            {
                                MosraLogError(<< "ERROR in " << segmentName.toStdString() << "!" << std::endl);
                                return false;
                            }
                        }

                        // 'forward' execution to the last step of the loop, i.e.
                        // step we're about to complete ...
                        oep = activeSegment[eqnName].last()[1];

                        // pop the current 'loop-range' range out of the stack
                        activeSegment[eqnName].pop();

                        // reset OPTIONS to -9999
                        dimValueMap[msOPTIONS] = -9999.0;

                        // remove dimension OPTIONS form the eqnLoopCounter list
                        removeLoopCounter(
                               eqnLoopCounterMap,
                               msOPTIONS,
                               eqnName
                                    );
                    }
                }
                break;

            case EQN_PARAMETER:
                {
                    const Param& op = oea.paramList[oeait.value().second];

                    // ----------------------------------------
                    //    VARIABLE

                    if (mslDecisionVars.contains(op.name))
                    {
                        bool bNum = false;
                        long long voff;
                        QVector<long long> dimValues;
                        foreach(const QString& dim, op.dimensions)
                        {
                            // need to double check whether we've got an explicitly
                            // assigned dimension value - and convert that into an actual number,
                            // or whether we got the dimension name
                            double dimVal = dim.toDouble(&bNum);
                            if (bNum)
                            {
                                dimValues.push_back(static_cast<long long>(dimVal-1));
                            }
                            else
                            {
                                dimValues.push_back(static_cast<long long>(dimValueMap[dim]));
                            }
                        }

                        if (    mmVarAdminMap[op.name].dimOffsetMap.constFind(dimValues)
                             != mmVarAdminMap[op.name].dimOffsetMap.cend()
                           )
                        {
                            voff = mmVarAdminMap[op.name].dimOffsetMap[dimValues];
                            nlStr << QString("v%1\n").arg(voff);
                            nlStr.flush();
                        }
                        else
                        {
                            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                          << "Lookup of variable offset for '" << op.name.toStdString()
                                          << "' failed in " << segmentName.toStdString() << " #" << segCounter << std::endl );
                            return false;
                        }

                        if (nlSeg == NL_SEG_O)
                        {
                            if (mvNonLinearObjVarCounter.constFind(voff) == mvNonLinearObjVarCounter.cend())
                            {
                                mvNonLinearObjVarCounter.insert(voff, 1);
                            }
                            else
                            {
                                mvNonLinearObjVarCounter[voff] += 1;
                            }
                        }
                        else
                        {
                            if (mvNonLinearConsVarCounter.constFind(voff) == mvNonLinearConsVarCounter.cend())
                            {
                                mvNonLinearConsVarCounter.insert(voff, 1);
                            }
                            else
                            {
                                mvNonLinearConsVarCounter[voff] += 1;
                            }

                            //if (mNonZeroJColCount.constFind(voff) == mNonZeroJColCount.cend())
                            //{
                            //    mNonZeroJColCount.insert(voff, 1);
                            //}
                            //else
                            //{
                            //    mNonZeroJColCount[voff] += 1;
                            //}
                        }
                    }
                    // -----------------------------------------
                    //    OPERAND or DIMENSION PARAMETER
                    else if (nameValPosMap.constFind(op.name) != nameValPosMap.cend())
                    {
                        // are we part of an iteration over OPTIONS,
                        // and if yes, what iterator value are we?

                        //const std::vector<size_t> offvec = namValPos[op.name];

                        // if we've got more than one offset
                        //long lookupIdx = -1;
                        //double pVal = 0;
                        /*
                        if (nameValPosMap[op.name].size() == mslOptions.size())
                        {
                            // either we've been given the exact 'OPTION' to look for
                            bool bConv = false;
                            foreach(const QString& dim, op.dimensions)
                            {
                                lookupIdx = dim.toDouble(&bConv);
                                if (bConv)
                                {
                                    break;
                                }
                                else
                                {
                                    lookupIdx = -1;
                                }
                            }

                            if (!bConv)
                            {
                                lookupIdx = eqnLoopCounterMap[eqnName][msOPTIONS]-1;
                            }
                        }
                        else
                        {
                            lookupIdx = 0;
                        }

                        bool bError = false;
                        if (lookupIdx >= 0 and lookupIdx < nameValPosMap[op.name].size())
                        {
                            pVal = getRecordValue(
                                        nameValPosMap,
                                        getvalues,
                                        op.name,
                                        lookupIdx
                                        );
                            if (pVal == -9999)
                            {
                                bError = true;
                            }
                            else
                            {
                                nlStr << QString("n%1\n").arg(pVal);
                            }
                        }
                        else
                        {
                            bError = true;
                        }
                        */

                        double pVal = 0;
                        if (!getParameterValue(
                                    pVal,
                                    eqnName,
                                    op,
                                    getvalues,
                                    nameValPosMap,
                                    eqnLoopCounterMap,
                                    dimValueMap
                                    )
                           )
                        {
                            MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                          << "Lookup of parameter value for '" << op.name.toStdString()
                                          << "' failed in " << segmentName.toStdString()
                                          << " #" << segCounter << std::endl );
                            return false;
                        }
                        nlStr << QString("n%1\n").arg(pVal);
                        nlStr.flush();
                    }
                }
                break;

            case EQN_EQUATION:
                {
                    const Equation& nestedEqn = oea.eqnList[oeait.value().second];
                    if (mmPrefixEquations.constFind(nestedEqn.eqn) == mmPrefixEquations.cend())
                    {
                        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                      << "That's odd, I couldn't find the nested equation '" << nestedEqn.eqn.toStdString()
                                      << "' in the prefixed equations list!" << std::endl);
                        return false;
                    }
                    const QStringList& eqnList = mmPrefixEquations[nestedEqn.eqn].second;
                    QStack<std::vector<int> > rangeStack;
                    std::vector<int> actSeg = {0, eqnList.size()-1};
                    rangeStack.push(actSeg);
                    activeSegment.insert(nestedEqn.eqn, rangeStack);

                    // 'forward' any active loops to nested equation, as
                    // it may need access to loop counter values ...
                    auto elcmit = eqnLoopCounterMap.constFind(eqnName);
                    if (elcmit != eqnLoopCounterMap.cend())
                    {
                        eqnLoopCounterMap.insert(nestedEqn.eqn, elcmit.value());
                    }

                    // nested equations ...
                    if (!populateEquations(
                                nestedEqn.eqn,
                                activeSegment,
                                nestingLevel+1,
                                nlSeg,
                                segCounter,
                                eqnLoopCounterMap,
                                dimValueMap,
                                nameValPosMap,
                                getvalues
                                )
                       )
                    {
                        MosraLogError(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                      << "Manno! Wrong again! " << std::endl );
                        return false;
                    }

                    // clean up ...
                    // remove active segments
                    auto asit = activeSegment.find(nestedEqn.eqn);
                    if (asit != activeSegment.end())
                    {
                        asit.value().pop();
                        if (asit.value().size() == 0)
                        {
                            activeSegment.erase(asit);
                        }
                    }

                    // remove eqn loop counters
                    auto nestedEqnLoops = eqnLoopCounterMap.find(nestedEqn.eqn);
                    if (nestedEqnLoops != eqnLoopCounterMap.end())
                    {
                        eqnLoopCounterMap.erase(nestedEqnLoops);
                    }
                }
                break;
            }
        }
        else
        {
            // paste the command in
            nlStr << oeqn[oep] << eol;
            nlStr.flush();
        }

        // if we've worked through the equation, we can drop
        // the active range entry
        if (segrange[0] == expectedNewSegStartValue && oep == segrange[1])
        {
            activeSegment[eqnName].pop();
            auto arsv = activeSegment.find(eqnName);
            if (arsv.value().size() == 0)
            {
                activeSegment.erase(arsv);
            }
        }
    }

    // clean up ...
    if (eqnLoopCounterMap.constFind(eqnName) != eqnLoopCounterMap.cend())
    {
        foreach(const QString& dimname, removeDimCounter)
        {
            /*
            auto rmit = eqnLoopCounterMap[eqnName].find(dimname);
            if (rmit != eqnLoopCounterMap[eqnName].end())
            {
                eqnLoopCounterMap[eqnName].erase(rmit);

                auto ecm = eqnLoopCounterMap.find(eqnName);
                if (ecm != eqnLoopCounterMap.end())
                {
                    if (ecm.value().size() == 0)
                    {
                        eqnLoopCounterMap.erase(ecm);
                    }
                }
            }
            */
            removeLoopCounter(eqnLoopCounterMap,
                              dimname,
                              eqnName);

            if (dimname.compare(msOPTIONS) == 0)
            {
                dimValueMap[msOPTIONS] = -9999.0;
            }
        }
    }


    return true;
}

void NMMosra::removeLoopCounter(
        QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
        const QString& dimName,
        const QString& eqnName
        )
{
    if (eqnLoopCounterMap.constFind(eqnName) != eqnLoopCounterMap.cend())
    {
        auto opit = eqnLoopCounterMap[eqnName].find(dimName);
        if (opit != eqnLoopCounterMap[eqnName].end())
        {
            eqnLoopCounterMap[eqnName].erase(opit);

            auto ecm = eqnLoopCounterMap.find(eqnName);
            if (ecm != eqnLoopCounterMap.end())
            {
                if (ecm.value().size() == 0)
                {
                    eqnLoopCounterMap.erase(ecm);
                }
            }
        }
    }
}


bool NMMosra::getParameterValue(double &pVal,
        const QString& eqnName,
        const NMMosra::Param &pa,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues,
        QMap<QString, std::vector<size_t> > &nameValPosMap,
        QMap<QString, QMap<QString, int> > &eqnLoopCounterMap,
        QMap<QString, double> &dimValueMap
        )
{
    pVal = -9999;
    long lookupIdx = -1;
    if (nameValPosMap[pa.name].size() == mslOptions.size())
    {
        // either we've been given the exact 'OPTION' to look for
        bool bConv = false;
        QString lastDim;
        foreach(const QString& dim, pa.dimensions)
        {
            lastDim = dim;
            lookupIdx = dim.toDouble(&bConv);
            if (bConv)
            {
                break;
            }
            else
            {
                lookupIdx = -1;
            }
        }

        // need to account for the fact the user-specified
        // OPTIONS indices 1-based
        if (    bConv
             && lastDim.compare(msOPTIONS) == 0
             && lookupIdx > 0
           )
        {
            lookupIdx -= 1;
        }

        // ... or we take it from the loop, if we're in a loop ...
        if (!bConv)
        {
            lookupIdx = eqnLoopCounterMap[eqnName][msOPTIONS]-1;
        }

        // ... or we get it from the dimValueMap when we're not in a loop
        // but processing a full equation for each OPTION
        if (lookupIdx == -1)
        {
            lookupIdx = dimValueMap[msOPTIONS];
        }
    }
    else
    {
        lookupIdx = 0;
    }

    if (lookupIdx >= 0 and lookupIdx < nameValPosMap[pa.name].size())
    {
        pVal = getRecordValue(
                    nameValPosMap,
                    getvalues,
                    pa.name,
                    lookupIdx
                    );
        if (pVal == -9999)
        {
            return false;
        }

        if (mmParameterScaling.contains(pa.name))
        {
            pVal *= mmParameterScaling[pa.name].first;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool NMMosra::createEqnElemAdminMap(
        QMap<QString, QSet<QString> > &dimEqnMap,
        QMap<QString, std::vector<size_t> > &nameValPosMap,
        QMap<QString, int> &procVarInObjConsMap,
        std::vector<std::string> &getnames,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues,
        const QString& eqnname)
{
    if (mmPrefixEquations.constFind(eqnname) != mmPrefixEquations.cend())
    {
        bool bHighOnly = true;
        QStringList eqnDims;
        QSet<QVector<long long> > highDimCombo;
        const EquationAdmin& ea = mmPrefixEquations.value(eqnname).first;
        foreach(const Loop& loop, ea.loopList)
        {
            // is there a dimension parameter defined under this name?
            if (mmslParameters.constFind(loop.dim) != mmslParameters.cend())
            {
                // dimension parameters may only be defined over a single column
                if (mmslParameters.value(loop.dim).size() == 1)
                {
                    // map the dimension name to the equation
                    dimEqnMap[loop.dim] << eqnname;
                    if (nameValPosMap.constFind(loop.dim) == nameValPosMap.cend())
                    {
                        const QString epname = mmslParameters.value(loop.dim).at(0);
                        std::vector<size_t> ldpos;

                        if (mmLookupColsFstParam.constFind(epname) != mmLookupColsFstParam.cend())
                        {
                            const QString fstpara = mmLookupColsFstParam[epname];
                            ldpos = nameValPosMap[fstpara];
                        }
                        else
                        {
                            ldpos = {getnames.size()};
                            getnames.push_back(epname.toStdString());
                            otb::AttributeTable::ColumnValue cval;
                            getvalues.push_back(cval);

                            mmLookupColsFstParam.insert(epname, loop.dim);

                        }
                        nameValPosMap.insert(loop.dim, ldpos);
                    }

                    if (!mslLowDims.contains(loop.dim))
                    {
                        if (!eqnDims.contains(loop.dim))
                        {
                            eqnDims.push_back(loop.dim);
                        }
                    }
                    else
                    {
                        bHighOnly = false;
                    }
                }
                else if (mmslParameters.value(loop.dim).size() > 1)
                {
                    MosraLogError(<< "NMMosra::createEqnElemAdminMap() - ERROR: "
                                  << "Invalid definition of dimension parameter '"
                                  << loop.dim.toStdString()
                                  << "'! Individual dimensions may only be defined "
                                  << " over a single column of the dataset!")
                    return false;
                }
                else
                {
                    MosraLogError(<< "NMMosra::createEqnElemAdminMap() - ERROR: "
                                  << "Invalid definition of dimension parameter '"
                                  << loop.dim.toStdString()
                                  << "'! Please specify dataset column defining "
                                  << "the set of dimension values!");
                    return false;

                }
            }
            // do we have 'OPTIONS' here? ...
            // and we need 'OPTIONS' in the dimEqnMap!
            else if (loop.dim.compare(msOPTIONS) == 0)
            {
                dimEqnMap[loop.dim] << eqnname;
            }
        }

        foreach(const Param& param, ea.paramList)
        {
            // keep track of parameter dimension values
            foreach(const QString& pdim, param.dimensions)
            {
                // check that we've got a definition for that dimension parameter
                if (mmslParameters.constFind(pdim) != mmslParameters.cend())
                {
                    // keep track of individual columns
                    if(mmslParameters.value(pdim).size() == 1)
                    {
                        dimEqnMap[pdim] << eqnname;
                        if (nameValPosMap.constFind(pdim) == nameValPosMap.cend())
                        {
                            const QString ppname = mmslParameters.value(pdim).at(0);
                            std::vector<size_t> pdpos;// = {getnames.size()};

                            if (mmLookupColsFstParam.constFind(ppname) != mmLookupColsFstParam.cend())
                            {
                                const QString fstpara = mmLookupColsFstParam[ppname];
                                pdpos = nameValPosMap[fstpara];
                            }
                            else
                            {
                                pdpos = {getnames.size()};

                                getnames.push_back(ppname.toStdString());
                                otb::AttributeTable::ColumnValue cval;
                                getvalues.push_back(cval);

                                mmLookupColsFstParam.insert(ppname, pdim);
                            }
                            nameValPosMap.insert(pdim, pdpos);
                        }
                    }
                    else if (mmslParameters.value(pdim).size() > 1)
                    {
                        MosraLogError(<< "NMMosra::createEqnElemAdminMap() - ERROR: "
                                      << "Invalid definition of dimension parameter '"
                                      << pdim.toStdString()
                                      << "'! Individual dimensions may only be defined "
                                      << " over a single column of the dataset!")
                        return false;
                    }
                    else
                    {
                        MosraLogError(<< "NMMosra::createEqnElemAdminMap() - ERROR: "
                                      << "Invalid definition of dimension parameter '"
                                      << pdim.toStdString()
                                      << "'! Please specify dataset column defining "
                                      << "the set of dimension values!");
                        return false;

                    }
                }
                // do we have 'OPTIONS' here? ...
                // and we need 'OPTIONS' in the dimEqnMap!
                else if (pdim.compare(msOPTIONS) == 0)
                {
                    dimEqnMap[pdim] << eqnname;
                }
                else
                {
/// ToDo : generalize
                    bool bconv = false;
                    const double tval = pdim.toDouble(&bconv);
                    if (bconv)
                    {
                        if (param.dimensions.size() > 1 && param.dimensions.last().compare(pdim) == 0)
                        {
                            dimEqnMap[msOPTIONS] << eqnname;
                        }
                        continue;
                    }
                }

                // high dimension tracking ...
                if (!mslLowDims.contains(pdim))
                {
                    if (!eqnDims.contains(pdim))
                    {
                        eqnDims.push_back(pdim);
                    }
                }
                else
                {
                    bHighOnly = false;
                }
            }

            // keep track of parameter values
            if (mmslParameters.constFind(param.name) != mmslParameters.cend())
            {
                if (mmslParameters.value(param.name).size() == 0)
                {
                    MosraLogError(<< "NMMosra::createEqnElemAdminMap() - ERROR: "
                                  << "No dataset columns referenced by parameter '"
                                  << param.name.toStdString() << "'! Please specify "
                                  << "(a) dataset column(s) defining the parameter's "
                                  << "values!");
                    return false;
                }

                if (nameValPosMap.constFind(param.name) == nameValPosMap.cend())
                {
                    std::vector<size_t> pcpos;
                    foreach(const QString& pcol, mmslParameters.value(param.name))
                    {
                        //if (mmLookupColsFstParam.constFind(pcol) != mmLookupColsFstParam.cend())
                        //{
                        //    const QString fstParam = mmLookupColsFstParam[pcol];
                        //    pcpos = nameValPosMap[fstParam];
                        //}
                        //else
                        //{
                            pcpos.push_back(getnames.size());
                            getnames.push_back(pcol.toStdString());
                            otb::AttributeTable::ColumnValue cval;
                            getvalues.push_back(cval);

                            //mmLookupColsFstParam.insert(pcol, param.name);
                        //}
                    }
                    nameValPosMap.insert(param.name, pcpos);
                }
            }

            // count how often a given variable
            if(mslProcessVars.contains(param.name))
            {
                if (procVarInObjConsMap.constFind(param.name) == procVarInObjConsMap.cend())
                {
                    procVarInObjConsMap[param.name] = 1;
                }
                else
                {
                    procVarInObjConsMap[param.name] += 1;
                }
            }
        }

        foreach(const Equation& equation, ea.eqnList)
        {
            if (!createEqnElemAdminMap(
                        dimEqnMap,
                        nameValPosMap,
                        procVarInObjConsMap,
                        getnames,
                        getvalues,
                        equation.eqn
               ))
            {
                 return false;
            }
        }

        if (bHighOnly && eqnDims.size() > 0)
        {
            mmslHighDimValComboTracker.insert(eqnname, highDimCombo);
            mmHighDimEqnMap.insert(eqnname, eqnDims);
        }
    }


    return true;
}

double NMMosra::getRecordValue(QMap<QString, std::vector<size_t> > &nameValPosMap,
        std::vector<otb::AttributeTable::ColumnValue> &getvalues,
        const QString &paramName,
        const size_t optionsIdx
        )
{
    double retVal = -9999;

    if (nameValPosMap.constFind(paramName) != nameValPosMap.cend())
    {
        const size_t offset = nameValPosMap[paramName][optionsIdx];
        const otb::AttributeTable::ColumnValue cval = getvalues[offset];
        const otb::AttributeTable::TableColumnType vtype = cval.type;
        switch(vtype)
        {
        case otb::AttributeTable::ATTYPE_INT:
            retVal = static_cast<double>(cval.ival);
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            retVal = cval.dval;
            break;
        default:
            ;
        }
    }

    return retVal;
}


int NMMosra::makeNL(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    //if (!this->calcBaseline())
    //{
    //    return 0;
    //}
    //MosraLogInfo(<< "calculating baseline - OK");


    /*
     *  param ha_min {PARCEL} >= 0
     *  param ha_max {j in PARCEL} >= ha_min[i]
     *
     *  param nleach {USE, PARCEL} >= 0
     *
     *  var LuHa {i in USE, j in PARCEL} >= 0, <= ha_max[j]
     *
     *
     *
     *  minimize leaching: sum {i in USE, j in PARCEL} nleach[i,j] * LuHa[i,j]
     *
     *  s.t. max_alloc {j in PARCEL}: sum {i in USE} LuHa[i,j] <= ha_max[j]
     *  s.t. max_dai: sum {j in PARCEL} LuHa[1,j] >= 24480
     *  s.t. max_dai_i: sum {j in PARCEL} LuHa[2,j] >= 8000
     *  s.t. ...
     *
     *  - calc number of variables
     *  - count number of constraints
     *  - count non-zeros in constraint columns, i.e. the number of times a non-zero
     *    constraint coefficient is used
     *  - count non-zeros in gradient columns, i.e. the number of times a non-zero
     *    objective function coefficient is used
     *
     *
     */

    QFileInfo fifo(this->getLosFileName());

    QString nlFileName = QString("%1/%2.nl").arg(this->msDataPath).arg(fifo.baseName());
    this->msSolFileName = QString("%1/%2.sol").arg(this->msDataPath).arg(fifo.baseName());
    QFile nlFile(nlFileName);
    if (!nlFile.open(QIODevice::WriteOnly))
    {
        MosraLogError(<< "failed creating '" << nlFileName.toStdString()
                      << "'!");
        return 0;
    }

    // ---------------------------------------
    // initiate header

    QString nlheader;
    QTextStream nlstr(&nlheader);

    QString sp = " ";
    QString endl = "\n";

    // first line never changes
    nlstr << "g3 1 1 0" << endl;

    // num variables
    long nvars = 0;
    //QString b_seg;
    //if (!processVariables(b_seg, &nvars))
    //{
    //    return 0;
    //}


    for (int dv=0; dv < mslDecisionVars.size(); ++dv)
    {
        auto vait = mmVarAdminMap.constFind(mslDecisionVars[dv]);
        nvars += vait.value().dimOffsetMap.size();
    }

    MosraLogInfo(<< "Number of variables for '" << mScenarioName.toStdString()
                 << "': " << nvars);

    nlstr << nvars << sp;

    // num constraints
    const int n_cons = this->mlNumOptFeat + this->mmslAreaCons.size() +
            this->mmslCriCons.size() + this->mmslFeatCons.size() +
            this->mmslAreaZoneCons.size();
    nlstr << n_cons << sp;

    // num objectives, ranges,
    const int n_objs = this->mmslObjectives.size();
    nlstr << n_objs << sp << n_cons << " 0 0" << endl;

    // non-linear constraints, objectives
    const int n_nlin_cons = this->mmslNonLinearConstraints.size();
    int n_nlin_objs = 0;
    auto oit = this->mmslObjectives.cbegin();
    while (oit != this->mmslObjectives.cend())
    {
        if (this->mmsEquations.find(oit.key()) != mmsEquations.end())
        {
            ++n_nlin_objs;
        }
        ++oit;
    }
    nlstr << n_nlin_cons << sp << n_nlin_objs << sp << endl;

    // network constraints: non-linear, linear
    nlstr << "0 0" << endl;

    // non-linear vars in constraints, objectives, both
    // non-linear vars:
    //   - defined vars popping up in defined equations
    //   - defined vars popping up in non-linear constraints
    // we count the coccurence of uniqe non-linear vars in
    // (non-linear) constraints and (non-linear) objectives

    QSet<QString> nlVars;
    foreach(const QString& var, mmslVariables.keys())
    {
        // check equation 'names'
        if (mmEquationAdmins.keys().contains(var))
        {
            nlVars << var;
        }
        else
        {
            // now look inside each equation ...
            auto eit = mmEquationAdmins.cbegin();
            while (eit != mmEquationAdmins.cend())
            {
                // check each parameter in the equation (as it could
                // be actually a variable)
                foreach(const Param& par, eit.value().paramList)
                {
                    // if we found a match, we double check whether the
                    // dimensions also match, just to be sure ...
                    if (var.compare(par.name, Qt::CaseSensitive) == 0)
                    {
                        // checking the number, order, and names of
                        // the dimensions
                        const QString ds1 = mmslVariables[var].join(" ");
                        const QString ds2 = par.dimensions.join(" ");

                        if (ds1.compare(ds2, Qt::CaseSensitive) == 0)
                        {
                            // need to query / calc the actual number of vars
                            // involved here


                            nlVars << var;
                        }
                    }
                }
                ++eit;
            }
        }
    }

    // now determine how many of the nl vars occur in nl constraints and nl objectives respectively
    int num_nlConsVars = 0;
    int num_nlObjsVars = 0;
    int num_nlConsObjVars = 0;

    foreach(const QString nlvar, nlVars)
    {
        int sum = 0;
        if (mmslObjectives.keys().contains(nlvar))
        {
            ++num_nlObjsVars;
            ++sum;
        }

        if (mmslNonLinearConstraints.contains(nlvar))
        {
            ++num_nlConsVars;
            ++sum;
        }

        if (sum == 2)
        {
            ++num_nlConsObjVars;
        }
    }

    // non-linear vars in constraints, objectives, both
    nlstr << num_nlConsVars << sp
          << num_nlObjsVars << sp
          << num_nlConsObjVars << endl;

    // linear network vars, functions, arith, flags
    nlstr << "0 0 0 0" << endl;

    // discrete variables: binary, integer, non-linear binary, non-linear integer
    nlstr << "0 0 0 0 0" << endl;

    //QString newString;
    //nlstr.setString(&newString);

    /*
     *  # stuff we know
     *
     *  msAreaField     - name of AreaHa column
     *  mdAreaTotal     - total area of SDUs
     *  mlNumOptFeat    - number of SDUs
     *  miNumOptions    - number of land uses
     *  mlNumArealDVar  - number of spatial desc vars (no SDUs * no land-uses)
     *  mlNumDVar       = mlNumArealDVar
     *
     *  mslOptions      - list of land uses
     *  mmslAreaCons    - areal constraints
     *  mmslCriCons     - performance constraints
     */

    //
    long long lNumCells = mDataSet->getNumRecs();
    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);
    int sduIdx = 0;

    std::vector<int> viDVarConsNonZeros(this->mlNumDVar, 0);
    std::vector<int> viDVarObjNonZeros(this->mlNumDVar, 0);

    // =======================================================================
    // explicit area constraints

    std::vector<QString> vsConsLabel;
    std::vector<int> vnZoneLength;
    std::vector<QString> vsZoneField;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    if (!processExplicitArealCons(vsConsLabel, vnZoneLength, vsZoneField,
                             vvnOptionIndex, vnConsType, vdRHS))
    {
        return false;
    }

    QString Cpart;
    QTextStream strC(&Cpart);

    QString Jpart;
    QTextStream strJ(&Jpart);

    QString rpart;
    QTextStream strr(&rpart);

    int constrIdx = 0;

    // initiate 'r' section
    strr << "r" << sp << "   # constraints' RHS (thresholds)" << endl;

    QMap<QString, QStringList>::const_iterator it = this->mmslAreaCons.constBegin();
    for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r, ++constrIdx)
    {
        // -----------------------
        // C - algebraic constraints body
        // -----------------------

        strC << "C" << constrIdx << sp << "   # " << it.key() << endl;
        strC << "n0" << endl;

        // ------------------------
        // r - bounds on algebraic constraint bodies ("ranges") (RHS)
        // -----------------------

        // body <= rhs
        if (vnConsType.at(r) == 1)
        {
            strr << 1 << sp << vdRHS.at(r) << endl;
        }
        // body >= rhs
        else if (vnConsType.at(r) == 2)
        {
            strr << 2 << sp << vdRHS.at(r) << endl;
        }
        // body = rhs
        else if (vnConsType.at(r) == 3)
        {
            strr << 4 << sp << vdRHS.at(r) << endl;
        }
        // no constraints on body
        else
        {
            strr << 3 << endl;
        }

        // ---------------------------
        // J - Jacobian sparsity, linear  terms
        // ---------------------------

        QStringList consOptions = this->mslOptions;
        strJ << "J" << constrIdx << sp << vnZoneLength.at(r) * vvnOptionIndex.at(r).size() << sp;
        strJ << "   # " << it.key() << endl;

        sduIdx = 0;
        for (int feat=0; feat < lNumCells; ++feat)
        {
            if (optFeatIdx != -1 && this->mDataSet->getIntValue(optFeatIdx, feat) == 0)
            {
                continue;
            }

            if (!vsZoneField.at(r).isEmpty())
            {
                const QString zoneVal = mDataSet->getStrValue(vsZoneField.at(r), feat);
                consOptions = zoneVal.split(" ", Qt::SkipEmptyParts);
            }

            for (int oi=0; oi < vvnOptionIndex.at(r).size(); ++oi)
            {
                const int optIdx = vvnOptionIndex.at(r).at(oi);
                if (consOptions.contains(this->mslOptions.at(optIdx)))
                {
                    const int offset = optIdx + sduIdx * this->miNumOptions;
                    strJ << offset << sp << 1 << endl;
                    viDVarConsNonZeros[offset] += 1;
                }
            }

            // sdu counter
            ++sduIdx;
        }
    }

    //===============================================
    // add implicit areal constraints  (&& and bounds on spatial desc. var)
    QString bpart;
    QTextStream strb(&bpart);

    strb << "b" << sp << "  # variable bounds" << endl;

    const int AreaFieldIdx = this->mDataSet->getColumnIndex(this->msAreaField);
    if (AreaFieldIdx == -1)
    {
        MosraLogError(<< "Oh, this went horribly wrong! "
                      << "Couldn't get the area column!");
        return 0;
    }

    sduIdx = 0;
    for (int cell=0; cell < lNumCells; ++cell)
    {
        // SDU or not?
        if (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, cell) == 0)
        {
            continue;
        }

        // -----------------
        // C

        strC << "C" << constrIdx << sp
             << "   # feat=" << cell << " sdu=" << sduIdx << endl;
        strC << "n0" << endl;

        // -----------------
        // r

        const double dCellArea = this->mDataSet->getDblValue(AreaFieldIdx, cell);

        // body <= rhs
        strr << 1 << sp << dCellArea << endl;

        // -----------------
        // J

        strJ << "J" << constrIdx << sp << this->miNumOptions << sp;
        strJ << "   # feat=" << cell << " sdu=" << sduIdx << endl;

        for (int o=0; o < this->miNumOptions; ++o)
        {
            // var bounds
            strb << 0 << sp << 0 << sp << dCellArea << sp
                 << "  # feat=" << cell << " sdu=" << sduIdx << " option=" << this->mslOptions.at(o) << endl;

            // non-zero jacobian values
            const int varoffset = o + sduIdx * this->miNumOptions;
            strJ << varoffset << sp << 1 << endl;
            viDVarConsNonZeros[varoffset] += 1;
        }

        // -------------------
        // counters
        ++constrIdx;
        ++sduIdx;
    }


    //========================================================
    // objective function - non-linear 'O' & linear 'G' part

    QString Opart;
    QTextStream strO(&Opart);

    QString Gpart;
    QTextStream strG(&Gpart);

    oit = this->mmslObjectives.constBegin();
    for (int no=0; oit != this->mmslObjectives.constEnd(); ++oit, ++no)
    {
        // -------------------------
        // O - objective function (non-linear part)

        const QString minmaxStr = oit.value().at(0).left(3);
        int minmax = 0;
        if (minmaxStr.contains("max"))
        {
            minmax = 1;
        }

        strO << "O" << no << sp << minmax << sp << "   # " << oit.key() << endl;
        strO << "n0" << endl;

        // --------------------------
        // G - objective function (linear part)

        strG << "G" << no << sp << this->mlNumDVar << sp << "   # " << oit.key() << endl;

        const QString sCriterion = oit.key();
        const QStringList criFieldList = this->mmslCriteria.value(sCriterion);

        sduIdx = 0;
        for (long cell=0; cell < lNumCells; ++cell)
        {
            if (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, cell) == 0)
            {
                continue;
            }

            for (int option=0; option < this->miNumOptions; ++option)
            {
                QString sField = "";
                if (option <= criFieldList.size()-1)
                {
                    sField = criFieldList.at(option);
                }

                const int fieldIdx = this->mDataSet->getColumnIndex(sField);
                if (fieldIdx == -1)
                {
                    MosraLogError(<< "Oh, this went horribly wrong!"
                                  << "Couldn't get the performance indicator '"
                                  << oit.key().toStdString() << "' for '"
                                  << this->mslOptions.at(option).toStdString() << "'!");
                    return 0;
                }

                const double dValue = this->mDataSet->getDblValue(fieldIdx, cell);
                const int varOffset = option + sduIdx * this->miNumOptions;
                viDVarObjNonZeros[varOffset] += 1;

                strG << varOffset << sp << dValue << endl;
            }

            //-----------
            //counters
            ++sduIdx;
        }
    }


    // =========================================
    // finalise header

    // count non zeros in jacobian
    int nonZerosJ = 0;
    for (int j=0; j < viDVarConsNonZeros.size(); ++j)
    {
        nonZerosJ += viDVarConsNonZeros[j];
    }

    // count non zeros in objective function
    int nonZerosG = 0;
    for (int g=0; g < viDVarObjNonZeros.size(); ++g)
    {
        nonZerosG += viDVarObjNonZeros[g];
    }

    // non-zeros constraint and objective func. coefficients
    nlstr << nonZerosJ << sp << nonZerosG << endl;

    // max name length: constraints, variables
    nlstr << "0 0" << endl;

    // common exprs: b,c,o,c1,o1
    nlstr << "0 0 0 0 0" << endl;

    // write the header
    QByteArray nlbar(nlstr.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();
    nlheader.clear();


    // =========================================
    // write 'C' part
    nlbar.push_back(strC.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    Cpart.clear();
    strC.setString(&Cpart);


    // =========================================
    // write 'O' part
    nlbar.push_back(strO.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    Opart.clear();
    strO.setString(&Cpart);


    //===========================================
    // write 'r' part
    nlbar.push_back(strr.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    rpart.clear();
    strr.setString(&rpart);

    //===========================================
    // write the 'b' part
    nlbar.push_back(strb.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    bpart.clear();
    strb.setString(&bpart);

    //====================================================
    // writing the 'k' section
    // cumulative sum of J-columns' non-zeros

    const int klen = viDVarConsNonZeros.size()-1;
    QString kpart;
    QTextStream strk(&kpart);

    strk << "k" << klen << sp << "    # cumulative non-zeros in jacobian" << endl;
    int cumVar = 0;
    for (int k=0; k < klen; ++k)
    {
        cumVar += viDVarConsNonZeros.at(k);
        strk << cumVar << endl;
    }

    nlbar.push_back(strk.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    kpart.clear();
    strk.setString(&kpart);

    //======================================================
    // write the J part
    nlbar.push_back(strJ.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    Jpart.clear();
    strJ.setString(&Jpart);

    //======================================================
    // write the G part
    nlbar.push_back(strG.readAll().toStdString().c_str());
    nlFile.write(nlbar);
    nlFile.flush();
    nlbar.clear();

    Gpart.clear();
    strG.setString(&Gpart);


    // close file
    nlFile.close();

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::makeSTECLp(int proc)
{
    NMDebugCtx(ctxNMMosra, << "...");

    /* STEC Lp structure
     *
     * i     : feature index (SVID) only for units with long-term annual avg landslide > 0
     * j     : daily time step index (0-based)
     * b_i_j : fail indicator (i=239, j=365, ixj=87235)
     *     - we doe have 365 time steps
     *
     * cho     : global decision variable for cohesion parameter (1)
     * tan_phi : global decision variable for angle of inner friction (1)
     *
     *col/row :     0      1         2      3           4          5
     *                 |  coh | tan_phi |   b_1_1 |   b_2_1 |   b_1_2 |   b_2_2 ...  OP  RHS
     *obj fun:                            -tl_1_1 | -tl_2_1 | -tl_1_2 | -tl_2_2
     *b_1_1_cons                     1    cst_1_1     maxS                                     >=  SS_1_1
     *b_2_1_cons                     1    cst_2_1               maxS                           >=  SS_2_1
     * ...
     * ...
     *coh_upper                      1                                                         <=  400000
     *tan_phi_lower                         1                                                  >=  0.083
     *tan_phi_upper                         1                                                  <=  0.83
     *
     */

    // structure of the input dataset
    // columns: rowidx, recid, TimeStamp, lscoeff, costress, shstress
    // rows: 1254 x 365 = 457710
    //

    // --------------------------------------------------
    //              GET BEARINGS ...
    // --------------------------------------------------


    long long lNumCells = mDataSet->getNumRecs();
    this->mlNumArealDVar = lNumCells / 365;
    this->mlNumDVar = lNumCells + 2;
    this->mlLpCols = this->mlNumDVar + 1;
    this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE;

    this->mLp->MakeLp(0, this->mlLpCols);

    // --------------------------------------------------
    //          GREEDY DATA GRAB
    // --------------------------------------------------
    NMMsg(<< "Greedily fetching required values from the table ..." << endl);

    otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab == nullptr)
    {
        NMMsg(<< "BAD LUCK folks! This is gonna take until the end of time ...!" << endl);
        return 0;
    }

    std::vector<std::string> colnames;
    colnames.push_back("rowidx");       //0
    colnames.push_back("SVID");         //1
    colnames.push_back("lscoeff");      //2
    colnames.push_back("costress");     //3
    colnames.push_back("shstress");     //4

    std::map<int, std::map<long long, double> > valstore;
    for (int c=1; c < colnames.size(); ++c)
    {
        std::map<long long, double> dblmap;
        valstore.insert(std::pair<int, std::map<long long, double> >(c, dblmap));
    }

    if (!sqltab->GreedyNumericFetch(colnames, valstore))
    {
        NMMsg(<< "Greedy fetch didn't work :-( ... " << endl);
        return 0;
    }

    // --------------------------------------------------
    //              CREATE EMPTY LP MATRIX
    // --------------------------------------------------
    MosraLogInfo(<< "Creating LP matrix ...");
    NMMsg(<< "Creating LP matrix ..." << endl);

    //std::vector<std::string> mainColNames = {"snl_sum", "coh", "tan_phi"};
    std::vector<std::string> mainColNames = {"coh", "tan_phi"};

    long colPos = 1;
    int featCount = 1;
    int yearCount = 1;

    for (int i=0; i < 2 ; ++i, ++colPos)
    {
        this->mLp->SetColName(colPos, mainColNames[i]);
    }


    std::stringstream colname;
    for (int cell=1; cell <= lNumCells; ++cell, ++colPos, ++featCount)
    {
        //colname << "b_" << featCount << "_" << yearCount;
        colname << "b_" << valstore[1][cell] << "_" << yearCount;
        //std::cout << "adding column '" << colname.str() << "'" << endl;
        this->mLp->SetColName(colPos, colname.str());
        this->mLp->SetBinary(colPos, true);
        colname.str("");
        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            ++yearCount;
        }
    }


    // --------------------------------------------------
    //              ADD THE OBJECTIVE FUNCTION
    // --------------------------------------------------
    MosraLogInfo(<< "Adding the objective function ...");
    NMMsg(<< "Adding the objective function:  " << endl);

    const long objsize = lNumCells;
    double* pcval = new double[objsize];
    int* pcid = new int[objsize];

    //pcid[0] = 1;
    //pcval[0] = 1.0;

    int colID = 3;
    int arpos = 0;

    featCount = 1;
    yearCount = 1;

    for (int feat=1; feat <= lNumCells; ++feat, ++featCount, ++arpos, ++colID)
    {
        pcval[arpos] = valstore[2][feat];
        pcid[arpos]  = colID;

        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            if (std::fmod(feat, lNumCells*0.1) == 0) std::cout << ".";
            ++yearCount;
        }
    }
    std::cout << endl << endl;

    this->mLp->SetAddRowmode(true);
    this->mLp->SetObjFnEx(objsize, pcval, pcid);
    this->mLp->SetAddRowmode(false);
    this->mLp->SetMinim();

    NMMsg(<< " done!" << endl << endl);

    delete[] pcval;
    delete[] pcid;


    // --------------------------------------------------
    //              ADD CONSTRAINTS
    // --------------------------------------------------
    MosraLogInfo(<< "Adding objective constraints to LP ...");
    NMMsg(<< "Adding objective constraints to LP: " << endl);

    const double sn_tls = 1364256.0;
    const double sn_tls_98 = 1336971;
    const double sn_tls_up = 1364256.0 + 68212;
    const double sn_tls_lo = 1364256.0 - 68212;
    pcval = new double[objsize];
    pcid = new int[objsize];

    //pcval2 = new double[objsize-1];

    arpos = 0;
    colID = 3;

    featCount = 1;
    yearCount = 1;

    for (int ff=1; ff <= lNumCells; ++ff, ++arpos, ++featCount, ++colID)
    {
        pcval[arpos] = valstore[2][ff];
        //pcval2[arpos] = -valstore[1][feat];
        pcid[arpos]  = colID;

        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            if (std::fmod(ff, lNumCells*0.1) == 0) std::cout << ".";
            ++yearCount;
        }
    }
    std::cout << endl << endl;

    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    this->mLp->AddConstraintEx(objsize, pcval, pcid, 2, sn_tls);
    ++lRowCounter;
    this->mLp->SetRowName(lRowCounter, "Obj cons #1 sn_ls >= +sum");

    //this->mLp->AddConstraintEx(objsize-1, pcval, pcid, 2, sn_tls_lo);
    //++lRowCounter;
    //this->mLp->SetRowName(lRowCounter, "Obj cons #2 sn_ls >= -sum");

    delete[] pcval;
    //delete[] pcval2;
    delete[] pcid;


    // -------------------------------------------
    // add the limit constraints for c and tan(phi)

    double co_lim_up = 0;//40000;
    double tanphi_lim[2] = {0.08, 0.83};
    double dvcoeff = 1.0;
    int colCoh = 1;
    int colTanPhi = 2;

    this->mLp->AddConstraintEx(1, &dvcoeff, &colCoh, 1, co_lim_up);
    ++lRowCounter;
    this->mLp->SetRowName(lRowCounter, "cohesion_upper_cons");

    this->mLp->AddConstraintEx(1, &dvcoeff, &colTanPhi, 2, tanphi_lim[0]);
    ++lRowCounter;
    this->mLp->SetRowName(lRowCounter, "tan(phi)_lower_cons");

    this->mLp->AddConstraintEx(1, &dvcoeff, &colTanPhi, 1, tanphi_lim[1]);
    ++lRowCounter;
    this->mLp->SetRowName(lRowCounter, "tan(phi)_upper_cons");

    // -------------------------------------------
    //     add binary & stress coeff constraints
    MosraLogInfo(<< "Adding switch & stress constraints to LP ...");
    NMMsg(<< "Adding switch and stress constraints to LP: " << endl);

    double pdRow[3]   = {1,0,500000};
    int piColno[3] = {1,2,3};
    featCount = 1;
    yearCount = 1;

    std::stringstream rowname;
    for (int feat=1; feat <= lNumCells; ++feat, ++featCount)
    {
        const double shstress = valstore[4][feat];
        pdRow[1] = valstore[3][feat];

        this->mLp->AddConstraintEx(3, pdRow, piColno, 2, shstress);
        ++lRowCounter;

        rowname << "b_" << valstore[1][feat] << "_" << yearCount << "_cons";
        this->mLp->SetRowName(lRowCounter, rowname.str());
        rowname.str("");

        ++piColno[2];

        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            ++yearCount;
        }
    }
    NMMsg(<< " done!" << endl << endl);

    this->mLp->SetAddRowmode(false);

    // --------------------------------------------------
    //              Write LP
    // --------------------------------------------------
    MosraLogInfo(<< "Exporting your problem ...");
    NMMsg( << "Exporting your problem ...");

    // if the user wishes, we write out the problem we've just created ...
    //otb::SQLiteTable::Pointer tab = static_cast<otb::SQLiteTable*>(this->mDataSet->getOtbAttributeTable().GetPointer());
    //NMSqlTableModel* sqltab = static_cast<NMSqlTableModel*>(mDataSet->getQSqlTableModel());
    //if (sqltab == nullptr)
    //{
    //    MosraLogError(<< "Mmmh, didn't get the underlying table model ... I quit!");
    //    return 0;
    //}

    QString pathname;
    QString basename = this->mScenarioName;

    if (sqltab != nullptr)
    {
        QString tabFN = sqltab->GetDbFileName().c_str();
        QFileInfo fifo(tabFN);
        pathname = fifo.canonicalPath();
        QString lpname = QString("%1/%2.lp").arg(pathname).arg(basename);

        this->mProblemFilename = lpname;
        if (!this->mProblemFilename.isEmpty())
        {
            if (this->mProblemType == NM_MOSO_LP)
            {
                this->mLp->WriteLp(this->mProblemFilename.toStdString());
            }
            else
            {
                this->mLp->WriteMps(this->mProblemFilename.toStdString());
            }
        }
    }

    // --------------------------------------------------
    //              SOLVE LP
    // --------------------------------------------------
    MosraLogInfo(<< "Solving your problem ...");
    NMMsg(<< "Solving your problem ..." << endl);

    this->mLp->SetBreakAtFirst(true);
    //this->mLp->SetBreakAtValue(1405184);
    //this->mLp->SetTimeout(-1);
    //this->mLp->SetTimeout(180);
    this->mbCanceled = false;

    this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);
    this->mLp->SetLogFunc((void*)this, NMMosra::lpLogCallback);

    //this->mLp->SetPresolve(PRESOLVE_COLS |
    //                       PRESOLVE_ROWS |
    //                       PRESOLVE_IMPLIEDFREE |
    //                       PRESOLVE_REDUCEGCD |
    //                       PRESOLVE_MERGEROWS |
    //                       PRESOLVE_ROWDOMINATE |
    //                       PRESOLVE_COLDOMINATE |
    //                       PRESOLVE_KNAPSACK |
    //                       PRESOLVE_PROBEFIX);
    //
    //this->mLp->SetScaling(SCALE_GEOMETRIC |
    //                      SCALE_DYNUPDATE);

    this->mLp->Solve();

    // --------------------------------------------------
    //              Report results
    // --------------------------------------------------
    MosraLogInfo(<< "Report results ...");
    NMMsg(<< "Report results ..." << endl);

    QString stecReport = QString("%1/report_%2.txt").arg(pathname).arg("ParaOpt_STEC_landslide");
    // this is where the desc vars for landslideing are retrieved and reported
    this->writeSTECReport(stecReport, valstore);

    MosraLogInfo(<< this->msReport.toStdString() << endl);
    NMMsg(<< this->msReport.toStdString() << endl << endl);


    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::makeSTECLp2(int proc)
{
    NMDebugCtx(ctxNMMosra, << "...");

    /* STEC Lp structure
     *
     * i     : feature index (0-based)
     * j     : time step index (0-based)
     * b_i_j : fail indicator (i=239, j=365, ixj=87235)
     *     - we doe have 365 time steps
     *
     * cho     : global decision variable for cohesion parameter (1)
     * tan_phi : global decision variable for angle of inner friction (1)
     *                                                               |  failOffset = 2 * i + 1
     *col/row :     0      1         2            2 * i-1   2 * i    | 2 x i + 1 |  offset+1  offset+i-1
     *                |  coh_1 | tan_phi_1 | ... | coh_i | tan_phi_i |   b_1_1   |   b_2_1 |  b_i_1      | b_i_j          ...  OP  RHS
     *obj fun (min z):                                                    tl_1        tl_2     tl_i        tl_i
     *tot sn ls                                                           tl_1        tl_2     tl_i        tl_i          >= SedNetNZ landslide
     *b_1_1_cons           1      cst_1_1                                 maxS                                           >=  SS_1_1
     *b_i_j_cons                                     1      st_i_j                                          maxS         >=  SS_2_1
     * ...
     * ...
     *coh_1_upper          1                                                                                             <=  400000
     *tan_phi_1_lower               1                                                                                    >=  0.083
     *tan_phi_1_upper               1                                                                                    <=  0.83
     *coh_2_upper                                    1                                                                   <=  400000
     *tan_phi_2_lower                                         1                                                          >=  0.083
     *tan_phi_2_upper                                         1                                                          <=  0.83
     *
     */

    // structure of the input dataset
    // columns: rowidx, recid, TimeStamp, lscoeff, costress, shstress
    // rows: 239 x 365 = 87235
    //

    // --------------------------------------------------
    //              GET BEARINGS ...
    // --------------------------------------------------


    long long lNumCells = mDataSet->getNumRecs();
    this->mlNumArealDVar = lNumCells / 365;
    //this->mlNumDVar = lNumCells + 2 * this->mlNumArealDVar;
    this->mlNumDVar = lNumCells + 1 * this->mlNumArealDVar;
    this->mlLpCols = this->mlNumDVar + 1;
    this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE;

    this->mLp->MakeLp(0, this->mlLpCols);

    // --------------------------------------------------
    //          GREEDY DATA GRAB
    // --------------------------------------------------
    NMMsg(<< "Greedily fetching required values from the table ..." << endl);

    otb::SQLiteTable* sqltab = static_cast<otb::SQLiteTable*>(mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab == nullptr)
    {
        NMMsg(<< "BAD LUCK folks! This is gonna take until the end of time ...!" << endl);
        return 0;
    }

    std::vector<std::string> colnames;
    colnames.push_back("rowidx");       //0
    colnames.push_back("SVID");         //1
    colnames.push_back("lscoeff");      //2
    colnames.push_back("costress");     //3
    colnames.push_back("shstress");     //4

    std::map<int, std::map<long long, double> > valstore;
    for (int c=1; c < colnames.size(); ++c)
    {
        std::map<long long, double> dblmap;
        valstore.insert(std::pair<int, std::map<long long, double> >(c, dblmap));
    }

    if (!sqltab->GreedyNumericFetch(colnames, valstore))
    {
        NMMsg(<< "Greedy fetch didn't work :-( ... " << endl);
        return 0;
    }

    // --------------------------------------------------
    //              CREATE EMPTY LP MATRIX
    // --------------------------------------------------
    MosraLogInfo(<< "Creating LP matrix ...");
    NMMsg(<< "Creating LP matrix ..." << endl);

    long colPos = 1;
    int featCount = 1;
    int yearCount = 1;
    std::stringstream coh_name, tan_name, fail_name;

    for (int i=1; i <= this->mlNumArealDVar; ++i)
    {
        //coh_name << "coh_" << valstore[1][i];
        tan_name << "tan_" << valstore[1][i];

        //this->mLp->SetColName(colPos, coh_name.str());
        //coh_name.str("");
        //++colPos;
        this->mLp->SetColName(colPos, tan_name.str());
        tan_name.str("");
        ++colPos;
    }

    for (int cell=1; cell <= lNumCells; ++cell, ++colPos, ++featCount)
    {
        fail_name << "b_" << valstore[1][cell] << "_" << yearCount;
        this->mLp->SetColName(colPos, fail_name.str());
        this->mLp->SetBinary(colPos, true);
        fail_name.str("");
        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            ++yearCount;
        }
    }


    // --------------------------------------------------
    //              ADD THE OBJECTIVE FUNCTION
    // --------------------------------------------------
    MosraLogInfo(<< "Adding the objective function ...");
    NMMsg(<< "Adding the objective function:  " << endl);

    const long objsize = lNumCells;
    double* pcval = new double[objsize];
    int* pcid = new int[objsize];

    //const int failOffset = 2 * this->mlNumArealDVar + 1;
    const int failOffset = 1 * this->mlNumArealDVar + 1;

    int colID = failOffset;
    int arpos = 0;

    featCount = 1;
    yearCount = 1;

    for (int feat=1; feat <= lNumCells; ++feat, ++featCount, ++arpos, ++colID)
    {
        pcval[arpos] = valstore[2][feat];
        pcid[arpos]  = colID;

        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            if (std::fmod(feat, lNumCells*0.1) == 0) std::cout << ".";
            ++yearCount;
        }
    }
    std::cout << endl << endl;

    this->mLp->SetAddRowmode(true);
    this->mLp->SetObjFnEx(objsize, pcval, pcid);
    this->mLp->SetAddRowmode(false);
    this->mLp->SetMinim();

    NMMsg(<< " done!" << endl << endl);

    delete[] pcval;
    delete[] pcid;


    // --------------------------------------------------
    //              ADD CONSTRAINTS
    // --------------------------------------------------
    MosraLogInfo(<< "Adding objective constraints to LP ...");
    NMMsg(<< "Adding objective constraints to LP: " << endl);

    const double sn_tls = 1364256.0;
    const double sn_tls_98 = 1336971;
    const double sn_tls_up = 1364256.0 + 68212;
    const double sn_tls_lo = 1364256.0 - 68212;
    pcval = new double[objsize];
    pcid = new int[objsize];

    //pcval2 = new double[objsize-1];

    arpos = 0;
    colID = failOffset;

    featCount = 1;
    yearCount = 1;

    for (int ff=1; ff <= lNumCells; ++ff, ++arpos, ++featCount, ++colID)
    {
        pcval[arpos] = valstore[2][ff];
        pcid[arpos]  = colID;

        if (featCount == this->mlNumArealDVar)
        {
            featCount = 0;
            if (std::fmod(ff, lNumCells*0.1) == 0) std::cout << ".";
            ++yearCount;
        }
    }
    std::cout << endl << endl;

    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    this->mLp->AddConstraintEx(objsize, pcval, pcid, 2, sn_tls);
    ++lRowCounter;
    this->mLp->SetRowName(lRowCounter, "Obj cons #1 sn_ls >= +sum");

    //this->mLp->AddConstraintEx(objsize-1, pcval, pcid, 2, sn_tls_lo);
    //++lRowCounter;
    //this->mLp->SetRowName(lRowCounter, "Obj cons #2 sn_ls >= -sum");

    delete[] pcval;
    //delete[] pcval2;
    delete[] pcid;


    // -------------------------------------------
    // add the limit constraints for c and tan(phi)

    double co_lim_up = 0;//400000;
    double tanphi_lim[2] = {0.08, 0.83};
    double dvcoeff = 1.0;
    //int colCoh = 1;
    //int colTanPhi = 2;
    int colTanPhi = 1;

    std::stringstream coh_upper, tan_lower, tan_upper;
    for (int a=1; a < this->mlNumArealDVar; ++a)
    {
        const int svid = valstore[1][a];

        //coh_upper << "coh_" << svid << "_upper_cons";
        //this->mLp->AddConstraintEx(1, &dvcoeff, &colCoh, 3, co_lim_up);
        //++lRowCounter;
        //this->mLp->SetRowName(lRowCounter, coh_upper.str());

        tan_lower << "tan_" << svid << "_lower_cons";
        this->mLp->AddConstraintEx(1, &dvcoeff, &colTanPhi, 2, tanphi_lim[0]);
        ++lRowCounter;
        this->mLp->SetRowName(lRowCounter, tan_lower.str());

        tan_upper << "tan_" << svid << "_upper_cons";
        this->mLp->AddConstraintEx(1, &dvcoeff, &colTanPhi, 1, tanphi_lim[1]);
        ++lRowCounter;
        this->mLp->SetRowName(lRowCounter, tan_upper.str());

        coh_upper.str("");
        tan_lower.str("");
        tan_upper.str("");
        //colCoh += 2;
        //colTanPhi += 2;
        ++colTanPhi;
    }

    // -------------------------------------------
    //     add binary & stress coeff constraints
    MosraLogInfo(<< "Adding switch & stress constraints to LP ...");
    NMMsg(<< "Adding switch and stress constraints to LP: " << endl);

    //double pdRow[3]   = {1,0,500000};
    double pdRow[2]   = {0,500000};
    //int piColno[3] = {1,2,failOffset};
    int piColno[2] = {1,failOffset};

    featCount = 1;
    yearCount = 1;


    std::stringstream rowname;
    for (int feat=1; feat <= lNumCells; ++feat, ++featCount)
    {
        const double shstress = valstore[4][feat];
        pdRow[0] = valstore[3][feat];

        this->mLp->AddConstraintEx(2, pdRow, piColno, 2, shstress);
        ++lRowCounter;

        rowname << "b_" << valstore[1][feat] << "_" << yearCount << "_cons";
        this->mLp->SetRowName(lRowCounter, rowname.str());
        rowname.str("");

        //piColno[0] += 2;
        //piColno[1] += 2;
        //++piColno[2];

        ++piColno[0];
        ++piColno[1];

        if (featCount == this->mlNumArealDVar)
        {
            //piColno[0] = 1;
            //piColno[1] = 2;
            piColno[0] = 1;
            featCount = 0;
            ++yearCount;
        }
    }
    NMMsg(<< " done!" << endl << endl);

    this->mLp->SetAddRowmode(false);

    // --------------------------------------------------
    //              Write LP
    // --------------------------------------------------
    MosraLogInfo(<< "Exporting your problem ...");
    NMMsg( << "Exporting your problem ...");

    // if the user wishes, we write out the problem we've just created ...
    //otb::SQLiteTable::Pointer tab = static_cast<otb::SQLiteTable*>(this->mDataSet->getOtbAttributeTable().GetPointer());
    //NMSqlTableModel* sqltab = static_cast<NMSqlTableModel*>(mDataSet->getQSqlTableModel());
    //if (sqltab == nullptr)
    //{
    //    MosraLogError(<< "Mmmh, didn't get the underlying table model ... I quit!");
    //    return 0;
    //}

    QString pathname;
    QString basename = this->mScenarioName;

    if (sqltab != nullptr)
    {
        QString tabFN = sqltab->GetDbFileName().c_str();
        QFileInfo fifo(tabFN);
        pathname = fifo.canonicalPath();
        QString lpname = QString("%1/%2.lp").arg(pathname).arg(basename);

        this->mProblemFilename = lpname;
        if (!this->mProblemFilename.isEmpty())
        {
            if (this->mProblemType == NM_MOSO_LP)
            {
                this->mLp->WriteLp(this->mProblemFilename.toStdString());
            }
            else
            {
                this->mLp->WriteMps(this->mProblemFilename.toStdString());
            }
        }
    }

    // --------------------------------------------------
    //              SOLVE LP
    // --------------------------------------------------
    MosraLogInfo(<< "Solving your problem ...");
    NMMsg(<< "Solving your problem ..." << endl);

    //this->mLp->SetBreakAtFirst(true);
    this->mLp->SetBreakAtValue(1432468); // +5%
    //this->mLp->SetTimeout(-1);
    //this->mLp->SetTimeout(180);
    this->mbCanceled = false;

    this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);
    this->mLp->SetLogFunc((void*)this, NMMosra::lpLogCallback);

    //this->mLp->SetPresolve(PRESOLVE_COLS |
    //                       PRESOLVE_ROWS |
    //                       PRESOLVE_IMPLIEDFREE |
    //                       PRESOLVE_REDUCEGCD |
    //                       PRESOLVE_MERGEROWS |
    //                       PRESOLVE_ROWDOMINATE |
    //                       PRESOLVE_COLDOMINATE |
    //                       PRESOLVE_KNAPSACK |
    //                       PRESOLVE_PROBEFIX);
    //
    //this->mLp->SetScaling(SCALE_GEOMETRIC |
    //                      SCALE_DYNUPDATE);

    this->mLp->Solve();

    // --------------------------------------------------
    //              Report results
    // --------------------------------------------------
    MosraLogInfo(<< "Report results ...");
    NMMsg(<< "Report results ..." << endl);

    QString stecReport = QString("%1/report_%2.txt").arg(pathname).arg("ParaOpt_STEC_landslide_multi");
    // this is where the desc vars for landslideing are retrieved and reported
    this->writeSTECReport2(stecReport, valstore);

    MosraLogInfo(<< this->msReport.toStdString() << endl);
    NMMsg(<< this->msReport.toStdString() << endl << endl);


    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}


int NMMosra::makeLp(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    if (this->mLp == 0)
        return 0;

    /*
     * structure of the lp matrix  assuming that
     * we've got two options (land uses)
     *
     * i : feature index  (0-based)
     * r : land use index (1-based) note: 0 represents the whole parcel (s. incentives below)
     *
     * X_i_r: area share of land-use r allocated to feature i
     * NOT USED! b_i  : binary conditional variable we need to model that
     *           a feature has to be either completely covered by
     *           any combination of the land use options or not at all
     *
     * note: row-0 contains the objective function and cell 0,0 contains
     *       the objective function value; row-1 and beyond contain the
     *       constraints of the model
     *
     *colindex:     0     1       2       3       4       5
     *row-0           | X_0_1 | X_0_2 | X_1_1 | X_1_2 | X_2_1 ...
     *row-1
     *
     * .....................................................................
     * if incentives are specified, the matrix structure becomes:
     *
     * q : incentive index
     *
     * R_i_r_q   note: r includes '0' value here, representing the whole parcel!
     *
     *colindex:     0     1       2        3         4         5         6         7         8        9       10       11        12
     *row-0           | X_0_1 | X_0_2 | R_0_0_1 | R_0_1_1 | R_0_2_1 | R_0_0_2 | R_0_1_2 | R_0_2_2 | X_1_1 | X_1_2 | R_1_0_1 | R_1_1_1 | R_1_2_1 | R_1_1_2 | R_1_2_2 | ...
     *row-1                                ^                                       ^
     *                                     |                                       |
     *                             reduction due to                        reduction through
     *                             retirement of parcel                  LU 1 w.r.t criterion 2
     *                             w.r.t criterion 1
     *                             (not used at the moment)
     * ......................................................................
     * DEPRECATED
     *colindex:     0     1       2      3      4       5
     *row-0           | X_0_1 | X_0_2 | b_0 | X_1_1 | X_1_2 | b_1 | ...
     *row-1 (constraint #1: i.e. coefficients for decision variables)
     *row-2 (constraint #2)
     *row-3  etc.
     *etc.                               ^
     *                                   |
     *                               deprecated
     */

    // create the LP structure (i.e. columns (i.e. decision vars),
    // column names and column types; note: the default type is REAL
    int lNumCells = mDataSet->getNumRecs();
    bool hole = mDataSet->hasColumn("nm_hole");
    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    this->mLp->MakeLp(0,this->mlLpCols);
    long colPos = 1;
    int featCount = 0;

    // we name the decision variables such that the feature index
    // matches the actual feature rowidx in the dataset, otherwise
    // would have trouble mapping the final results back to the
    // right features!
    for (int of=0; of < lNumCells; ++of)//, ++colPos)
    {
        if (    (hole && mDataSet->getIntValue("nm_hole", of) == 1)
             || (optFeatIdx != -1 && mDataSet->getIntValue(this->msOptFeatures, of) == 0)
           )
        {
            continue;
        }

        QString colname;
        for (int opt=1; opt <= this->miNumOptions; ++opt, ++colPos)
        {
            colname = QString(tr("X_%1_%2")).arg(of).arg(opt);
            this->mLp->SetColName(colPos, colname.toStdString());

            //MosraLogInfo(<< "#" << colPos << ": " << colname.toStdString() << std::endl);

            // set variable type for areal decision variables
            switch(this->meDVType)
            {
            case NMMosra::NM_MOSO_BINARY:
                this->mLp->SetBinary(colPos, true);
                break;
            case NMMosra::NM_MOSO_INT:
                this->mLp->SetInt(colPos, true);
                break;
            }
        }

        // add vars for incentivsed reduction (depending on x_i_r)
        for (int inc=1; inc <= this->mmslIncentives.size(); ++inc)
        {
            // the LU index here is '0' to indicate this variable
            // refers to the whole parcel!
            colname = QString(tr("R_%1_0_%2")).arg(of).arg(inc);
            this->mLp->SetColName(colPos, colname.toStdString());
            ++colPos;

            for (int opt=1; opt <= this->miNumOptions; ++opt, ++colPos)
            {
                colname = QString(tr("R_%1_%2_%3")).arg(of).arg(opt).arg(inc);
                this->mLp->SetColName(colPos, colname.toStdString());
            }
        }

        //		colname = QString(tr("b_%1")).arg(of);
        //		this->mLp->SetColName(colPos, colname.toStdString());
        //		this->mLp->SetBinary(colPos, true);
        //  ++colPos;

        ++featCount;
    }

    if (featCount != this->mlNumOptFeat)
    {
        MosraLogError(<< "makeLp(): mmh, the number of processed features "
                      << "does not match the number of identified optimisation features!")
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }





//	// initiate the objective function to zero; note that all unspecified
//	// obj functions values are initiated to zero by lp_solve, so we
//	// just set one value and then we're done
//	double row = 0;
//	int colno = 1;
//
//	this->mLp->SetAddRowmode(true);
//	this->mLp->SetObjFnEx(1, &row, &colno);
//	this->mLp->SetAddRowmode(false);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

vtkSmartPointer<vtkTable>
NMMosra::getDataSetAsTable()
{
    return mDataSet->getDataSetAsVtkTable();
}


bool
NMMosra::perturbCriterion(const QString& criterion,
        const QList<float>& percent)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // check, whether the dataset has been set yet
    if (   this->mDataSet == 0
        || mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_NONE
       )
    {
        MosraLogError( << "Input dataset has not been set!");
        return false;
    }

    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);


    //	vtkSmartPointer<vtkTable> tabCalc = this->getDataSetAsTable();

    // extract individual constraint or criterion identifier
    // from the 'criterion' parameter list, e.g.
    // constraint identifier: "OBJ:Nleach,OBJ:Sediment"
    // criterion identifier: "Nleach,Sediment"
    // -> split by ','

    QStringList metaList = criterion.split(",", Qt::SkipEmptyParts);

    // check for criterion or constraint
    if (metaList.size() == 0)
    {
        MosraLogError( << "Empty PERTURB parameter!");
        return false;
    }

    QString fstItem = metaList.at(0).trimmed();

    // -------------------------------------------------------------
    // CONSTRAINT
    if (    fstItem.startsWith(QStringLiteral("OBJ"))
         || fstItem.startsWith(QStringLiteral("CRI"))
         //|| fstItem.startsWith(QString("AREA"))
       )
    {
        // constraint
        for (int cri=0; cri < metaList.size(); ++cri)
        {
            float perc = percent.last();
            if (cri < percent.size())
                perc = percent.at(cri);
            if (!this->varyConstraint(metaList.at(cri), perc))
            {
                return false;
            }
        }
    }
    // -------------------------------------------------------------
    // INCENTIVE
    else if (fstItem.startsWith(QStringLiteral("INC")))
    {
        mDataSet->beginTransaction();
        for (int m=0; m < metaList.size(); ++m)
        {
            // get the incentives field
            const QString incSpec = metaList.at(m).trimmed();
            const QStringList incDetails = incSpec.split(":", Qt::SkipEmptyParts);

            QString incField;
            if (incDetails.size() >= 2)
            {
                incField = this->mmslIncentives[incDetails.at(1)].at(0);
            }

            if (incField.isEmpty() || !this->mDataSet->hasColumn(incField))
            {
                MosraLogError(<< "Couldn't find incentive field '"
                              << incField.toStdString() << "' for '"
                              << incDetails.at(1).toStdString() << "'!");
                mDataSet->rollBack();
                mDataSet->endTransaction();
                return false;
            }


            for (int r=0; r < mDataSet->getNumRecs(); ++r)
            {
                const double inval = mDataSet->getDblValue(incField, r);
                const double outval = inval + (inval * percent.at(m) / 100.0);
                mDataSet->setDblValue(incField, r, outval);
            }
        }
        mDataSet->endTransaction();
    }
    // -------------------------------------------------------------
    // CRITERION
    else
    {
        mDataSet->beginTransaction();

        for (int ptbItem=0; ptbItem < metaList.size(); ++ptbItem)
        {
            QString isolatedCriterion = metaList.at(ptbItem).trimmed();

            // extract the land use
            QStringList splitCriterion = isolatedCriterion.split(":", Qt::SkipEmptyParts);
            if (splitCriterion.size() < 2)
            {
                MosraLogError( << "Invalid criterion identifier: '"
                                  << isolatedCriterion.toStdString() << "'!");
                return false;
            }

            QString criLabel = splitCriterion.at(0);
            QString optLabel = splitCriterion.at(1);

            int optIdx = this->mslOptions.indexOf(optLabel);

            // get the performance score field for the specified land use criterion
            // combination
            QStringList fields = this->mmslCriteria.value(criLabel);
            if (fields.size() == 0)
            {
                MosraLogError( << "Got an empty field list for '"
                                  << criLabel.toStdString() << "'!");
                return false;
            }

            QString field = fields.at(optIdx);

            // identify the criterion
            //            QStringList fields = this->mmslCriteria.value(isolatedCriterion);
            //            if (fields.size() == 0)
            //            {
            //                MosraLogInfo(<< "nothing to perturb for criterion '"
            //                        << criterion.toStdString() << "'" << endl);
            //                return false;
            //            }

            // perturb field values by +/- percent of field value
            //srand(time(0));
            //for (int f=0; f < fields.size(); ++f)
            {
                //const QString& field = fields.at(f);

                // grab corresponding uncertainty level, or re-use
                // the last one, if there aren't any more
                float perc = percent.last();
                //if (f < percent.size())
                //    perc = percent.at(f);
                if (ptbItem < percent.size())
                {
                    perc = percent.at(ptbItem);
                }

                for (int r=0; r < mDataSet->getNumRecs(); ++r)
                {
                    if (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, r) == 0)
                    {
                        continue;
                    }
                    double inval = mDataSet->getDblValue(field, r);
                    double uncval = inval * ((rand() % ((int)perc+1))/100.0);
                    double newval;
                    if (rand() % 2)
                        newval = inval + uncval;
                    else
                        newval = inval - uncval;

                    newval = newval < 0 ? 0 : newval;
                    mDataSet->setDblValue(field, r, newval);
                }
            }
        }

        mDataSet->endTransaction();
    }

    NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

bool
NMMosra::varyConstraint(const QString& constraint,
                        float percent)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // identifying the constraint
    QStringList identlist = constraint.split(":");
    if (identlist.size() < 2)
    {
        MosraLogError( << "Invalid pertubation item specified: "
                << constraint.toStdString() << std::endl);
        NMDebugCtx(ctxNMMosra, << "done!");

        //NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
        //e.setDescription("Invalid pertubation item specified!");
        //throw e;

        return false;
    }

    // read information
    QString type = identlist.at(0);
    QString label = identlist.at(1);
    QString use = "";
    QString zone = "";
    if (identlist.size() >= 3)
        use = identlist.at(2);
    if (identlist.size() == 4)
        zone = identlist.at(3);

    if (    type.compare("CRI", Qt::CaseInsensitive) == 0
        &&  !use.isEmpty()
       )
    {
        // construct the 'key' we're looking for
        QString tident = use;
        if (!zone.isEmpty())
        {
            tident = QString("%1:%2").arg(use).arg(zone);
        }

        // since we could have more than one constraint with the same
        // label, we just go through and check the use
        QMap<QString, QStringList> lufm;
        QMultiMap<QString, QMap<QString, QStringList> >::iterator it =
                mmslCriCons.begin();
        while(it != mmslCriCons.end())
        {
            if (it.key().compare(label) == 0)
            {
                if (it.value().find(tident) != it.value().end())
                {
                    lufm = mmslCriCons.take(it.key());
                    break;
                }
            }

            ++it;
        }


        if (lufm.isEmpty())
        {
            MosraLogError( << "Land use field map is empty!");
            NMDebugCtx(ctxNMMosra, << "done!");
            //NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
            //e.setDescription("Land use field map is empty!");
            //throw e;
            return false;
        }

        // get the field list + the operator + cap
        QStringList fieldvaluelist = lufm.value(tident);
        if (fieldvaluelist.empty())
        {
            MosraLogError( << "Field value list is empty!");
            NMDebugCtx(ctxNMMosra, << "done!");
            //NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
            //e.setDescription("field value list is empty!");
            //throw e;
            return false;
        }

        fieldvaluelist = lufm.take(tident);

        bool bok;
        double cap = fieldvaluelist.last().toDouble(&bok);
        if (!bok)
        {
            MosraLogError( << "Constraint threshold is not a number!")
            NMDebugCtx(ctxNMMosra, << "done!");
            //NMMfwException e(NMMfwException::NMMosra_InvalidParameter);
            //e.setDescription("Land use field map is empty!");
            //throw e;
            return false;
        }

        MosraLogDebug(<< "old value for " << constraint.toStdString()
                << " = " << cap << endl);


        // adjust the cap value by adding the percentage of change
        cap += (cap * percent/100.0);

        MosraLogDebug(<< "new value for " << constraint.toStdString()
                << " = " << cap << endl);

        // vary the constraint value (cap)
        QString strVal = QString("%1").arg(cap);
        fieldvaluelist.replace(fieldvaluelist.size()-1, strVal);

        // put the multi-map back together again
        lufm.insert(tident, fieldvaluelist);
        mmslCriCons.insert(label, lufm);

    }
    else if (type.compare("AREA", Qt::CaseInsensitive) == 0)
    {
        //ToDo
    }
    else if (type.compare("OBJ", Qt::CaseInsensitive) == 0)
    {
        // look for the objective contraint label (not key!) and
        // vary the contraint
        QMap<QString, QStringList>::iterator oconsIt =
                this->mmslObjCons.begin();

        while(oconsIt != this->mmslObjCons.end())
        {
            if (oconsIt.value().contains(label))
            {
                double val = oconsIt.value().takeLast().toDouble();
                MosraLogDebug(<< label.toStdString() << " old: " << val << std::endl);
                val += (val * percent/100.0);
                MosraLogDebug(<< label.toStdString() << " new: " << val << std::endl);
                oconsIt.value().push_back(QString("%1").arg(val));
                break;
            }

            ++oconsIt;
        }
    }

    NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

int NMMosra::addObjFn(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // get the hole array
    bool hole = mDataSet->hasColumn("nm_hole");
    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    // some vars we need

    // init some mem for the row and column objects of the
    // problem
    const long dvars = this->mlNumDVar;
    double* pdRow = new double[dvars];
    int* piColno = new int[dvars];
    int* plFieldIndices = new int[this->miNumOptions];

    // each incentive has only one field associated with it
    const int numIncentives = this->mmslIncentives.size();
    int* piIncFieldIdx = numIncentives > 0 ? new int[numIncentives] : nullptr;

    const int holeidx = mDataSet->getColumnIndex("nm_hole");
    const int skipIncentivesOffset = numIncentives * miNumOptions + numIncentives;

    const long lNumCells = mDataSet->getNumRecs();
    double dCoeff, dWeight = 1;
    bool convOK;

    QString s1Maxmin = this->mmslObjectives.begin().value().at(0).left(3);
    QString sxMaxmin;

    // ------------------------------------------------------------------for each objective
    // set coefficients of areal decision variables
    QMap<QString, QStringList>::const_iterator it =
            this->mmslObjectives.constBegin();

    unsigned int objcount = 0;
    for (; it != this->mmslObjectives.constEnd(); ++it, ++objcount)
    {
        // need to check whether we've got to account for incentives or not
        bool bIncentivise = false;
        if (numIncentives > 0)
        {
            // need to check whether the current objective is incentivsed at all
            QString objCri = it.key();
            QMap<QString, QStringList>::const_iterator incIt = mIncNamePair.cbegin();
            while (incIt != mIncNamePair.cend())
            {
                if (!incIt.value().isEmpty())
                {
                    if (incIt.value().at(0).compare(objCri, Qt::CaseInsensitive) == 0)
                    {
                        bIncentivise = true;
                        exit;
                    }
                }
                ++incIt;
            }
        }

        // get field indices for the performance indicators
        MosraLogDebug( << "objective '" << it.key().toStdString() << "' ..." << endl);

        MosraLogDebug( << "criterion / option: <performance indi field> <field index> ... " << endl);
        for (int option=0; option < this->miNumOptions; option++)
        {
            // field name of performance indicator for
            // current criterion and option
            QString sCriterion = it.key();
            QStringList criFieldList = this->mmslCriteria.value(sCriterion);
            QString sField = "";
            if (option <= criFieldList.size()-1)
                sField = criFieldList.at(option);

            plFieldIndices[option] = mDataSet->getColumnIndex(sField);
            if (plFieldIndices[option] == -1)
            {
                MosraLogError( << "failed to get performance indicator '"
                        << sField.toStdString() << "' for option '"
                        << this->mslOptions.at(option).toStdString() << "'!");

                // free some memory
                delete[] pdRow;
                delete[] piColno;
                delete[] plFieldIndices;

                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }

            MosraLogDebug( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
                    << ": " << sField.toStdString() << " " << plFieldIndices[option] << endl);
        }

        MosraLogDebug( << "incentive: <incentive field> <field index> ..." << std::endl);
        QMap<QString, QStringList>::ConstIterator incIt = this->mmslIncentives.constBegin();
        int inccount = 0;
        while (incIt != this->mmslIncentives.constEnd())
        {
            const int ifidx = mDataSet->getColumnIndex(incIt.value().at(0));
            if (ifidx == -1)
            {
                MosraLogError( << "Failed to retreive columnn index for '"
                               << incIt.value().at(0).toStdString() << "!");

                delete[] pdRow;
                delete[] piColno;
                delete[] plFieldIndices;
                delete[] piIncFieldIdx;

                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }
            else
            {
                piIncFieldIdx[inccount] = ifidx;
            }
            ++inccount;
            ++incIt;
        }


        // maximise or minimise ?
        sxMaxmin = it.value().at(0).left(3);

        // align optimisation task with that of objective 1
        bool bMalMinusEins = false;
        if (s1Maxmin.compare(sxMaxmin, Qt::CaseInsensitive) != 0)
            bMalMinusEins = true;

        // get the weight for this objective
        dWeight = 1.0;
        if (it.value().size() == 2)
        {
            dWeight = it.value().at(1).toDouble(&convOK);
        }
        MosraLogDebug( << "sxMaxmin: " << sxMaxmin.toStdString() << " | adjust task: " << bMalMinusEins << " | weight: " << dWeight << endl);

        MosraLogDebug( << "processing individual features now" << endl);
        // --------------------------------------------------for each feature
        long colPos = 1;
        long arpos = 0;
        for (int f=0; f < lNumCells; ++f)
        {
            // leap over holes
            if (    (hole && mDataSet->getIntValue(holeidx, f) == 1)
                 || (optFeatIdx != -1 && mDataSet->getIntValue(this->msOptFeatures, f) == 0)
               )
            {
                continue;
            }

            /*
            if (arpos + miNumOptions + skipIncentivesOffset > dvars)
            {
                NMDebugAI(<< "NMMosra::addObjFn() - decision variable buffer out of bounds: "
                           << "max_idx=" << (dvars-1) << ", but will reach arpos=" << (arpos + miNumOptions + skipIncentivesOffset-1)
                           << " for optFeatCount=" << optFeatCount << "!");
                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }
            */

            // ----------------------------------------------for each option
            for (int option=0; option < this->miNumOptions; option++, arpos++, colPos++)
            {
                // get performance indicator for current feature and
                // criterion and option
                dCoeff = mDataSet->getDblValue(plFieldIndices[option], f);

                // DEBUG
                //if (f < 100)
                //	MosraLogDebug( << lCounter << " : " << this->mslOptions.at(option).toStdString()
                //			<< " : " << da->GetName() << " : " << dCoeff << endl);

                // adjust according to MinMax of objective 1
                dCoeff = bMalMinusEins ? dCoeff * -1 : dCoeff;

                // apply objective weight if applicable
                if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
                    dCoeff *= dWeight;

                // do we have binary decision variables?
                //              double area = mDataSet->getDblValue(this->msAreaField, f);
                //				QString sArea = QString(tr("%1")).arg(area, 0, 'g');
                //				bool bok;
                //				switch(this->meDVType)
                //				{
                //				case NM_MOSO_BINARY:
                //                    dCoeff = vtkMath::Floor(dCoeff);
                //					dCoeff = (int)dCoeff * (int)sArea.toDouble(&bok);
                //					break;
                //				case NM_MOSO_INT:
                //					dCoeff = vtkMath::Floor(dCoeff);
                //					break;
                //				}


                // write coefficient into row array
                if (objcount)
                    pdRow[arpos] += dCoeff;
                else
                    pdRow[arpos] = dCoeff;
                piColno[arpos] = colPos;

                //NMDebug(<< this->mLp->GetColName(colPos) << "=" << pdRow[arpos] << "  ");

            }

            if (bIncentivise)
            {
                for (int in=0; in < numIncentives; ++in)
                {
                    dCoeff = mDataSet->getDblValue(piIncFieldIdx[in], f);
                    dCoeff = bMalMinusEins ? dCoeff * -1 : dCoeff;
                    if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
                    {
                        dCoeff *= dWeight;
                    }

                    // add the coefficient for the parcel-retirement reduction variable
                    if (objcount)
                        pdRow[arpos] += 0.0;//dCoeff;
                    else
                        pdRow[arpos] = 0.0;//dCoeff;
                    piColno[arpos] = colPos;

                    ++arpos;
                    ++colPos;


                    // add the coefficients for the land-use specific reduction variables
                    for (int option=0; option < miNumOptions; ++option, ++arpos, ++colPos)
                    {
                        if (objcount)
                            pdRow[arpos] += dCoeff;
                        else
                            pdRow[arpos] = dCoeff;
                        piColno[arpos] = colPos;
                    }
                }
            }
            else
            {
                colPos += skipIncentivesOffset;
            }

            // increment the column number counter to the next areal decision variable
            // (leaping one binary deciscion var associated with each areal decision variable)
            //colPos++;

            if (f % 500 == 0)
                NMDebug(<< ".");
        }

        NMDebug(<< " finished!" << endl);
    }

    //int ncols = this->mLp->GetNColumns();
    //MosraLogDebug( << "num cols: " << ncols << " | NumArealDVar: " << this->mlNumArealDVar << endl);


    // set the objective function
    this->mLp->SetAddRowmode(true);
    this->mLp->SetObjFnEx(this->mlNumDVar, pdRow, piColno);
    this->mLp->SetAddRowmode(false);

    // set the optimisation task
    if (s1Maxmin.compare(tr("min"), Qt::CaseInsensitive) == 0)
        this->mLp->SetMinim();
    else
        this->mLp->SetMaxim();

    // free some memory
    delete[] pdRow;
    delete[] piColno;
    delete[] plFieldIndices;
    delete[] piIncFieldIdx;

    NMDebugCtx(ctxNMMosra, << "done!");

    return 1;
}

int NMMosra::addObjCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // get data set attributes
    // get the hole array
    bool hole = mDataSet->hasColumn("nm_hole");
    long lNumCells = mDataSet->getNumRecs();
    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    // create vectors holding the constraint types
    // and objective constraint labels
    std::vector<unsigned int> vnConsType;
    std::vector<QString> vsObjConsLabel;
    QMap<QString, QStringList>::const_iterator it = this->mmslObjCons.constBegin();

    MosraLogDebug( << "reading props" << endl);
    for (; it != this->mmslObjCons.constEnd(); it++)
    {
        // add the operator
        if (it.value().at(1).compare(tr("<=")) == 0)
            vnConsType.push_back(1);
        else if (it.value().at(1).compare(tr(">=")) == 0)
            vnConsType.push_back(2);
        else // =
            vnConsType.push_back(3);

        // add the key i.e. objective as label
        vsObjConsLabel.push_back(it.key());

        MosraLogDebug( << it.key().toStdString() << ": " << it.value().join(tr(" ")).toStdString() << endl);
    }

    // allocate the required memory
    double* pdRow = nullptr; //new double[this->mlNumArealDVar];
    int* piColno = nullptr; //new int[this->mlNumArealDVar];
    int* piFieldIndices = new int[this->miNumOptions];

    // check whether we've got to account for incentives
    const int iNumIncentives = this->mmslIncentives.size();
    int* piIncFieldIdx = iNumIncentives > 0 ? new int[iNumIncentives] : nullptr;


    // some more vars
    double dConsVal;
    double dCoeff;

    // turn on row mode
    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    it = this->mmslObjCons.constBegin();
    // ------------------------------------------------for each objective
    for (int obj=0; it != this->mmslObjCons.constEnd(); it++, obj++)
    {
        // need to check whether we've got to account for incentives or not
        bool bIncentivise = false;
        if (iNumIncentives > 0)
        {
            // need to check whether the current objective is incentivsed at all
            QString objCri = it.value().at(0);
            QMap<QString, QStringList>::const_iterator incNameIt = mIncNamePair.cbegin();
            while (incNameIt != mIncNamePair.cend())
            {
                if (!incNameIt.value().isEmpty())
                {
                    if (incNameIt.value().at(0).compare(objCri, Qt::CaseInsensitive) == 0)
                    {
                        bIncentivise = true;
                        exit;
                    }
                }
                ++incNameIt;
            }
        }

        if (bIncentivise)
        {
            pdRow = new double[this->mlNumDVar];
            piColno = new int[this->mlNumDVar];
        }
        else
        {
            pdRow = new double[this->mlNumArealDVar];
            piColno = new int[this->mlNumArealDVar];
        }


        // get the constraint value
        bool bConvOK;
        double dVal =
        dVal = it.value().at(2).toDouble(&bConvOK);
        if (bConvOK)
            dConsVal = dVal;
        else
            MosraLogError( << "couldn't convert " << it.value().at(2).toStdString() << " into a double!" << endl);

        MosraLogDebug( << it.key().toStdString() << endl);

        MosraLogDebug( << "criterion / option: <performance indi field> <field index> ... " << endl);
        for (int option=0; option < this->miNumOptions; option++)
        {
            // field name of performance indicator for
            // current criterion and option
            QString sCriterion = it.value().at(0);
            QString sField = this->mmslCriteria.value(sCriterion).at(option);

            piFieldIndices[option] = mDataSet->getColumnIndex(sField);

            if (piFieldIndices[option] == -1)
            {
                MosraLogError( << "failed to get performance indicator '"
                        << sField.toStdString() << "'!");

                // free some memory
                delete[] pdRow;
                delete[] piColno;
                delete[] piFieldIndices;

                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }

            MosraLogDebug( << sCriterion.toStdString() << " / " << this->mslOptions.at(option).toStdString()
                    << ": " << sField.toStdString() << " " << piFieldIndices[option] << endl);
        }

        MosraLogDebug( << "incentive: <incentive field> <field index> ..." << std::endl);
        QMap<QString, QStringList>::ConstIterator incIt = this->mmslIncentives.constBegin();
        int inccount = 0;
        while (incIt != this->mmslIncentives.constEnd())
        {
            const int ifidx = mDataSet->getColumnIndex(incIt.value().at(0));
            if (ifidx == -1)
            {
                MosraLogError( << "Failed to retreive columnn index for '"
                               << incIt.value().at(0).toStdString() << "!");

                delete[] pdRow;
                delete[] piColno;
                delete[] piFieldIndices;
                delete[] piIncFieldIdx;

                NMDebugCtx(ctxNMMosra, << "done!");
                return 0;
            }
            else
            {
                piIncFieldIdx[inccount] = ifidx;
            }
            ++inccount;
            ++incIt;
        }


        // ------------------------------------------------------------for each spatial alternative
        const int skipIncentivesOffset = iNumIncentives * miNumOptions + iNumIncentives;
        long lCounter = 0;
        long colpos = 1;
        for (int f=0; f < lNumCells; f++)
        {
            // leap frog holes in polygons
            if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                 || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
               )
            {
                continue;
            }

            for (int option=0; option < this->miNumOptions; option++)
            {
                dCoeff = mDataSet->getDblValue(piFieldIndices[option], f);

                // do we have binary decision variables?
                //              double area = mDataSet->getDblValue(this->msAreaField, f);
                //				QString sArea = QString(tr("%1")).arg(area, 0, 'g');
                //				bool bok;
                //				switch(this->meDVType)
                //				{
                //				case NM_MOSO_BINARY:
                //					dCoeff = vtkMath::Floor(dCoeff);
                //					dCoeff = (int)dCoeff * (int)sArea.toDouble(&bok);
                //					break;
                //				case NM_MOSO_INT:
                //					dCoeff = vtkMath::Floor(dCoeff);
                //					break;
                //				}

                pdRow[lCounter] = dCoeff;
                piColno[lCounter] = colpos;

                // increment the counter
                lCounter++;
                colpos++;


            }

            if (bIncentivise)
            {
                for (int in=0; in < iNumIncentives; ++in)
                {
                    dCoeff = mDataSet->getDblValue(piIncFieldIdx[in], f);

                    // add the coefficient for the parcel-retirement reduction variable
                    pdRow[lCounter] = 0.0; //dCoeff;
                    piColno[lCounter] = colpos;

                    ++lCounter;
                    ++colpos;


                    // add the coefficients for the land-use specific reduction variables
                    for (int option=0; option < miNumOptions; ++option, ++lCounter, ++colpos)
                    {
                        pdRow[lCounter] = dCoeff;
                        piColno[lCounter] = colpos;
                    }
                }
            }
            else
            {
                colpos += skipIncentivesOffset;
            }

            // leap frog the binary decision variable for this feature
            //colpos++;

            if (f % 100 == 0)
                NMDebug(<< ".");

        } // end cell iteration

        NMDebug(<< " finished!" << endl);

        // add the constraint to the matrix
        this->mLp->AddConstraintEx(iNumIncentives > 0 ? this->mlNumDVar : this->mlNumArealDVar,
                                   pdRow, piColno, vnConsType.at(obj), dConsVal);

        // increment the row counter
        lRowCounter++;

        // add the label for this constraint
        this->mLp->SetRowName(lRowCounter, vsObjConsLabel.at(obj).toStdString());

        delete[] pdRow;
        delete[] piColno;

    } // end objective constraint iteration


    this->mLp->SetAddRowmode(false);

    // free some memory
    delete[] piFieldIndices;

    NMDebugCtx(ctxNMMosra, << "done!");

    return 1;
}


int NMMosra::addFeatureSetConsDb(void)
{
    /// ToDo: add incentives support for constraining criteria that benefit from incentives!!!

    NMDebugCtx(ctxNMMosra, << "...");
    MosraLogInfo(<<"adding feature-set constraints ...");

    // get a list of feature-set ids
    otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());

    if (sqltab.IsNull())
    {
        MosraLogError(<< "Failed fetching the SQLite-based attribute table (*.ldb)!");
        NMDebugCtx(ctxNMMosra, << "done!");
        return 0;
    }



    // ==============================================================================================
    // process individual feature-set constraints
    // key: <option1[+option2[+option3[+...]]] | total>:<id_column_name>
    //               0                 1               2
    // value: <criterion_label> < <= | = | >= > <rhs_column_name>
    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    QMap<QString, QStringList>::ConstIterator fsIt = this->mslFeatSetCons.constBegin();
    while (fsIt != this->mslFeatSetCons.constEnd())
    {
        MosraLogInfo(<< "   -> " << this->msFeatureSetConsLabel[fsIt.key()].toStdString() << "...");
        // extract the options & feature-set id column index
        QStringList optColPair = fsIt.key().split(":", Qt::SkipEmptyParts);
        QStringList optraw = optColPair.at(0).split("+", Qt::SkipEmptyParts);
        QStringList options;
        if (optraw.size() == 1)
        {
            if (this->mslOptions.contains(optraw.at(0)))
            {
                options << optraw.at(0);
            }
            else if (optraw.at(0).compare(QStringLiteral("total"), Qt::CaseInsensitive) == 0)
            {
                for (int i=0; i < this->mslOptions.size(); ++i)
                {
                    options << this->mslOptions.at(i);
                }
            }
        }
        else
        {
            for (int r=0; r < optraw.size(); ++r)
            {
                if (this->mslOptions.contains(optraw.at(r)))
                {
                    options << optraw.at(r);
                }
            }
        }

        // workout operator
        int operatorId = -1;
        QString compOp = fsIt.value().at(1);
        QString compTypeLabel;
        if (compOp.compare(tr("<=")) == 0)
        {
            operatorId = 1;
            compTypeLabel = "upper";
        }
        else if (compOp.compare(tr(">=")) == 0)
        {
            operatorId = 2;
            compTypeLabel = "lower";
        }
        else if (compOp.compare(tr("=")) == 0)
        {
            operatorId = 3;
            compTypeLabel = "equals";
        }

        // -----------------------------------------------------------------------------
        // fetch a table of unique feature set ids and the associated threshold values
        // EXAMPLE: select distinct cat_n, TN_MAL from luopt_hl2_1 where optFeat == 1 and cat_n > 0;

        //SQLiteTable::TableDataFetch(std::vector<std::vector<ColumnValue> > &restab,
        //                            const std::vector<TableColumnType> &coltypes,
        //                            const std::string &query)

        QString criterion = fsIt.value().at(0);
        std::string idColName = optColPair.at(1).toStdString();
        std::string rhsColName = fsIt.value().at(2).toStdString();

        bool bOptFeat = false;
        std::string optfeatColName = "";
        if (!this->msOptFeatures.isEmpty())
        {
            bOptFeat = true;
            optfeatColName = this->msOptFeatures.toStdString();
        }

        std::vector< std::vector< otb::AttributeTable::ColumnValue > > fsids;
        std::vector<otb::AttributeTable::TableColumnType> types;
        types.push_back(otb::AttributeTable::ATTYPE_INT);
        types.push_back(otb::AttributeTable::ATTYPE_DOUBLE);

        std::stringstream q_fstable;
        q_fstable << "SELECT distinct \"" << idColName << "\", "
                  << "\"" << rhsColName << "\" from \"" << sqltab->GetTableName() << "\" "
                  << "where \"" << idColName << "\" > 0";
        if (bOptFeat)
        {
            q_fstable << " and \"" << optfeatColName << "\" == 1";
        }
        q_fstable << ";";

        //MosraLogInfo(<< "   -> query:" << q_fstable.str() << std::endl);

        if (!sqltab->TableDataFetch(fsids, types, q_fstable.str()))
        {
            MosraLogError(<< "NMMosra::addFeatureSetCons() - " << sqltab->getLastLogMsg());
            return 0;
        }

        // get option index/position within option set / criterion score field sets
        std::vector<int> optionIdx;
        for (int k=0; k < options.size(); ++k)
        {
            optionIdx.push_back(this->mslOptions.indexOf(options[k]));
        }

        // -------------------------------------------------------------------------------
        // iterate over individual feature-sets and add a constraint for each
        for (int fs=0; fs < fsids.size(); ++fs)
        {
            long long id = fsids[fs][0].ival;
            //MosraLogDebug(<< "      ID: " << id << "(" << fs+1 << "/" << fsids.size() << ")...");
            const double rhsValue = fsids[fs][1].dval;

            // .....................................................
            // select option_criterion_score_column1, ...2, ...3, ....,
            // from table_name where cat_n == and optFeat == 1
            // EXAMPLE: select rowid, n_dai, n_snb, n_for from luopt_hl2_1 where optFeat == 1 and cat_n == 13068931;

            std::vector< std::vector< otb::AttributeTable::ColumnValue > > scoretab;
            std::vector< otb::AttributeTable::TableColumnType > scoretypes(options.size()+2, otb::AttributeTable::ATTYPE_DOUBLE);
            scoretypes[0] = otb::AttributeTable::ATTYPE_INT;

            std::stringstream q_scores;

            q_scores << "SELECT rowid, \"" << this->msAreaField.toStdString() << "\", ";
            for (int sc=0; sc < optionIdx.size(); ++sc)
            {
                q_scores << "\"" << this->mmslCriteria[criterion][optionIdx[sc]].toStdString() << "\"";
                if (sc < optionIdx.size()-1)
                {
                    q_scores << ",";
                }
            }

            q_scores << " from \"" << sqltab->GetTableName() << "\" where ";
            if (bOptFeat)
            {
                q_scores << "\"" << optfeatColName << "\" == 1 and ";
            }

            q_scores << "\"" << idColName << "\" == " << id << ";";

            if (!sqltab->TableDataFetch(scoretab, scoretypes, q_scores.str()))
            {
                MosraLogError(<< "NMMosra::addFeatureSetCons() - " << sqltab->getLastLogMsg());
                return 0;
            }


            // ..........................................................................
            // adding the constraint for this feature set
            /// WARNING: use rowid-1 for colpos!
            //const int skipIncentiveFeatOffset = this->mmslIncentives.size() * miNumOptions + mmslIncentives.size();
            const int bsize = optionIdx.size() * scoretab.size();
            //MosraLogInfo(<< "       SCORETAB: " << optionIdx.size() << " x " << scoretab.size() << " = " << bsize);


            double* pdRow = new double[bsize];
            int* piColno = new int[bsize];
            long arpos = 0;

            for (int sr=0; sr < scoretab.size(); ++sr)
            {
                // get 1-based rowid / feature id from db
                const long long rowid = scoretab[sr][0].ival;

                //MosraLogInfo(<< "         FEAT: " << rowid << "(" << sr+1 << "/" << scoretab.size() << ")...");

                // calc 1-based column position within the optimsation matrix pointing at the first option
                // for a given feature (cf. makeLp() for details)
                //const int colPos = static_cast<int>(rowid-1) * (miNumOptions + skipIncentiveFeatOffset) + 1;
                std::stringstream colname;
                colname << "X_" << rowid-1 << "_1";
                const int colPos = this->mLp->GetNameIndex(colname.str(), false);
                if (colPos < 0)
                {
                    MosraLogError(<< "Serious ERROR! Invalid column index detected!");
                    return 0;
                }


                for (int opt=0; opt < optionIdx.size(); ++opt, ++arpos)
                {
                    double coeff = 0.0;
                    switch(this->meDVType)
                    {
                    case NMMosra::NM_MOSO_BINARY:
                        // add + 2 because the first column in scoretab is 'rowid',
                        // the second is 'AreaHa' [1]!
                        coeff = scoretab[sr][opt+2].dval * scoretab[sr][1].dval;
                        break;
                    default:
                        coeff = scoretab[sr][opt+2].dval;
                        break;
                    }

                    pdRow[arpos] = coeff;
                    // add the option offset (index) to the current colPos
                    piColno[arpos] = colPos + optionIdx[opt];
                }
            }

            // add constraint & constraint name
            this->mLp->AddConstraintEx(bsize, pdRow, piColno, operatorId, rhsValue);

            ++lRowCounter;

            QString rowlabel = QString("%1_%2_%3_%4")
                    .arg(this->msFeatureSetConsLabel[fsIt.key()])
                    .arg(fsIt.key())
                    .arg(id)
                    .arg(compTypeLabel);

            this->mLp->SetRowName(lRowCounter, rowlabel.toStdString());
            MosraLogDebug(<< "      Added constraint: " << rowlabel.toStdString() << "!"
                          << std::endl << std::endl);

            delete[] pdRow;
            delete[] piColno;
            pdRow = nullptr;
            piColno = nullptr;
        }
        MosraLogInfo(<< "   -> " << this->msFeatureSetConsLabel[fsIt.key()].toStdString() << " done!");
        ++fsIt;
    }

    this->mLp->SetAddRowmode(false);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::addZoneCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");
    MosraLogInfo(<< "adding non-overlapping zone constraints ...");

    // iterate over the table to fetch the data and put it into the maps
    // and vectors
    QMap<QString, QStringList>::ConstIterator zconsIt = mslZoneConstraints.constBegin();
    while (zconsIt != mslZoneConstraints.cend())
    {
        // ... for each zone
        QMap<int, double>           mRHS;
        QMap<int, QVector<int> >    mZoneRowIds;
        QMap<int, QStringList>      mvPerformanceFields;
        QMap<int, QStringList>      mvIncFields;
        QMap<int, QVector<int> >    mvResourceIdx;
        int                         consOp;

        QString consLabel = zconsIt.key();
        QString opStr = zconsIt.value().at(3);
        if (opStr.compare("<=") == 0)
        {
            consOp = 1;
        }
        else if (opStr.compare(">=") == 0)
        {
            consOp = 2;
        }
        else
        {
            consOp = 3;
        }

        QString rhsField = zconsIt.value().at(4);
        QString zoneField = zconsIt.value().at(0);
        QString resField = zconsIt.value().at(1);

        bool bAllRes = false;
        QVector<int> allResIds;
        if (resField.compare("total") == 0)
        {
            bAllRes = true;
            for (int r=0; r < this->miNumOptions; ++r)
            {
                allResIds.push_back(r);
            }
        }
        QStringList allPerfFields = mmslCriteria[zconsIt.value().at(2)];

        //========================================================================================
        // loop over the data set and identify zone and track required information
        const int miNumInc = mmslIncentives.size();
        const int skipIncColOffset = miNumInc * miNumOptions + miNumInc;
        const bool hole = mDataSet->hasColumn("nm_hole");
        const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);
        const long lNumCells = mDataSet->getNumRecs();
        long lRowCounter = this->mLp->GetNRows();
        this->mLp->SetAddRowmode(true);

        for (long f=0; f < lNumCells; ++f)
        {
            if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                    || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
               )
            {
                continue;
            }

            int zoneID = mDataSet->getIntValue(zoneField, f);
            if (mZoneRowIds.contains(zoneID))
            {
                mZoneRowIds[zoneID].push_back(f);
            }
            else
            {
                // add a new zone to the zone map
                QVector<int> rows;
                rows.push_back(f);
                mZoneRowIds[zoneID] = rows;

                // add a new rhs value to the rhs map
                double rhs = mDataSet->getDblValue(rhsField, f);
                mRHS[zoneID] = rhs;

                // get the resource indices for this zone
                // and the associated performance indicator fields
                if (!bAllRes)
                {
                    QVector<int> resIdx;
                    QStringList perfFields;
                    QStringList zoneRes = mDataSet->getStrValue(resField, f).split(" ", Qt::SkipEmptyParts);
                    foreach(const QString& res, zoneRes)
                    {
                        int idx = mslOptions.indexOf(QRegExp(res, Qt::CaseInsensitive,  QRegExp::FixedString));
                        if (idx > 0)
                        {
                            resIdx.push_back(idx);
                            perfFields.push_back(allPerfFields.at(idx));
                        }
                    }

                    mvResourceIdx[zoneID] = resIdx;
                    mvPerformanceFields[zoneID] = perfFields;
                }

                // get the incentives fields, if any, for this zone
                QStringList iFields;
                if (this->mIncNamePair.size() > 0)
                {
                    QMap<QString, QStringList>::ConstIterator incIt = mIncNamePair.cbegin();
                    while (incIt != mIncNamePair.cend())
                    {
                        if (zconsIt.value().at(2).compare(incIt.value().at(0), Qt::CaseInsensitive) == 0)
                        {
                            iFields.push_back(incIt.value().at(1));
                        }
                        ++incIt;
                    }
                }
                mvIncFields[zoneID] = iFields;
            }
        }

        //=================================================================================
        // process identified zones

        int zoneCounter = 0;
        QMap<int, QVector<int> >::ConstIterator zoneIt = mZoneRowIds.cbegin();
        while (zoneIt != mZoneRowIds.cend())
        {
            const int zoneID = zoneIt.key();
            double rhs = mRHS[zoneID];
            int numCoeffs = 0;
            int numRes = 0;
            int numIncs = mvIncFields[zoneID].size();

            if (bAllRes)
            {
                numRes = allPerfFields.size();
            }
            else
            {
                numRes = mvPerformanceFields[zoneID].size();
            }

            // ...............................................................
            // allocate row buffers and populate
            const QVector<int>& vRows = zoneIt.value();
            numCoeffs = (numRes + numIncs) * vRows.size();

            double* pdRow = new double[numCoeffs];
            int* piColno = new int[numCoeffs];

            long coeffCounter = 0;
            for (int r=0; r < vRows.size(); ++r)
            {
                long colPos = 1 + vRows[r] * (this->miNumOptions + (miNumOptions * mmslIncentives.size()) + mmslIncentives.size());
                double coeff = 0.0;
                if (bAllRes)
                {
                    for (int arpos=0; arpos < numRes; ++arpos)
                    {
                        if (this->meDVType == NMMosoDVType::NM_MOSO_BINARY)
                        {
                            coeff = mDataSet->getDblValue(this->msAreaField, vRows[r])
                                    * mDataSet->getDblValue(allPerfFields[arpos], vRows[r]);
                        }
                        else
                        {
                            coeff = mDataSet->getDblValue(allPerfFields[arpos], vRows[r]);
                        }
                        pdRow[coeffCounter] = coeff;
                        piColno[coeffCounter] = colPos + allResIds[arpos];
                        ++coeffCounter;
                    }
                }
                else
                {
                    QStringList coeffFields = mvPerformanceFields[zoneID];
                    QVector<int> resix = mvResourceIdx[zoneID];
                    for (int arpos=0; arpos < numRes; ++arpos)
                    {
                        if (this->meDVType == NMMosoDVType::NM_MOSO_BINARY)
                        {
                            coeff = mDataSet->getDblValue(this->msAreaField, vRows[r])
                                    * mDataSet->getDblValue(coeffFields[arpos], vRows[r]);
                        }
                        else
                        {
                            coeff = mDataSet->getDblValue(coeffFields[arpos], vRows[r]);
                        }
                        pdRow[coeffCounter] = coeff;
                        piColno[coeffCounter] = colPos + resix[arpos];
                        ++coeffCounter;
                    }
                }

                // add incentives, if applicable
                if (mvIncFields[zoneID].size() > 0)
                {
                    QVector<int> optidx = mvResourceIdx[zoneID];
                    QStringList incFields = mvIncFields[zoneID];
                    for (int inc=0; inc < numIncs; ++inc)
                    {
                        if (this->meDVType == NMMosoDVType::NM_MOSO_BINARY)
                        {
                            coeff = mDataSet->getDblValue(incFields[inc], vRows[r])
                                     * mDataSet->getDblValue(msAreaField, vRows[r]);
                        }
                        else
                        {
                            coeff = mDataSet->getDblValue(incFields[inc], vRows[r]);
                        }

                        if (bAllRes)
                        {
                            for (int opt=0; opt < numRes; ++opt)
                            {
                                pdRow[coeffCounter] = coeff;
                                piColno[coeffCounter] = colPos + allResIds[opt] + (inc+1) * miNumOptions + 1 + inc;
                                ++coeffCounter;
                            }
                        }
                        else
                        {
                            for (int opt=0; opt < numRes; ++opt)
                            {
                                pdRow[coeffCounter] = coeff;
                                piColno[coeffCounter] = colPos + optidx[opt] + (inc+1) * miNumOptions + 1 + inc;
                                ++coeffCounter;
                            }
                        }
                    }
                }
            }

            // add or modify constraint
            this->mLp->AddConstraintEx(numCoeffs, pdRow, piColno, consOp, rhs);
            ++lRowCounter;

            QString zoneLabel = QString("%1_%2").arg(consLabel).arg(zoneID);
            this->mLp->SetRowName(lRowCounter, zoneLabel.toStdString());

            delete[] pdRow;
            delete[] piColno;

            if (zoneCounter % 200 == 0)
            {
                NMDebug(<< ".");
            }
            ++zoneCounter;
            ++zoneIt;
        }
        ++zconsIt;
    }

    this->mLp->SetAddRowmode(false);
    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::addFeatureCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    const bool hole = mDataSet->hasColumn("nm_hole");
    const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    std::vector<QString> vsConsLabel;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    long lNumCells = mDataSet->getNumRecs();
    double dUserVal;
    double dUserArea = 0.0;
    double dAreaTotal = this->mdAreaTotal;

    QMap<QString, QStringList>::const_iterator it =
            this->mmslFeatCons.constBegin();

    MosraLogDebug(<< this->mmslFeatCons.size() << " cons to process" << endl);
    // ------------------------------------------------------ for each constraint
    for(int iConsCounter=1; it != this->mmslFeatCons.constEnd(); it++, iConsCounter++)
    {
        MosraLogDebug( << it.key().toStdString() << " "
                << it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

        // set the constraint label
        QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
        vsConsLabel.push_back(sConsLabel);

        QStringList options = it.value().at(0).split(tr("+"), Qt::SkipEmptyParts);
        std::vector<unsigned int> noptidx;
        for(int no=0; no < options.size(); ++no)
        {
            unsigned int idx = this->mslOptions.indexOf(
                    QRegExp(options.at(no), Qt::CaseInsensitive, QRegExp::FixedString));
            noptidx.push_back(idx);
            MosraLogDebug( << "option index for '" << options.at(no).toStdString() << "' = " << idx + 1 << endl);
        }
        vvnOptionIndex.push_back(noptidx);

        bool bok;
        double drhs = it.value().at(2).toDouble(&bok);
        vdRHS.push_back(drhs);

        // set the constraint type
        if (it.value().at(1).compare(tr("<=")) == 0)
            vnConsType.push_back(1);
        else if (it.value().at(1).compare(tr(">=")) == 0)
            vnConsType.push_back(2);
        else
            vnConsType.push_back(3);
        MosraLogDebug( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);
    }


    //----------------------------------------------
    double* pdRow = 0;
    int* piColno = 0;

    long lId;
    double dCoeff;

    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    const int iOffset = this->miNumOptions;
    const int skipIncentivesFeatOffset = this->mmslIncentives.size() * miNumOptions + mmslIncentives.size();


    it = this->mmslFeatCons.constBegin();
    //-------------------------------------------------------------------- for each constraint
    for (int r=0; it != this->mmslFeatCons.constEnd(); ++it, ++r)
    {
        MosraLogDebug( << vsConsLabel.at(r).toStdString() << " - adding constraint" << endl);

        const long numOptions = vvnOptionIndex.at(r).size();
        pdRow = new double[numOptions];
        piColno = new int[numOptions];

        // determine initial offset for current option
        std::vector<long> vlCounter;
        for (int no=0; no < numOptions; ++no)
        {
            vlCounter.push_back(vvnOptionIndex.at(r).at(no)+1);
        }

        long nonHoleCounter = 0;

        // --------------------------------- for each feature
        for (int f=0; f < lNumCells; ++f)
        {
            if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                    || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
               )
            {
                continue;
            }

            dCoeff = mDataSet->getDblValue(this->msAreaField, f);
            QString sConsVal = QString(tr("%1")).arg(dCoeff, 0, 'g');

            long coeffCounter = 0;
            for (int opt=0; opt < numOptions; ++opt)
            {
                switch(this->meDVType)
                {
                case NMMosra::NM_MOSO_BINARY:
                    pdRow[coeffCounter] = ::atof(sConsVal.toStdString().c_str());
                    break;
                default: // i.e. for all other DV  types (real, integer)
                    pdRow[coeffCounter] = 1;
                    break;
                }

                piColno[coeffCounter] = vlCounter[opt];
                ++coeffCounter;
            }


            // add the constraint
            this->mLp->AddConstraintEx(numOptions, pdRow, piColno
                                       , vnConsType.at(r), vdRHS.at(r));

            ++lRowCounter;
            QString rowlabel = QString("feat%1_%2").arg(f).arg(vsConsLabel.at(r));
            this->mLp->SetRowName(lRowCounter, rowlabel.toStdString());


            for (int opt=0; opt < numOptions; ++opt)
            {
                vlCounter[opt] += iOffset + skipIncentivesFeatOffset;
            }

            if (f % 500 == 0)
            {
                NMDebug(<< ".");
            }

            ++nonHoleCounter;
        }
        NMDebug(<< "finished!" << endl);

        delete[] pdRow;
        delete[] piColno;
    }

    this->mLp->SetAddRowmode(false);


    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

bool NMMosra::processExplicitArealCons(
        std::vector<QString> &vsConsLabel,
        std::vector<int> &vnZoneLength,
        std::vector<QString> &vsZoneField,
        std::vector<std::vector<unsigned int> > &vvnOptionIndex,
        std::vector<unsigned int> &vnConsType,
        std::vector<double> &vdRHS)
{
    //long lNumCells = mDataSet->getNumRecs();
    double dUserVal;
    double dUserArea = 0.0;
//    double dAreaTotal = this->mdAreaTotal;
//    double dAreaSelected = this->mdAreaSelected <= 0 ? this->mdAreaTotal : this->mdAreaSelected;

    QMap<QString, QStringList>::const_iterator it =
            this->mmslAreaCons.constBegin();

    MosraLogDebug( << this->mmslAreaCons.size() << " cons to process" << endl);

    mvExplicitAreaConsVarOffsets.clear();

    // ----------------------------------------------------for each constraint
    for(int iConsCounter=1; it != this->mmslAreaCons.constEnd(); it++, iConsCounter++)
    {
        MosraLogDebug( << it.key().toStdString() << " "
                << it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

        // set the constraint label
        QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
        vsConsLabel.push_back(sConsLabel);

        // look for a 'zoned' constraint
        QString zone;
        QStringList optzones;
        QStringList options;
        if (it.value().at(0).contains(tr(":"), Qt::CaseInsensitive))
        {
            optzones = it.value().at(0).split(tr(":"), Qt::SkipEmptyParts);
            options = optzones.at(0).split(tr("+"), Qt::SkipEmptyParts);
            zone = optzones.at(1);
        }
        else
        {
            options = it.value().at(0).split(tr("+"), Qt::SkipEmptyParts);
            zone = "";
        }
        vsZoneField.push_back(zone);


        // set the user value
        bool bConvOK;
        dUserVal = it.value().at(2).toDouble(&bConvOK);
        dUserVal *= mdAreaScalingFactor;
        double dtval = 0;
        MosraLogDebug( << "dUserVal (" << dUserVal << ") as '" << it.value().at(3).toStdString() << "' = " << dtval << endl);

        int maxzonelen = 0;

        // set the option index
        // and the right hand side value
        std::vector<unsigned int> noptidx;
        for(int no=0; no < options.size(); ++no)
        {
            unsigned int idx = this->mslOptions.indexOf(
                    QRegExp(options.at(no), Qt::CaseInsensitive, QRegExp::FixedString));
            if (    !mslOptions.contains(options.at(no).simplified())
                 || idx > mslOptions.size()
               )
            {
                MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                              << "Invalid OPTION '" << options.at(no).toStdString() << "' "
                              << "specified for Areal Constraint #" << iConsCounter << "!");
                return false;
            }


            noptidx.push_back(idx);
            MosraLogDebug( << "option index for '" << options.at(no).toStdString() << "' = " << idx + 1 << endl);

            QStringList ozspec;
            ozspec << options.at(no) << zone;

            double oval = this->convertAreaUnits(dUserVal, it.value().at(3), ozspec);
            dtval = oval > dtval ? oval : dtval;
            dtval *= mdAreaScalingFactor;

            int zonelen = 0;
            if (!zone.isEmpty())
            {
                zonelen = this->mmslZoneLength.find(zone).value().find(options.at(no)).value();
            }
            maxzonelen = zonelen > maxzonelen ? zonelen : maxzonelen;
        }
        vvnOptionIndex.push_back(noptidx);
        vdRHS.push_back(dtval);

        if (zone.isEmpty())
        {
            vnZoneLength.push_back(this->mlNumOptFeat);
        }
        else
        {
            vnZoneLength.push_back(maxzonelen);
        }

        dUserArea += dtval;

        // set the constraint type
        if (it.value().at(1).compare(tr("<=")) == 0)
            vnConsType.push_back(1);
        else if (it.value().at(1).compare(tr(">=")) == 0)
            vnConsType.push_back(2);
        else
            vnConsType.push_back(3);
        MosraLogDebug( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);

        // add offset OPTIONS offset vector for each explicit areal constraint
        std::vector<size_t> luOffSets;
        mvExplicitAreaConsVarOffsets.push_back(luOffSets);
    }

    return true;
}

bool NMMosra::processFeatureSetConstraints(
        std::vector<QString> &vsFSCZoneField,
        std::vector<std::vector<unsigned int> > &vvnFSCOptionIndex
        )
{
    NMDebugCtx(ctxNMMosra, << "...");
    MosraLogInfo(<<"processing feature-set constraints ...");

    // get a list of feature-set ids
    otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());

    if (sqltab.IsNull())
    {
        MosraLogError(<< "Failed fetching the SQLite-based attribute table (*.ldb)!");
        NMDebugCtx(ctxNMMosra, << "done!");
        return false;
    }

    QMap<QString, QStringList>::ConstIterator fsIt = this->mslFeatSetCons.constBegin();
    for(; fsIt != this->mslFeatSetCons.constEnd(); ++fsIt)
    {
        MosraLogInfo(<< "   -> " << fsIt.key().toStdString() << "...");
        // extract the options & feature-set id column index
        std::vector<unsigned int> optionsIndex;

        QStringList optColPair = fsIt.key().split(":", Qt::SkipEmptyParts);
        QStringList optraw = optColPair.at(0).split("+", Qt::SkipEmptyParts);
        QStringList options;
        if (optraw.size() == 1)
        {
            if (this->mslOptions.contains(optraw.at(0).simplified()))
            {
                unsigned int idx = this->mslOptions.indexOf(
                        QRegExp(optraw.at(0).simplified(), Qt::CaseInsensitive, QRegExp::FixedString));
                if (idx == -1)
                {
                    MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                  << "Invalid OPTION '" << optraw.at(0).simplified().toStdString() << "' "
                                  << "specified for FeatureSet Constraint '" << fsIt.key().toStdString() << "'!");
                    return false;
                }
                options << optraw.at(0).simplified();
                optionsIndex.push_back(idx);
            }
            else if (optraw.at(0).simplified().compare(QStringLiteral("total"), Qt::CaseInsensitive) == 0)
            {
                for (int i=0; i < this->mslOptions.size(); ++i)
                {
                    options << this->mslOptions.at(i);
                    optionsIndex.push_back(i);
                }
            }
        }
        else
        {
            for (int r=0; r < optraw.size(); ++r)
            {
                if (this->mslOptions.contains(optraw.at(r).simplified()))
                {
                    options << optraw.at(r).simplified();
                    unsigned int idx = this->mslOptions.indexOf(
                            QRegExp(optraw.at(r).simplified(), Qt::CaseInsensitive, QRegExp::FixedString));
                    if (idx == -1)
                    {
                        MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
                                      << "Invalid OPTION '" << optraw.at(0).simplified().toStdString() << "' "
                                      << "specified for FeatureSet Constraint '" << fsIt.key().toStdString() << "'!");
                        return false;
                    }
                    optionsIndex.push_back(idx);
                }
            }
        }

        vvnFSCOptionIndex.push_back(optionsIndex);

        // workout operator
        unsigned int operatorId = 0;
        QString compOp = fsIt.value().at(1);
        QString compTypeLabel;
        if (compOp.compare(QStringLiteral("<=")) == 0)
        {
            operatorId = 1;
            compTypeLabel = "upper";
        }
        else if (compOp.compare(QStringLiteral(">=")) == 0)
        {
            operatorId = 2;
            compTypeLabel = "lower";
        }
        else if (compOp.compare(QStringLiteral("=")) == 0)
        {
            operatorId = 3;
            compTypeLabel = "equals";
        }

        //if (operatorId > 0)
        //{
        //    vnFSCConsType.push_back(operatorId);
        //}
        //else
        //{
        //    MosraLogDebug(<< ctxNMMosra << "::" << __FUNCTION__ << "() - ERROR: "
        //                  << "Invalid comparison operator '" << compOp.toStdString() << "' "
        //                  << "specified for FeatureSet Constraint #" << iConsCounter << "!");
        //    return false;
        //}

        // -----------------------------------------------------------------------------
        // fetch a table of unique feature set ids and the associated threshold values
        // EXAMPLE: select distinct cat_n, TN_MAL from luopt_hl2_1 where optFeat == 1 and cat_n > 0;

        //SQLiteTable::TableDataFetch(std::vector<std::vector<ColumnValue> > &restab,
        //                            const std::vector<TableColumnType> &coltypes,
        //                            const std::string &query)

        QString criterion = fsIt.value().at(0);
        std::string idColName = optColPair.at(1).toStdString();
        vsFSCZoneField.push_back(idColName.c_str());
        std::string rhsColName = fsIt.value().at(2).toStdString();

        bool bOptFeat = false;
        std::string optfeatColName = "";
        if (!this->msOptFeatures.isEmpty())
        {
            bOptFeat = true;
            optfeatColName = this->msOptFeatures.toStdString();
        }

        std::vector< std::vector< otb::AttributeTable::ColumnValue > > fsids;
        std::vector<otb::AttributeTable::TableColumnType> types;
        types.push_back(otb::AttributeTable::ATTYPE_INT);
        types.push_back(otb::AttributeTable::ATTYPE_DOUBLE);

        std::stringstream q_fstable;
        q_fstable << "SELECT distinct \"" << idColName << "\", "
                  << "\"" << rhsColName << "\" from \"" << sqltab->GetTableName() << "\" "
                  << "where \"" << idColName << "\" > 0";
        if (bOptFeat)
        {
            q_fstable << " and \"" << optfeatColName << "\" == 1";
        }
        q_fstable << ";";

        //MosraLogInfo(<< "   -> query:" << q_fstable.str() << std::endl);

        if (!sqltab->TableDataFetch(fsids, types, q_fstable.str()))
        {
            MosraLogError(<< "NMMosra::addFeatureSetCons() - " << sqltab->getLastLogMsg());
            return 0;
        }

        // get option index/position within option set / criterion score field sets
        //std::vector<int> optionIdx;
        //for (int k=0; k < options.size(); ++k)
        //{
        //    optionIdx.push_back(this->mslOptions.indexOf(options[k]));
        //}
        QMap<size_t, unsigned int> fscCntMap;
        QMap<size_t, QStringList> fscMap;
        for (int fs=0; fs < fsids.size(); ++fs)
        {
            QString fsc_seg;
            QString rhs_seg = QString("%1 %2").arg(operatorId)
                                              .arg(fsids.at(fs).at(1).dval);
            size_t fid = fsids.at(fs).at(0).ival;
            QStringList segs_list;
            segs_list << fsc_seg << rhs_seg;
            fscMap.insert(fid, segs_list);
            fscCntMap.insert(fid, 0);
        }
        mvvFSCUniqueIDSegmentRHSMap.push_back(fscMap);
        mvvFSCUniqueIDCounterMap.push_back(fscCntMap);
    }

    NMDebugCtx(ctxNMMosra, << "done!");
    return true;
}

int NMMosra::addExplicitAreaCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // get the hole array
    const bool hole = mDataSet->hasColumn("nm_hole");
    const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    std::vector<QString> vsConsLabel;
    std::vector<int> vnZoneLength;
    std::vector<QString> vsZoneField;
    std::vector<std::vector<unsigned int> > vvnOptionIndex;
    std::vector<unsigned int> vnConsType;
    std::vector<double> vdRHS;

    long lNumCells = mDataSet->getNumRecs();
    double dUserVal;
    double dUserArea = 0.0;
//    double dAreaTotal = this->mdAreaTotal;
//    double dAreaSelected = this->mdAreaSelected <= 0 ? this->mdAreaTotal : this->mdAreaSelected;

    QMap<QString, QStringList>::const_iterator it =
            this->mmslAreaCons.constBegin();

    MosraLogDebug( << this->mmslAreaCons.size() << " cons to process" << endl);

    // ----------------------------------------------------for each constraint
    for(int iConsCounter=1; it != this->mmslAreaCons.constEnd(); it++, iConsCounter++)
    {
        MosraLogDebug( << it.key().toStdString() << " "
                << it.value().join(tr(" ")).toStdString() << " - reading props" << endl);

        // set the constraint label
        QString sConsLabel = it.key() + QString(tr("_%1")).arg(it.value().at(0));
        vsConsLabel.push_back(sConsLabel);

        // look for a 'zoned' constraint
        QString zone;
        QStringList optzones;
        QStringList options;
        if (it.value().at(0).contains(tr(":"), Qt::CaseInsensitive))
        {
            optzones = it.value().at(0).split(tr(":"), Qt::SkipEmptyParts);
            options = optzones.at(0).split(tr("+"), Qt::SkipEmptyParts);
            zone = optzones.at(1);
        }
        else
        {
            options = it.value().at(0).split(tr("+"), Qt::SkipEmptyParts);
            zone = "";
        }
        vsZoneField.push_back(zone);


        // set the user value
        bool bConvOK;
        dUserVal = it.value().at(2).toDouble(&bConvOK);
        double dtval = 0;
        MosraLogDebug( << "dUserVal (" << dUserVal << ") as '" << it.value().at(3).toStdString() << "' = " << dtval << endl);

        int maxzonelen = 0;

        // set the option index
        // and the right hand side value
        std::vector<unsigned int> noptidx;
        for(int no=0; no < options.size(); ++no)
        {
            unsigned int idx = this->mslOptions.indexOf(
                    QRegExp(options.at(no), Qt::CaseInsensitive, QRegExp::FixedString));
            noptidx.push_back(idx);
            MosraLogDebug( << "option index for '" << options.at(no).toStdString() << "' = " << idx + 1 << endl);

            QStringList ozspec;
            ozspec << options.at(no) << zone;

            double oval = this->convertAreaUnits(dUserVal, it.value().at(3), ozspec);
            dtval = oval > dtval ? oval : dtval;

            int zonelen = 0;
            if (!zone.isEmpty())
            {
                zonelen = this->mmslZoneLength.find(zone).value().find(options.at(no)).value();
            }
            maxzonelen = zonelen > maxzonelen ? zonelen : maxzonelen;
        }
        vvnOptionIndex.push_back(noptidx);
        vdRHS.push_back(dtval);

        if (zone.isEmpty())
        {
            vnZoneLength.push_back(this->mlNumOptFeat);
        }
        else
        {
            vnZoneLength.push_back(maxzonelen);
        }

        dUserArea += dtval;

        // set the constraint type
        if (it.value().at(1).compare(tr("<=")) == 0)
            vnConsType.push_back(1);
        else if (it.value().at(1).compare(tr(">=")) == 0)
            vnConsType.push_back(2);
        else
            vnConsType.push_back(3);
        MosraLogDebug( << "constraint type (1: <= | 2: >= | 3: =): " << vnConsType.at(iConsCounter-1) << endl);

    }

    // todo: we need to account for selected areas at one stage

    //	if (dUserArea > dAreaSelected)
    //	{
    //		MosraLogError( << "The specified areal constraints are "
    //				<< "inconsistent with the available area!");
    //		return 0;
    //	}

    double* pdRow = 0;
    int* piColno = 0;

    long lId;
    double dCoeff;

    // set add row mode
    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    // calc the offset required to jump from one feature-option-column to the next
    // one; since we've got one binary conditional decision variable for each feature
    // we've got to add 1 to the offset;
    //int iOffset = this->miNumOptions + 1;
    int iOffset = this->miNumOptions;

    // offset to jump over any incentives decision-variables
    int iSkipIncentivesOffset = this->mmslIncentives.size() * miNumOptions + mmslIncentives.size();

    // adjust option offset
    iOffset += iSkipIncentivesOffset;

    it = this->mmslAreaCons.constBegin();
    // ------------------------------------------------ for each constraint
    for (int r=0; it != this->mmslAreaCons.constEnd(); ++it, ++r)
    {
        MosraLogDebug( << vsConsLabel.at(r).toStdString() << " - adding constraint" << endl);

        // array defining zones for this constraints
        bool bZoneCons = false;
        QString inLabel = vsConsLabel.at(r);

        const long numOptions = vvnOptionIndex.at(r).size();
        pdRow = new double[vnZoneLength.at(r) * numOptions];
        piColno = new int[vnZoneLength.at(r) * numOptions];

        if (!vsZoneField.at(r).isEmpty())
        {
            bZoneCons = true;

            inLabel = QString(tr("%1")).arg(vsConsLabel.at(r));
        }

        // determine the initial offset for the current option (i.e. the first column number
        // of the lp-matrix); since the option index is 0-based, but the columns are 1-based
        // we've got to add 1!
        std::vector<long> vlCounter;
        for(int no=0; no < numOptions; ++no)
        {
            vlCounter.push_back(vvnOptionIndex.at(r).at(no) + 1);
        }

        long nonHoleCounter = 0;
        long coeffCounter = 0;
        // ---------------------------------------------for each feature
        for (int f=0; f < lNumCells; f++)
        {
            // skip holes
            if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                 || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
               )
            {
                continue;
            }

            dCoeff = mDataSet->getDblValue(this->msAreaField, f);
            QString sConsVal = QString(tr("%1")).arg(dCoeff, 0, 'g');


            //const unsigned int& optidx = vvnOptionIndex.at(r).at(no);
            //const long& lCounter = vlCounter.at(no);
            // set coefficients depending on whether we've got a zoned constraint or not
            if (bZoneCons)
            {
                for (int no=0; no < numOptions; ++no)
                {
                    // set coefficients for zone polygons
                    const QString zoneArVal = mDataSet->getStrValue(vsZoneField.at(r), f);

                    const QStringList zoneValueList = zoneArVal.split(" ", Qt::SkipEmptyParts);
                    if (zoneValueList.contains(this->mslOptions.at(vvnOptionIndex.at(r).at(no))))
                    {
                        // set the coefficient
                        switch(this->meDVType)
                        {
                        case NMMosra::NM_MOSO_BINARY:
                            bool bok;
                            pdRow[coeffCounter] = sConsVal.toDouble(&bok);
                            break;
                        default: // i.e. for all other DV  types (real, integer)
                            pdRow[coeffCounter] = 1;
                            break;
                        }

                        // set the column number
                        piColno[coeffCounter] = vlCounter[no];
                        ++coeffCounter;
                    }
                }
            }
            else // set the coefficients for a non-zone constraintcoeffCcoeffCounterounter
            {
                for (int no=0; no < numOptions; ++no)
                {
                    // set the coefficient
                    switch(this->meDVType)
                    {
                    case NMMosra::NM_MOSO_BINARY:
                        pdRow[coeffCounter] = ::atof(sConsVal.toStdString().c_str());
                        break;
                    default: // i.e. for all other DV  types (real, integer)
                        pdRow[coeffCounter] = 1;
                        break;
                    }

                    // set the column number
                    piColno[coeffCounter] = vlCounter[no];
                    ++coeffCounter;
                }
            }

            for(int no=0; no < numOptions; ++no)
            {
                vlCounter[no] += iOffset;
            }

            if (f % 500 == 0)
            {
                NMDebug(<< ".");
            }

            // increment the counter for valid cells (i.e. valid spatial opt features
            ++nonHoleCounter;
        }

        NMDebug(<< " finished!" << endl);

        // add the constraint
        this->mLp->AddConstraintEx((vnZoneLength.at(r) * numOptions),
                pdRow, piColno, vnConsType.at(r),
                vdRHS.at(r));

        // increment the row counter
        ++lRowCounter;

        // assign a label for this constraint and store label for
        // retrieval of post-optimisation constraint values
        this->mLp->SetRowName(lRowCounter, inLabel.toStdString());
        this->mmslAreaConsLabel.append(inLabel);

        // free memory
        delete[] pdRow;
        delete[] piColno;
    }


    // turn off rowmode
    this->mLp->SetAddRowmode(false);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::addImplicitAreaCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // allocate memory for arrays
    // we cover each feature separately, so we need miNumOptions coefficients
    // plus the binary conditional to realise the either zero or all area
    // constraint for each feature
    double* pdRow = new double[this->miNumOptions];
    int* piColno = new int[this->miNumOptions];

    // get the hole array
    const bool hole = mDataSet->hasColumn("nm_hole");
    const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    const long lNumCells = mDataSet->getNumRecs();
    //long lId;
    double dConsVal;
    //double dFeatID;
    //double dVal;

    const int iNumIncentives = this->mmslIncentives.size();
    const int skipIncentivesOffset = iNumIncentives * miNumOptions + iNumIncentives;

    // set add_row_mode for adding constraints
    this->mLp->SetAddRowmode(true);

    long lRowCounter = this->mLp->GetNRows();
    long colpos = 1;
    // --------------------------------------------for each feature
    for (int f=0; f < lNumCells; f++)
    {
        // leap over holes!
        if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
           )
        {
            continue;
        }

        // get the area of the current feature
        dConsVal = mDataSet->getDblValue(this->msAreaField, f);

        // round value when dealing with integer decision variables
        if (this->meDVType == NMMosra::NM_MOSO_INT)
            dConsVal = vtkMath::Floor(dConsVal);

        // can't really comprehend this dirty hack again
        // but it its meaning in the original LUMASS version ...
        QString sConsVal = QString(tr("%1")).arg(dConsVal, 0, 'g');

        //-----------------------------------------for each option
        for (int option=0; option < this->miNumOptions; option++, colpos++)
        {
            // set the coefficient
            switch(this->meDVType)
            {
            case NMMosra::NM_MOSO_BINARY:
                pdRow[option] = ::atof(sConsVal.toStdString().c_str());
                break;
            default: // i.e. for all other DV  types (real, integer)
                pdRow[option] = 1;
                break;
            }

            // set the column number
            piColno[option] = colpos;

            //MosraLogInfo( << "feat cons: option | colps = " << option << " | " << lCounter+1 << endl);

        }
        colpos += skipIncentivesOffset;

        // ......................................................................................
        // add the first constraint: SUM(x_i_r) - A_i * b_i >= 0

        // set the coefficient for the binary decision variable for the current feature
        //		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;
        //		piColno[this->miNumOptions] = colpos;

        //		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
        //				piColno, 2, 0);

        this->mLp->AddConstraintEx(this->miNumOptions, pdRow,
                piColno, 1, ::atof(sConsVal.toStdString().c_str()));


        // increment row (constraint) counter
        lRowCounter++;

        // label the currently added constraint
        QString sRowName = QString(tr("Feature_%1a")).arg(f);
        this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

        // ......................................................................................
        // add the second constraint: SUM(x_i_r) - A_i * b_i <= 0
        //		pdRow[this->miNumOptions] = ::atof(sConsVal.toStdString().c_str()) * -1;

        //		this->mLp->AddConstraintEx(this->miNumOptions+1, pdRow,
        //				piColno, 1, 0);
        //		lRowCounter++;

        //		sRowName = QString(tr("Feature_%1b")).arg(f);
        //		this->mLp->SetRowName(lRowCounter, sRowName.toStdString());

        // ........................................................................................
        // column position counter
        //colpos++;

        if (f % 500 == 0)
            NMDebug(<< ".");
    }
    NMDebug(<< " finished!" << endl);

    // turn off row mode
    this->mLp->SetAddRowmode(false);

    // free memory
    delete[] pdRow;
    delete[] piColno;

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int NMMosra::addCriCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    // extract relevant information for building constraints
    // this could have been incorporated into the actual "value loop"
    // but it is much more readable like this, at least to me, and
    // it doesn't hurt the performance too much because there are
    // usually not too many constraints

    QVector<QString> vLabels;
    // one land use per constraint (or 'total'=all land uses)
    QVector<QString> vLandUses;
    // vector of zones (may be an empty string though, if no zone was specified for a particular
    // constraint
    QVector<QString> vZones;
    // vector of vector of field names per constraint (only when 'total' is given more than one)
    QVector<QVector<QString> > vvFieldNames;

    // vector of incentive criterion field names
    // need to have the incentive benefit criterion as constraint name
    QVector<QVector<QString> > vvIncCriterionFieldNames;


    // vector of vector of land use indices (i.e. position in mslOptions and thus
    // relative position within the optimisation matrix per feature)
    QVector<QVector<unsigned int> > vvIdxLandUses;
    // vector of comparison operator-id (one per constraint)
    QVector<int> vOperators;
    // vector of values of the right hand side
    QVector<double> vRHS;

    QMap<QString, QStringList>::ConstIterator incIt;
    QMultiMap<QString, QMap<QString, QStringList> >::const_iterator crilabit;
    QMap<QString, QStringList>::const_iterator criit;
    for (crilabit = this->mmslCriCons.begin(); crilabit != this->mmslCriCons.end(); ++crilabit)
    {
        // test whether we've got incentives for this criterion
        QVector<QString> vIncCriterionFieldNames;
        incIt = this->mmslIncentives.constBegin();
        while (incIt != this->mmslIncentives.constEnd())
        {
            if (incIt.key().startsWith(crilabit.key(), Qt::CaseInsensitive))
            {
                foreach(const QString& fn, incIt.value())
                {
                    vIncCriterionFieldNames.push_back(fn);
                }
            }
            ++incIt;
        }
        vvIncCriterionFieldNames.push_back(vIncCriterionFieldNames);


        QMap<QString, QStringList> luFieldList = crilabit.value();
        for (criit = luFieldList.begin(); criit != luFieldList.end(); ++criit)
        {
            QString compOp = criit.value().at(criit.value().size()-2);
            QString compTypeLabel;
            if (compOp.compare(tr("<=")) == 0)
            {
                vOperators.push_back(1);
                compTypeLabel = "upper";
            }
            else if (compOp.compare(tr(">=")) == 0)
            {
                vOperators.push_back(2);
                compTypeLabel = "lower";
            }
            else if (compOp.compare(tr("=")) == 0)
            {
                vOperators.push_back(3);
                compTypeLabel = "equals";
            }
            else
            {
                MosraLogError( << "CRITERIA_CONSTRAINTS: " << crilabit.key().toStdString()
                        << "_" << criit.key().toStdString() <<	": invalid comparison operator '"
                        << compOp.toStdString() << "'!");
                return 0;
            }

            // construct the label for each constraint
            QString label = crilabit.key() + QString(tr("_%1_%2")).arg(criit.key()).arg(compTypeLabel);
            vLabels.push_back(label);
            vLandUses.push_back(criit.key());

            bool convertable;
            QString sRhs = criit.value().last();
            double rhs = sRhs.toDouble(&convertable);
            if (!convertable)
            {
                MosraLogError( << "CRITERIA_CONSTRAINTS: " << label.toStdString() << ": invalid value '"
                        << sRhs.toStdString() << "'!");
                return 0;
            }
            vRHS.push_back(rhs);

            // ......................................................................
            // extract field names and land use indices

            // first of all, account for zoning
            QStringList zonespec;
            QString opt;
            QString zone;

            if (criit.key().contains(tr(":"), Qt::CaseInsensitive))
            {
                zonespec = criit.key().split(tr(":"), Qt::SkipEmptyParts);
                opt = zonespec.at(0);
                zone = zonespec.at(1);
            }
            else
            {
                opt = criit.key();
                zone = "";
            }
            vZones.push_back(zone);

            // now look for resource indices and criterion field names
            QVector<QString> vCriterionFieldNames;
            QVector<unsigned int> vCriterionLandUseIdx;
            QStringList fieldList = criit.value();
            if (opt.compare(tr("total"), Qt::CaseInsensitive) == 0)
            {
                // look through m_iNumOptions fields
                for (int f=0; f < this->miNumOptions; ++f)
                {
                    vCriterionFieldNames.push_back(fieldList.at(f));
                    vCriterionLandUseIdx.push_back(f);
                }
            }
            else
            {
                vCriterionLandUseIdx.push_back(
                        this->mslOptions.indexOf(
                                QRegExp(opt, Qt::CaseInsensitive, QRegExp::FixedString)));
                vCriterionFieldNames.push_back(fieldList.at(0));
            }
            vvFieldNames.push_back(vCriterionFieldNames);
            vvIdxLandUses.push_back(vCriterionLandUseIdx);
        }
    }

    // get the hole array
    bool hole = mDataSet->hasColumn("nm_hole");
    int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);
    long lNumCells = mDataSet->getNumRecs();

    // iterate over constraints and process them one at a time
    // (or should we process them while looping over all features)?

    const int skipIncentiveFeatOffset = this->mmslIncentives.size() * miNumOptions + mmslIncentives.size();

    this->mLp->SetAddRowmode(true);
    long lRowCounter = this->mLp->GetNRows();

    for (int labelidx = 0; labelidx < vLabels.size(); ++labelidx)
    {
        MosraLogDebug(<< "preparing constraint " << vLabels[labelidx].toStdString() << endl);

        // get the performance indicator fields, land use indices
        // and allocate the constraint buffers
        const QVector<unsigned int> landUseIdxs = vvIdxLandUses[labelidx];
        const QVector<QString> fieldNames = vvFieldNames[labelidx];
        const int numCriOptions = fieldNames.size();

        const QVector<QString> incFieldNames = vvIncCriterionFieldNames[labelidx];
        const int iNumIncentives = incFieldNames.size();

        // debug
        const int varspace = (numCriOptions + numCriOptions * iNumIncentives) * this->mlNumOptFeat;

        double* pdRow = new double[varspace];
        int* piColno = new int[varspace];

        long arpos = 0;
        long colPos = 1;
        for (long f=0; f < lNumCells; ++f)
        {
            //leap over holes
            if (    (hole && mDataSet->getIntValue("nm_hole", f) == 1)
                 || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, f) == 0)
               )
            {
                continue;
            }

            for(int i=0; i < numCriOptions; ++i)
            {
                const int optIdx = landUseIdxs[i];
                const int performanceIndicatorIdx = mDataSet->getColumnIndex(fieldNames[i]);

                double coeff = 0.0;
                bool bAddCoeff = true;

                // check whether, we've got zoning and whether the criterion constraint
                // for the current land use option shall be restricted to this zone
                if (!vZones.at(labelidx).isEmpty())
                {
                    const QString zoneArVal = mDataSet->getStrValue(vZones.at(labelidx), f);
                    const QStringList zoneArValList = zoneArVal.split(" ", Qt::SkipEmptyParts);
                    if (!zoneArValList.contains(this->mslOptions.at(optIdx)))
                    {
                        bAddCoeff = false;
                    }
                }

                if (bAddCoeff)
                {
                    switch(this->meDVType)
                    {
                    case NMMosra::NM_MOSO_BINARY:
                        coeff = mDataSet->getDblValue(this->msAreaField, f) * mDataSet->getDblValue(performanceIndicatorIdx, f);
                        break;
                    default:
                        coeff = mDataSet->getDblValue(performanceIndicatorIdx, f);
                        break;
                    }
                }

                //debug
                //int criPos = colPos + optIdx;

                pdRow[arpos] = coeff;
                piColno[arpos] = colPos + optIdx;
                ++arpos;

                // account for incentives, if applicable
                for (int inc=0; inc < iNumIncentives; ++inc, ++arpos)
                {
                    const int incFieldIdx = mDataSet->getColumnIndex(incFieldNames[inc]);
                    coeff = 0.0;
                    if (bAddCoeff)
                    {
                        switch(this->meDVType)
                        {
                        case NMMosra::NM_MOSO_BINARY:
                            coeff = mDataSet->getDblValue(this->msAreaField, f) * mDataSet->getDblValue(incFieldIdx, f);
                            break;
                        default:
                            coeff = mDataSet->getDblValue(incFieldIdx, f);
                            break;
                        }
                    }

                    // arpos is the current postion based on no incentives; + 1 move it to the next pos; +inc positions
                    // the pointer at the inc+1ths position
                    pdRow[arpos] = coeff;
                    // piColno:
                    //  - colpos is start of current feature;
                    //  - optIdx picks the right decision var for the given landuse at hand
                    //  - adding ((inc+1) * miNumOptions) jumps at the current base-line reduction decision var
                    //       (out of the number of incentives and therefore reduction vars per land use)
                    //  - adding (+1) makes sure we're skipping the whole parcel retirement reduction variable (which is always at
                    //       the start of the list of reduction variables)
                    //
                    piColno[arpos] = colPos + optIdx + (inc+1) * miNumOptions + 1 + inc;
                }
            }

            // jump to the first option of the next feature
            //          DEPRECATED(keep in mind that we've got one binary decision var (i.e. column) per feature, hence the +1
            colPos += this->miNumOptions + skipIncentiveFeatOffset;

            if (f % 200 == 0)
                NMDebug(<< ".");

        }
        NMDebug(<< " finished!" << std::endl);

        // add constraint
        MosraLogDebug(<< "adding constraint to LP ..." << std::endl);
        this->mLp->AddConstraintEx(varspace, pdRow, piColno, vOperators[labelidx], vRHS[labelidx]);

        // increment the row counter
        ++lRowCounter;


        //  set the constraint label
        MosraLogDebug(<< "labeling constraint ..." << std::endl);
        this->mLp->SetRowName(lRowCounter, vLabels[labelidx].toStdString().c_str());

        delete[] pdRow;
        delete[] piColno;
        pdRow = nullptr;
        piColno = nullptr;

    }

    // turn off rowmode
    this->mLp->SetAddRowmode(false);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int
NMMosra::addIncentCons(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    const bool hole = mDataSet->hasColumn("nm_hole");
    const int optFeatIdx = mDataSet->getColumnIndex(this->msOptFeatures);
    const int curResourceIdx = mDataSet->getColumnIndex(this->msLandUseField);
    const long lNumCells = mDataSet->getNumRecs();

    const int iNumInc = this->mmslIncentives.size();

    // set up the baseline constraints arrays
    const int varspace = this->miNumOptions + 1;//(2 * this->miNumOptions) + 1;
    double* pdRow = new double[varspace];
    int* piColNo = new int[varspace];

    // set up the reduction-tie constraints
    double* pdTieRow = new double[2];
    int* piTieColNo = new int[2];

    std::vector<std::vector<int> > vvIncCriFieldIdx;
    std::vector<int>                vBaselineFieldIdx;

    // get field indices
    QMap<QString, QStringList>::ConstIterator incIt = mIncNamePair.constBegin();
    while (incIt != mIncNamePair.constEnd())
    {
        const QString& incCri = incIt.value().at(1);
        const QString& baselineField = QString("%1_CUR").arg(incCri);

        const int baselineFieldIdx = mDataSet->getColumnIndex(baselineField);
        if (baselineFieldIdx == -1)
        {
            MosraLogError(<< "Couldn't find baseline performance score for "
                          << " '" << incCri.toStdString() << "' in the dataset!");
            return 0;
        }
        vBaselineFieldIdx.push_back(baselineFieldIdx);

        std::vector<int> vCriFieldIdx;
        for (int lu=0; lu < miNumOptions; ++lu)
        {
            const QString& incCriField = mmslCriteria[incCri].at(lu);
            const int incCriFieldIdx = mDataSet->getColumnIndex(incCriField);
            if (incCriFieldIdx == -1)
            {
                MosraLogError( << "Couldn't find criterion performance score '"
                               << incCriField.toStdString() << "' in the dataset!");
                return 0;
            }
            vCriFieldIdx.push_back(incCriFieldIdx);
        }
        vvIncCriFieldIdx.push_back(vCriFieldIdx);

        ++incIt;
    }


    // add baseline criteria!
    const int featColOffset = miNumOptions + iNumInc * miNumOptions + iNumInc;
    int featPos = 1;

    long lRowCounter = this->mLp->GetNRows();
    this->mLp->SetAddRowmode(true);
    for (long cell=0; cell < lNumCells; ++cell)
    {
        if (    (hole && mDataSet->getIntValue("nm_hole", cell) == 1)
                || (optFeatIdx != -1 && mDataSet->getIntValue(optFeatIdx, cell) == 0)
           )
        {
            continue;
        }

        const QString curResource = this->mDataSet->getStrValue(this->msLandUseField, cell);
        const int curLUIdx = mslOptions.indexOf(
                    QRegExp(curResource, Qt::CaseInsensitive, QRegExp::FixedString));

        for (int inc=0; inc < iNumInc; ++inc)
        {
            //const double baselinePerf = mDataSet->getDblValue(vBaselineFieldIdx.at(inc), cell);
            const double curScore = mDataSet->getDblValue(vvIncCriFieldIdx[inc][curLUIdx], cell);

            // set parcel retirement reduction (r_i_0_q) coefficient
            int arpos = 0;
            pdRow[arpos] = 0.0;//1.0;
            piColNo[arpos] = featPos + (inc+1) * miNumOptions + inc;
            ++arpos;

            for (int lu=0; lu < miNumOptions; ++lu, ++arpos)
            {
                //-----------------------------------------------
                // set the reduction constraint (for a given landuse and incentive)

                // .............................
                // set x_i_r coefficient
                const double criScore = mDataSet->getDblValue(vvIncCriFieldIdx[inc][lu], cell);
                double scoreCoeff = (curScore - criScore);// > 0 ? (curScore - criScore) : 0.0;
                pdTieRow[0] = scoreCoeff;
                piTieColNo[0] = featPos + lu;

                // the option's reduction variable coefficient
                pdTieRow[1] = -1.0;
                piTieColNo[1] = featPos + lu + (inc+1) * miNumOptions + 1 + inc;

                // upper
                this->mLp->AddConstraintEx(2, pdTieRow, piTieColNo, 1, 0.0);
                ++lRowCounter;

                QString tieName = QString("ReducCons-upper_%1_%2_%3").arg(cell).arg(lu+1).arg(inc+1);
                this->mLp->SetRowName(lRowCounter, tieName.toStdString().c_str());

                // lower
                this->mLp->AddConstraintEx(2, pdTieRow, piTieColNo, 2, 0.0);
                ++lRowCounter;

                tieName = QString("ReducCons-lower_%1_%2_%3").arg(cell).arg(lu+1).arg(inc+1);
                this->mLp->SetRowName(lRowCounter, tieName.toStdString().c_str());


                // ------------------------------------------------
                // set r_i_r_q coefficient for baseline constraint
                pdRow[arpos] = 1.0;
                piColNo[arpos] = featPos + lu + (inc+1) * miNumOptions + 1 + inc;
            }

            // add constraint
            // for a given parcel: baseline >= reduction_retirement + reduction_lu1 + reduction_lu2 + reduction_lu3 + ...
            //this->mLp->AddConstraintEx(varspace, pdRow, piColNo, 1, baselinePerf);
            //++lRowCounter;

            //const QString& consName = QString("BaselineCons_%1_%2").arg(cell).arg(inc+1);
            //this->mLp->SetRowName(lRowCounter, consName.toStdString().c_str());
        }

        featPos += featColOffset;

        if (cell % 200 == 0)
        {
            NMDebug(<< ".");
        }
    }
    NMDebug(<< " finished!" << std::endl);

    delete[] pdRow;
    delete[] piColNo;

    this->mLp->SetAddRowmode(false);

    NMDebugCtx(ctxNMMosra, << "done!");
    return 1;
}

int
NMMosra::calcBaseline(void)
{
    MosraLogDebug(<< "Calculating baseline performance ... ");

    int ret = 0;
    QStringList criteria = mmslCriteria.keys();
    foreach(const QString& cri, criteria)
    {
        QString colname = QString("%1_CUR").arg(cri);
        mDataSet->addColumn(colname, NMMosraDataSet::NM_MOSRA_DATATYPE_DOUBLE);
    }

    if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_OTBTAB)
    {
        otb::AttributeTable::Pointer otbtab = mDataSet->getOtbAttributeTable();
        if (otbtab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
        {
            ret = calcBaselineDb();
        }
    }
    else if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_QTSQL)
    {
        ret = calcBaselineQtSql();
    }
    else if (mDataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_VTKDS)
    {
        ret = calcBaselineTab();
    }

    MosraLogDebug(<< "Done with baseline performance!");
    return ret;
}

int
NMMosra::calcBaselineQtSql(void)
{
    int ret = 1;

    QSqlTableModel* sqlMod = mDataSet->getQSqlTableModel();
    QSqlDatabase db = sqlMod->database();
    QSqlDriver* drv = db.driver();

    QString sOptFeatCond = "";
    if (!this->msOptFeatures.isEmpty())
    {
        sOptFeatCond = QString(" and %1 == 1 ")
                .arg(drv->escapeIdentifier(this->msOptFeatures, QSqlDriver::FieldName));
    }

    db.transaction();
    QStringList criteria = mmslCriteria.keys();
    foreach(const QString& cri, criteria)
    {
        const QStringList scorefields = mmslCriteria[cri];

        QString colname = QString("%1_CUR").arg(cri);
        QString qStr = QString("UPDATE %1 SET %2 = CASE ")
                .arg(drv->escapeIdentifier(sqlMod->tableName(), QSqlDriver::TableName))
                .arg(drv->escapeIdentifier(colname, QSqlDriver::FieldName));

        QString vStr;
        foreach(const QString& luopt, mslOptions)
        {
            const int fidx = mslOptions.indexOf(luopt);
            vStr += QString("WHEN %1 == '%2' %3 then %4 * %5 ")
                    .arg(drv->escapeIdentifier(this->msLandUseField, QSqlDriver::FieldName))
                    .arg(luopt)
                    .arg(sOptFeatCond)
                    .arg(drv->escapeIdentifier(scorefields.at(fidx), QSqlDriver::FieldName))
                    .arg(drv->escapeIdentifier(this->msAreaField, QSqlDriver::FieldName));
        }
        vStr += QString("ELSE 0.0 END;");
        qStr += vStr;

        QSqlQuery q(db);
        if (!q.exec(qStr))
        {
            MosraLogError(<< "Failed updating baseline performance for criterion '"
                          << cri.toStdString() << "'!");
            ret = 0;
        }
        q.finish();
    }
    db.commit();

    return ret;
}

int
NMMosra::calcBaselineTab(void)
{
    // what we do ...
    // for each parcel
    //    for each criterion
    //       - get current land use
    //       - work out performance of land use (area * criterion performance)
    //

    int luArIdx = mDataSet->getColumnIndex(this->msLandUseField);
    int areaArIdx = mDataSet->getColumnIndex(this->msAreaField);
    int holeArIdx = mDataSet->getColumnIndex("nm_hole");
    int optFeatArIdx = mDataSet->getColumnIndex(this->msOptFeatures);

    long lNumCells = mDataSet->getNumRecs();

    QStringList criteria = mmslCriteria.keys();

    for (long cell=0; cell < lNumCells; ++cell)
    {
        QString landuse = mDataSet->getStrValue(luArIdx, cell);
        landuse = landuse.trimmed();
        const int fidx = mslOptions.indexOf(landuse);
        double area = mDataSet->getDblValue(areaArIdx, cell);

        if (    mDataSet->getIntValue(holeArIdx, cell) == 1
             || (optFeatArIdx != -1 && mDataSet->getIntValue(optFeatArIdx, cell) == 0)
           )
        {
            area = 0.0;
        }

        foreach(const QString& cri, criteria)
        {
            const QString colname = QString("%1_CUR").arg(cri);
            const QStringList scorefields = mmslCriteria[cri];
            double scoreha = fidx >= 0 ? mDataSet->getDblValue(scorefields.at(fidx), cell) : 0.0;

            mDataSet->setDblValue(colname, cell, scoreha * area);
        }
    }
    return 1;
}

int NMMosra::calcOptPerformanceDb(void)
{
    int ret = 0;


    //get return value from the solve function
    int lastresult = this->mLp->GetLastReturnFromSolve();

    //sRes << tr("Solver returned: ");
    //switch (ret)
    //{
    //case -2: sRes << tr("NOMEMORY\n"); break;
    //case 0: sRes << tr("OPTIMAL\n"); break;
    //case 1: sRes << tr("SUBOPTIMAL\n"); break;
    //case 2: sRes << tr("INFEASIBLE\n"); break;
    //case 3: sRes << tr("UNBOUNDED\n"); break;
    //case 4: sRes << tr("DEGENERATE\n"); break;
    //case 5: sRes << tr("NUMFAILURE\n"); break;
    //case 6: sRes << tr("USERABORT\n"); break;
    //case 7: sRes << tr("TIMEOUT\n"); break;
    //case 10: sRes << tr("PROCFAIL\n"); break;
    //case 11: sRes << tr("PROCBREAK\n"); break;
    //case 12: sRes << tr("FEASFOUND\n"); break;
    //case 13: sRes << tr("NOFEASFOUND\n"); break;
    //}

    if (lastresult < 0 || lastresult > 1)
    {
        MosraLogDebug(<< "No (sub)optimal solution found -  "
                      << "so no point evaluating the optimal performance!")
        return 1;
    }

    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(
                this->mDataSet->getOtbAttributeTable().GetPointer());
    if (sqltab.IsNull())
    {
        MosraLogInfo(<< "Not evaluating optimal performance for non-SQLite datasets! "
                     << "Please refer to summary results provided as *.csv files!");
        return 1;
    }

    const std::string idColName = sqltab->GetPrimaryKey();

    // ========================================================================================

    // getnames: [0 , 1]: rowid, OPT_STR,
    //           [2 , (numCrit * numOptions - 1)]: n_daiA, n_daiB, ..., p_daiA, p_daiB, ...,
    //           [(numCrit * numOptions + 2) , ((numCrit * numOptions + 2) + numOptions - 1)]: OPT1_VAL, OPT2_VAL, ...
    std::vector<std::string> getnames;
    getnames.push_back(idColName);
    getnames.push_back("OPT_STR");

    std::vector<otb::AttributeTable::ColumnValue> getvalues;
    otb::AttributeTable::ColumnValue idval, optstrval;
    getvalues.push_back(idval);
    getvalues.push_back(optstrval);


    // setnames: [0 , (numCrit-1)]: crit1_OPT, crit2_OPT, ...
    std::vector<std::string> setnames;
    std::vector<otb::AttributeTable::ColumnValue> setvalues;

    QStringList slCrit = this->mmslCriteria.keys();
    for (int c=0; c < slCrit.size(); ++c)
    {

        // prepare the setnames and set values
        std::string optcol = slCrit.at(c).toStdString() + "_OPT";
        setnames.push_back(optcol);

        otb::AttributeTable::ColumnValue setval;
        setval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        setval.dval = 0;
        setvalues.push_back(setval);

        // add 'setvalue columns' to attribute table
        if (!sqltab->AddColumn(optcol, otb::AttributeTable::ATTYPE_DOUBLE))
        {
            MosraLogInfo(<< "NMMosra::calcOptPerformanceDb() - Failed adding column '"
                         << optcol << "' to table '" << sqltab->GetTableName() << "': "
                         << sqltab->getLastLogMsg());
        }

        for (int r=0; r < this->mslOptions.size(); ++r)
        {
            getnames.push_back(this->mmslCriteria[slCrit[c]][r].toStdString());
            otb::AttributeTable::ColumnValue getval;
            getval.type = otb::AttributeTable::ATTYPE_DOUBLE;
            getval.dval = 0;
            getvalues.push_back(getval);
        }
    }

    // need to add the 'get' fields holding allocated areas for each option
    for (int r=0; r < this->mslOptions.size(); ++r)
    {
        std::stringstream fn;
        fn << "OPT" << r+1 << "_VAL";
        getnames.push_back(fn.str());

        otb::AttributeTable::ColumnValue getval;
        getval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        getval.dval = 0;
        getvalues.push_back(getval);
    }

    // ===============================================================================

    // iterate over optFeat==1 set and calc opt performance
    // and write into db
    sqltab->PrepareBulkGet(getnames, "where optFeat == 1");
    sqltab->PrepareBulkSet(setnames, false);
    sqltab->BeginTransaction();
    bool brow = sqltab->DoBulkGet(getvalues);
    if (!brow)
    {
        MosraLogWarn(<< "NMMosra::calcOptPerformanceDb() - Failed fetching a single row from "
                     << "the database! Double check your OPT_FEATURES setting and table values!");
        sqltab->EndTransaction();
        return 0;
    }

    do
    {
        const long long rowid = getvalues[0].ival;
        const std::string opt_str = getvalues[1].tval;

        for (int k=0; k < slCrit.size(); ++k)
        {
            double dScore = 0.0;
            for (int i=0; i < this->miNumOptions; ++i)
            {
                const QString optStr = opt_str.c_str();
                const QStringList optStrList = optStr.split(" ", Qt::SkipEmptyParts);
                if (optStrList.contains(this->mslOptions[i]))
                {
                    const int scoreOffset = k * this->miNumOptions + 2 + i;
                    const int areaOffset = slCrit.size() * this->miNumOptions + 2 + i;
                    dScore += getvalues[scoreOffset].dval * getvalues[areaOffset].dval;
                }
            }
            setvalues[k].dval = dScore;
        }


        if (!sqltab->DoBulkSet(setvalues, rowid))
        {
            MosraLogError(<< "NMMosra::calcOptPerformanceDb() - Failed setting values for "
                          << idColName << "==" << rowid << "!");
            sqltab->EndTransaction();
            return 0;
        }
        brow = sqltab->DoBulkGet(getvalues);

    } while (brow);

    sqltab->EndTransaction();



    ret = 1;
    return ret;
}

int NMMosra::calcBaselineDb(void)
{
    otb::SQLiteTable::Pointer sqltab = dynamic_cast<otb::SQLiteTable*>(this->mDataSet->getOtbAttributeTable().GetPointer());

    long long lNumCells = sqltab->GetNumRows();

    std::vector< std::string > getnames = {this->msLandUseField.toStdString(),
                                           this->msAreaField.toStdString()};
    std::vector< otb::AttributeTable::TableColumnType > gettypes = {otb::AttributeTable::ATTYPE_STRING,
                                                                    otb::AttributeTable::ATTYPE_DOUBLE};

    bool optFeat = false;
    if (!this->msOptFeatures.isEmpty())
    {
        getnames.push_back(this->msOptFeatures.toStdString());
        gettypes.push_back(otb::AttributeTable::ATTYPE_INT);

        optFeat = true;
    }

    std::vector< std::string > setnames;
    std::vector< otb::AttributeTable::ColumnValue > setvalues;

    const QStringList criteria = this->mmslCriteria.keys();
    foreach(const QString& cri, criteria)
    {
        const QString setcolname = QString("%1_CUR").arg(cri);
        setnames.push_back(setcolname.toStdString());
        otb::AttributeTable::ColumnValue cval;
        cval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        setvalues.push_back(cval);

        const QVector<QString> scorefields = mmslCriteria[cri].toVector();
        for (int f=0; f < scorefields.size(); ++f)
        {
            getnames.push_back(scorefields.at(f).toStdString());
            gettypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
        }
    }


    sqltab->PrepareBulkGet(getnames);
    sqltab->PrepareBulkSet(setnames);
    sqltab->BeginTransaction();

    std::vector< otb::AttributeTable::ColumnValue > getvalues;
    getvalues.resize(gettypes.size());

    const int luoffset = optFeat == true ? 3 : 2;
    for (int cell=0; cell < lNumCells; ++cell)
    {
        if (!sqltab->DoBulkGet(getvalues))
        {
            MosraLogError( << "failed calculating the baseline performance!");
            return 0;
        }

        bool optrow = true;
        if (optFeat && getvalues.at(2).ival == 0)
        {
            optrow = false;
        }

        double area = getvalues.at(1).dval;
        const QString& landuse = static_cast<const char*>(getvalues.at(0).tval);
        const int luidx = mslOptions.indexOf(landuse);

        int cricount=0;
        foreach(const QString& cri, criteria)
        {
            if (optrow && luidx >= 0)
            {
                double scoreha = getvalues.at(luidx + luoffset).dval;
                setvalues[cricount].dval = scoreha * area;
            }
            else
            {
                setvalues[cricount].dval = 0.0;
            }

            ++cricount;
        }

        if (!sqltab->DoBulkSet(setvalues, cell))
        {
            MosraLogError( << "failed calculating the baseline performance!");
            return 0;
        }
    }
    sqltab->EndTransaction();

    // clean up
    //    for (int vv=0; vv < getvalues.size(); ++vv)
    //    {
    //        if (getvalues.at(vv).type == otb::AttributeTable::ATTYPE_STRING)
    //        {
    //            delete[] (getvalues[vv].tval);
    //        }
    //    }

    return 1;
}

vtkSmartPointer<vtkTable> NMMosra::sumResults(vtkSmartPointer<vtkTable>& changeMatrix)
{
    NMDebugCtx(ctxNMMosra, << "...");

    MosraLogDebug(<< "getting input arrays (attributes) ..." << std::endl);
    // get hold of the input attributes we need

//    int luArIdx = mDataSet->getColumnIndex(this->msLandUseField);
//    int areaArIdx = mDataSet->getColumnIndex(this->msAreaField);
//    bool hole = mDataSet->hasColumn("nm_hole");

//    int optStrArIdx = mDataSet->getColumnIndex("OPT_STR");
//    if (optStrArIdx == -1)
//	{
//        MosraLogError( << "failed to fetch result array 'OPT_STR'!");
//		return 0;
//	}

//    QList<int> optValArsIdx;
//	for (int option=0; option < this->miNumOptions; ++option)
//	{
//		QString arName = QString(tr("OPT%1_VAL")).arg(option+1);
//        int idx = mDataSet->getColumnIndex(arName);
//        if (idx == -1)
//		{
//            MosraLogError( << "failed to fetch result array '"
//					<< arName.toStdString().c_str() << "'!");
//			return 0;
//		}
//        optValArsIdx.append(idx);
//	}

    /* ##################################################################################
     *                          CREATE PERFORMANCE SUMMARY TABLE
     * ################################################################################## */

    /* create the vtkTable, which will hold the summarised optimisation results
     * - the column sequence is area and then the individual criteria and for each of those categories we've
     *   got in turn four columns: current (CUR), optimised (OPT), difference (DIFF), relative
     *   difference (REL) (i.e. percent);
     * - the row sequence equals the sequence of land use options (resources) as specified by
     *   the user by the OPTIONS specifier in the parameter file (*.los)
     * - after the land use options, we've got a row of total values, i.e. across all land-use options
     * - the above pattern may be repeated m times for m different spatial (performance)zones
     *
     *  Resource   | Area_CUR | Area_OPT | Area_DIFF | Area_REL | CRI1_CUR | CRI1_OPT | ... | CONS1_CUR | CONS1_OPT ...
     *  -----------+----------+--------------------------------------------------------------------------------------
     *  resource_1 |          |
     *  resource_2 |		  |
     *  ...        |
     *  resource_n |
     *  total      |
     *  zone1:res_1|
     *  zone1:res_2|
     *  ...
     *  zone1:res_n|
     *  zone1:total|
     *  zone2:res_1|
     *  ...
     *  zonem:res_n
     *
     *
     * ---------------------------
     * column identifiers for columnvalues
     *
     *
     *
     * 01: curLU       cur resource: luopt
     * 02: optLU       opt resource: opt_str
     * 03: cellArea    AreaHa
     * 04: optArea     opt<x>_val
     * 05: .
     * 06: .
     * 07: .
     * 08: sumZones    zoneArrayColumn<x> (mslPerfSumZones)
     * 09: .
     * 10: .
     * 11: <EV_cri 1>     evalfield<x> mmslEvalFields
     * 12: .
     * 13: .
     * 14: .
     * 15: <EV_cri 2>     evalfields<x> ...
     * 16: .
     *   : .
     *   : <PS_cri 1>  performance score field<x> ...
     *   : .
     *   : .
     *   : <PS_cri 2>  performance score field<x> ....
     *
     *
     *
     * 17: .
     * 18: .
     * 19: <inc 1>     incField<x>
     * 20: <inc 2>     incField<x>
     * 21: <BL_cri_1>  baseline performance of cri 1
     * 22: <BL_cri_2>  baseline performance of cri 2
     * 23: .
     * 24: .
     * 25: hole       nm_hole
     * 26: msOptFeatures    OPT_FEATURES identifier
     *
     */

    MosraLogDebug(<< "creating the results table..." << std::endl);

    // preparing some variables required later on
    QVariant sval(QVariant::String);
    QMap<QString, int> valOffsets;
    QVariantList colvalues;
    QStringList colnames;
    colnames << this->msLandUseField << "OPT_STR" << this->msAreaField;
    colvalues << sval << sval << QVariant::Double;
    valOffsets.insert("curLU", 0);
    valOffsets.insert("optLU", 1);
    valOffsets.insert("cellArea", 2);

    valOffsets.insert("optArea", colvalues.size());
    for (int opt=0; opt < this->miNumOptions; ++opt)
    {
        const QString cname = QString("OPT%1_VAL").arg(opt+1);
        colnames << cname;
        colvalues << QVariant::Double;
    }

    // determine the number of rows and columns
    //int resNumCols = (this->mmslCriteria.size() + this->mmslCriCons.size()) * 3 +1;

    // we sum over each resource option (land use), all mixed landuses (mixed) and
    // the overall total
    int resNumRows = this->miNumOptions + 1;   // without mixed

    // note: each zone defines an <zone>:IN and <zone>:OUT zone
    // (expecting boolean values {0,1})

    int numZones = 1; // we've got at least the 'global zone' encompassing all
                      // spatial options (i.e. parcels)!
    if (this->mslPerfSumZones.size() > 0)
    {
        valOffsets.insert("sumZones", colvalues.size());
        for (int nz=0; nz < this->mslPerfSumZones.size(); ++nz)
        {
            resNumRows  += this->miNumOptions + 1;
            numZones += 1;

            colnames << this->mslPerfSumZones.at(nz);
            colvalues << QVariant::Double;
        }
    }

    QMap<QString, QStringList>::const_iterator efit =
            this->mmslEvalFields.constBegin();
    for (; efit != this->mmslEvalFields.constEnd(); ++efit)
    {
        const QString& evlKey = QString("EV_%1").arg(efit.key());
        valOffsets.insert(evlKey, colvalues.size());
        for (int p=0; p < efit.value().size(); ++p)
        {
            colnames << efit.value().at(p);
            colvalues << QVariant::Double;
        }
    }

    QMap<QString, QStringList>::const_iterator psit =
            this->mmslCriteria.constBegin();
    for (; psit != this->mmslCriteria.constEnd(); ++psit)
    {
        const QString& psKey = QString("PS_%1").arg(psit.key());
        valOffsets.insert(psKey, colvalues.size());
        for (int p=0; p < psit.value().size(); ++p)
        {
            colnames << psit.value().at(p);
            colvalues << QVariant::Double;
        }
    }

    QMap<QString, QStringList>::const_iterator incIt =
            this->mmslIncentives.constBegin();
    for (; incIt != this->mmslIncentives.constEnd(); ++incIt)
    {
        valOffsets.insert(incIt.key(), colvalues.size());
        colnames << incIt.value().at(0);
        colvalues << QVariant::Double;
    }

    // add current performance score fields;
    // only applicable and available when incentives
    // have been specified for the given scenario
    if (this->mmslIncentives.size() > 0)
    {
        const QStringList& criteria = this->mmslCriteria.keys();
        foreach(const QString& cri, criteria)
        {
            const QString blname = QString("BL_%1").arg(cri);
            valOffsets.insert(blname, colvalues.size());
            const QString curname = QString("%1_CUR").arg(cri);
            colnames << curname;
            colvalues << QVariant::Double;
        }
    }

    const bool hole = mDataSet->hasColumn("nm_hole");
    if (hole)
    {
        valOffsets.insert("hole", colvalues.size());
        colnames << "nm_hole";
        colvalues << QVariant::Int;
    }

    const bool optFeat = mDataSet->hasColumn(this->msOptFeatures);
    if (optFeat)
    {
        valOffsets.insert(this->msOptFeatures, colvalues.size());
        colnames << this->msOptFeatures;
        colvalues << QVariant::Int;
    }




    //----------------------------------

    const long ncells = mDataSet->getNumRecs();

    vtkSmartPointer<vtkTable> restab = vtkSmartPointer<vtkTable>::New();
    restab->SetNumberOfRows(resNumRows);

    // add the row header array (land use options) + total
    vtkSmartPointer<vtkStringArray> rowheads = vtkSmartPointer<vtkStringArray>::New();
    rowheads->SetName("Resource");
    rowheads->SetNumberOfComponents(1);
    rowheads->SetNumberOfTuples(resNumRows);

    int rowCount = 0;
    QString rowHead;
    for (int nz=0; nz < numZones; ++nz)
    {
        for (int r=0; r < this->miNumOptions; ++r)
        {
            if (nz == 0)
            {
                rowHead = this->mslOptions.at(r);
            }
            else
            {
                rowHead = QString("%1:%2")
                        .arg(this->mslPerfSumZones.at(nz-1))
                        .arg(this->mslOptions.at(r));
            }
            rowheads->SetValue(rowCount, (const char*)rowHead.toStdString().c_str());
            //MosraLogDebug(<< "RowHead #" << rowCount << ": " << rowHead.toStdString() << std::endl);
            ++rowCount;
        }

        if (nz == 0)
        {
            rowHead = QString("Total");
        }
        else
        {
            rowHead = QString("%1:Total")
                    .arg(this->mslPerfSumZones.at(nz-1));
        }
        rowheads->SetValue(rowCount, (const char*)rowHead.toStdString().c_str());
        //MosraLogDebug(<< "RowHead #" << rowCount << ": " << rowHead.toStdString() << std::endl);
        ++rowCount;

    }

    //rowheads->SetValue(resNumRows-2, "Mixed");
    //rowheads->SetValue(resNumRows-1, "Total");
    restab->AddColumn(rowheads);

    // the series to the table

    QStringList colsuffix;
    colsuffix << "CUR" << "OPT" << "DIFF" << "REL";

    // add the area results
    for (int s=0; s < colsuffix.size(); ++s)
    {
        QString colname = QString(tr("%1_%2")).arg(this->msAreaField).
                arg(colsuffix.at(s));
        vtkSmartPointer<vtkDoubleArray> aar = vtkSmartPointer<vtkDoubleArray>::New();
        aar->SetName(colname.toLatin1());
        aar->SetNumberOfComponents(1);
        aar->SetNumberOfTuples(resNumRows);
        aar->FillComponent(0, 0);

        restab->AddColumn(aar);
    }


    // add the criterion related columns
    QMap<QString, QStringList>::const_iterator criit = this->mmslCriteria.constBegin();
    for (; criit != this->mmslCriteria.constEnd(); ++criit)
    {
        for (int s=0; s < colsuffix.size(); ++s)
        {
            QString colname = QString(tr("%1_%2")).arg(criit.key()).arg(colsuffix[s]);
            vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
            da->SetName(colname.toLatin1());
            da->SetNumberOfComponents(1);
            da->SetNumberOfTuples(resNumRows);
            da->FillComponent(0, 0);

            restab->AddColumn(da);
        }
    }

    //	// add columns for attribute constraints
    //	QMultiMap< QString, QMap< QString, QStringList > >::const_iterator constrit =
    //			this->mmslCriCons.constBegin();
    //	for (; constrit != this->mmslCriCons.constEnd(); ++constrit)
    //	{
    //		criit = constrit.value().constBegin();
    //		for (; criit != constrit.value().constEnd(); ++criit)
    //		{
    //			for (int s=0; s < colsuffix.size(); ++s)
    //			{
    //				QString colname = QString(tr("c_%1%2_%3")).arg(constrit.key()).
    //						arg(criit.key()).arg(colsuffix.at(s));
    //				vtkSmartPointer<vtkDoubleArray> daa = vtkSmartPointer<vtkDoubleArray>::New();
    //                daa->SetName(colname.toLatin1());
    //				daa->SetNumberOfComponents(1);
    //				daa->SetNumberOfTuples(resNumRows);
    //				daa->FillComponent(0, 0);

    //				restab->AddColumn(daa);
    //			}
    //		}
    //	}

    /* ##################################################################################
     *                          CREATE RESOURCE CHANGE MATRIX
     * ################################################################################## */
    changeMatrix = vtkSmartPointer<vtkTable>::New();
    changeMatrix->SetNumberOfRows(this->miNumOptions+2);

    // add the row header array (land use options) + total
    vtkSmartPointer<vtkStringArray> chngheads = vtkSmartPointer<vtkStringArray>::New();
    chngheads->SetName("from/to");
    chngheads->SetNumberOfComponents(1);
    chngheads->SetNumberOfTuples(this->miNumOptions+2);

    for (int no=0; no < this->miNumOptions; ++no)
    {
        chngheads->SetValue(no, (const char*)this->mslOptions.at(no).toLatin1());
    }
    chngheads->SetValue(this->miNumOptions, "other");
    chngheads->SetValue(this->miNumOptions+1, "SUM");
    changeMatrix->AddColumn(chngheads);

    for (int no=0; no < this->miNumOptions+2; ++no)
    {
        vtkSmartPointer<vtkDoubleArray> car = vtkSmartPointer<vtkDoubleArray>::New();
        if (no < this->miNumOptions)
        {
            car->SetName((const char*)this->mslOptions.at(no).toLatin1());
        }
        else if (no == this->miNumOptions)
        {
            car->SetName("other");
        }
        else //if (no == this->miNumOptions+1)
        {
            car->SetName("SUM");
        }
        car->SetNumberOfComponents(1);
        car->SetNumberOfTuples(this->miNumOptions+2);
        car->FillComponent(0,0);
        changeMatrix->AddColumn(car);
    }


    /* ##################################################################################
     *                          SUMMARISE PERFORMANCES AND FILL CHANGE MATRIX
     * ################################################################################## */

    MosraLogDebug(<< "summarising results ..." << std::endl << std::endl);

    mDataSet->beginTransaction();
    mDataSet->prepareRowGet(colnames);

    int resRec = 0;
    int rec = 0;
    int zoneCounter = 0;
    // loop over the data set and sequentially process (update) all target columns
    // only after the final iteration, the table holds correct values;
    for (long cell=0; cell < ncells; ++cell)
    {
        mDataSet->getRowValues(colvalues, cell);

        // don't tumble into holes
        if (    (hole && colvalues.at(valOffsets["hole"]).toInt() == 1)
             || (optFeat && colvalues.at(valOffsets[this->msOptFeatures]).toInt() == 0)
           )
        {
            continue;
        }

        // read the current and optimised resource
        QString curResource = colvalues.at(valOffsets["curLU"]).toString();
        // make sure, we're not fooled by any leading or trailing white spaces
        curResource = curResource.simplified();
        QString optResource = colvalues.at(valOffsets["optLU"]).toString();
        QStringList optResList = optResource.split(tr(" "), Qt::SkipEmptyParts);
        const double curArea = colvalues.at(valOffsets["cellArea"]).toDouble();

        // ===============================================================================
        //                      CHANGE ANALYSIS
        // ===============================================================================

        QVector<int> toIdx;

        // set from initially to 'other'
        int fromIdx = this->miNumOptions;
        for (int no=0; no < this->miNumOptions; ++no)
        {
            if (curResource.compare(this->mslOptions.at(no), Qt::CaseInsensitive) == 0)
            {
                fromIdx = no;
            }

            if (optResList.contains(this->mslOptions.at(no)))
            {
                toIdx.push_back(no);
            }
        }
        // add 'other' land use as recipient, if the optResList doesn't
        // contain curResource
        if (toIdx.size() == 0)
        {
            toIdx.push_back(this->miNumOptions);
        }

        int coloff = 1;
        for (int t=0; t < toIdx.size(); ++t)
        {
            double chngVal = changeMatrix->GetValue(fromIdx, toIdx.at(t)+coloff).ToDouble();
            double newValue = 0;
            if (toIdx.at(t) < this->miNumOptions)
            {
                newValue = colvalues.at(valOffsets["optArea"]+toIdx.at(t)).toDouble();
            }
            else   // get the area value from the AreaHa field for 'other' land uses
            {
                newValue = curArea;
            }

            chngVal += newValue;
            changeMatrix->SetValue(fromIdx, toIdx.at(t)+coloff, vtkVariant(chngVal));
        }

        // DEBUG
        //		NMDebugInd(ind, << "curResource: " << curResource.toStdString() << endl);
        //		NMDebugInd(ind, << "optResource: " << optResource.toStdString() << endl);
        //		NMDebugInd(ind, << "optResList: ");
        //		for (int r=0; r < optResList.size(); ++r)
        //		{
        //			NMDebug(<< "-" << optResList.at(r).toStdString() << "-");
        //		}
        //		NMDebug(<< endl << endl);

        // ===============================================================================
        //                      PERFORMANCE ANALYSIS (TOTAL and per ZONE)
        // ===============================================================================
        for (int zone=0; zone < numZones; ++zone)
        {
            rec = zone * (this->miNumOptions+1);

            if (zone >= 1)
            {
                if (colvalues.at(valOffsets["sumZones"]+zone-1).toDouble() == 0)
                {
                    continue;
                }
            }

            resRec = rec;
            double totalCellOptArea = 0;
            for (int option=0; option < this->miNumOptions; ++option)
            {
                // common vars
                double performanceValue;

                // handle current land uses
                double curaccumArea;
                if (curResource.compare(this->mslOptions.at(option), Qt::CaseInsensitive) == 0)
                {
                    // process area
                    curaccumArea = curArea + restab->GetValue(resRec, 1).ToDouble();
                    restab->SetValue(resRec,1, vtkVariant(curaccumArea));

                    // update criteria stats
                    QMap<QString, QStringList>::const_iterator evalit =
                            this->mmslEvalFields.constBegin();
                    int coloffset = 1;
                    for (; evalit != this->mmslEvalFields.constEnd(); ++evalit, ++coloffset)
                    {
                        const QString& evKey = QString("EV_%1").arg(evalit.key());
                        performanceValue = colvalues.at(valOffsets[evKey]+option).toDouble() * curArea +
                                restab->GetValue(resRec, coloffset*4 + 1).ToDouble();

                        restab->SetValue(resRec, coloffset*4 + 1, vtkVariant(performanceValue));

                    }
                }

                // handle the optimised land uses
                double optArea = 0;
                double accumArea = 0;
                if (optResList.contains(this->mslOptions.at(option), Qt::CaseInsensitive))
                {
                    // get the allocated area for this resource category
                    optArea = colvalues.at(valOffsets["optArea"]+option).toDouble();

                    // define variable for row index in result table
                    int resTabRow = resRec;

                    // update area stats for actual resource category
                    accumArea = optArea + restab->GetValue(resTabRow, 2).ToDouble();
                    restab->SetValue(resTabRow, 2, vtkVariant(accumArea));

                    // update criteria stats
                    QMap<QString, QStringList>::const_iterator evalit =
                            this->mmslEvalFields.constBegin();
                    int coloffset = 1;
                    for (; evalit != this->mmslEvalFields.constEnd(); ++evalit, ++coloffset)
                    {
                        const QString& evKey = QString("EV_%1").arg(evalit.key());
                        performanceValue = colvalues.at(valOffsets[evKey]+option).toDouble() * optArea +
                                restab->GetValue(resTabRow, coloffset*4 + 2).ToDouble();

                        restab->SetValue(resTabRow, coloffset*4 + 2, vtkVariant(performanceValue));
                    }
                }
                totalCellOptArea += optArea;

                ++resRec;
            } // end land use options iteration

            /*
            if ((cell == 801 || cell == 1170 || cell == 3019))
            {
                const int a = 3;
            }
            */

            // account for the case where a current land use was 'retired', i.e. we have no allocation of any land use option
            // to that cell in question AND that we use INCENTIVES to support maximising our benefit criterion (e.g. REVENUE)
            if (    !mIncNamePair.isEmpty()
                 && curArea > 0
               )
            {
                const int curOpt = this->mslOptions.indexOf(curResource);
                const int curResRec = resRec - (this->miNumOptions - curOpt);

                std::vector<int> vOpt;
                std::vector<int> vResRec;
                std::vector<double> vOptArea;
                double totalOptArea = 0.0;

                // if there's no land use option allocated to this parcel
                // we put in the current land use's option id and table row rec num
                // to process 'retirement';
                if (optResList.isEmpty())
                {
                    vOpt.push_back(curOpt);
                    vResRec.push_back(curResRec);
                    vOptArea.push_back(curArea);
                }
                else
                {
                    for (int opt=0; opt < optResList.size(); ++opt)
                    {
                        vOpt.push_back(this->mslOptions.indexOf(optResList.at(opt)));   // option idx
                        vResRec.push_back((resRec - (this->miNumOptions - vOpt.back())));   // option res rec
                        vOptArea.push_back(colvalues.at(valOffsets["optArea"]+vOpt.back()).toDouble());
                        totalOptArea += vOptArea.back();
                    }
                }

                QMap<QString, QStringList>::const_iterator evalit =
                        this->mmslEvalFields.constBegin();
                int coloffset = 1;
                for (; evalit != this->mmslEvalFields.constEnd(); ++evalit, ++coloffset)
                {
                    for (int oopt=0; oopt < vOpt.size(); ++oopt)
                    {
                        double incPerfVal = 0.0;
                        double performanceValue = restab->GetValue(vResRec[oopt], coloffset*4 + 2).ToDouble();

                        // go through the incentives and work out the extra benefit for the given 'optimal' land use option
                        QMap<QString, QStringList>::const_iterator incnamesIt = this->mIncNamePair.constBegin();
                        for (; incnamesIt != this->mIncNamePair.constEnd(); ++incnamesIt)
                        {
                            // check whether the currently processed criterion benefits from
                            // incentivised reduction of another criterion
                            if (incnamesIt.value().at(0).compare(evalit.key(), Qt::CaseInsensitive) == 0)
                            {
                                const QString& blkey = QString("BL_%1").arg(incnamesIt.value().at(1));
                                const QString& pskey = QString("PS_%1").arg(incnamesIt.value().at(1));

                                const double baseline = colvalues.at(valOffsets[blkey]).toDouble();
                                const double incentive = colvalues.at(valOffsets[incnamesIt.key()]).toDouble();

                                // if we've got some new land use options allocated to this parcel
                                if (totalOptArea > 0)
                                {
                                    const double curLUScore = colvalues.at(valOffsets[pskey]+curOpt).toDouble();
                                    const double optLUScore = colvalues.at(valOffsets[pskey]+vOpt[oopt]).toDouble();
                                    const double optArea = vOptArea[oopt];

                                    const double optLUReduction = (curLUScore - optLUScore) > 0 ? (curLUScore - optLUScore) * optArea : 0.0;

                                    // 'pay' incentives based on the proportional reduction
                                    // achieved by the given land use
                                    incPerfVal += optLUReduction * incentive;

                                }
                                // if this parcel has been retired from productive land use
                                else
                                {
                                    incPerfVal += baseline * incentive;
                                }
                            }
                        }

                        if (incPerfVal > 0)
                        {
                            performanceValue += incPerfVal;
                            restab->SetValue(vResRec[oopt], coloffset*4 + 2, vtkVariant(performanceValue));

                            /*
                            if ((cell == 801 || cell == 1255 || cell == 2052 || cell == 3019))
                            {
                                if (totalOptArea == 0)
                                {
                                    NMDebug(<< "free-#" << cell << ": inc" << evalit.key().toStdString()
                                               << "(" << curResource.toStdString() << ", " << vResRec[oopt]
                                                << ") = " << incPerfVal << "\n");
                                }
                                else
                                {
                                    NMDebug(<< "repl-#" << cell << ": inc" << evalit.key().toStdString()
                                               << "(" << curResource.toStdString() << ", " << vResRec[oopt]
                                                << ") = " << incPerfVal << "\n");
                                }
                            }
                            */

                        }

                    } // 'optimal' option iteration
                } // end eval criteria iteration

            } // end of INCENTIVES HANDLING

        } // end zones iteration


        if (cell % 200 == 0)
        {
            NMDebug(<< ".");
        }
    }
    mDataSet->endTransaction();

    NMDebug(<< " finished!" << std::endl);

    // ===============================================================================
    //                  CALCULATE PERFORMANCE TOTALS
    // ===============================================================================


    // sum totals
    int ncols = restab->GetNumberOfColumns();
    for (int sp=1; sp < ncols; ++sp)
    {
        for (int zone=0; zone < numZones; ++zone)
        {
            double sum = 0.0;
            int rec = zone * (this->miNumOptions+1);
            for (int opt=0; opt < this->miNumOptions; ++opt)
            {
                sum += restab->GetValue(rec, sp).ToDouble();
                ++rec;
            }
            restab->SetValue(rec, sp, vtkVariant(sum));
        }
    }

    // calc differences
    for (int zone=0; zone < numZones; ++zone)
    {
        int rec = zone * (this->miNumOptions+1);
        for (int opt=0; opt < this->miNumOptions+1; ++opt)
        {
            for (int sp=0; sp < ncols-3; sp += 4)
            {
                double diff = restab->GetValue(rec, sp+2).ToDouble() -
                        restab->GetValue(rec, sp+1).ToDouble();
                restab->SetValue(rec, sp+3, vtkVariant(diff));
                double denom = restab->GetValue(rec, sp+1).ToDouble();
                if (denom)
                {
                    double rel = diff / denom * 100.0;
                    restab->SetValue(rec, sp+4, vtkVariant(rel));
                }
                else if (diff && denom == 0)
                {
                    restab->SetValue(rec, sp+4, vtkVariant(100.0));
                }
                else
                {
                    restab->SetValue(rec, sp+4, vtkVariant(0.0));
                }
            }
            ++rec;
        }
    }

    // ===============================================================================
    //                      CALCULATE CHANGE TOTALS
    // ===============================================================================

    // calc row totals
    int coloff = 1;
    for (int r=0; r < this->miNumOptions+1; ++r)
    {
        double val = 0;
        for (int c=0; c < this->miNumOptions+1+coloff; ++c)
        {
            val += changeMatrix->GetValue(r, c+coloff).ToDouble();
        }
        changeMatrix->SetValue(r, this->miNumOptions+1+coloff, vtkVariant(val));
    }

    for (int c=0; c < this->miNumOptions+1+coloff; ++c)
    {
        double val = 0;
        for (int r=0; r < this->miNumOptions+1; ++r)
        {
            val += changeMatrix->GetValue(r, c+coloff).ToDouble();
        }
        changeMatrix->SetValue(this->miNumOptions+1, c+coloff, vtkVariant(val));
    }


    NMDebugCtx(ctxNMMosra, << "done!");
    return restab;
}

//QStandardItemModel* NMMosra::prepareResChartModel(vtkTable* restab)
//{
//	if (restab == 0)
//		return 0;
//
//	NMDebugCtx(ctxNMMosra, << "...");
//
//
//	int nDestCols = restab->GetNumberOfRows();
//	int nSrcCols = restab->GetNumberOfColumns();
//	int nDestRows = (nSrcCols-1) / 4;
//
//	QStandardItemModel* model = new QStandardItemModel(nDestRows, nDestCols, this->parent());
//	model->setItemPrototype(new QStandardItem());
//
//	MosraLogInfo( << "populating table ..." << endl);
//
//	QStringList slVHeaderLabels;
//	int srccol = 4;
//	for (int row=0; row < nDestRows; ++row, srccol+=4)
//	{
//		QString sVHeader = restab->GetColumnName(srccol);
//		slVHeaderLabels.append(sVHeader);
//		model->setVerticalHeaderItem(row, new QStandardItem());
//		model->verticalHeaderItem(row)->setData(QVariant((int)row*40), Qt::DisplayRole);
//
//		for (int col=0; col < nDestCols; ++col)
//		{
//			if (row == 0)
//			{
//				QString sHHeader = restab->GetValue(col, 0).ToString().c_str();
//				model->setHorizontalHeaderItem(col, new QStandardItem());
//				model->horizontalHeaderItem(col)->setData(QVariant(sHHeader), Qt::DisplayRole);
//			}
//
//			model->setItem(row, col, new QStandardItem());
//			model->item(row, col)->setData(QVariant(restab->GetValue(col, srccol).ToDouble()),
//					Qt::DisplayRole);
//		}
//	}
//	model->setVerticalHeaderLabels(slVHeaderLabels);
//
//	NMDebugCtx(ctxNMMosra, << "done!");
//
//	return model;
//
//}

HLpHelper* NMMosra::getLp()
{
    return this->mLp;
}

int NMMosra::callbackIsSolveCanceled(lprec *lp, void *userhandle)
{
    if (userhandle == 0)
        return 1;

    NMMosra *mosra = static_cast<NMMosra*>(userhandle);
    return mosra->isSolveCanceled();
}

void
NMMosra::lpLogCallback(lprec *lp, void *userhandle, char *buf)
{
    NMMosra* mosra = static_cast<NMMosra*>(userhandle);
    if (mosra)
    {
        mosra->forwardLpLog(buf);
    }
}

void NMMosra::forwardLpLog(const char* log)
{
    MosraLogInfo(<< "lp_solve: " << log);
}

int NMMosra::isSolveCanceled()
{
    return this->mbCanceled;
}

double NMMosra::convertAreaUnits(double dUserVal, AreaUnitType otype)
{
    bool bConvOK;
    double dtval;
    switch (otype)
    {
    case NMMosra::NM_MOSO_PERCENT_TOTAL:
//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
//		else
            dtval = this->mdAreaTotal * dUserVal/100.0;
        break;
    case NMMosra::NM_MOSO_PERCENT_SELECTED:

//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(this->mdAreaSelected * dUserVal/100.0);
//		else
            dtval = this->mdAreaSelected * dUserVal/100.0;
        break;
    default:
//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(dUserVal);
//		else
            dtval = dUserVal;
        break;
    }

    return dtval;
}

double NMMosra::convertAreaUnits(double dUserVal, const QString& otype, const QStringList& zoneSpec)
{
    bool bConvOK;
    double dtval;

    if (otype.compare(tr("percent_of_total"), Qt::CaseInsensitive) == 0)
    {
            dtval = this->mdAreaTotal * dUserVal/100.0;
    }
    else if (otype.compare(tr("percent_of_selected"), Qt::CaseInsensitive) == 0)
    {

            dtval = this->mdAreaSelected * dUserVal/100.0;
    }
    else if (otype.compare(tr("percent_of_zone"), Qt::CaseInsensitive) == 0)
    {
        // if there is no zone spec given, we interpret it as percent_of_total
        if (zoneSpec.size() == 0)
        {
                dtval = this->mdAreaTotal * dUserVal/100.0;

            // todo: issue a warning message, that a percent_of_total is used instead
        }
        else
        {
            double zonearea = this->mmslZoneAreas.find(zoneSpec.at(1)).value().find(zoneSpec.at(0)).value();
                dtval = zonearea * dUserVal/100.0;
        }
    }
    else
    { // -> "map_units"
            dtval = dUserVal;
    }

    return dtval;
}
