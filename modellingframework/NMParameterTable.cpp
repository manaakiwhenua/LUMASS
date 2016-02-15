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

void
NMParameterTable::update(const QMap<QString, NMModelComponent*>& repo)
{

}

void
NMParameterTable::reset(void)
{

}











