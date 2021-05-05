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

#include "NMMosra.h"
#include "nmlog.h"
#include "itkNMLogEvent.h"
#include "NMLogger.h"

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


//#include "NMTableCalculator.h"
//#include "NMMfwException.h"
#include <string>
#include <iostream>
#include <sstream>

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
#include <QSqlDriver>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlIndex>
#include <QSqlError>
#include <QVariantList>

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

#include "lp_lib.h"


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
                    val = sa->GetValue(row);
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
                    val = sa->GetValue(row);
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
                        varVal = QVariant::fromValue(QString(sa->GetValue(row).operator const char *()));
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

const std::string NMMosra::ctxNMMosra = "NMMosra";

NMMosra::NMMosra(QObject* parent) //: QObject(parent)
    : mProblemFilename(""), mProblemType(NM_MOSO_LP)
{
	NMDebugCtx(ctxNMMosra, << "...");

    this->mDataSet = new NMMosraDataSet(this);

    this->mLogger = 0;
    this->mProcObj = 0;
	this->setParent(parent);
	this->mLp = new HLpHelper();
	this->reset();

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
        incentives,
		objectives,
		arealcons,
        featcons,
        zonecons,
		cricons,
		objcons,
		batch,
		nosection
	};

	ParSec section = nosection;

    sReport << "Import Report" << endl << endl;
    MosraLogDebug( << "parsing settings file ..." << endl)
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
			sReport << "Line " << numline << " does not belong to a valid data section" << endl;
			continue;
		}

		//look for an equal sign to indicate data assignment
		sep = sLine.indexOf('=');

		//if there is no equal sign skip processing of this line
		if (sep == -1)
		{
			sReport << "Line " << numline << " contains invalid data" << endl;
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

                MosraLogInfo( << "DEVTYPE: " << this->meDVType << endl)
			}
			else if (sVarName.compare(tr("CRITERION_LAYER"), Qt::CaseInsensitive) == 0)
			{
				this->msLayerName = sValueStr;
                MosraLogInfo( << "LayerName: " << this->msLayerName.toStdString() << endl)
			}
			else if (sVarName.compare(tr("LAND_USE_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msLandUseField = sValueStr;
                MosraLogInfo( << "LandUseField: " << this->msLandUseField.toStdString() << endl)
			}
			else if (sVarName.compare(tr("AREA_FIELD"), Qt::CaseInsensitive) == 0)
			{
				this->msAreaField = sValueStr;
                MosraLogInfo( << "AreaField: " << this->msAreaField.toStdString() << endl)
			}
            else if (sVarName.compare(tr("PERFORMANCE_SUM_ZONES"), Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->mslPerfSumZones = sValueStr.split(" ", QString::SkipEmptyParts);
                }
                MosraLogInfo( << "PerformanceSumZones: " << mslPerfSumZones.join(" ").toStdString() << endl)
            }
            else if (sVarName.compare("TIMEOUT", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    if (sValueStr.compare("break_at_first", Qt::CaseInsensitive) == 0)
                    {
                        this->muiTimeOut = 0;
                        this->mbBreakAtFirst = true;
                        MosraLogInfo(<< "Solver timeout: break at first feasible solution!" << endl);
                    }
                    else
                    {
                        bool bok;
                        unsigned int timeout = sValueStr.toUInt(&bok);
                        if (bok)
                        {
                            this->mbBreakAtFirst = false;
                            this->muiTimeOut = timeout;
                            MosraLogInfo(<< "Solver timeout: " << timeout << endl);
                        }
                    }
                }
            }
            else if (sVarName.compare("DATAPATH", Qt::CaseInsensitive) == 0)
            {
                this->msDataPath = sValueStr;
                MosraLogInfo(<< "batch data path: " << this->msDataPath.toStdString() << endl);
            }
            else if (sVarName.compare("OPT_FEATURES", Qt::CaseInsensitive) == 0)
            {
                if (!sValueStr.isEmpty())
                {
                    this->msOptFeatures = sValueStr;
                    MosraLogInfo(<< "OPT_FEATURES: " << this->msOptFeatures.toStdString() << endl);
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
                    MosraLogError(<< "Line " << numline << " contains an invalid number" << endl;)
                    sReport << "Line " << numline << " contains an invalid number" << endl;
                }


                MosraLogInfo( << "number of resource options: " << this->miNumOptions << endl)
			}
			else if (sVarName.compare(tr("OPTIONS"), Qt::CaseInsensitive) == 0)
			{
				QStringList tmpList = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (tmpList.size() == this->miNumOptions)
				{
					this->mslOptions.clear();
					this->mslOptions << tmpList;
				}
				else
				{
                    MosraLogError(<< "Line " << numline << " contains an invalid number of options" << endl)
					sReport << "Line " << numline << " contains an invalid number of options" << endl;
				}

                MosraLogInfo( << "options: " << this->mslOptions.join(tr(" ")).toStdString() << endl)

			}
			else if (sVarName.indexOf(tr("CRI_"), Qt::CaseInsensitive) != -1)
			{
				QStringList criFieldNames = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (criFieldNames.size() == this->miNumOptions + 1)
				{
					QString scri = criFieldNames.at(0);
					criFieldNames.removeAt(0);
					this->mmslCriteria.insert(scri, criFieldNames);

                    MosraLogDebug( << "criterion: " << scri.toStdString() << " " << this->mmslCriteria.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criteria" << endl;)
					sReport << "Line " << numline << " contains an invalid number of criteria" << endl;
				}
			}
			else if (sVarName.indexOf(tr("EVAL_"), Qt::CaseInsensitive) != -1)
			{
				QStringList evalFieldNames = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (evalFieldNames.size() == this->miNumOptions + 1)
				{
					QString scri = evalFieldNames.at(0);
					evalFieldNames.removeAt(0);
					this->mmslEvalFields.insert(scri, evalFieldNames);

                    MosraLogDebug( << "criterion evaluation fields: " << scri.toStdString() << " " << this->mmslEvalFields.find(scri).value().join(tr(" ")).toStdString() << endl);
				}
				else
				{
                    MosraLogError(<< "Line " << numline << " contains an invalid number of criterion evaluation fields" << endl;)
					sReport << "Line " << numline << " contains an invalid number of criterion evaluation fields" << endl;
				}
			}

		}
		break;
        //----------------------------------------INCENTIVES----------
        case incentives:
        {
            if (sVarName.indexOf(tr("INC_"), Qt::CaseInsensitive) != -1)
            {
                QStringList incfields = sValueStr.split(tr(" "), QString::SkipEmptyParts);
                if (!incfields.isEmpty())
                {
                    QString incName = incfields.takeFirst();
                    QStringList namePair = incName.split("_", QString::SkipEmptyParts);
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
                                      << "whose name is given after the incentive name." << endl;
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
                MosraLogInfo( << "Scalarisation method is '" << sAggrMeth.toStdString() << "'" << endl)
			}
			else if (sVarName.indexOf(tr("OBJ_"), Qt::CaseInsensitive) != -1)
			{
				 QStringList objs = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				 if (objs.size() != 0)
				 {
					 QString obj = objs.takeAt(1);
					 this->mmslObjectives.insert(obj, objs);
                     MosraLogInfo( << "obj: " << obj.toStdString() << ": "
                             << this->mmslObjectives.find(obj).value().join(tr(" ")).toStdString() << endl)
				 }
			}
		}
		break;
        //-----------------------------------------AREAL_CONS------------
		case arealcons:
		{
			if (sVarName.indexOf(tr("AREAL_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList arCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (arCons.size() != 0)
				{
					this->mmslAreaCons.insert(sVarName, arCons);
                    MosraLogDebug( << "areal cons: " << sVarName.toStdString() << ": "
							<< this->mmslAreaCons.find(sVarName).value().join(tr(" ")).toStdString() << endl);

					// check, whether we've got a zoning constraint here and if so, initialise the
					// zones area with 0
					QString luopt = arCons.at(0);
					if (luopt.contains(':', Qt::CaseInsensitive))
					{
						QStringList luoptlist = luopt.split(tr(":"), QString::SkipEmptyParts);
						QString zonefield = luoptlist.at(1);
                        // allow for multiple land use options being specified separated by comata
                        // !without whitespace!
                        QStringList options = luoptlist.at(0).split("+", QString::SkipEmptyParts);

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
                QStringList featCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
                if (featCons.size() != 0)
                {
                    this->mmslFeatCons.insert(sVarName, featCons);
                    MosraLogDebug( << "feature cons: " << sVarName.toStdString() << ": "
                            << this->mmslFeatCons.find(sVarName).value().join(tr(" ")).toStdString() << endl);
                }
            }
        }
        break;
        //----------------------------------------------ZONE_CONST-------
        case zonecons:
        {
            if (sVarName.indexOf(tr("ZONE_CONS_"), Qt::CaseInsensitive) != -1)
            {
                QStringList fullZoneCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
                if (fullZoneCons.size() == 3)
                {
                    QString label = sVarName;
                    QStringList zonespec = fullZoneCons.at(0).split(":", QString::SkipEmptyParts);
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
                MosraLogDebug(<< "\tgonna split raw list: " << sValueStr.toStdString() << endl);
				QStringList outerList = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (outerList.size() != 0)
				{
					QString criLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tcriLabel is '" << criLabel.toStdString() << "'" << endl);
					QString luLabel = outerList.takeAt(0);
                    MosraLogDebug( << "\tland use is '" << luLabel.toStdString() << "'" << endl);

					QMap<QString, QStringList> innerMap;
					innerMap.insert(luLabel, outerList);

					this->mmslCriCons.insert(criLabel, innerMap);

                    MosraLogDebug( << "cri cons: " << criLabel.toStdString() << ": "
							<< luLabel.toStdString() << ": "
							<< outerList.join(tr(" ")).toStdString() << endl);
				}
			}
		}
		break;
        //------------------------------------------------OBJ_CONS-------
		case objcons:
		{
			if (sVarName.indexOf(tr("OBJ_CONS_"), Qt::CaseInsensitive) != -1)
			{
				QStringList objCons = sValueStr.split(tr(" "), QString::SkipEmptyParts);
				if (objCons.size() > 0)
				{
					QString objkey = sVarName + QString(tr("_%1")).arg(objCons.value(0));
					this->mmslObjCons.insert(objkey, objCons);
                    MosraLogInfo( << objkey.toStdString() << ": "
							<< this->mmslObjCons.find(objkey).value().join(tr(" ")).toStdString() << endl);
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
                MosraLogInfo(<< "batch data path: " << this->msDataPath.toStdString() << endl);
			}
			else if (sVarName.compare("PERTURB", Qt::CaseInsensitive) == 0)
			{
				this->mslPerturbItems = sValueStr.split(" ");
				if (mslPerturbItems.size() > 0)
				{
                    MosraLogInfo(<< "Criteria/constraints to be perturbed: ");
					foreach(const QString& pc, this->mslPerturbItems)
					{
						NMDebug(<< pc.toStdString() << " ");
					}
					NMDebug(<< endl);
				}
				else
				{
                    MosraLogInfo(<< "No perturbation criteria provided!" << endl);
				}
			}
			else if (sVarName.compare("UNCERTAINTIES", Qt::CaseInsensitive) == 0)
			{
				QStringList lunsure = sValueStr.split(" ");
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
                    logstr << endl;
                    MosraLogDebug(<< logstr.str())
				}
				else
				{
                    MosraLogInfo(<< "No uncertainty levels for perturbation provided!" << endl);
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
                        MosraLogInfo(<< "Number of perturbations: " << reps << endl);
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
                        MosraLogInfo(<< "Solver timeout: break at first feasible solution!" << endl);
                    }
                    else
                    {
                        bool bok;
                        unsigned int timeout = sValueStr.toUInt(&bok);
                        if (bok)
                        {
                            this->mbBreakAtFirst = false;
                            this->muiTimeOut = timeout;
                            MosraLogInfo(<< "Solver timeout: " << timeout << endl);
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


	NMDebug(<< endl << "Report..." << endl << sReport.readAll().toStdString() << endl);

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

void NMMosra::solveProblem(void)
{
    NMDebugCtx(ctxNMMosra, << "...");

    this->mLp->Solve();

    this->createReport();

    MosraLogInfo(<< "Optimisation Report ... \n" << this->getReport().toStdString() << endl);

    NMDebugCtx(ctxNMMosra, << "done!");
}

int NMMosra::solveLp(void)
{
//	NMDebugCtx(ctxNMMosra, << "...");

//	// check, whether all settings are ok
//	if (!this->checkSettings())
//	{
//		NMDebugCtx(ctxNMMosra, << "done!");
//		return 0;
//	}
//    MosraLogInfo(<< "checking optimisation settings - OK")

//    // calc baseline
//    this->calcBaseline();
//    MosraLogInfo(<< "calculdated baseline alright!");


//    this->makeLp();

//	if (!this->addObjFn())
//	{
//		NMDebugCtx(ctxNMMosra, << "done!");
//		return 0;
//	}
//    MosraLogInfo(<< "adding objective function - OK")

//	if (this->meScalMeth == NMMosra::NM_MOSO_INTERACTIVE &&
//			this->mmslObjCons.size() > 0)
//    {
//		this->addObjCons();
//        MosraLogInfo(<< "adding objective constraints - OK")
//    }

//	// doe we have any additional constraints?
//	if (this->mmslAreaCons.size() > 0)
//	{
//		if (!this->addExplicitAreaCons())
//		{
//			NMDebugCtx(ctxNMMosra, << "done!");
//			return 0;
//		}
//        MosraLogInfo(<< "adding allocation constraints - OK")
//	}

//	if (this->mmslCriCons.size() > 0)
//	{
//		if (!this->addCriCons())
//		{
//			NMDebugCtx(ctxNMMosra, << "done!");
//			return 0;
//		}
//        MosraLogInfo(<< "adding performance constraints - OK")
//	}

//    if (this->mmslIncentives.size() > 0)
//    {
//        if (!this->addIncentCons())
//        {
//            NMDebugCtx(ctxNMMosra, << "done!");
//            return 0;
//        }
//        MosraLogInfo(<< "adding incentives 'constraints' - OK")
//    }

//    if (this->mslZoneConstraints.size() > 0)
//    {
//        if (!this->addZoneCons())
//        {
//            NMDebugCtx(ctxNMMosra, << "done!");
//            return 0;
//        }
//        MosraLogInfo(<< "adding zone constraints - OK")
//    }


//    if (this->mmslFeatCons.size() > 0)
//    {
//        if (!this->addFeatureCons())
//        {
//            NMDebugCtx(ctxNMMosra, << "done!");
//            return 0;
//        }
//        MosraLogInfo(<< "adding feature constraints - OK");
//    }
//    else
//    {
//        if (!this->addImplicitAreaCons())
//        {
//            NMDebugCtx(ctxNMMosra, << "done!");
//            return 0;
//        }
//        MosraLogInfo(<< "adding internal areal (consistency) constraints - OK")
//    }

//    if (this->mbBreakAtFirst)
//    {
//        this->mLp->SetBreakAtFirst(true);

//        MosraLogInfo(<< "solver stops at first feasible solution!" << std::endl);
//    }
//    else
//    {
//        this->mLp->SetTimeout(this->muiTimeOut);

//        MosraLogInfo(<< "solver times out after " << this->muiTimeOut
//                << " seconds!" << std::endl);
//    }
//    this->mbCanceled = false;
//    this->mLp->SetAbortFunc((void*)this, NMMosra::callbackIsSolveCanceled);
//    this->mLp->SetLogFunc((void*)this, NMMosra::lpLogCallback);

//    // if the user wishes, we write out the problem we've just created ...
//    if (!this->mProblemFilename.isEmpty())
//    {
//        if (this->mProblemType == NM_MOSO_LP)
//        {
//            this->mLp->WriteLp(this->mProblemFilename.toStdString());
//        }
//        else
//        {
//            this->mLp->WriteMps(this->mProblemFilename.toStdString());
//        }
//    }

//    this->mLp->SetPresolve(PRESOLVE_COLS |
//                           PRESOLVE_ROWS |
//                           PRESOLVE_IMPLIEDFREE |
//                           PRESOLVE_REDUCEGCD |
//                           PRESOLVE_MERGEROWS |
//                           PRESOLVE_ROWDOMINATE |
//                           PRESOLVE_COLDOMINATE |
//                           PRESOLVE_KNAPSACK |
//                           PRESOLVE_PROBEFIX);

//    this->mLp->SetScaling(SCALE_GEOMETRIC |
//                          SCALE_DYNUPDATE);
    if (!this->configureProblem())
    {
        return 0;
    }

    this->solveProblem();
//	this->mLp->Solve();

//	this->createReport();

//    MosraLogInfo(<< "Optimisation Report ... \n" << this->getReport().toStdString() << endl);

//	NMDebugCtx(ctxNMMosra, << "done!");
	return 1;
}

void NMMosra::createReport(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	// let's put out some info about the solution, if any
	QString streamstring;
	QTextStream sRes(&streamstring);
	sRes.setRealNumberNotation(QTextStream::SmartNotation);
	sRes.setRealNumberPrecision(15);

	sRes << endl << endl;
	sRes << tr("\tProblem setting details") << endl;
	sRes << tr("\t-----------------------") << endl << endl;
	sRes << this->msSettingsReport << endl;

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
				"RETURN CODE = " << ret << " - see lp_solve doc!\n" << endl;
	}

	//log the number of solutions
	sRes << "Number of solutions: " << this->mLp->GetSolutionCount() << endl << endl;

	//log the objective function
	sRes << "Objective function result = " << this->mLp->GetObjective() << endl << endl;

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
					<< oit.value().at(1) << " " << oit.value().at(2) << " )" << endl;
		}
		sRes << endl;
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
				zonespec = ait.value().at(0).split(tr(":"), QString::SkipEmptyParts);
                options = zonespec.at(0).split(tr("+"), QString::SkipEmptyParts);
				//nit = 2;
			}
            else
            {
                options << ait.value().at(0).split(tr("+"), QString::SkipEmptyParts);
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
                        << ait.value().at(1) << " " << maxval << " )" << endl;
				}
				++totalcount;
			}
		}
		sRes << endl;
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
					sRes << sCL << " " << compOp << " " << pdCons[index -1] << endl;
				}
				else
				{
					QString tmplab = crilabit.key() + QString(tr("_%1")).arg(criit.key());
					sRes << "error reading cirterion constraint for '"
							<< tmplab << "'!" << endl;
				}
			}
		}
		sRes << endl;
	}

	// DEBUG - we just dump all constraints values here
//    MosraLogDebug( << endl << "just dumping all constraint values .... " << endl);
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
//        MosraLogDebug(<< name.toStdString() << " " << op << " " << fv << endl);
//	}
    //NMDebug(<< endl);


	sRes << endl;

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
                        pDuals[r] << platz << pDualsFrom[r] << platz << pDualsTill[r] << endl;
            }
		}
	}


	this->msReport.clear();
	this->msReport = sRes.readAll();

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

            incstr << endl << endl;
            incstr << "Let's get a feeling for those reduction variables (r_i_r_q) ...." << endl;
            incstr << "     with: i: feature index, r: land use index, q: incentive index" << endl << endl;

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
                incstr << endl;
            }
            incstr << endl << endl;
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
#ifdef LUMASS_DEBUG
    QFileInfo fifo(fileName);
    QString brfn = QString("%1/%2_%3.txt").arg(fifo.path()).arg(fifo.baseName()).arg("BL_reduction");
    this->writeBaselineReductions(brfn);
#endif
}

int NMMosra::checkSettings(void)
{
	NMDebugCtx(ctxNMMosra, << "...");

	QString settrep;
	QTextStream sstr(&settrep);

	sstr << "type of DV (0=REAL | 1=INT | 2=BINARY): " << this->meDVType << endl;

	// get the attributes of the layer
    MosraLogInfo(<< "Optimisation - Checking settings ...")

	//  get the total area of the layer (summing the provided field's data)
	if (this->msAreaField.isEmpty())
	{
        MosraLogError( << "no area field specified!");
		NMDebugCtx(ctxNMMosra, << "done!");
		return 0;
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
    //MosraLogInfo(<< "calculating area and counting features ..." << endl);
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
                std::string zoneVal = mDataSet->getStrValue(zonesIt.key(), cs).toStdString();
                if (zoneVal.find(optIt.key().toStdString()) != std::string::npos)
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
			<< "(" << numTuples << " | " << numFeat << ")" << " is " << this->mdAreaTotal << endl;


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
					"' = " << optIt.value() << endl;
			sstr << "no of features for option '" << optLenIt.key() << "' with respect to zone field '"
					<< zonesLenIt.key() << "' = " << optLenIt.value() << endl;
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
			QStringList ozlist = OptZone.split(tr(":"), QString::SkipEmptyParts);
            QStringList options = ozlist.at(0).split(tr("+"), QString::SkipEmptyParts);
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
					zonespec = criit.key().split(tr(":"), QString::SkipEmptyParts);
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
            << this->mlNumOptFeat << endl;

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
            //<< " + " << this->mlNumOptFeat << endl;

	// number of columns of the decision matrix
	this->mlLpCols = this->mlNumDVar + 1;
	sstr << "mlLpCols = mlNumDvar + 1  = " << this->mlLpCols << endl;

	// Scalarisation method
	QString sMeth;
	if (this->meScalMeth == NMMosra::NM_MOSO_WSUM)
		sMeth = tr("Weighted Sum");
	else
		sMeth = tr("Interactive");
	sstr << "Scalarisation Method: " << sMeth << endl << endl;

	this->msSettingsReport = sstr.readAll();

    MosraLogDebug(<< "Optimisation Settings Report ...\n"
               << this->msSettingsReport.toStdString() << endl);

	NMDebugCtx(ctxNMMosra, << "done!");
    if (!criValid || !criConsValid || !evalValid || !arealCriValid || !incentivesValid)
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
    const int iOffset = this->miNumOptions;

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

    QStringList metaList = criterion.split(",", QString::SkipEmptyParts);

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
            const QStringList incDetails = incSpec.split(":", QString::SkipEmptyParts);

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
            QStringList splitCriterion = isolatedCriterion.split(":", QString::SkipEmptyParts);
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
                    QStringList zoneRes = mDataSet->getStrValue(resField, f).split(" ", QString::SkipEmptyParts);
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

        QStringList options = it.value().at(0).split(tr("+"), QString::SkipEmptyParts);
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
			optzones = it.value().at(0).split(tr(":"), QString::SkipEmptyParts);
            options = optzones.at(0).split(tr("+"), QString::SkipEmptyParts);
			zone = optzones.at(1);
		}
		else
		{
            options = it.value().at(0).split(tr("+"), QString::SkipEmptyParts);
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
                    std::string zoneArVal = mDataSet->getStrValue(vsZoneField.at(r), f).toStdString();

                    if (zoneArVal.find(this->mslOptions.at(vvnOptionIndex.at(r).at(no)).toStdString())
                              != std::string::npos)
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
				zonespec = criit.key().split(tr(":"), QString::SkipEmptyParts);
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
                    const std::string zoneArVal = mDataSet->getStrValue(vZones.at(labelidx), f).toStdString();
                    if (zoneArVal.find(this->mslOptions.at(optIdx).toStdString()) == std::string::npos)
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
		QStringList optResList = optResource.split(tr(" "), QString::SkipEmptyParts);
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
//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
//		else
			dtval = this->mdAreaTotal * dUserVal/100.0;
	}
	else if (otype.compare(tr("percent_of_selected"), Qt::CaseInsensitive) == 0)
	{

//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(this->mdAreaSelected * dUserVal/100.0);
//		else
			dtval = this->mdAreaSelected * dUserVal/100.0;
	}
	else if (otype.compare(tr("percent_of_zone"), Qt::CaseInsensitive) == 0)
	{
		// if there is no zone spec given, we interpret it as percent_of_total
		if (zoneSpec.size() == 0)
		{
//			if (this->meDVType == NMMosra::NM_MOSO_INT)
//				dtval = vtkMath::Floor(this->mdAreaTotal * dUserVal/100.0);
//			else
				dtval = this->mdAreaTotal * dUserVal/100.0;

			// todo: issue a warning message, that a percent_of_total is used instead
		}
		else
		{
			double zonearea = this->mmslZoneAreas.find(zoneSpec.at(1)).value().find(zoneSpec.at(0)).value();
//			if (this->meDVType == NMMosra::NM_MOSO_INT)
//				dtval = vtkMath::Floor(zonearea * dUserVal/100.0);
//			else
				dtval = zonearea * dUserVal/100.0;
		}
	}
	else
	{ // -> "map_units"
//		if (this->meDVType == NMMosra::NM_MOSO_INT)
//			dtval = vtkMath::Floor(dUserVal);
//		else
			dtval = dUserVal;
	}

	return dtval;
}
