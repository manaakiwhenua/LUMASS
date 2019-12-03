/*****************************t*************************************************
 * Created by Alexander Herzige
 * 2016 Landcare Research New Zealand Ltd
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
 * NMParameterTable.cpp
 *
 *  Created on: 04/02/2016
 *      Author: alex
 */
#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include <string>
#include <sstream>

#include <QVariant>
#include <QFileInfo>

#include "NMParameterTable.h"
#include "NMDataComponent.h"
#include "NMMfwException.h"

#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

const std::string NMParameterTable::ctx = "NMParameterTable";

NMParameterTable::NMParameterTable(QObject* parent)
    : NMDataComponent(parent)
{
    this->setObjectName("ParameterTable");
}

NMParameterTable::~NMParameterTable()
{
    this->mFileName.clear();
    NMDataComponent::reset();
}

void
NMParameterTable::setFileName(QString fn)
{
    if (mFileName.compare(fn, Qt::CaseInsensitive) == 0)
    {
        return;
    }

    this->mFileName = fn;

    if (mFileName.isEmpty())
    {
        NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
        me.setSource(this->objectName().toStdString());
        QString msg = QString("Invalid FileName '%1'!")
                .arg(mFileName);
        me.setDescription(msg.toStdString());
        NMDebugCtx(ctx, << "done!");
        throw me;
        return;
    }

    if (!mDataWrapper.isNull() && mDataWrapper->getOTBTab().IsNotNull())
    {
        otb::SQLiteTable* theTab = static_cast<otb::SQLiteTable*>(mDataWrapper->getOTBTab().GetPointer());
        if (theTab != 0)
        {
            if (    mFileName.compare(QString(theTab->GetDbFileName().c_str()), Qt::CaseInsensitive) == 0
                &&  mTableName.compare(QString(theTab->GetTableName().c_str()), Qt::CaseInsensitive) == 0
               )
            {
                NMDebugAI(<< "No need to update, still pointing to the same data base table!" << std::endl);
                NMDebugCtx(ctx, << "done!");
                return;
            }
        }
    }

    QStringList dbFormats;
    dbFormats << "ldb" << "db" << "sqlite";
    QFileInfo finfo(mFileName);

    otb::SQLiteTable::Pointer tab = otb::SQLiteTable::New();
    tab->SetUseSharedCache(false);
    if (!dbFormats.contains(finfo.suffix()))
    {
        if (!tab->CreateFromVirtual(this->mFileName.toStdString()))
        {
            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
            me.setSource(this->objectName().toStdString());
            QString msg = QString("%1 failed opening '%2'!").arg(this->objectName())
                    .arg(mFileName);
            me.setDescription(msg.toStdString());
            NMDebugCtx(ctx, << "done!");
            throw me;
            return;
        }
    }
    else
    {
        if (mTableName.isEmpty())
        {
            std::vector<std::string> fninfo = tab->GetFilenameInfo(mFileName.toStdString());
            if (fninfo.size() >= 2)
            {
                this->setTableName(fninfo.at(1).c_str());
            }
        }

        // here's how it goes:
        // 1: set the db file name
        // 2: open a connection
        // 3: set the table name
        // 4: populate the table admin data
        tab->SetDbFileName(mFileName.toStdString());
        if (!tab->openConnection())
        {
            QString msg = QString("%1 failed opening '%2'!").arg(this->objectName())
                    .arg(mFileName);
            NMLogError(<< ctx << ": " << msg.toStdString());
            NMDebugCtx(ctx, << "done!");
            mFileName.clear();
            return;
        }

        if (    !tab->SetTableName(mTableName.toStdString())
            ||  !tab->PopulateTableAdmin()
           )
        {
            QString msg = QString("Failed populating table '%1' of '%2'!").arg(mTableName)
                    .arg(mFileName);
            mTableName.clear();
            NMLogError(<< ctx << ": " << msg.toStdString());
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }

    //    this->setUserID(tab->GetTableName().c_str());
    //    this->setDescription(tab->GetTableName().c_str());

    QSharedPointer<NMItkDataObjectWrapper> wrapper(new NMItkDataObjectWrapper(this));
    otb::AttributeTable::Pointer stab = tab.GetPointer();
    wrapper->setOTBTab(stab);
    this->setNthInput(0, wrapper);


    this->mSourceMTime.setMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch());


    emit NMDataComponentChanged();

}


//QVariant
//NMParameterTable::getModelParameter(const QString &paramSpec)
//{
//    QVariant param;

//    if (    paramSpec.isEmpty()
//        ||  this->mDataWrapper->getOTBTab().IsNull()
//       )
//    {
//        return param;
//    }

//    //  <columnName>:<rowNumber>
//    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
//    long long row = 0;
//    if (specList.size() == 2)
//    {
//        bool bok;
//        row = specList.at(1).toLongLong(&bok);
//        if (!bok)
//        {
//            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
//            QString msg = QString("Specified row number '%1' is invalid!")
//                    .arg(this->objectName()).arg(specList.at(1));
//            me.setDescription(msg.toStdString());
//            throw me;
//            return param;
//        }
//    }

//    otb::AttributeTable::Pointer tab = this->mDataWrapper->getOTBTab();

//    int colidx = tab->ColumnExists(specList.at(0).toStdString().c_str());
//    if (colidx > 0)
//    {
//        const otb::AttributeTable::TableColumnType type = tab->GetColumnType(colidx);
//        switch (type)
//        {
//        case otb::AttributeTable::ATTYPE_STRING:
//            {
//                QString strVal = tab->GetStrValue(colidx, row).c_str();
//                param = QVariant::fromValue(strVal);
//            }
//            break;
//        case otb::AttributeTable::ATTYPE_INT:
//            param = QVariant::fromValue(tab->GetIntValue(colidx, row));
//            break;
//        case otb::AttributeTable::ATTYPE_DOUBLE:
//            param = QVariant::fromValue(tab->GetDblValue(colidx, row));
//            break;
//        }
//    }

//    return param;
//}

void
NMParameterTable::linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    this->processUserID();

    // we're self sufficienct - no need to link anything else
}

void
NMParameterTable::update(const QMap<QString, NMModelComponent*>& repo)
{
    this->setFileName(mFileName);
}

void
NMParameterTable::reset(void)
{
}











