 /******************************************************************************
 * Created by Alexander Herzig is-
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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
 * SQLiteTable.cxx
 *
 *  Created on: 23/09/2015
 *      Author: Alexander Herzig
 */
#include "nmlog.h"
//#include "itkNMLogEvent.h"
#define _ctxotbtab "SQLiteTable"
#include "otbSQLiteTable.h"
#include <limits>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <locale>
#include <algorithm>
//#include "otbMacro.h"
#include <spatialite.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
    #include "libgen.h"
    #include <unistd.h>
    //#define NM_SPATIALITE_LIB "libspatialite"
#else
    #include <stdlib.h>
    //#define NM_SPATIALITE_LIB "spatialite"
#endif

namespace otb
{

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ---------------------  PUBLIC GETTER and SETTER functions to manage the Attribute table
//int SQLiteTable::GetNumCols()
//{
//	return this->m_vNames.size();
//}
SQLiteTable::Pointer
SQLiteTable::New()
{
    SQLiteTable::Pointer smartPtr = new SQLiteTable();
    smartPtr->UnRegister();
    return smartPtr;
}

long long SQLiteTable::GetNumRows()
{
    if (m_db == 0)
    {
        return 0;
    }

    // note: we're currently only really supporting one table,
    // although, we're actually hosting a whole db, rather than
    // just a table; anyway, so we're safe to give it away
    // since it is reset once we close the table (db)
    if (m_iNumRows > 0)
    {
        return m_iNumRows;
    }

    if (sqlite3_step(m_StmtRowCount) == SQLITE_ROW)
    {
        m_iNumRows = sqlite3_column_int64(m_StmtRowCount, 0);
    }
    sqlite3_reset(m_StmtRowCount);

    return m_iNumRows;
}

bool
SQLiteTable::sqliteError(const int& rc, sqlite3_stmt** stmt)
{
    //this->DebugOn();

    if (rc != SQLITE_OK)
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        //NMErr(_ctxotbtab, << "SQLite3 ERROR #" << rc << ": " << errmsg);
        std::stringstream errstr;
        errstr << "SQLite3 ERROR #" << rc << ": " << errmsg;
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        if (stmt != 0 && *stmt != 0)
        {
            sqlite3_clear_bindings(*stmt);
            sqlite3_reset(*stmt);
        }
        return true;
    }
    return false;
}

//int
//SQLiteTable::ColumnExists(const std::string& sColName)
//{
//    // returns -1 if column doesn't exist, otherwise 1

//    int idx = -1;
//    //    const char** pszDataType = 0;
//    //    const char** pszCollSeq = 0;
//    //    int* pbNotNull = 0;
//    //    int* pbPrimaryKey = 0;
//    //    int* pbAutoinc = 0;

//    //    if (m_db == 0)
//    //    {
//    //        return idx;
//    //    }

//    //    int rc = ::sqlite3_table_column_metadata(
//    //                m_db,
//    //                "main",
//    //                m_tableName,
//    //                sColName.c_str(),
//    //                pszDataType,
//    //                pszCollSeq,
//    //                pbNotNull,
//    //                pbPrimaryKey,
//    //                pbAutoinc);

//    //    if (rc == SQLITE_OK)
//    //    {
//    //        return 1;
//    //    }
//    //    else
//    //    {
//    //        return -1;
//    //    }

//    //    itkDebugMacro(<< std::endl
//    //                  << "column: " << sColName << " | "
//    //                  << "data type: " << pszDataType);


//    // we do store column availability information for backward compatility
//    // with the old vector-based interface (to support the idx-based
//    // column specification in Get/Set functions), so why don't we use it
//    // then?
//    // =================================================================
//    // old implementation
//    // =================================================================
//    for (int c=0; c < m_vNames.size(); ++c)
//    {
//        if (::strcmp(m_vNames[c].c_str(), sColName.c_str()) == 0)
//        {
//            idx = c;
//            break;
//        }
//    }
//	return idx;
//}

bool
SQLiteTable::AddConstrainedColumn(const std::string& sColName,
                          TableColumnType eType,
                          const std::string &sColConstraint)
{
    //NMDebugCtx(_ctxotbtab, << "...");
    // check, whether the db is valid, and try to create a tmp one
    // if it hasn't been created yet
    if (m_db == 0)
    {
        // create a table if there's none
        if (CreateTable("") == ATCREATE_ERROR)
        {
            //NMDebugCtx(_ctxotbtab, << "done!");
            return false;
        }
    }

    if ((	 eType != ATTYPE_STRING
		 &&  eType != ATTYPE_INT
		 &&  eType != ATTYPE_DOUBLE
		)
		||  this->ColumnExists(sColName) >= 0
	   )
	{
        NMDebugAI( << "Column '" << sColName << "'"
                   << " already exists!" << std::endl);
        //NMDebugCtx(_ctxotbtab, << "done!");
		return false;
	}


    std::string sType;
    switch (eType)
    {
    case ATTYPE_INT:
        sType = "INTEGER";
        break;
    case ATTYPE_DOUBLE:
        sType = "REAL";
        break;
    default:
        sType = "TEXT";
    }

    std::stringstream ssql;
    ssql << "ALTER TABLE main." << "\"" << m_tableName << "\"" << " ADD \"" << sColName << "\" "
         << sType;
    if (!sColConstraint.empty())
    {
        ssql << " " << sColConstraint;
    }
    ssql << ";";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    sqliteError(rc, 0);

    // update admin infos
    this->m_vNames.push_back(sColName);
    this->m_vTypes.push_back(eType);

    this->createPreparedColumnStatements(sColName);

//    // prepare an update statement for this column
//    sqlite3_stmt* stmt_upd;
//    ssql.str("");
//    ssql <<  "UPDATE main." << m_tableName << " SET \"" << sColName << "\" = "
//         <<  "@VAL WHERE " << m_idColName << " = @IDX ;";
//    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
//                            1024, &stmt_upd, 0);
//    sqliteError(rc, &stmt_upd);
//    this->m_vStmtUpdate.push_back(stmt_upd);

//    // prepare a get value statement for this column
//    sqlite3_stmt* stmt_sel;
//    ssql.str("");
//    ssql <<  "SELECT \"" << sColName << "\" from main." << m_tableName << ""
//         <<  " WHERE " << m_idColName << " = @IDX ;";
//    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
//                            1024, &stmt_sel, 0);
//    sqliteError(rc, &stmt_sel);
//    this->m_vStmtSelect.push_back(stmt_sel);

//    // prepare a get rowidx by value statement for this column
//    sqlite3_stmt* stmt_rowidx;
//    ssql.str("");
//    ssql <<  "SELECT " << m_idColName << " from main." << m_tableName << ""
//         <<  " WHERE \"" << sColName << "\" = @IDX ;";
//    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
//                            1024, &stmt_rowidx, 0);
//    sqliteError(rc, &stmt_rowidx);
//    this->m_vStmtGetRowidx.push_back(stmt_rowidx);

    //NMDebugCtx(_ctxotbtab, << "done!");

	return true;
}

void
SQLiteTable::createPreparedColumnStatements(const std::string& colname)
{
    std::string sColName = colname;

    // prepare an update statement for this column
    sqlite3_stmt* stmt_upd;
    std::stringstream ssql;
    ssql <<  "UPDATE main." << "\"" << m_tableName << "\"" << " SET \"" << sColName << "\" = "
         <<  "?1 WHERE " << m_idColName << " = ?2 ;";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_upd, 0);
    sqliteError(rc, &stmt_upd);
    this->m_vStmtUpdate.push_back(stmt_upd);


    // prepare a get value statement for this column
    sqlite3_stmt* stmt_sel;
    ssql.str("");
    ssql <<  "SELECT \"" << sColName << "\" from main." << "\"" << m_tableName << "\"" << ""
         <<  " WHERE " << m_idColName << " = ?1 ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_sel, 0);
    sqliteError(rc, &stmt_sel);
    this->m_vStmtSelect.push_back(stmt_sel);


    // prepare a get rowidx by value statement for this column
    sqlite3_stmt* stmt_rowidx;
    ssql.str("");
    ssql <<  "SELECT " << m_idColName << " from main." << "\"" << m_tableName << "\"" << ""
         <<  " WHERE \"" << sColName << "\" = ?1 ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_rowidx, 0);
    sqliteError(rc, &stmt_rowidx);
    this->m_vStmtGetRowidx.push_back(stmt_rowidx);
}

bool
SQLiteTable::isRowidColumn(const std::string &sColName)
{
    std::string colname = sColName;
    std::transform(colname.begin(), colname.end(), colname.begin(), ::tolower);
    bool ret = false;
    if (colname.compare("rowid") == 0)
    {
        ret = true;
    }

    return ret;
}

bool
SQLiteTable::PrepareBulkGet(const std::vector<std::string> &colNames,
                               const std::string& whereClause)
{
    //NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMDebugAI(<< "Database is NULL!" << std::endl);
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // scan where clause for '?' indicating number of
    // where clause parameters, i.e. columns and associated
    // values
    std::string::size_type pos = 0;
    m_iStmtBulkGetNumParam = 0;
    while ((pos = whereClause.find('?', pos)) != std::string::npos)
    {
        ++m_iStmtBulkGetNumParam;
        ++pos;
    }

    m_vTypesBulkGet.clear();
    std::stringstream ssql;
    ssql << "SELECT ";
    for (int c=0; c < colNames.size(); ++c)
    {
        int colidx = this->ColumnExists(colNames[c]);
        if (colidx < 0)
        {
            if (isRowidColumn(colNames[c]))
            {
                m_vTypesBulkGet.push_back(ATTYPE_INT);
            }
            else
            {
                sqlite3_finalize(m_StmtBulkGet);
                m_StmtBulkGet = 0;
                //NMDebugCtx(_ctxotbtab, << "done!");
            return false;
            }
        }
        else
        {
            m_vTypesBulkGet.push_back(this->GetColumnType(colidx));
        }
        ssql << "\"" << colNames.at(c) << "\"";
        if (c < colNames.size()-1)
        {
            ssql << ",";
        }
    }
    ssql << " from main." << "\"" << m_tableName << "\"" << "";

    if (whereClause.empty())
    {
        ssql << ";";
    }
    else
    {

        ssql << " " << whereClause
             << ";";
    }

    // we finalise any bulk get statement, that might have
    // been prepared earlier, but whose execution and hasn't
    // been cleaned up
    sqlite3_finalize(m_StmtBulkGet);


    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &m_StmtBulkGet, 0);
    if (sqliteError(rc, &m_StmtBulkGet))
    {
        sqlite3_finalize(m_StmtBulkGet);
        m_StmtBulkGet = 0;
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    //NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::PrepareBulkSet(const std::vector<std::string>& colNames,
                               const bool& bInsert)
{
    //NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMDebugAI(<< "Database is NULL!" << std::endl);
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    m_vTypesBulkSet.clear();
    for (int i=0; i < colNames.size(); ++i)
    {
        const int idx = this->ColumnExists(colNames.at(i));
        if (idx < 0)
        {
            NMLogWarn(<< "Column \"" << colNames.at(i)
                            << "\" does not exist in the table!");
            NMDebugAI(<< "Column '" << colNames.at(i) << "' not found!" << std::endl);
            //NMDebugCtx(_ctxotbtab, << "done!");
            return false;
        }
        m_vTypesBulkSet.push_back(this->GetColumnType(idx));
    }

    int valueCounter = 1;
    std::stringstream ssql;
    if (bInsert)
    {
        ssql << "INSERT OR REPLACE INTO main." << "\"" << m_tableName << "\"" << " (";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << "\"" << colNames.at(c) << "\"";
            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ") VALUES (";
        for (int c=0; c < colNames.size(); ++c)
        {
            //            if (c < autoValue.size() && !autoValue.at(c).empty())
            //            {
            //                std::string av = autoValue.at(c);
            //                size_t pos = 0;
            //                while ((pos = av.find('?', pos)) != std::string::npos)
            //                {
            //                    std::stringstream v;
            //                    v << valueCounter;

            //                    av = av.insert(pos+1, v.str().c_str());
            //                    ++valueCounter;
            //                    ++pos;
            //                }
            //                ssql << av;
            //            }
            //            else
            {
                ssql << "?" << valueCounter;
                ++valueCounter;
            }

            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ");";
    }
    else
    {
        ssql << "UPDATE main." << "\"" << m_tableName << "\"" << " SET ";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << "\"" << colNames.at(c) << "\" = ";

            //            if (c < autoValue.size() && !autoValue.at(c).empty())
            //            {
            //                std::string av = autoValue.at(c);
            //                size_t pos = 0;//av.find('?');
            //                while ((pos = av.find('?', pos)) != std::string::npos)
            //                {
            //                    std::stringstream v;
            //                    v << valueCounter;

            //                    av = av.insert(pos+1, v.str().c_str());
            //                    ++valueCounter;
            //                    ++pos;
            //                }
            //                ssql << av;
            //            }
            //            else
            {
                 ssql << "?" << valueCounter;
                 ++valueCounter;
            }

            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << " WHERE " << m_idColName << " = ?" << colNames.size()+1
             << " ;";
    }

    // we finalise any bulk set statement, that might have
    // been prepared earlier, but whose execution or binding
    // failed and hasn't been cleaned up
    sqlite3_finalize(m_StmtBulkSet);

    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &m_StmtBulkSet, 0);
    if (sqliteError(rc, &m_StmtBulkSet))
    {
        sqlite3_finalize(m_StmtBulkSet);
        m_StmtBulkSet = 0;
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

   // NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::PrepareAutoBulkSet(const std::vector<std::string>& colNames,
                    const std::vector<std::string> &autoValue,
                    const std::vector<std::vector<SQLiteTable::TableColumnType> >& autoTypes,
                    const bool& bInsert)
{
    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMDebugAI(<< "Database is NULL!" << std::endl);
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    m_vTypesBulkSet.clear();
    for (int i=0; i < colNames.size(); ++i)
    {
        const int idx = this->ColumnExists(colNames.at(i));
        if (idx < 0)
        {
            NMLogWarn(<< "Column \"" << colNames.at(i)
                            << "\" does not exist in the table!");
            NMDebugAI(<< "Column '" << colNames.at(i) << "' not found!" << std::endl);
            NMDebugCtx(_ctxotbtab, << "done!");
            return false;
        }

        if (i < autoValue.size() && autoValue.at(i) != "" && i < autoTypes.size())
        {
            std::string av = autoValue.at(i);
            size_t pos = 0;
            std::vector<SQLiteTable::TableColumnType>::const_iterator typeIt =
                    autoTypes.at(i).begin();
            int cnt = 0;
            while (     (pos = av.find('?', pos)) != std::string::npos
                    &&  typeIt != autoTypes.at(i).end()
                  )
            {
                m_vTypesBulkSet.push_back(*typeIt);

                ++typeIt;
                ++pos;
                ++cnt;
            }
            if (cnt != autoTypes.at(i).size())
            {
                //NMErr(_ctxotbtab, << "Number of provided types doesn't match "
                //                  << "the number of expression values for "
                //                  << "column '" << colNames.at(i) << "'");
                std::stringstream errstr;
                errstr << "Number of provided types doesn't match "
                       << "the number of expression values for "
                       << "column '" << colNames.at(i) << "'";
                m_lastLogMsg = errstr.str();
                //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
                itkExceptionMacro(<< errstr.str());
                m_vTypesBulkSet.clear();
                sqlite3_finalize(m_StmtBulkSet);
                m_StmtBulkSet = 0;
                NMDebugCtx(_ctxotbtab, << "done!");
                return false;
            }

        }
        else
        {
            m_vTypesBulkSet.push_back(this->GetColumnType(idx));
        }
    }

    int valueCounter = 1;
    std::stringstream ssql;
    if (bInsert)
    {
        ssql << "INSERT OR REPLACE INTO main." << "\"" << m_tableName << "\"" << " (";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << "\"" << colNames.at(c) << "\"";
            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ") VALUES (";
        for (int c=0; c < colNames.size(); ++c)
        {
            if (c < autoValue.size() && !autoValue.at(c).empty())
            {
                std::string av = autoValue.at(c);
                size_t pos = 0;
                while ((pos = av.find('?', pos)) != std::string::npos)
                {
                    std::stringstream v;
                    v << valueCounter;

                    av = av.insert(pos+1, v.str().c_str());
                    ++valueCounter;
                    ++pos;
                }
                ssql << av;
            }
            else
            {
                ssql << "?" << valueCounter;
                ++valueCounter;
            }

            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ");";
    }
    else
    {
        ssql << "UPDATE main." << "\"" << m_tableName << "\"" << " SET ";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << "\"" << colNames.at(c) << "\" = ";

            if (c < autoValue.size() && !autoValue.at(c).empty())
            {
                std::string av = autoValue.at(c);
                size_t pos = 0;//av.find('?');
                while ((pos = av.find('?', pos)) != std::string::npos)
                {
                    std::stringstream v;
                    v << valueCounter;

                    av = av.insert(pos+1, v.str().c_str());
                    ++valueCounter;
                    ++pos;
                }
                ssql << av;
            }
            else
            {
                 ssql << "?" << valueCounter;
                 ++valueCounter;
            }

            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << " WHERE " << m_idColName << " = ?" << colNames.size()+1
             << " ;";
    }

    // we finalise any bulk set statement, that might have
    // been prepared earlier, but whose execution or binding
    // failed and hasn't been cleaned up
    sqlite3_finalize(m_StmtBulkSet);

    NMDebugAI(<< "Preparing the expression ..." << std::endl);
    NMDebug(<< ssql.str() << std::endl);

    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &m_StmtBulkSet, 0);
    if (sqliteError(rc, &m_StmtBulkSet))
    {
        sqlite3_finalize(m_StmtBulkSet);
        m_StmtBulkSet = 0;
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}



bool
SQLiteTable::CreateIndex(const std::vector<std::string> &colNames,
                            bool unique, const std::string &table, const std::string &db)
{
    if (m_db == 0)
    {
        return false;
    }

    std::string tableName = table;
    if (table.empty())
    {
        tableName = m_tableName;
    }

    std::string idxName = db;//"main.";
    idxName += ".";
    for (int n=0; n < colNames.size(); ++n)
    {
        if (db.compare("main") == 0 && this->ColumnExists(colNames.at(n)) < 0)
        {
            NMLogWarn(<< "Invalid Column Name!");
            return false;
        }

        idxName += colNames[n];
        if (n < colNames.size()-1)
        {
            idxName += "_";
        }
    }
    idxName += "_idx";

    std::stringstream ssql;
    if (unique)
    {
        ssql << "CREATE UNIQUE";
    }
    else
    {
        ssql << "CREATE";
    }

    ssql << " INDEX IF NOT EXISTS " << idxName
         << " on " << tableName << " (";
    for (int s=0; s < colNames.size(); ++s)
    {
        ssql << colNames.at(s);
        if (s < colNames.size()-1)
        {
            ssql << ", ";
        }
    }
    ssql << ");";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        return false;
    }
    this->m_vIndexNames.push_back(idxName);

    return true;
}

bool
SQLiteTable::DoBulkGet(std::vector< ColumnValue >& values)
{
   //NMDebugCtx(_ctxotbtab, << "...");
   if (    m_db == 0
        ||  m_StmtBulkGet == 0
       )
    {
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // NOTE: the where clause parameters, if any, can optinally be
    // changed at any point, which requires to provide a values
    // vector containing extra 'values' which are then bound to the
    // prepared parameters in the statement; if the values vector's
    // equals the size of the m_vTypesBulkGet vector, either no
    // parameters have been set at all (i.e. m_iStmtBulkGetNumParam == 0)
    // or they won't changed, instead another row from the query result set
    // is fetched from sqlite3
    int rc;
    int parIdx = values.size();
    if (m_iStmtBulkGetNumParam)
    {
        parIdx = values.size() - m_iStmtBulkGetNumParam;
        for (int i=parIdx, si=1; i < values.size(); ++i, ++si)
        {
            switch(values[i].type)
            {
            case ATTYPE_INT:
                {
                    rc = sqlite3_bind_int64(m_StmtBulkGet, si, values[i].ival);
                }
                break;
            case ATTYPE_DOUBLE:
                {
                    rc = sqlite3_bind_int64(m_StmtBulkGet, si, values[i].dval);
                }
                break;

            case ATTYPE_STRING:
                {
                    rc = sqlite3_bind_text(m_StmtBulkGet, si,
                                           values[i].tval,
                                           -1, 0);
                }
                break;
            default:
                {

                    //NMErr(_ctxotbtab, << "UNKNOWN data type!");
                    std::stringstream errstr;
                    errstr << "UNKNOWN data type!";
                    m_lastLogMsg = errstr.str();
                    //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
                    return false;
                }
            }
        }
    }

    rc = sqlite3_step(m_StmtBulkGet);
    if (rc == SQLITE_ROW)
    {
        for (int col=0; col < m_vTypesBulkGet.size(); ++col)
        {
            switch(m_vTypesBulkGet[col])
            {
            case ATTYPE_DOUBLE:
                values[col].type = ATTYPE_DOUBLE;
                values[col].dval = sqlite3_column_double(m_StmtBulkGet, col);
                break;
            case ATTYPE_INT:
                values[col].type = ATTYPE_INT;
                values[col].ival = sqlite3_column_int64(m_StmtBulkGet, col);
                break;
            case ATTYPE_STRING:
                {
                    values[col].type = ATTYPE_STRING;
                    values[col].slen = std::max(sqlite3_column_bytes(m_StmtBulkGet, col), 1) + 1;
                    const char* val = reinterpret_cast<char*>(
                                        const_cast<unsigned char*>(
                                          sqlite3_column_text(m_StmtBulkGet, col)));
                    values[col].tval = new char[values[col].slen];
                    if (val != 0)
                    {
                        ::sprintf((values[col]).tval, "%s", sqlite3_column_text(m_StmtBulkGet, col));
                    }
                    else
                    {
                        ::sprintf(values[col].tval, "%s", "\0");
                    }
                }
                break;
            }
        }
    }
    else
    {
        sqliteStepCheck(rc);
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // only if we've changed the where clause parameters, we
    // have to reset the prepared statement and clear the bindings;
    // otherwise, we just fetch the next row in the next call
    // of this function
    if (parIdx < values.size())
    {
        sqlite3_clear_bindings(m_StmtBulkGet);
        sqlite3_reset(m_StmtBulkGet);
    }
    //NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::GreedyNumericFetch(const std::vector<std::string> &columns,
                                std::map<int, std::map<long, double> > &valstore)
{
    bool ret = false;
    if (m_db == 0)
    {
        return ret;
    }

    std::stringstream ssql;
    ssql << "SELECT "; //<< this->GetPrimaryKey() << ", ";
    std::vector<bool> bStrCol;
    for (int c=0; c < columns.size(); ++c)
    {
        if (this->GetColumnType(this->ColumnExists(columns[c])) == AttributeTable::ATTYPE_STRING)
        {
            bStrCol.push_back(true);
        }
        else
        {
            bStrCol.push_back(false);
        }

        ssql << columns[c];
        if (c < columns.size()-1)
        {
            ssql << ", ";
        }
    }

    ssql << " FROM main." << "\"" << m_tableName << "\"" << ";";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1, &stmt, 0);
    if (sqliteError(rc, &stmt))
    {
        sqlite3_finalize(stmt);
        return false;
    }


    std::map<int, std::map<long, double> >::iterator storeIter;

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        long id = static_cast<long>(sqlite3_column_int64(stmt, 0));

        storeIter = valstore.begin();
        for (int c=1; c < columns.size() && storeIter != valstore.end(); ++c, ++storeIter)
        {
            const double val = bStrCol[c] ? static_cast<double>(id) : sqlite3_column_double(stmt, c);
            storeIter->second.insert(std::pair<long, double>(id, val));
        }
    }
    sqlite3_finalize(stmt);

    return true;
}

bool
SQLiteTable::DoPtrBulkSet(std::vector<int *> &intVals,
                             std::vector<double *> &dblVals,
                             std::vector<char **> &chrVals,
                             std::vector<int> &colpos,
                             const int &chunkrow,
                             const long long int &row)
{
    //NMDebugCtx(_ctxotbtab, << "done!");
    if (    m_db == 0
        ||  m_StmtBulkSet == 0
        ||  colpos.size() != m_vTypesBulkSet.size()
       )
    {
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    int rc;
    for (int i=0; i < colpos.size(); ++i)
    {
        switch(m_vTypesBulkSet.at(i))
        {
        case ATTYPE_DOUBLE:
            {
                //const double val = *(static_cast<double*>(values[i]));
                rc = sqlite3_bind_double(m_StmtBulkSet, i+1,
                                         dblVals[colpos[i]][chunkrow]);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_INT:
            {
                //const int ival = *(static_cast<int*>(values[i]));
                rc = sqlite3_bind_int(m_StmtBulkSet, i+1,
                                      intVals[colpos[i]][chunkrow]);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_STRING:
            {
                //const char* val = values.at(i).c_str();
                rc = sqlite3_bind_text(m_StmtBulkSet, i+1,
                                       chrVals[colpos[i]][chunkrow],
                                       -1, 0);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        default:
            {
                //NMErr(_ctxotbtab, << "UNKNOWN data type!");
                //NMDebugCtx(_ctxotbtab, << "done!");

                std::stringstream errstr;
                errstr << "UNKNOWN data type!";
                m_lastLogMsg = errstr.str();
                //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
                return false;
            }
        }
    }

    if (row >= 0)
    {
        rc = sqlite3_bind_int64(m_StmtBulkSet, colpos.size()+1, row);
        //if (sqliteError(rc, &m_StmtBulkSet)) return false;
    }

    rc = sqlite3_step(m_StmtBulkSet);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(m_StmtBulkSet);
    sqlite3_reset(m_StmtBulkSet);
    //NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::DoBulkSet(std::vector<ColumnValue> &values, const long long int &row)
{
    //NMDebugCtx(_ctxotbtab, << "...");
    if (    m_db == 0
        ||  m_StmtBulkSet == 0
        ||  values.size() != m_vTypesBulkSet.size()
       )
    {
        //        NMErr(_ctxotbtab, << "Number of types doesn't match "
        //                          << "the number of values provided!");
        std::stringstream errstr;
        errstr << "UNKNOWN data type!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        return false;
    }

    int rc;
    for (int i=0; i < values.size(); ++i)
    {
        switch(m_vTypesBulkSet.at(i))
        {
        case ATTYPE_DOUBLE:
            {
                //const double val = *(static_cast<double*>(values[i]));
                rc = sqlite3_bind_double(m_StmtBulkSet, i+1, values[i].dval);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_INT:
            {
                //const int ival = *(static_cast<int*>(values[i]));
                rc = sqlite3_bind_int64(m_StmtBulkSet, i+1, values[i].ival);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_STRING:
            {
                //const char* val = values.at(i).c_str();
                rc = sqlite3_bind_text(m_StmtBulkSet, i+1,
                                       values[i].tval,
                                       -1, 0);
                //if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        default:
            {
            //NMErr(_ctxotbtab, << "UNKNOWN data type!");
            //NMDebugCtx(_ctxotbtab, << "done!");

            std::stringstream errstr;
            errstr << "UNKNOWN data type!";
            m_lastLogMsg = errstr.str();
            //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
            return false;
            }
        }
    }

    if (row >= 0)
    {
        rc = sqlite3_bind_int64(m_StmtBulkSet, values.size()+1, row);
        //if (sqliteError(rc, &m_StmtBulkSet)) return false;
    }
    else
    {
        ++m_iNumRows;
    }

    rc = sqlite3_step(m_StmtBulkSet);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(m_StmtBulkSet);
    sqlite3_reset(m_StmtBulkSet);

    //NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

//bool SQLiteTable::AddRows(long long int numRows)
//{
//    //NMDebugCtx(_ctxotbtab, << "...");
//    // connection open?
//    if (m_db == 0)
//    {
//        //NMDebugCtx(_ctxotbtab, << "done!");
//        return false;
//    }

//	// check for presence of columns
//    if (this->ColumnExists(m_idColName) == -1)
//    {
//        //NMDebugCtx(_ctxotbtab, << "done!");
//		return false;
//    }

//    int rc;
//    int bufSize = 256;
//    std::stringstream ssql;
//    ssql << "INSERT INTO main." << m_tableName
//         << " (" << m_idColName << ") VALUES (@IDX)";

//    char* tail = 0;
//    sqlite3_stmt* stmt;
//    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), bufSize, &stmt, 0);
//    if (sqliteError(rc, 0)) return false;

//    bool bEndTransaction = false;
//    if (sqlite3_get_autocommit(m_db))
//    {
//        if (!this->beginTransaction())
//        {
//            sqlite3_finalize(stmt);
//            //NMDebugCtx(_ctxotbtab, << "done!");
//            return false;
//        }
//        bEndTransaction = true;
//    }

//    for (long long int r=0; r < numRows; ++r, ++m_iNumRows)
//    {
//        sqlite3_bind_int(stmt, 1, m_iNumRows);
//        sqlite3_step(stmt);
//        sqlite3_clear_bindings(stmt);
//        sqlite3_reset(stmt);
//    }

//    if (bEndTransaction)
//    {
//        this->endTransaction();
//    }

//    // clean up the prepared statement
//    sqlite3_finalize(stmt);

//    //	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
//    //	{
//    //		const int& tidx = m_vPosition[colidx];
//    //		switch (this->m_vTypes[colidx])
//    //		{
//    //		case ATTYPE_STRING:
//    //			{
//    //			m_mStringCols.at(tidx)->resize(m_iNumRows+numRows, m_sNodata);
//    //			break;
//    //			}
//    //		case ATTYPE_INT:
//    //			{
//    //			m_mIntCols.at(tidx)->resize(m_iNumRows+numRows, m_iNodata);
//    //			break;
//    //			}
//    //		case ATTYPE_DOUBLE:
//    //			{
//    //			m_mDoubleCols.at(tidx)->resize(m_iNumRows+numRows, m_dNodata);
//    //			break;
//    //			}
//    //		default:
//    //			return false;
//    //		}
//    //	}

//	// increase the row number counter
//    //this->m_iNumRows += numRows;

//    //NMDebugCtx(_ctxotbtab, << "done!");
//	return true;
//}

//bool SQLiteTable::AddRow()
//{
//    //NMDebugCtx(_ctxotbtab, << "...");
//    // connection open?
//    if (m_db == 0)
//    {
//        //NMDebugCtx(_ctxotbtab, << "done!");
//        return false;
//    }

//    // check for presence of columns
//    if (this->ColumnExists(m_idColName) == -1)
//    {
//        //NMDebugCtx(_ctxotbtab, << "done!");
//        return false;
//    }

//    std::stringstream ssql;
//    ssql << "INSERT INTO main." << m_tableName
//         << " (" << m_idColName << ") VALUES ("
//         << m_iNumRows << ");";

//    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
//    if (sqliteError(rc, 0))
//    {
//        //NMDebugCtx(_ctxotbtab, << "done!");
//        return false;
//    }

//    ++m_iNumRows;


////	// check for presence of columns
////	if (this->m_vNames.size() == 0)
////		return false;

////	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
////	{
////		const int& tidx = m_vPosition[colidx];
////		switch (this->m_vTypes[colidx])
////		{
////		case ATTYPE_STRING:
////			{
////			m_mStringCols.at(tidx)->push_back(this->m_sNodata);
////			break;
////			}
////		case ATTYPE_INT:
////			{
////			m_mIntCols.at(tidx)->push_back(this->m_iNodata);
////			break;
////			}
////		case ATTYPE_DOUBLE:
////			{
////			m_mDoubleCols.at(tidx)->push_back(this->m_dNodata);
////			break;
////			}
////		default:
////			return false;
////		}
////	}

////	// increase the row number counter
////	++this->m_iNumRows;

//    //NMDebugCtx(_ctxotbtab, << "done!");
//	return true;
//}

bool
SQLiteTable::BeginTransaction()
{
	//NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMLogWarn(<< "No database connection!");
        //NMWarn(_ctxotbtab, << "No database connection!");
        //NMDebugCtx(_ctxotbtab, << "done!");

        std::stringstream errstr;
        errstr << "No database connection!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_WARN));
        return false;
    }

    if (sqlite3_get_autocommit(m_db) == 0)
    {
        NMLogWarn(<< "Transaction alrady in progress - bail out!");
        //NMWarn(_ctxotbtab, << "Transaction already in progress - bail out!");
        //NMDebugCtx(_ctxotbtab, << "done!");

        std::stringstream errstr;
        errstr << "Transaction already in progress - bail out!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_WARN));
        return true;
    }

    int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        //NMErr(_ctxotbtab, << "Failed starting transaction!");
        //NMDebugCtx(_ctxotbtab, << "done!");

        std::stringstream errstr;
        errstr << "Failed starting transaction!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        return false;
    }
    else
    {
        //NMDebugCtx(_ctxotbtab, << "done!");
        //NMDebugAI(<< "NMSQLiteTable --> started new transaction!" << std::endl);
        std::stringstream errstr;
        errstr << "Started new transaction!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_DEBUG));
        return true;
    }
}

bool
SQLiteTable::EndTransaction()
{
    //NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        //NMWarn(_ctxotbtab, << "No datbase connection!");
        //NMLogWarn(<< "No database connection!");
        // NMDebugCtx(_ctxotbtab, << "done!");
        std::stringstream errstr;
        errstr << "No datbase connection!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_WARN));
        return false;
    }

    if (sqlite3_get_autocommit(m_db))
    {
        //NMLogWarn(<< "Cannot commit, no active transaction!");
        //NMWarn(_ctxotbtab, << "Nothing to commit - no active transaction!");
        //NMDebugCtx(_ctxotbtab, << "done!");
        std::stringstream errstr;
        errstr << "Nothing to commit - no active transaction!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_WARN));
        return false;
    }

    int rc = sqlite3_exec(m_db, "END TRANSACTION;", 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        //NMErr(_ctxotbtab, << "Failed commit!");
        //NMDebugCtx(_ctxotbtab, << "done!");
        std::stringstream errstr;
        errstr << "Commit failed!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        return false;
    }
    else
    {
        //NMDebugCtx(_ctxotbtab, << "done!");
        //NMDebugAI(<< "NMSQLiteTable --> ended current transaction!" << std::endl);
        std::stringstream errstr;
        errstr << "Ended current transaction!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_DEBUG));
        return true;
    }
}

void
SQLiteTable::sqliteStepCheck(const int& rc)
{
    switch(rc)
    {
    case SQLITE_BUSY:
        sqlite3_step(m_StmtRollback);
        sqlite3_reset(m_StmtRollback);
        break;

    case SQLITE_ERROR:
        sqliteError(rc, 0);
        break;

    case SQLITE_DONE:
    default:
        break;
    }
}


void
SQLiteTable::SetValue(const std::string& sColName, long long int idx, double value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    // we just check for the database and leave the rest
    // to sqlite3
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);

    //    int rc = sqlite3_bind_text(m_vStmtUpdate.at(colidx), 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_double(stmt, 1, value);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_bind_int64(stmt, 2, idx);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }


    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

//    NMDebugCtx(_ctxotbtab, << "done!");
}

void
SQLiteTable::SetValue(const std::string& sColName, long long int idx, long long int value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);


    //    int rc = sqlite3_bind_text(m_StmtUpdate, 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_int64(stmt, 1, value);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_bind_int64(stmt, 2, idx);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

//    NMDebugCtx(_ctxotbtab, << "done!");
}

void
SQLiteTable::SetValue(const std::string& sColName, long long int idx, std::string value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);

    //    int rc = sqlite3_bind_text(m_StmtUpdate, 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_text(stmt, 1, value.c_str(), -1, 0);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_bind_int64(stmt, 2, idx);
    if (sqliteError(rc, &stmt))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

//    NMDebugCtx(_ctxotbtab, << "done!");
}

void
SQLiteTable::SetValue(const std::string& sColName, const std::string& whereClause, double value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    // we just check for the database and leave the rest
    // to sqlite3
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    sqlite3_stmt* stmt_upd;
    std::stringstream ssql;
    ssql <<  "UPDATE main." << "\"" << m_tableName << "\"" << " SET \"" << sColName << "\" = "
         <<  value << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_upd, 0);
    if (sqliteError(rc, &stmt_upd))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_step(stmt_upd);
    sqliteStepCheck(rc);

    sqlite3_finalize(stmt_upd);
//    NMDebugCtx(_ctxotbtab, << "done!");
}

void
SQLiteTable::SetValue(const std::string& sColName, const std::string& whereClause, long long int value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    sqlite3_stmt* stmt_upd;
    std::stringstream ssql;
    ssql <<  "UPDATE main." << "\"" << m_tableName << "\"" << " SET \"" << sColName << "\" = "
         <<  value << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_upd, 0);
    if (sqliteError(rc, &stmt_upd))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_step(stmt_upd);
    sqliteStepCheck(rc);

    sqlite3_finalize(stmt_upd);


//    NMDebugCtx(_ctxotbtab, << "done!");
}

void
SQLiteTable::SetValue(const std::string& sColName, const std::string& whereClause, std::string value)
{
//    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    sqlite3_stmt* stmt_upd;
    std::stringstream ssql;
    ssql <<  "UPDATE main." << "\"" << m_tableName << "\"" << " SET \"" << sColName << "\" = "
         <<  value << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_upd, 0);
    if (sqliteError(rc, &stmt_upd))
    {
//        NMDebugCtx(_ctxotbtab, << "done!");
        return;
    }

    rc = sqlite3_step(stmt_upd);
    sqliteStepCheck(rc);

    sqlite3_finalize(stmt_upd);

//    NMDebugCtx(_ctxotbtab, << "done!");
}

bool
SQLiteTable::PrepareColumnByIndex(const std::string &colname)//,
                                      //const std::string &whereClause)
{
    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return 0;
    }

    sqlite3_finalize(m_StmtColIter);

    std::stringstream ssql;
    ssql << "SELECT " << colname
         << " from main." << "\"" << m_tableName << "\""
         << " where " << m_idColName << " = ?1;";

    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &m_StmtColIter, 0);
    if (sqliteError(rc, &m_StmtColIter))
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::PrepareColumnIterator(const std::string &colname,
                                   const std::string &whereClause)
{
    NMDebugCtx(_ctxotbtab, << "...");
    if (m_db == 0)
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    sqlite3_finalize(m_StmtColIter);

    std::stringstream ssql;
    ssql << "SELECT " << colname
         << " from main." << "\"" << m_tableName << "\"";
    if (!whereClause.empty())
    {
         ssql << whereClause;
    }
    ssql << ";";

    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &m_StmtColIter, 0);
    if (sqliteError(rc, &m_StmtColIter))
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

double SQLiteTable::GetDblValue(const std::string& sColName, long long int idx)
{
	//check for valid name and index parameters
	int colidx = this->ColumnExists(sColName);
    if (colidx < 0)
		return m_dNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int64(stmt, 1, idx);
    if (sqliteError(rc, &stmt)) return m_dNodata;

    double ret = m_dNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_double(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;
}

long long
SQLiteTable::GetIntValue(const std::string& sColName, long long idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
    if (colidx < 0)// || idx < 0)// || idx > m_iNumRows)
		return m_iNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int64(stmt, 1, idx);
    if (sqliteError(rc, &stmt)) return m_iNodata;

    long long int ret = m_iNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        //        int ncols = sqlite3_column_count(stmt);
        //        std::string name = sqlite3_column_name(stmt, 0);
        //        NMDebugAI(<< "#cols: " << ncols << std::endl);
        //        NMDebugAI(<< "#name: " << name << std::endl);
        ret = sqlite3_column_int64(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;
}

std::string
SQLiteTable::GetStrValue(const std::string& sColName, long long idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
    if (colidx < 0)// || idx < 0)// || idx > m_iNumRows)
		return m_sNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int64(stmt, 1, idx);
    if (sqliteError(rc, &stmt)) return m_sNodata;

    std::stringstream ret;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char* sval = sqlite3_column_text(stmt, 0);
        if (sval)
        {
            ret << sval;
        }
        else
        {
            ret << m_sNodata;
        }
    }
    else
    {
        ret << m_sNodata;
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret.str();
}

double SQLiteTable::GetDblValue(const std::string& sColName, const std::string& whereClause)
{
    //check for valid name and index parameters
    int colidx = this->ColumnExists(sColName);
    if (colidx < 0 && !isRowidColumn(sColName))
        return m_dNodata;

    sqlite3_stmt* stmt;
    std::stringstream ssql;
    ssql << "SELECT \"" << sColName << "\" from main." << "\"" << m_tableName << "\""
         << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &stmt, 0);
    if (sqliteError(rc, &stmt)) return m_dNodata;

    double ret = m_dNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_double(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_finalize(stmt);

    return ret;
}

long long
SQLiteTable::GetIntValue(const std::string& sColName, const std::string& whereClause)
{
    // check given index and column name
    int colidx = this->ColumnExists(sColName);
    if (colidx < 0 && !isRowidColumn(sColName))
        return m_iNodata;

    sqlite3_stmt* stmt;
    std::stringstream ssql;
    ssql << "SELECT \"" << sColName << "\" from main." << "\"" << m_tableName << "\""
         << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &stmt, 0);

    if (sqliteError(rc, &stmt)) return m_iNodata;

    long long int ret = m_iNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
         ret = sqlite3_column_int64(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_finalize(stmt);

    return ret;
}

std::string
SQLiteTable::GetStrValue(const std::string& sColName, const std::string& whereClause)
{
    // check given index and column name
    int colidx = this->ColumnExists(sColName);
    if (colidx < 0 && !isRowidColumn(sColName))
        return m_sNodata;

    sqlite3_stmt* stmt;
    std::stringstream ssql;
    ssql << "SELECT \"" << sColName << "\" from main." << "\"" << m_tableName << "\""
         << " WHERE " << whereClause << ";";
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1,
                                &stmt, 0);
    if (sqliteError(rc, &stmt)) return m_sNodata;

    std::stringstream ret;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char* sval = sqlite3_column_text(stmt, 0);
        if (sval)
        {
            ret << sval;
        }
        else
        {
            ret << m_sNodata;
        }
    }
    else
    {
        ret << m_sNodata;
        sqliteStepCheck(rc);
    }

    sqlite3_finalize(stmt);

    return ret.str();
}

long long
SQLiteTable::GetRowIdx(const std::string& column, void* value)
{
    long long int idx = m_iNodata;

	int colidx = ColumnExists(column);
    if (colidx < 0)// || value == 0)
		return idx;

    sqlite3_stmt* stmt = m_vStmtGetRowidx.at(colidx);
    int rc;
    switch(m_vTypes[colidx])
    {
    case ATTYPE_DOUBLE:
        rc = sqlite3_bind_double(stmt, 1, *(static_cast<double*>(value)));
        break;
    case ATTYPE_INT:
        rc = sqlite3_bind_int64(stmt, 1, *(static_cast<long long*>(value)));
        break;
    case ATTYPE_STRING:
        rc = sqlite3_bind_text(stmt, 1,
                               (static_cast<std::string*>(value))->c_str(),
                               -1, 0);
        break;
    default:
        return idx;
        break;
    }

    if (sqliteError(rc, &stmt)) return idx;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        idx = sqlite3_column_int64(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return idx;
}

//void
//SQLiteTable::SetColumnName(int col, const std::string& name)
//{
//	if (	col < 0
//		||  col > this->m_vNames.size()-1
//		||  name.empty()
//	   )
//		return;

//	this->m_vNames[col] = name;
//}

bool
SQLiteTable::RemoveColumn(int col)
{
	if (col < 0 || col > this->m_vNames.size()-1)
		return false;

    this->RemoveColumn(m_vNames[col]);

	return true;
}

bool
SQLiteTable::RemoveColumn(const std::string& name)
{
    const int idx = ColumnExists(name);
    if (idx < 0)
    {
        return true;
    }

    std::vector<std::string> colsvec = this->m_vNames;
    colsvec.erase(colsvec.begin()+idx);

    std::stringstream collist;
    std::stringstream ssql;

    for (int c=0; c < colsvec.size(); ++c)
    {
        collist << colsvec.at(c);
        if (c < colsvec.size()-1)
        {
            collist << ",";
        }
    }

    ssql << "BEGIN TRANSACTION;"
         << "CREATE TEMPORARY TABLE " << "\"" << m_tableName << "\"" << "_backup(" << collist.str() << ");"
         << "INSERT INTO " << "\"" << m_tableName << "\"" << "_backup SELECT "  << collist.str() << " FROM main." << "\"" << m_tableName << "\"" << ";"
         << "DROP TABLE main." << "\"" << m_tableName << "\"" << ";"
         << "CREATE TABLE main." << "\"" << m_tableName << "\"" << "(" << collist.str() << ");"
         << "INSERT INTO main." << "\"" << m_tableName << "\"" << " SELECT " << collist.str() << " FROM " << "\"" << m_tableName << "\"" << "_backup;"
         << "DROP TABLE " << "\"" << m_tableName << "\"" << "_backup;"
         << "END TRANSACTION";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        return false;
    }

    this->m_vNames.erase(m_vNames.begin()+idx);
    this->m_vTypes.erase(m_vTypes.begin()+idx);

    return true;
}

void SQLiteTable::SetValue(int col, long long int row, double value)
{
    if (col < 0 || col >= m_vNames.size())
    {
		return;
    }

	if (row < 0 || row >= m_iNumRows)
    {
		return;
    }


    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_double(stmt, 1, value);
    if (sqliteError(rc, &stmt))
    {
        return;
    }

    rc = sqlite3_bind_int64(stmt, 2, row);
    if (sqliteError(rc, &stmt))
    {
        return;
    }

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
}

void SQLiteTable::SetValue(int col, long long int row, long long int value)
{
    if (col < 0 || col >= m_vNames.size())
    {
		return;
    }

	if (row < 0 || row >= m_iNumRows)
    {
        return;
    }

    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_int64(stmt, 1, value);
    if (sqliteError(rc, &stmt))
    {
        return;
    }

    rc = sqlite3_bind_int64(stmt, 2, row);
    if (sqliteError(rc, &stmt))
    {
        return;
    }

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
}

void SQLiteTable::SetValue(int col, long long int row, std::string value)
{
    if (col < 0 || col >= m_vNames.size())
    {
        return;
    }

    if (row < 0 || row >= m_iNumRows)
    {
        return;
    }

    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_text(stmt, 1, value.c_str(), -1, 0);
    if (sqliteError(rc, &stmt))
    {
        return;
    }


    rc = sqlite3_bind_int64(stmt, 2, row);
    if (sqliteError(rc, &stmt))
    {
        return;
    }


    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
}

double SQLiteTable::GetDblValue(int col, long long row)
{
    if (col < 0 || col >= m_vNames.size())
		return m_dNodata;

    //	if (row < 0 || row >= m_iNumRows)
    //		return m_dNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int64(stmt, 1, row);
    if (sqliteError(rc, &stmt)) return m_dNodata;

    double ret = m_dNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_double(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;
}

long long
SQLiteTable::GetIntValue(int col, long long row)
{
    if (col < 0 || col >= m_vNames.size())
		return m_iNodata;

    //	if (row < 0 || row >= m_iNumRows)
    //		return m_iNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int64(stmt, 1, row);
    if (sqliteError(rc, &stmt)) return m_iNodata;

    long long ret = m_iNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;
}

long long
SQLiteTable::GetMinMaxPKValue(bool bmax)
{
    if (m_db == 0)
    {
        return m_iNodata;
    }

    long long ret = m_iNodata;
    std::stringstream ssql;
    ssql << "SELECT ";
    if (bmax)
    {
        ssql << "max(";
    }
    else
    {
        ssql << "min(";
    }

    ssql << m_idColName << ") from "
         << "\"" << m_tableName << "\"";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                                -1, &stmt, 0);
    if (sqliteError(rc, &stmt))
    {
        sqlite3_finalize(stmt);
        return ret;
    }


    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return ret;

}


long long
SQLiteTable::GetMaxPKValue()
{
    return GetMinMaxPKValue(true);
}


long long
SQLiteTable::GetMinPKValue()
{
    return GetMinMaxPKValue(false);
}


std::string SQLiteTable::GetStrValue(int col, long long row)
{
    if (col < 0 || col >= m_vNames.size())
		return m_sNodata;

    //	if (row < 0 || row >= m_iNumRows)
    //		return m_sNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int64(stmt, 1, row);
    if (sqliteError(rc, &stmt)) return m_sNodata;

    std::stringstream ret;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char* sval = sqlite3_column_text(stmt, 0);
        if (sval)
        {
            ret << sval;
        }
        else
        {
            ret << m_sNodata;
        }
    }
    else
    {
        ret << m_sNodata;
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret.str();
}


//// ------------------------------------------------------- other useful public functions
//void SQLiteTable::Print(std::ostream& os, itk::Indent indent, int nrows)
//{
//	os << indent << "\n";
//	os << indent << "OTB raster attribute table print-out\n";
//	os << indent << "File: " <<	this->m_sImgName << "\n";
//	os << indent << "Band: " << this->m_iBand << "\n";
//	os << indent << "Columns: " << this->m_vNames.size() << "\n";
//	os << indent << "Rows: " << this->m_iNumRows << "\n";
//	os << "\n";

//	itk::Indent ii = indent.GetNextIndent();
//	itk::Indent iii = ii.GetNextIndent();

//	os << indent << "Columns:\n";
//	for (int c = 0; c < this->m_vNames.size(); c++)
//		os << ii << c << ": " << this->m_vNames[c] << " (" << typestr(this->m_vTypes[c]) << ")\n";

//	os << indent << "Rows:\n";
//	char val[256];
//	nrows = nrows < this->m_iNumRows ? nrows : this->m_iNumRows;
//	for (int r=0; r < nrows; ++r)
//	{
//		os << ii << "#" << r << " - ROW\n";
//		for (int c=0; c < this->m_vNames.size(); c++)
//		{
//			switch (this->GetColumnType(c))
//			{
//			case ATTYPE_DOUBLE:
//				::sprintf(val, "%g", this->GetDblValue(c, r));
//				break;
//			case ATTYPE_INT:
//				::sprintf(val, "%ld", this->GetIntValue(c, r));
//				break;
//			case ATTYPE_STRING:
//				::sprintf(val, "%s", this->GetStrValue(c, r).c_str());
//				break;
//			default:
//				::sprintf(val, "%s", this->m_sNodata.c_str());
//			}
//			os << iii << this->m_vNames[c] << ": " << val << "\n";
//		}
//	}
//}

//void SQLiteTable::PrintStructure(std::ostream& os, itk::Indent indent)
//{
//	  itk::Indent i = indent;
//	  itk::Indent ii = i.GetNextIndent();
//	  itk::Indent iii = ii.GetNextIndent();
//	  os << i << "Raster Attribute Table, band #" << this->GetBandNumber() << " ('" <<
//			  this->GetImgFileName() << "')\n";
//	  os << ii << "#columns: " << this->GetNumCols() << ", #rows: " << this->GetNumRows() << "\n";
//	  os << ii << "ColumnIndex: ColumnName (ColumnType):\n";

//	  for (int c = 0; c < m_vNames.size(); c++)
//		  os << iii << c << ": " << m_vNames[c] << " (" << typestr(m_vTypes[c]) << ")\n";
//}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ---------------------  PROTECTED FUNCTIONS -----------------------------------------------

//std::string SQLiteTable::typestr(TableColumnType type)
//{
//	switch(type)
//	{
//	case ATTYPE_DOUBLE:
//		return "double";
//		break;
//	case ATTYPE_INT:
//		return "integer";
//		break;
//	case ATTYPE_STRING:
//		return "string";
//		break;
//	default:
//		return "unknown";
//	}
//}

//int
//SQLiteTable::valid(const std::string& sColName, int idx)
//{
//	//check if the column exists -> column exists returns the zero-based index of the
//	//particular column within the set of columns or -1 if the column does not exist
//	int colidx = ColumnExists(sColName);
//	if (colidx < 0)
//		return -1;

//	//check for the index
//	if (idx > m_iNumRows - 1 || idx < 0)
//		return -1;

//	return colidx;
//}

bool
SQLiteTable::SetRowIDColName(const std::string& name)
{
    // only makes sense before creation of the data base or
    // reading another one from disk
    if (m_db == 0)
    {
        this->m_idColName = name;
        return true;
    }

    return false;
}

bool
SQLiteTable::DropTable(const std::string &tablename)
{
    NMDebugCtx(_ctxotbtab, << "...");

    if (    m_db == 0
        ||  (   tablename.empty()
             && m_tableName.empty())
       )
    {
        NMProcWarn(<< "Didn't drop anything - no database connection!");
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    std::stringstream ssql;

    ssql << "Drop table ";
    if (!tablename.empty())
    {
        ssql << "\"" << tablename << "\"" << ";";
    }
    else
    {
        ssql << "\"" << m_tableName << "\"";
    }

    NMDebugAI( << "SQL command: " << ssql.str() << std::endl);

    char* errmsg = 0;
    sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, &errmsg);
    if (errmsg)
    {
        NMProcWarn( << "ERROR - DropTable: " << errmsg);
        sqlite3_free(errmsg);
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::DeleteDatabase()
{
    this->CloseTable(true);
    return !remove(m_dbFileName.c_str());
}

std::string
SQLiteTable::GetRandomString(int len)
{
    if (len < 1)
    {
        return "";
    }

    std::srand(std::time(0));
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
    std::string ret = nam;
    delete[] nam;

    return ret;
}

bool
SQLiteTable::openConnection(void)
{
    // ============================================================
    // open or create the host data base
    // ============================================================


    int openFlags = SQLITE_OPEN_URI;

    if (m_bOpenReadOnly)
    {
        openFlags |= SQLITE_OPEN_READONLY;
    }
    else
    {
        openFlags |= SQLITE_OPEN_READWRITE |
                     SQLITE_OPEN_CREATE;
    }

    if (m_bUseSharedCache)
    {
        openFlags |= SQLITE_OPEN_SHAREDCACHE;
    }

    int rc = ::sqlite3_open_v2(m_dbFileName.c_str(),
                               &m_db, openFlags, 0);
    if (rc != SQLITE_OK)
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        //NMErr(_ctxotbtab, << "SQLite3 ERROR #" << rc << ": " << errmsg)
        //itkDebugMacro(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
        std::stringstream errstr;
        errstr << "SQLite3 ERROR #" << rc << ": " << errmsg;
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        m_dbFileName.clear();
        ::sqlite3_close(m_db);
        m_db = 0;
        //NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    rc = sqlite3_exec(m_db, "PRAGMA cache_size = 70000;", 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        //NMWarn(_ctxotbtab, << "Failed to adjust cache_size!");
        //itkDebugMacro(<< "Failed to adjust cache_size!");
        m_lastLogMsg = "Failed to adjust cache size!";
        //this->InvokeEvent(itk::NMLogEvent("Failed to adjust cache_size!",
          //                                itk::NMLogEvent::NM_LOG_ERROR));
    }

	// alloc spatialite caches
    m_SpatialiteCache = spatialite_alloc_connection();
    if (m_SpatialiteCache == 0)
    {
        //NMErr(_ctxotbtab, << "Failed allocating spatialite connection!")
        std::stringstream errstr;
        errstr << "Failed allocating spatialite connection!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        return false;
    }
	
    NMDebugAI( << _ctxotbtab << ": Opened connection to '"
               << m_dbFileName << "'" << std::endl);

    std::stringstream errstr;
    errstr << "Opened connection to '" << m_dbFileName << "'";
    //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_DEBUG));

    spatialite_init_ex(m_db, m_SpatialiteCache, 0);

    return true;
}

int
SQLiteTable::deleteOldLDB(const std::string& vt, const std::string& ldb)
{
#ifdef _WIN32
#define stat64 _stat64
#endif

	bool ret = 1;

	struct stat64 resVt;
	if (stat64(vt.c_str(), &resVt) == 0)
	{
		struct stat64 resLdb;
		if (stat64(ldb.c_str(), &resLdb) == 0)
		{
			// vt is more recent
			if (resVt.st_mtime > resLdb.st_mtime)
			{
				// delete old ldb
				if (remove(ldb.c_str()))
				{
					std::stringstream errmsg;
					errmsg << "SQLiteTable::deleteOldLDB() - ERROR: Failed deleting old '"
						<< ldb << "'!";
					m_lastLogMsg = errmsg.str();
					ret = -1;
				}
			}
			// ldb is more recent
			else
			{
				// don't do anything, just open 
				ret = 0;
			}
		}
	}
	else
	{
		std::stringstream errmsg;
		errmsg << "SQLiteTable::deleteOldLDB() - ERROR: The provided virtual table '"
			<< vt << "' is not accessible!";
		m_lastLogMsg = errmsg.str();
		ret = -1;
	}
	return ret;
}

bool
SQLiteTable::CreateFromVirtual(const std::string &fileName,
                               const std::string &encoding, const int &srid)
{
    NMDebugCtx(_ctxotbtab, << "...");

    std::vector<std::string> vinfo = GetFilenameInfo(fileName);
    if (vinfo.size() == 0)
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

	/*
	std::string tmp = vinfo[1];
	std::string torepl = "-+&=*(){}[]%$#@!~/\\";
	for (int i=0; i < torepl.size(); ++i)
	{
		size_t pos = tmp.find(torepl[i]);
		if (pos != std::string::npos)
		{
			tmp = tmp.replace(pos, 1, "_");
		}
	}

    m_tableName = tmp;
	*/

	m_tableName = vinfo[1];

    std::string ext = vinfo[2];

    std::string vname = "vt_";
    vname += m_tableName;

	// -------------------------------
	// craft *.ldb filename
	// -------------------------------
    m_dbFileName = vinfo[0];
    m_dbFileName += "/";
    m_dbFileName += m_tableName;
    //m_dbFileName += ext;
    m_dbFileName += ".ldb";

	// --------------------------------
	// delete existing *.ldb if older
	// than virtual table provided 
	// in filename
	// ---------------------------------

    int deloldb = this->deleteOldLDB(fileName, m_dbFileName);
    if (deloldb < 0)
	{
		NMDebugCtx(_ctxotbtab, << "done!");
		return false;
	}

	// ------------------------------
	// create / open *.ldb database
	// ----------------------------
    if (!openConnection())
    {
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // if we've got a m_tableName set, we see
    // whether we can find it in the database ...
    if (FindTable(m_tableName))
    {
        NMDebugAI(<< "There's already a table '" << m_tableName
                  << "' - so we just use that ..." << std::endl);
        PopulateTableAdmin();
        return true;
    }

    std::stringstream ssql;
    ssql << "CREATE VIRTUAL TABLE \"" << vname << "\" USING ";

    std::string ecode = "'";
    ecode += encoding;
    ecode += "'";

    if (ext.compare(".csv") == 0 || ext.compare(".txt") == 0)
    {
        ssql << "VirtualText('" << fileName << "', " << ecode
             << ", 1, POINT, DOUBLEQUOTE, ',');";
    }
    else if (ext.compare(".shp") == 0 || ext.compare(".shx") == 0)
    {
        std::string sname = "'";
        sname += vinfo[0];
#ifdef _WIN32
        sname += "\\";
#else
        sname += "/";
#endif
        sname += m_tableName;
        sname += "'";
        ssql << "VirtualShape(" << sname << ", " << ecode
             << ", " << srid << ");";
    }
    else if (ext.compare(".dbf") == 0)
    {
        ssql << "VirtualDbf('" << fileName << "', " << ecode << ");";
    }
    else if (ext.compare(".xls") == 0)
    {
        ssql << "VirtualXL('" << fileName << "', 0, 1);";
    }
    else
    {
        //NMErr(_ctxotbtab, << "File format not supported!");
        m_lastLogMsg = "File format not supported!";
        //this->InvokeEvent(itk::NMLogEvent("File format not supported!",
                        //  itk::NMLogEvent::NM_LOG_ERROR));
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // create the virtual table first ...
    if (!SqlExec(ssql.str()))
    {
        //NMErr(_ctxotbtab, << "Creating virtual table from "
        //      << m_tableName << "." << ext << " failed!");
        std::stringstream errstr;
        errstr << "Creating virtual table from "
               << m_tableName << "." << ext << " failed!";
        m_lastLogMsg = errstr.str();
        //this->InvokeEvent(itk::NMLogEvent(errstr.str(), itk::NMLogEvent::NM_LOG_ERROR));
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // double check whether we've got any records here, since
    // spatialite creates empty tables if the import fails ...
    ssql.str("");
    ssql << "SELECT COUNT(*) FROM \"" << vname << "\";";

    sqlite3_stmt* rcnt = 0;
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                                -1, &rcnt, 0);
    if (sqliteError(rc, &rcnt))
    {
        //NMProcWarn(<< "Failed querying the number of records in the VT!");
        m_lastLogMsg = "Failed querying the number of records in the VT!";
        //this->InvokeEvent(itk::NMLogEvent("Failed querying the number of records in the VT!",
          //                                itk::NMLogEvent::NM_LOG_WARN));
        sqlite3_finalize(rcnt);
        m_dbFileName.clear();
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    //    if (sqlite3_step(rcnt) == SQLITE_ROW)
    //    {
    //        m_iNumRows = sqlite3_column_int64(rcnt, 0);
    //    }
    //    NMDebugAI( << vname << " has " << m_iNumRows
    //               << " records" << std::endl);
    //    sqlite3_finalize(rcnt);

    //    if (m_iNumRows < 1)
    //    {
    //        NMProcWarn(<< "Import failed or VT is empty!")
    //        m_dbFileName.clear();
    //        NMDebugCtx(_ctxotbtab, << "done!");
    //        return false;
    //    }

    // ... and then make a proper one of it ...
    ssql.str("");
    ssql << "CREATE TABLE " << "\"" << m_tableName << "\"" << " AS SELECT * FROM \"" << vname << "\";";
    ssql << "DROP TABLE \"" << vname << "\";";

    if (SqlExec(ssql.str()))
    {
        return PopulateTableAdmin();
    }
    else
    {
        return false;
    }

    NMDebugCtx(_ctxotbtab, << "done!");

    return true;
}

std::vector<std::string>
SQLiteTable::GetFilenameInfo(const std::string& fileName)
{
    /*! content of filename info vector
     *  0: path
     *  1: basename
     *  2: (lower case) extension
     */
    std::vector<std::string> vinfo;

    if (fileName.empty()
        || fileName.compare("file::memory:") == 0)
    {
        return vinfo;
    }

    size_t lwin = fileName.find_last_of('\\');
    size_t lunx = fileName.find_last_of('/');
    lwin = lwin == std::string::npos ? 0 : lwin;
    lunx = lunx == std::string::npos ? 0 : lunx;
    size_t pos = lwin > lunx ? lwin : lunx;
    std::string path = fileName.substr(0, pos);

    size_t epos = fileName.find_last_of('.');
    std::string ext = fileName.substr(epos, fileName.length()-epos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    ++pos;
    std::string bname = fileName.substr(pos, fileName.length()-pos-ext.length());

    vinfo.push_back(path);
    vinfo.push_back(bname);
    vinfo.push_back(ext);

    return vinfo;
}

bool
SQLiteTable::openAsInMemDb(const std::string &dbName, const std::string &tablename)
{
    if (this->m_db != nullptr)
    {
        this->CloseTable(false);
    }

    // check filename
    std::string indbfn = dbName.empty() ? this->m_dbFileName : dbName;
    if (indbfn.empty())
    {
        this->m_lastLogMsg = "The provided filename is empty!";
        return false;
    }

    std::string errmsg;
    std::stringstream errstr;
    std::string tabname = tablename.empty() ? this->m_tableName : tablename;

    // open the db to copy from
    sqlite3* pFileDb = nullptr;
    int rc = sqlite3_open_v2(indbfn.c_str(), &pFileDb,
                         SQLITE_OPEN_READONLY |
                         SQLITE_OPEN_URI | SQLITE_OPEN_PRIVATECACHE,
                         nullptr);
    if (rc != SQLITE_OK)
    {
        errmsg = sqlite3_errmsg(pFileDb);
        errstr.str("");
        errstr << "SQLite3 ERROR #" << rc << ": " << errmsg;
        m_lastLogMsg = errstr.str();
        ::sqlite3_close(pFileDb);
        pFileDb = nullptr;
        return false;
    }


    // create in mem db to receive the backup
    rc = sqlite3_open(":memory:", &m_db);

    if (rc != SQLITE_OK)
    {
        errmsg = sqlite3_errmsg(m_db);
        errstr.str("");
        errstr << "SQLite3 ERROR #" << rc << ": " << errmsg;
        m_lastLogMsg = errstr.str();
        ::sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // do the backup
    sqlite3_backup* pBackup = sqlite3_backup_init(m_db, "main",
                                                  pFileDb, "main");
    if (pBackup)
    {
//        do
//        {
//            rc = sqlite3_backup_step(pBackup, 5);
//            if (    rc == SQLITE_OK
//                ||  rc == SQLITE_BUSY
//                ||  rc == SQLITE_LOCKED
//               )
//            {
//                sqlite3_sleep(0.025);
//            }

//            ++bcycles;

//        } while (    rc == SQLITE_OK
//                 ||  rc == SQLITE_BUSY
//                 ||  rc == SQLITE_LOCKED
//                );
        rc = sqlite3_backup_step(pBackup, -1);
        sqlite3_backup_finish(pBackup);
    }

    if (pBackup == 0 || (pBackup && (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)))
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        std::stringstream errstr;
        errstr << "SQLite3 ERROR #" << rc << ": " << errmsg;
        m_lastLogMsg = errstr.str();
        m_dbFileName.clear();
        ::sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // if tablename is not specified, we
    // just grab the first table we find
    if (tablename.empty())
    {
        std::vector<std::string> tabs = this->GetTableList();
        this->m_tableName = tabs.at(0);
    }
    else
    {
        this->m_tableName = tablename;
    }

    this->m_dbFileName = ":memory:";
    this->PopulateTableAdmin();

    return true;
}

SQLiteTable::TableCreateStatus
SQLiteTable::CreateTable(std::string filename, std::string tag)
{
    //this->DebugOn();

    NMDebugCtx(_ctxotbtab, << "...");

    if (filename.empty())
    {
        m_dbFileName = GetRandomString(5);
        m_dbFileName += ".ldb";
    }
    else
    {
        m_dbFileName = filename;
    }

    NMDebugAI(<< "using '" << m_dbFileName
              << "' as filename for the db" << std::endl);

    // ============================================================
    // open / create the data base with spatialte support
    // ============================================================

    if (!openConnection())
    {
        return ATCREATE_ERROR;
    }

    // ============================================================
    // check, whether we've already got a table
    // ============================================================
    char* errMsg = 0;
    std::stringstream ssql;
    ssql.str("");

    if (!m_dbFileName.empty())
    {
        std::string fullname = m_dbFileName;

        if (fullname.compare("file::memory:") != 0)
        {

#ifndef _WIN32
            m_tableName = basename(const_cast<char*>(fullname.c_str()));
#else
            char fname[256];
            _splitpath(fullname.c_str(), 0, 0, fname, 0);
            m_tableName = fname;
#endif

            size_t pos = m_tableName.find_last_of('.');
            if (pos > 0)
            {
                m_tableName = m_tableName.substr(0, pos);
            }
			
			// avoid ugly table names, if nothing helps, double quote ...
			//m_tableName = this->formatTableName(m_tableName);

			/*
            std::locale loc;
            if (std::isdigit(m_tableName[0], loc))
            {
                std::string prefix = "nm_";
                m_tableName = prefix + m_tableName;
            }
			*/
        }
        else if (m_tableName.empty())
        {
            m_tableName = GetRandomString(5);
        }
    }

    if (!tag.empty())
    {
        int b = atoi(tag.c_str());
        if (b > 0)
        {
            this->m_iBand = b;
        }
        m_tableName += "_";
        m_tableName += tag;
    }

    NMDebugAI( << "looking for table '" << m_tableName << "' ..."
               << std::endl);

    int bTableExists = 0;
    if (FindTable(m_tableName))
    {
        bTableExists = 1;
    }

    // ============================================================
    // populate table info, if we've got one already
    // ============================================================

    if (bTableExists)
    {
        NMDebugAI( << "found table '"
                   << m_tableName << "'" << std::endl);

        this->PopulateTableAdmin();
    }


    // ============================================================
    // create the table, if not already exist
    // ============================================================
    int rc;
    if (!bTableExists)
    {
        NMDebugAI( << "no '" << m_tableName << "' found!"
                   << std::endl);
        NMDebugAI(<< "creating one ..." << std::endl);

        if (m_idColName.empty())
        {
            m_idColName = "rowidx";
        }
        ssql.str("");
        ssql << "begin transaction;";
        ssql << "CREATE TABLE " << "\"" << m_tableName << "\"" << " "
            << "(" << m_idColName << " INTEGER PRIMARY KEY);";
        ssql << "commit;";

        char* errMsg = 0;
        rc = ::sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, &errMsg);

        if (rc != SQLITE_OK)
        {
            std::string errmsg = sqlite3_errmsg(m_db);
            NMProcWarn(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
            NMProcWarn(<< "Failed creating the table!");
            m_dbFileName.clear();
            ::sqlite3_close(m_db);
            m_db = 0;
            NMDebugCtx(_ctxotbtab, << "done!");
            return ATCREATE_ERROR;
        }

        // add rowidx column to list of columns
        //m_vNames.push_back("rowidx");
        m_vNames.push_back(m_idColName);
        m_vTypes.push_back(SQLiteTable::ATTYPE_INT);
        NMDebugAI(<< m_tableName << " successfully created" << std::endl);
    }

    //==================================================
    // 'dummy' statements for rowidx set/get
    //==================================================
    NMDebugAI( << "create prepared statements for recurring tasks ... "
               << std::endl);

    // we prepare those  statements simply for synchorinsing
    // column index with vector indices for the prepared
    // statements (update & select); actually it doesn't really
    // make sense to use those, however you never know what the
    // user wants and this way we avoid any segfaults

    if (!bTableExists)
    {
        // prepare an update statement for this column
        sqlite3_stmt* stmt_upd;
        ssql.str("");
        ssql <<  "UPDATE main." << "\"" << m_tableName << "\"" << " SET " << m_idColName << " = "
             <<  "@VAL WHERE " << m_idColName << " = @IDX ;";
        rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                                -1, &stmt_upd, 0);
        sqliteError(rc, &stmt_upd);
        this->m_vStmtUpdate.push_back(stmt_upd);


        // prepare a get value statement for this column
        sqlite3_stmt* stmt_sel;
        ssql.str("");
        ssql <<  "SELECT " << m_idColName << " from main." << "\"" << m_tableName << "\"" << ""
             <<  " WHERE " << m_idColName << " = @IDX ;";
        rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                                -1, &stmt_sel, 0);
        sqliteError(rc, &stmt_sel);
        this->m_vStmtSelect.push_back(stmt_sel);

     }

    // ============================================================
    // prepare statements for recurring tasks
    // ============================================================
    ssql.str("");
    ssql << "SELECT count(" << this->m_idColName << ") "
         << "FROM " << "\"" << m_tableName << "\"" << ";";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(), -1, &m_StmtRowCount, 0);
    sqliteError(rc, &m_StmtRowCount);

    rc = sqlite3_prepare_v2(m_db, "BEGIN TRANSACTION;", -1, &m_StmtBegin, 0);
    sqliteError(rc, &m_StmtBegin);

    rc = sqlite3_prepare_v2(m_db, "END TRANSACTION;", -1, &m_StmtEnd, 0);
    sqliteError(rc, &m_StmtEnd);

    rc = sqlite3_prepare_v2(m_db, "ROLLBACK TRANSACTION;", -1, &m_StmtRollback, 0);
    sqliteError(rc, &m_StmtRollback);

    NMDebugAI(<< "all good!" << std::endl);
    NMDebugCtx(_ctxotbtab, << "done!");

    TableCreateStatus state = ATCREATE_CREATED;
    if (bTableExists)
    {
        state = ATCREATE_READ;
    }

    return state;

}

SQLiteTable::SQLiteTable()
    : m_db(nullptr),
      m_StmtBegin(nullptr),
      m_StmtEnd(nullptr),
      m_StmtRollback(nullptr),
      m_StmtBulkSet(nullptr),
      m_StmtBulkGet(nullptr),
      m_StmtColIter(nullptr),
      m_StmtRowCount(nullptr),
      m_SpatialiteCache(nullptr),
      m_CurPrepStmt(nullptr),
      //m_idColName(""),
      m_tableName(""),
      m_bUseSharedCache(true),
      m_bOpenReadOnly(false),
      m_lastLogMsg(""),
      m_bPersistentRowIdColName(false)
{
    //this->createTable("");
    this->m_ATType = ATTABLE_TYPE_SQLITE;
}

void
SQLiteTable::disconnectDB(void)
{
    if (m_db != 0)
    {
        sqlite3_close(m_db);
        spatialite_cleanup_ex(m_SpatialiteCache);
		m_SpatialiteCache = 0;
        m_db = 0;

        NMDebugAI(<< _ctxotbtab << ": Closed connection to '"
                   << m_dbFileName << "'" << std::endl);
    }
}

void
SQLiteTable::resetTableAdmin(void)
{
    // if there's no db connection any more
    // it is very likely one of the below finalization
    // statements will cause a segfault
    if (m_db == 0)
    {
        return;
    }

    // clean up
    if (m_StmtBegin != 0)
    {
        sqlite3_finalize(m_StmtBegin);
    }
    if (m_StmtEnd != 0)
    {
        sqlite3_finalize(m_StmtEnd);
    }
    if (m_StmtRollback != 0)
    {
        sqlite3_finalize(m_StmtRollback);
    }
    if (m_StmtBulkSet != 0)
    {
        sqlite3_finalize(m_StmtBulkSet);
    }
    if (m_StmtBulkGet != 0)
    {
        sqlite3_finalize(m_StmtBulkGet);
    }
    if (m_StmtColIter != 0)
    {
        sqlite3_finalize(m_StmtColIter);
    }
    if (m_StmtRowCount != 0)
    {
        sqlite3_finalize(m_StmtRowCount);
    }

    for (int v=0; v < m_vStmtUpdate.size(); ++v)
    {
        if (m_vStmtUpdate.at(v) != 0)
        {
            sqlite3_finalize(m_vStmtUpdate.at(v));
        }
    }

    for (int s=0; s < m_vStmtSelect.size(); ++s)
    {
        if (m_vStmtSelect.at(s) != 0)
        {
            sqlite3_finalize(m_vStmtSelect.at(s));
        }
    }

    for (int s=0; s < m_vStmtGetRowidx.size(); ++s)
    {
        if (m_vStmtGetRowidx.at(s) != 0)
        {
            sqlite3_finalize(m_vStmtGetRowidx.at(s));
        }
    }

    m_vTypes.clear();
    m_vIndexNames.clear();
    m_vNames.clear();
    m_vTypesBulkGet.clear();
    m_vTypesBulkSet.clear();
    m_vStmtUpdate.clear();
    m_vStmtSelect.clear();
    m_vStmtGetRowidx.clear();

    m_iNumRows = 0;
    m_iBand = 1;
    m_iNodata = -std::numeric_limits<long long>::max();
    m_dNodata = -std::numeric_limits<double>::max();
    m_sNodata = "NULL";
    //m_db = 0;
    m_bUseSharedCache = true;
    m_StmtBegin = 0;
    m_StmtEnd = 0;
    m_StmtRollback = 0;
    m_StmtBulkSet = 0;
    m_StmtBulkGet = 0;
    m_StmtColIter = 0;
    m_StmtRowCount = 0;
    m_CurPrepStmt = "";
    m_idColName = "";
    //m_tableName = "";
}

bool
SQLiteTable::loadExtension(const std::string &lib, const std::string &entry)
{
    char* errMsg;
    int rc = sqlite3_load_extension(m_db, lib.c_str(), entry.c_str(), &errMsg);
    if (rc)
    {
        NMProcErr(<< _ctxotbtab<< ": " << errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool
SQLiteTable::PopulateTableAdmin()
{
    NMDebugCtx(_ctxotbtab, << "...");

    if (!FindTable(this->m_tableName))
    {
        m_lastLogMsg = "Couldn't find table '" + m_tableName + "' in the database!";
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    const std::string tableName = m_tableName;
    const std::string rowidname = m_idColName;
    this->resetTableAdmin();

    m_tableName = tableName;
    if (m_bPersistentRowIdColName)
    {
        m_idColName = rowidname;
    }

    // -------------------------------------------------
    // get table info
    // -------------------------------------------------
    std::stringstream ssql;
    ssql.str("");
    ssql << "pragma table_info(" << "\"" << m_tableName << "\"" << ")";

    sqlite3_stmt* stmt_exists;
    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_exists, 0);
    if (sqliteError(rc, &stmt_exists))
    {
        sqlite3_finalize(stmt_exists);
        m_dbFileName.clear();
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // -------------------------------------------------
    // analyse structure
    // -------------------------------------------------

    NMDebugAI(<< "analysing table structure ..." << std::endl);
    bool browidx = false;
    std::string fstIntCol = "";
    while (sqlite3_step(stmt_exists) == SQLITE_ROW)
    {
        std::string name = reinterpret_cast<char*>(
                    const_cast<unsigned char*>(
                      sqlite3_column_text(stmt_exists, 1)));
        std::string type = reinterpret_cast<char*>(
                    const_cast<unsigned char*>(
                      sqlite3_column_text(stmt_exists, 2)));
        int pk = sqlite3_column_int(stmt_exists, 5);

        NMDebugAI( << "   "
                   << name << " | "
                   << type << " | "
                   << pk << std::endl);

        // convert type name into upperstring for
        // 'case insensitive comparison'
        std::transform(type.begin(), type.end(), type.begin(), ::toupper);

        if (name.compare("rowidx") == 0)
        {
            browidx = true;
        }

        // pick the first PRIMARY KEY column as THE PK
        if (pk && m_idColName.empty())
        {
            m_idColName = name;
        }

        m_vNames.push_back(name);
        if (    type.compare("INTEGER") == 0
            ||  type.compare("INT") == 0
           )
        {
            if (fstIntCol.empty())
            {
                fstIntCol = name;
            }
            m_vTypes.push_back(ATTYPE_INT);
        }
        else if (   type.compare("REAL") == 0
                 || type.compare("NUMERIC") == 0
                )
        {
            m_vTypes.push_back(ATTYPE_DOUBLE);
        }
        else
        {
            m_vTypes.push_back(ATTYPE_STRING);
        }

    }
    sqlite3_finalize(stmt_exists);

    // if we haven't got an id column yet, let's
    // whether we can persuade any of the others ...
    if (m_idColName.empty())
    {
        std::vector<std::string> ic;
        if (browidx)
        {
            ic.push_back("rowidx");
            CreateIndex(ic, false);
            m_idColName = "rowidx";
        }
        else if (!fstIntCol.empty())
        {
            ic.push_back(fstIntCol);
            CreateIndex(ic, false);
            m_idColName = fstIntCol;
        }
        else
        {
            m_idColName = "rowid";
        }
    }

    // well, if we haven't got any names/types, we'd better bail
    // out here, something seems to be wrong
    if (m_vNames.size() == 0)
    {
        m_lastLogMsg = "Failed fetching column info or unsupported table structure!";
        m_dbFileName.clear();
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    // prepare Prepared statements for the detected columns
    for (int c=0; c < m_vNames.size(); ++c)
    {
        this->createPreparedColumnStatements(m_vNames.at(c));
    }

    // -------------------------------------------------
    // number of records
    // -------------------------------------------------

    // now we count the number of records in the table
    ssql.str("");
    ssql << "SELECT count(" << m_idColName << ") "
    << "from " << "\"" << m_tableName << "\"" << ";";


    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
        -1, &stmt_exists, 0);
    if (sqliteError(rc, &stmt_exists))
    {
        m_lastLogMsg = "Failed fetching number of records!";
        NMProcWarn(<< "Failed fetching number of records!");
        sqlite3_finalize(stmt_exists);
        m_dbFileName.clear();
        NMDebugCtx(_ctxotbtab, << "done!");
        return false;
    }

    if (sqlite3_step(stmt_exists) == SQLITE_ROW)
    {
        m_iNumRows = sqlite3_column_int64(stmt_exists, 0);
    }
    NMDebugAI(<< m_tableName << " has " << m_iNumRows
        << " records" << std::endl);
    sqlite3_finalize(stmt_exists);

    NMDebugCtx(_ctxotbtab, << "done!");
    return true;
}

bool
SQLiteTable::FindTable(const std::string &tableName)
{
	if (m_db == 0)
	{
		return false;
	}

	int bTableExists = 0;
	sqlite3_stmt* stmt_exists;
	std::stringstream ssql;
	ssql << "SELECT count(name) FROM sqlite_master WHERE "
		<< "type='table' AND name='" << tableName << "';";

	int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
		-1, &stmt_exists, 0);
	if (sqliteError(rc, &stmt_exists))
	{
		sqlite3_finalize(stmt_exists);
		return false;
	}

	if (sqlite3_step(stmt_exists) == SQLITE_ROW)
	{
		bTableExists = sqlite3_column_int(stmt_exists, 0);
	}
	sqlite3_finalize(stmt_exists);

	return bTableExists;
}


bool
SQLiteTable::SetTableName(const std::string &tableName)
{
	bool ret = false;
	if (FindTable(tableName))
	{
		m_tableName = tableName;
		ret = true;
	}
	return ret;
}

/*
std::string
SQLiteTable::formatTableName(const std::string& tableName)
{
	// check for 'sqlite_' prefix of system tables
	std::string tn = tableName;
	if (tn.substr(0, 7).compare("sqlite_") == 0)
	{
		tn = tn.substr(7, tn.size() - 1);
	}
	
	// check for operator symbols and schema-name separator
	const std::locale loc;
	const char repl = '_';
	const std::vector<char> tok = { '-', '.', '+', '*', '/', '%', '|', '<', '>', '=', '!', '~'};
	for (int i = 0; i < tn.size(); ++i)
	{
		if (	std::find(tok.begin(), tok.end(), tn[i]) != tok.end()
			 || (i == 0 && std::isdigit(tn[i], loc))
		   )
		{
			tn[i] = repl;
		}
	}

	// double quote name if it is a SQL keyword after all!
	if (sqlite3_keyword_check(tn.c_str(), tn.size()))
	{
		tn = "\"" + tn + "\"";
	}

	return tn;
}
*/

bool
SQLiteTable::SetDbFileName(const std::string &dbFileName)
{
    bool ret = true;
    if (m_db != 0)
    {
        ret = false;
    }
    else
    {
        this->m_dbFileName = dbFileName;
    }

	return ret;
}

std::vector<std::string>
SQLiteTable::GetTableList(void)
{
    std::vector<std::string> vtables;

    if (m_db == 0)
    {
        return vtables;
    }

    sqlite3_stmt* stmt_tablelist;
    std::stringstream ssql;
    ssql << "SELECT name FROM sqlite_master WHERE "
        << "type = 'table';";

    int rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            -1, &stmt_tablelist, 0);
    if (sqliteError(rc, &stmt_tablelist))
    {
        sqlite3_finalize(stmt_tablelist);
        return vtables;
    }

    //std::cout << "THE QUERY: " << ssql.str() << std::endl;
    std::stringstream tnamestr;
    while (sqlite3_step(stmt_tablelist) == SQLITE_ROW)
    {
        tnamestr.str("");
        const unsigned char* sval = sqlite3_column_text(stmt_tablelist, 0);
        if (sval)
        {
            tnamestr << sval;
        }
        vtables.push_back(tnamestr.str());
    }

    //sqliteError(rc, &stmt_tablelist);

    sqlite3_finalize(stmt_tablelist);

    return vtables;
}

bool
SQLiteTable::SqlExec(const std::string& sqlstr)
{
    if (m_db == 0)
    {
        return false;
    }

    bool ret = true;

    char* errMsg;
    if (sqlite3_exec(m_db, sqlstr.c_str(), 0, 0, &errMsg) != 0)
    {
        m_lastLogMsg = errMsg;
        NMProcWarn(<< "SQLite3 ERROR: " << errMsg);
        sqlite3_free((void*)errMsg);
        ret = false;
    }

    return ret;
}

bool
SQLiteTable::JoinAttributes(const std::string& targetTable,
                    const std::string& targetJoinField,
                    const std::string& sourceDbFileName,
                    const std::string& sourceTable,
                    const std::string& sourceJoinField,
                    std::vector<std::string> sourceFields)
{
    bool ret = true;

    if (m_db == 0)
    {
        return false;
    }

    // attach database if necessary
    std::string sourceDb = "main";
    if (!this->FindTable(sourceTable))
    {
        sourceDb = this->GetRandomString(3);
        if (!this->AttachDatabase(sourceDbFileName, sourceDb))
        {
            NMProcWarn(<< "Couldn't attach source database!");
            return false;
        }
    }

    // create indices for join fields
    std::vector<std::string> vIdxCol;
    vIdxCol.push_back(sourceJoinField);
    if (!this->CreateIndex(vIdxCol, false, sourceTable, sourceDb))
    {
        NMProcWarn(<< "indexing '" << sourceTable << "."
                        << vIdxCol[0] << "' failed!");
    }

    vIdxCol.clear();
    vIdxCol.push_back(targetJoinField);
    if (!this->CreateIndex(vIdxCol, false, targetTable))
    {
        NMProcWarn(<< "indexing '" << targetTable << "."
                        << vIdxCol[0] << "' failed!");
    }

    //===================================================
    //      DEFINING SUB-QUERY AND FINAL TABLE STRUCTURE
    //===================================================

    // the final table structure (i.e. field names)
    std::string tempJoinTable = this->GetRandomString(5);
    std::stringstream finalTableFieldStr;
    for (int i=0; i < m_vNames.size(); ++i)
    {
        finalTableFieldStr << tempJoinTable << "." << m_vNames[i];
        if (i < m_vNames.size()-1)
        {
            finalTableFieldStr << ", ";
        }
    }

    // here we define the the sub-query structure (i.e. the fields to be joined
    // from the source table); and extend the final table structure
    // by the join fields from the source table (note: we don't include the
    // the source index field)
    std::stringstream srcFieldStr;
    bool bSrcJoinFieldIncluded = false;
    for (int i=0; i < sourceFields.size(); ++i)
    {
        if (sourceFields[i].compare(sourceJoinField) == 0)
        {
            bSrcJoinFieldIncluded = true;
        }
        else
        {
            finalTableFieldStr << ", " << tempJoinTable << "." << sourceFields[i];
        }

        srcFieldStr << sourceDb << "." << sourceTable << "." << sourceFields[i];
        if (i < sourceFields.size()-1)
        {
            srcFieldStr << ", ";
        }
    }

    // if the join field of the source table is not included in the join fields
    // list, we add it to the source fields list since required for joining the
    // the sub-query to the target table
    if (!bSrcJoinFieldIncluded)
    {
        srcFieldStr << ", " << sourceDb << "." << sourceTable << "." << sourceJoinField;
    }

    /*
     * SAMPLE QUERY: JOIN WITH SUBQUERY
        Select * from main.zz3_1 as t LEFT OUTER JOIN
               (Select srcDB.TANK_rec2ws.AreaHa, srcDB.TANK_rec2ws.SedNetID from srcDB.TANK_rec2ws)
                                      as s on t.rowidx = s.SedNetID;
    */

    // create join expression
    std::stringstream ssql;
    ssql << "CREATE TEMP TABLE " << tempJoinTable << " AS "
         << "SELECT * FROM main." << targetTable << " "
         << "LEFT OUTER JOIN "
         << "(SELECT " << srcFieldStr.str()
              << " FROM " << sourceDb << "." << sourceTable << ") AS s "
         << "ON main." << targetTable << "." << targetJoinField << " = "
         //<< sourceDb << "." << sourceTable << "." << sourceJoinField << ";";
         << " s." << sourceJoinField << ";";

    NMDebugAI(<< "THE QUERY:\n" << ssql.str() << std::endl);

    if (!this->SqlExec(ssql.str()))
    {
        NMProcWarn(<< "something went wrong!");
        this->DetachDatabase(sourceDb);
        return false;
    }

    ssql.str("");
    ssql << "DROP TABLE main." << targetTable << ";";
    ssql << "CREATE TABLE " << targetTable
         //<< " AS SELECT * FROM " << tempJoinTable << ";";
         << " AS SELECT " << finalTableFieldStr.str() << " FROM " << tempJoinTable << ";";

    ret = this->SqlExec(ssql.str());

    if (ret && targetTable.compare(m_tableName) == 0)
    {
        this->PopulateTableAdmin();
    }

    if (sourceDb.compare("main") != 0)
    {
        this->DetachDatabase(sourceDb);
    }

    return ret;
}

bool
SQLiteTable::AttachDatabase(const std::string& fileName, const std::string &dbName)
{
    std::stringstream sql;
    sql << "ATTACH DATABASE \"" << fileName << "\" "
        << "AS " << dbName << ";";
    return SqlExec(sql.str());
}

bool
SQLiteTable::DetachDatabase(const std::string &dbName)
{
    std::stringstream sql;
    sql << "DETACH DATABASE " << dbName << ";";
    return SqlExec(sql.str());
}


void
SQLiteTable::CloseTable(bool drop)
{
    if (drop)
    {
        this->DropTable();
    }
    //    NMProcInfo(<< "disconnecting "
    //                     << this->m_dbFileName << "."
    //                     << this->m_tableName);
    this->resetTableAdmin();
    this->disconnectDB();
}

// clean up
SQLiteTable::~SQLiteTable()
{
    this->CloseTable();
}


} // end of namespace otb
