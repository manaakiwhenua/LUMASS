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

#include "nmlog.h"

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <sys/stat.h>
#endif

#include "itkProcessObject.h"

#include "otbNMTableReader.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

namespace otb {

NMTableReader::NMTableReader()
    : m_FileName(""), m_TableName(""), m_CreateTable(false), m_InMemoryDb(false)
{
    otb::SQLiteTable::Pointer output = otb::SQLiteTable::New();
    itk::ProcessObject::AddOutput(output.GetPointer());
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

    // ----------------------------------------------------------
    // check, whether the given file exists or not
    bool fileExists = true;
    std::stringstream fileErrorMsg;

#ifdef _WIN32

    int wcharSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                       m_FileName.c_str(), -1,
                                       NULL, 0);

    LPWSTR utf16FN = new WCHAR[wcharSize];

    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        m_FileName.c_str(), -1,
                        utf16FN, wcharSize);

    DWORD attr = GetFileAttributesW(utf16FN);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );

        fileErrorMsg << "File '" << m_FileName << "' does not exist! "
                   << (const char*)lpMsgBuf;

        fileExists = false;
    }
    delete[] utf16FN;

#else

    struct stat attr;
    if (stat(m_FileName.c_str(), &attr) != 0)
    {
        perror(m_FileName.c_str());
        fileErrorMsg << "File '" << m_FileName << "' does not exist!";
        fileExists = false;
    }
    else
    {
        if (!S_ISREG(attr.st_mode))
        {
            fileErrorMsg << "'" << m_FileName << "' is not a file!";
            fileExists = false;
        }
    }

#endif

    // -----------------------------------------------------------
    // data structures and vars for provenance information tracking

    // provenance information
    std::stringstream sstr;

    // create table entity
    std::vector<std::string> args;
    std::vector<std::string> attrs;


    // ----------------------------------------------------------
    // if table not exists and we're supposed to create one,
    // we do just that
    if (!fileExists)
    {
        if (m_CreateTable)
        {
            if (tab->CreateTable(m_FileName) == otb::SQLiteTable::ATCREATE_ERROR)
            {
                NMProcErr( << "Failed creating table '" << m_FileName << "'!");
                return;
            }

            sstr << "db:" << this->GetTableName();
            args.push_back(sstr.str());

            NMProcProvN(itk::NMLogEvent::NM_PROV_GENERATION, args, attrs);

            sstr.str("");
            sstr << "db:DbFileName=\"" << tab->GetDbFileName() << "\"";
            attrs.push_back(sstr.str());

            sstr.str("");
            sstr << "db:TableName=\"" << tab->GetTableName() << "\"";
            attrs.push_back(sstr.str());

            NMProcProvN(itk::NMLogEvent::NM_PROV_ENTITY, args, attrs);

            return;
        }
        else
        {
            NMProcErr(<< fileErrorMsg.str());
            return;
        }
    }


    if (!m_RowIdColname.empty())
    {
        tab->SetRowIDColName(m_RowIdColname);
        tab->SetRowIdColNameIsPersistent(true);
    }

    if (m_TableName.empty())
    {
        if (m_InMemoryDb)
        {
            if (!tab->openAsInMemDb(m_FileName, m_TableName))
            {
                NMProcErr(<< "Failed opening table '"
                                  << m_FileName << "' as InMemoryDb!")
                return;
            }
        }
        else
        {
            if (!tab->CreateFromVirtual(m_FileName))
            {
                NMProcErr(<< "Failed reading table '"
                                  << m_FileName << "'! Double check FileName!")
                return;
            }
            if (tab->getLastLogMsg().find("Replaced") != std::string::npos)
            {
                NMProcInfo(<< tab->getLastLogMsg());
            }
        }
    }
    else
    {
        tab->SetDbFileName(m_FileName);
        if (!tab->openConnection())
        {
            NMProcErr(<<"Couldn't establish connection "
                              << " to '" << m_FileName << "'!");
            return;
        }

        tab->SetTableName(m_TableName);
        if (!tab->PopulateTableAdmin())
        {
            NMProcErr(<<"Failed reading table data! Double check FileName and TableName!");
            return;
        }
    }

    // ------------------------------------------------------------------------------------
    // format provenance information for
    // usage (i.e. just reading of tables)
    sstr << "db:" << tab->GetTableName();
    args.push_back(sstr.str());

    sstr.str("");
    sstr << "db:DbFileName=\"" << tab->GetDbFileName() << "\"";
    attrs.push_back(sstr.str());

    sstr.str("");
    sstr << "db:TableName=\"" << tab->GetTableName() << "\"";
    attrs.push_back(sstr.str());

    NMProcProvN(itk::NMLogEvent::NM_PROV_ENTITY, args, attrs);

    // provn used by
    args.clear();
    attrs.clear();

    sstr.str("");
    sstr << "db:" << tab->GetTableName();
    args.push_back(sstr.str());

    NMProcProvN(itk::NMLogEvent::NM_PROV_USAGE, args, attrs);
}

itk::DataObject*
NMTableReader::GetOutput(unsigned int idx)
{
    return static_cast<otb::SQLiteTable*>(this->ProcessObject::GetOutput(0));
}


}   // namespace


#endif // include guard
