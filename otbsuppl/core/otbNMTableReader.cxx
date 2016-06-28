/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This programs distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
/*
*  NMTableReader
*
*  Created on: 27/06/2016
*      Author: alex
*/


#ifndef otbNMTableReader_CXX_
#define otbNMTableReader_CXX_

#include "itkProcessObject.h"

#include "otbNMTableReader.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

namespace otb {

NMTableReader::NMTableReader()
    : m_FileName(""), m_TableName("")
{
}

NMTableReader::~NMTableReader()
{
}


itk::ProcessObject::DataObjectPointer
NMTableReader::MakeOutput(DataObjectPointerArraySizeType idx)
{
    return static_cast<itk::DataObject*>(SQLiteTable::New().GetPointer());
}

void
NMTableReader::GenerateData()
{
    SQLiteTable::Pointer tab = static_cast<SQLiteTable*>(this->GetOutput(0));

    if (m_FileName.empty())
    {
        itkExceptionMacro("No filename specified!")
        return;
    }

    if (m_TableName.empty())
    {
        if (!tab->CreateFromVirtual(m_FileName))
        {
            itkExceptionMacro("Failed reading table '"
                              << m_FileName << "'!")
            return;
        }
    }
    else
    {
        tab->SetDbFileName(m_FileName);
        if (!tab->openConnection())
        {
            itkExceptionMacro("Couldn't establish connection "
                              << " to '" << m_FileName << "'!");
            return;
        }

        tab->SetTableName(m_TableName);
        if (!tab->PopulateTableAdmin())
        {
            itkExceptionMacro("Failed reading table data!");
            return;
        }
    }
}

itk::DataObject*
NMTableReader::GetOutput(unsigned int idx)
{
    return static_cast<otb::SQLiteTable*>(this->ProcessObject::GetOutput(0));
}


}   // namespace


#endif // include guard
