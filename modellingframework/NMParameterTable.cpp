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
#include "nmlog.h"
#include <string>
#include <sstream>

#include "NMParameterTable.h"
#include "NMDataComponent.h"
#include "NMMfwException.h"

#include "otbAttributeTable.h"

const std::string NMParameterTable::ctx = "NMParameterTable";

NMParameterTable::NMParameterTable(QObject* parent)
    : NMDataComponent(parent)
{

}

NMParameterTable::~NMParameterTable()
{

}


void
NMParameterTable::setNthInput(unsigned int idx,
                              QSharedPointer<NMItkDataObjectWrapper> inputTable)
{
//    if (!mDataWrapper.isNull())
//    {
//        mDataWrapper.clear();
//    }

//    if (mDataWrapper->getOTBTab().IsNotNull())
//    {
//        mDataWrapper.clear();
//    }

//    if (inputTable->getOTBTab().IsNotNull())
//    {
//        mDataWrapper = inputTable;
//    }

//    emit NMDataComponentChanged;
}

QVariant
NMParameterTable::getModelParameter(const QString &paramSpec)
{
    QVariant param;

    if (    paramSpec.isEmpty()
        ||  this->mDataWrapper->getOTBTab().IsNull()
       )
    {
        return param;
    }

    //  <columnName>:<rowNumber>
    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
    long long row = 0;
    if (specList.size() == 2)
    {
        bool bok;
        row = specList.at(1).toLongLong(&bok);
        if (!bok)
        {
            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
            QString msg = QString("Specified row number '%1' is invalid!")
                    .arg(this->objectName()).arg(specList.at(1));
            me.setMsg(msg.toStdString());
            throw me;
            return param;
        }
    }

    otb::AttributeTable::Pointer tab = this->mDataWrapper->getOTBTab();

    int colidx = tab->ColumnExists(specList.at(0).toStdString().c_str());
    if (colidx > 0)
    {
        const otb::AttributeTable::TableColumnType type = tab->GetColumnType(colidx);
        switch (type)
        {
        case otb::AttributeTable::ATTYPE_STRING:
            param = QVariant::fromValue(tab->GetStrValue(colidx, row));
            break;
        case otb::AttributeTable::ATTYPE_INT:
            param = QVariant::fromValue(tab->GetIntValue(colidx, row));
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            param = QVariant::fromValue(tab->GetDblValue(colidx, row));
            break;
        }
    }

    return param;
}

void
NMParameterTable::update(const QMap<QString, NMModelComponent*>& repo)
{

}

void
NMParameterTable::reset(void)
{

}











