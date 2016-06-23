/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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
 * NMTableReader.h
 *
 *  Created on: 2016-06-23
 *  Author: Alexander Herzig
 */

#include <QVariant>
#include <QSharedPointer>

#include "nmlog.h"
#include "NMMfwException.h"
#include "NMTableReader.h"
#include "otbSQLiteTable.h"


const std::string NMTableReader::ctx = "NMTableReader";
NMTableReader::NMTableReader(QObject* parent)
    : mCurFileName(""), mCurTableName(""),
      mOldFileName("")
{
    this->setParent(parent);
    this->setObjectName(QString::fromLatin1("NMTableReader"));
}

NMTableReader::~NMTableReader()
{
}

void
NMTableReader::instantiateObject(void)
{
    this->mbIsInitialised = true;
}

void
NMTableReader::setNthInput(unsigned int numInput,
          QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
}

QSharedPointer<NMItkDataObjectWrapper>
NMExternalExecWrapper::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> dw(new NMItkDataObjectWrapper());
    dw->setOTBTab(mTable);
    return dw;
}


void
NMTableReader::update(void)
{
    NMDebugCtx(ctx, << "...");

    QString objName = this->parent()->objectName();
    emit signalExecutionStarted(objName);

    // =====================================================
    //              do some table magic
    // =====================================================
    // currently, we're just working with
    // otb::SQLiteTables

    otb::SQLiteTable::Pointer sqltab;
    bool success = false;

    // if we've got already a table object and the paramters
    // haven't changed, we just try to repopulate ...
    if (mTable.IsNotNull())
    {
        sqltab = static_cast<otb::SQLiteTable*>(mTable.GetPointer());

        // check, whether anything has changed
        if (    mCurFileName.compare(mOldFileName, Qt::CaseSensitive) == 0
            &&  mCurTableName.compare(sqltab->GetTableName().c_str(), Qt::CaseInsensitive) == 0
           )
        {
            // if not, we just repopulate the table admin, just to make sure ...
            if (sqltab->PopulateTableAdmin())
            {
                success = true;
            }
        }
    }
    // ... however, if that's not the case, we just create a new object
    //     and read again
    else
    {
        sqltab = otb::SQLiteTable::New();
        if (mCurTableName.isEmpty())
        {
            if (sqltab->CreateFromVirtual(mCurFileName.toStdString()))
            {
                success = true;
            }
        }
        else
        {
            sqltab->SetDbFileName(mCurFileName);
            if (sqltab->openConnection())
            {
                sqltab->SetTableName(mCurTableName);
                if (sqltab->PopulateTableAdmin())
                {
                    success = true;
                }
            }
        }
    }

    // in case something didn't go to plan, throw an exception
    if (!success)
    {
        NMMfwException exc(NMMfwException::NMProcess_ExecutionError);

        std::stringstream sstr;
        sstr << this->objectName().toStdString() << " failed reading ";
        if (!mCurTableName.isEmpty())
        {
            sstr << "'" << mCurTableName.toStdString() << "'";
        }
        else
        {
            sstr << "table data";
        }
        sstr << " from '" << mCurFileName.toStdString() << "'!";

        exc.setMsg(sstr.str());
        mTable = 0;
        NMDebugCtx(ctx, << "done!");
        throw exc;
    }

    mTable = sqltab;

    emit signalExecutionStopped(objName);

    this->mbLinked = false;
    NMDebugCtx(ctx, << "done!");
}


void
NMTableReader::linkParameters(unsigned int step,
                    const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");

    QVariant qvFileName = this->getParameter(QString::fromLatin1("FileNames"));
    if (qvFileName.type() != QVariant::String)
    {
        NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
        std::stringstream msg;
        msg << "'" << this->objectName().toStdString() << "'";
        e.setMsg(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }
    else
    {
        mOldFileName = mCurFileName;
        mCurFileName = qvFileName.toString();
    }

    QVariant qvTableName = this->getParameter(QString::fromLatin1("TableNames"));
    if (qvTableName.type() != QVariant::String)
    {
        NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
        std::stringstream msg;
        msg << "'" << this->objectName().toStdString() << "'";
        e.setMsg(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }
    else
    {
        mCurTableName = qvTableName.toString();
    }

    NMDebugCtx(ctx, << "done!")
}


