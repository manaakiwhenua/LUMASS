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
 *  NMTableReader.cpp
 *
 *  Created on: 2016-06-28
 *      Author: Alexander Herzig
 */

#include "itkProcessObject.h"
#include "otbImage.h"

#include "NMTableReader.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMItkDataObjectWrapper.h"


#include "otbSQLiteTable.h"
#include "otbNMTableReader.h"


NMTableReader
::NMTableReader(QObject* parent)
{
	this->setParent(parent);	this->setObjectName("NMTableReader");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMTableReader
::~NMTableReader()
{
}

void
NMTableReader::instantiateObject()
{
    otb::NMTableReader::Pointer tr = otb::NMTableReader::New();
    mOtbProcess = tr;
    this->mbIsInitialised = true;
}

void
NMTableReader::linkParameters(unsigned int step, const QMap<QString, NMModelComponent *> &repo)
{
    otb::NMTableReader* f = static_cast<otb::NMTableReader*>(mOtbProcess.GetPointer());

    QVariant curFileNameVar = getParameter("FileName");
    std::string curFileName;
    if (curFileNameVar.isValid())
    {
       curFileName = curFileNameVar.toString().toStdString();
        f->SetFileName(curFileName);
    }

    QVariant curTableNameVar = getParameter("TableName");
    std::string curTableName;
    if (curTableNameVar.isValid())
    {
       curTableName = curTableNameVar.toString().toStdString();
        f->SetTableName(curTableName);
    }
}

QSharedPointer<NMItkDataObjectWrapper>
NMTableReader::getOutput(unsigned int idx)
{
    if (!this->mbIsInitialised)
    {
        NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
        QString hostName = "";
        if (this->parent() != 0)
            hostName = this->parent()->objectName();
        QString msg = QString::fromLatin1("%1: NMTableReader::getOutput(%2) failed - Object not initialised!")
                .arg(hostName).arg(idx);
        e.setMsg(msg.toStdString());
        throw e;
    }

    otb::NMTableReader* reader = static_cast<otb::NMTableReader*>(mOtbProcess.GetPointer());
    otb::AttributeTable::Pointer tab = static_cast<otb::AttributeTable*>(reader->GetOutput());


    QSharedPointer<NMItkDataObjectWrapper> dw(new NMItkDataObjectWrapper());
    dw->setOTBTab(tab);

    return dw;
}




