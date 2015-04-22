 /******************************************************************************
 * Created by Alexander Herzig is-
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd
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
 * AttributeTable.cxx
 *
 *  Created on: 20/08/2010
 *      Author: Alexander Herzig
 */
#include "nmlog.h"
#define _ctxotbtab "AttributeTable"
#include "otbAttributeTable.h"
#include <limits>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include "otbMacro.h"

namespace otb
{

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ---------------------  PUBLIC GETTER and SETTER functions to manage the Attribute table
int AttributeTable::GetNumCols()
{
	return this->m_vNames.size();
}

int AttributeTable::GetNumRows()
{
	return m_iNumRows;
}

bool
AttributeTable::sqliteError(const int& rc, sqlite3_stmt** stmt)
{
    this->DebugOn();

    if (rc != SQLITE_OK)
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        itkDebugMacro(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
        if (*stmt)
        {
            sqlite3_clear_bindings(*stmt);
            sqlite3_reset(*stmt);
        }
        return true;
    }
    return false;
}

int
AttributeTable::ColumnExists(const std::string& sColName)
{
    // returns -1 if column doesn't exist, otherwise 1

    int idx = -1;
    //    const char** pszDataType = 0;
    //    const char** pszCollSeq = 0;
    //    int* pbNotNull = 0;
    //    int* pbPrimaryKey = 0;
    //    int* pbAutoinc = 0;

    //    if (m_db == 0)
    //    {
    //        return idx;
    //    }

    //    int rc = ::sqlite3_table_column_metadata(
    //                m_db,
    //                "main",
    //                "nmtab",
    //                sColName.c_str(),
    //                pszDataType,
    //                pszCollSeq,
    //                pbNotNull,
    //                pbPrimaryKey,
    //                pbAutoinc);

    //    if (rc == SQLITE_OK)
    //    {
    //        return 1;
    //    }
    //    else
    //    {
    //        return -1;
    //    }

    //    itkDebugMacro(<< std::endl
    //                  << "column: " << sColName << " | "
    //                  << "data type: " << pszDataType);


    // we do store column availability information for backward compatility
    // with the old vector-based interface (to support the idx-based
    // column specification in Get/Set functions), so why don't we use it
    // then?
    // =================================================================
    // old implementation
    // =================================================================
    for (int c=0; c < m_vNames.size(); ++c)
    {
        if (::strcmp(m_vNames[c].c_str(), sColName.c_str()) == 0)
        {
            idx = c;
            break;
        }
    }
	return idx;
}

bool
AttributeTable::AddColumn(const std::string& sColName, TableColumnType eType)
{
	if ((	 eType != ATTYPE_STRING
		 &&  eType != ATTYPE_INT
		 &&  eType != ATTYPE_DOUBLE
		)
		||  this->ColumnExists(sColName) >= 0
	   )
	{
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
    ssql << "ALTER TABLE main.nmtab ADD " << sColName << " "
         << sType << ";";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    sqliteError(rc, 0);

    // update admin infos
    this->m_vNames.push_back(sColName);
    this->m_vTypes.push_back(eType);

    // prepare an update statement for this column
    sqlite3_stmt* stmt_upd;
    ssql.str("");
    ssql <<  "UPDATE main.nmtab SET " << sColName << " = "
         <<  "@VAL WHERE rowidx = @IDX ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            1024, &stmt_upd, 0);
    sqliteError(rc, &stmt_upd);
    this->m_vStmtUpdate.push_back(stmt_upd);

    // prepare a get value statement for this column
    sqlite3_stmt* stmt_sel;
    ssql.str("");
    ssql <<  "SELECT " << sColName << " from main.nmtab"
         <<  " WHERE rowidx = @IDX ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            1024, &stmt_sel, 0);
    sqliteError(rc, &stmt_sel);
    this->m_vStmtSelect.push_back(stmt_sel);

    // prepare a get rowidx by value statement for this column
    sqlite3_stmt* stmt_rowidx;
    ssql.str("");
    ssql <<  "SELECT rowidx from main.nmtab"
         <<  " WHERE " << sColName << " = @IDX ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            1024, &stmt_rowidx, 0);
    sqliteError(rc, &stmt_rowidx);
    this->m_vStmtGetRowidx.push_back(stmt_rowidx);


    //	std::vector<std::string>* vstr;
    //	std::vector<long>* vint;
    //	std::vector<double>* vdbl;
    //	// create a new vector for the column's values
    //	switch(eType)
    //	{
    //	case ATTYPE_STRING:
    //		try{
    //		vstr = new std::vector<std::string>();
    //		vstr->resize(m_iNumRows, m_sNodata);
    //		} catch (std::exception& e) {NMErr(_ctxotbtab, << "Failed adding column: " << e.what());return false;}

    //		this->m_mStringCols.push_back(vstr);
    //		this->m_vPosition.push_back(m_mStringCols.size()-1);
    //		break;
    //	case ATTYPE_INT:
    //		try{
    //		vint = new std::vector<long>();
    //		vint->resize(m_iNumRows, m_iNodata);
    //		} catch (std::exception& e) {NMErr(_ctxotbtab, << "Failed adding column: " << e.what());return false;}

    //		this->m_mIntCols.push_back(vint);
    //		this->m_vPosition.push_back(m_mIntCols.size()-1);
    //		break;
    //	case ATTYPE_DOUBLE:
    //		try{
    //		vdbl = new std::vector<double>();
    //		vdbl->resize(m_iNumRows, m_dNodata);
    //		} catch (std::exception& e) {NMErr(_ctxotbtab, << "Failed adding column: " << e.what());return false;}

    //		this->m_mDoubleCols.push_back(vdbl);
    //		this->m_vPosition.push_back(m_mDoubleCols.size()-1);
    //		break;
    //	default:
    //		break;
    //	}

    //	// update admin infos
    //	this->m_vNames.push_back(sColName);
    //	this->m_vTypes.push_back(eType);

	return true;
}

bool
AttributeTable::prepareBulkGet(const std::vector<std::string> &colNames,
                               const std::string& whereClause)
{
    if (m_db == 0)
    {
        return false;
    }

    std::stringstream ssql;
    ssql << "SELECT ";
    for (int c=0; c < colNames.size(); ++c)
    {
        ssql << colNames.at(c);
        if (c < colNames.size()-1)
        {
            ssql << ",";
        }
    }
    ssql << " from main.nmtab";

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
        return false;
    }

    return true;
}

bool
AttributeTable::prepareBulkSet(const std::vector<std::string>& colNames,
                             const bool& bInsert)
{
    if (m_db == 0)
    {
        return false;
    }

    m_vTypesBulkSet.clear();
    for (int i=0; i < colNames.size(); ++i)
    {
        const int idx = this->ColumnExists(colNames.at(i));
        if (idx < 0)
        {
            otbWarningMacro(<< "Column '" << colNames.at(i)
                            << "' does not exist in the table!");
            return false;
        }
        m_vTypesBulkSet.push_back(this->GetColumnType(idx));
    }

    std::stringstream ssql;
    if (bInsert)
    {
        ssql << "INSERT INTO main.nmtab (";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << colNames.at(c);
            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ") VALUES (";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << "?" << c+1;
            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << ");";
    }
    else
    {
        ssql << "UPDATE main.nmtab SET ";
        for (int c=0; c < colNames.size(); ++c)
        {
            ssql << colNames.at(c) << " = "
                 << "?" << c+1;
            if (c < colNames.size()-1)
            {
                ssql << ",";
            }
        }
        ssql << " WHERE rowidx = ?" << colNames.size()+1
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
        return false;
    }

    return true;
}

bool
AttributeTable::createIndex(const std::vector<std::string> &colNames)
{
    if (m_db == 0)
    {
        return false;
    }

    std::stringstream ssql;
    ssql << "CREATE INDEX main.nmtab_index_" << m_IndexNames.size()
         << " on nmtab (";
    for (int s=0; s < colNames.size(); ++s)
    {
        if (this->ColumnExists(colNames.at(s)) < 0)
        {
            otbWarningMacro(<< "Invalid Column Name!");
            return false;
        }

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

    return true;
}

bool
AttributeTable::doBulkGet(std::vector<std::string> & retStr)
{
    if (    m_db == 0
        ||  m_StmtBulkGet == 0
       )
    {
        return false;
    }

    int rc = sqlite3_step(m_StmtBulkGet);
    sqliteStepCheck(rc);

    if (rc == SQLITE_ROW)
    {
        for (int col=0; col < this->m_vNames.size(); ++col)
        {
            std::stringstream ssVal;
            ssVal << sqlite3_column_text(m_StmtBulkGet, col);
            retStr.push_back(ssVal.str());
        }
    }
    else
    {
        return false;
    }

    return true;
}


bool
AttributeTable::doBulkSet(const std::vector<std::string> &values, const int &row)
{
    if (    m_db == 0
        ||  m_StmtBulkSet == 0
        ||  values.size() != m_vTypesBulkSet.size()
       )
    {
        return false;
    }

    int rc;
    for (int i=0; i < values.size(); ++i)
    {
        switch(m_vTypesBulkSet.at(i))
        {
        case ATTYPE_DOUBLE:
            {
                const double val = ::atof(values.at(i).c_str());
                rc = sqlite3_bind_double(m_StmtBulkSet, i+1, val);
                if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_INT:
            {
                const long val = ::atol(values.at(i).c_str());
                rc = sqlite3_bind_int(m_StmtBulkSet, i+1, val);
                if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        case ATTYPE_STRING:
            {
                const char* val = values.at(i).c_str();
                rc = sqlite3_bind_text(m_StmtBulkSet, i+1, val, -1, 0);
                if (sqliteError(rc, &m_StmtBulkSet)) return false;
            }
            break;
        default:
            return false;
        }
    }

    if (row >= 0)
    {
        rc = sqlite3_bind_int(m_StmtBulkSet, values.size()+1, row);
        if (sqliteError(rc, &m_StmtBulkSet)) return false;
    }
    else
    {
        ++m_iNumRows;
    }

    rc = sqlite3_step(m_StmtBulkSet);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(m_StmtBulkSet);
    sqlite3_reset(m_StmtBulkSet);

    return true;
}

bool AttributeTable::AddRows(long numRows)
{
    // connection open?
    if (m_db == 0)
    {
        return false;
    }

	// check for presence of columns
    if (this->ColumnExists("rowidx") == -1)
		return false;

    int rc;
    int bufSize = 256;
    std::string ssql = "INSERT INTO main.nmtab (rowidx) VALUES (@IDX)";

    char* tail = 0;
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(m_db, ssql.c_str(), bufSize, &stmt, 0);
    if (sqliteError(rc, 0)) return false;

    bool bEndTransaction = false;
    if (!m_InTransaction)
    {
        if (!this->beginTransaction())
        {
            sqlite3_finalize(stmt);
            return false;
        }
        bEndTransaction = true;
    }

    for (long r=0; r < numRows; ++r, ++m_iNumRows)
    {
        sqlite3_bind_int(stmt, 1, m_iNumRows);
        sqlite3_step(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }

    if (bEndTransaction)
    {
        this->endTransaction();
    }

    // clean up the prepared statement
    sqlite3_finalize(stmt);

    //	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
    //	{
    //		const int& tidx = m_vPosition[colidx];
    //		switch (this->m_vTypes[colidx])
    //		{
    //		case ATTYPE_STRING:
    //			{
    //			m_mStringCols.at(tidx)->resize(m_iNumRows+numRows, m_sNodata);
    //			break;
    //			}
    //		case ATTYPE_INT:
    //			{
    //			m_mIntCols.at(tidx)->resize(m_iNumRows+numRows, m_iNodata);
    //			break;
    //			}
    //		case ATTYPE_DOUBLE:
    //			{
    //			m_mDoubleCols.at(tidx)->resize(m_iNumRows+numRows, m_dNodata);
    //			break;
    //			}
    //		default:
    //			return false;
    //		}
    //	}

	// increase the row number counter
    //this->m_iNumRows += numRows;

	return true;
}

bool AttributeTable::AddRow()
{
    // connection open?
    if (m_db == 0)
    {
        return false;
    }

    // check for presence of columns
    if (this->ColumnExists("rowidx") == -1)
        return false;

    std::stringstream ssql;
    ssql << "INSERT INTO main.nmtab (rowidx) VALUES ("
         << m_iNumRows << ");";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    if (sqliteError(rc, 0)) return false;

    ++m_iNumRows;


//	// check for presence of columns
//	if (this->m_vNames.size() == 0)
//		return false;

//	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
//	{
//		const int& tidx = m_vPosition[colidx];
//		switch (this->m_vTypes[colidx])
//		{
//		case ATTYPE_STRING:
//			{
//			m_mStringCols.at(tidx)->push_back(this->m_sNodata);
//			break;
//			}
//		case ATTYPE_INT:
//			{
//			m_mIntCols.at(tidx)->push_back(this->m_iNodata);
//			break;
//			}
//		case ATTYPE_DOUBLE:
//			{
//			m_mDoubleCols.at(tidx)->push_back(this->m_dNodata);
//			break;
//			}
//		default:
//			return false;
//		}
//	}

//	// increase the row number counter
//	++this->m_iNumRows;

	return true;
}

bool
AttributeTable::beginTransaction()
{
    if (m_db == 0 || m_InTransaction)
    {
        otbWarningMacro(<< "No database connection!");
        return false;
    }

    int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION;", 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        m_InTransaction = false;
        return false;
    }
    else
    {
        m_InTransaction = true;
        return true;
    }
}

bool
AttributeTable::endTransaction()
{
    if (m_db == 0)
    {
        otbWarningMacro(<< "No database connection!");
        return false;
    }

    int rc = sqlite3_exec(m_db, "END TRANSACTION;", 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        return false;
    }
    else
    {
        m_InTransaction = false;
        return true;
    }
}

void
AttributeTable::sqliteStepCheck(const int& rc)
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
AttributeTable::SetValue(const std::string& sColName, int idx, double value)
{
    // we just check for the database and leave the rest
    // to sqlite3
    if (m_db == 0)
    {
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);

    //    int rc = sqlite3_bind_text(m_vStmtUpdate.at(colidx), 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_double(stmt, 1, value);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, idx);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    //    std::stringstream ssql;
    //    ssql << "UPDATE main.nmtab SET "
    //         << sColName << " = "
    //         << value
    //         << " where rowidx = "
    //         << idx << ";";

    //    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    //    sqliteError(rc);


//	// get the column index
//	int colIdx = this->valid(sColName, idx);
//	if (colIdx < 0)
//		return;

//	const int& tidx = m_vPosition[colIdx];
//	switch (m_vTypes[colIdx])
//	{
//		case ATTYPE_STRING:
//		{
//			std::stringstream sval;
//			sval << value;
//			this->m_mStringCols.at(tidx)->at(idx) = sval.str();
//			break;
//		}
//		case ATTYPE_INT:
//		{
//			this->m_mIntCols.at(tidx)->at(idx) = value;
//			break;
//		}
//		case ATTYPE_DOUBLE:
//		{
//			this->m_mDoubleCols.at(tidx)->at(idx) = value;
//			break;
//		}
//		default:
//			break;
//	}
}

void
AttributeTable::SetValue(const std::string& sColName, int idx, long value)
{
    if (m_db == 0)
    {
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);


    //    int rc = sqlite3_bind_text(m_StmtUpdate, 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_int(stmt, 1, value);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, idx);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);



//	int colIdx = this->valid(sColName, idx);
//	if (colIdx < 0)
//		return;

//	const int& tidx = m_vPosition[colIdx];
//	switch (m_vTypes[colIdx])
//	{
//		case ATTYPE_STRING:
//		{
//			std::stringstream sval;
//			sval << value;
//			this->m_mStringCols.at(tidx)->at(idx) = sval.str();
//			break;
//		}
//		case ATTYPE_INT:
//		{
//			this->m_mIntCols.at(tidx)->at(idx) = value;
//			break;
//		}
//		case ATTYPE_DOUBLE:
//		{
//			this->m_mDoubleCols.at(tidx)->at(idx) = value;
//			break;
//		}
//		default:
//			break;
//	}
}

void
AttributeTable::SetValue(const std::string& sColName, int idx, std::string value)
{
    if (m_db == 0)
    {
        return;
    }

    const int& colidx = this->ColumnExists(sColName);
    if (colidx < 0)
    {
        return;
    }
    sqlite3_stmt* stmt = m_vStmtUpdate.at(colidx);

    //    int rc = sqlite3_bind_text(m_StmtUpdate, 1, sColName.c_str(), -1, 0);
    //    if (sqliteError(rc, &m_StmtUpdate)) return;

    int rc = sqlite3_bind_text(stmt, 1, value.c_str(), -1, 0);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, idx);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);


//    std::stringstream ssql;
//    ssql << "UPDATE main.nmtab SET "
//         << sColName << " = "
//         << value
//         << " where rowidx = "
//         << idx << ";";

//    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
//    sqliteError(rc);

    //	int colIdx = this->valid(sColName, idx);
    //	if (colIdx < 0)
    //		return;

    //	const int& tidx = m_vPosition[colIdx];

    //	switch (m_vTypes[colIdx])
    //	{
    //		case ATTYPE_STRING:
    //		{
    //			this->m_mStringCols.at(tidx)->at(idx) = value;
    //			break;
    //		}
    //		case ATTYPE_INT:
    //		{
    //			this->m_mIntCols.at(tidx)->at(idx) = ::strtol(value.c_str(),0,10);
    //			break;
    //		}
    //		case ATTYPE_DOUBLE:
    //		{
    //			this->m_mDoubleCols.at(tidx)->at(idx) = ::strtod(value.c_str(),0);
    //			break;
    //		}
    //		default:
    //			break;
    //	}
}

double AttributeTable::GetDblValue(const std::string& sColName, int idx)
{
	//check for valid name and index parameters
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_dNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int(stmt, 1, idx);
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

    //	const int& tidx = m_vPosition[colidx];
    //	double ret;
    //	switch(m_vTypes[colidx])
    //	{
    //		case ATTYPE_STRING:
    //			ret = ::strtod(this->m_mStringCols.at(tidx)->at(idx).c_str(),0);
    //			break;
    //		case ATTYPE_INT:
    //			ret = this->m_mIntCols.at(tidx)->at(idx);
    //			break;
    //		case ATTYPE_DOUBLE:
    //			ret = this->m_mDoubleCols.at(tidx)->at(idx);
    //			break;
    //		default:
    //			ret = this->m_dNodata;
    //			break;
    //	}

    //	return ret;
}

long
AttributeTable::GetIntValue(const std::string& sColName, int idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_iNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int(stmt, 1, idx);
    if (sqliteError(rc, &stmt)) return m_iNodata;

    long ret = m_iNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;

    //	const int& tidx = m_vPosition[colidx];
    //	long ret;
    //	switch(m_vTypes[colidx])
    //	{
    //		case ATTYPE_STRING:
    //			ret = ::strtol(this->m_mStringCols.at(tidx)->at(idx).c_str(),0,10);
    //			break;
    //		case ATTYPE_INT:
    //			ret = this->m_mIntCols.at(tidx)->at(idx);
    //			break;
    //		case ATTYPE_DOUBLE:
    //			ret = this->m_mDoubleCols.at(tidx)->at(idx);
    //			break;
    //		default:
    //			ret = this->m_iNodata;
    //			break;
    //	}

    //	return ret;
}

std::string
AttributeTable::GetStrValue(const std::string& sColName, int idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_sNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(colidx);
    int rc = sqlite3_bind_int(stmt, 1, idx);
    if (sqliteError(rc, &stmt)) return m_sNodata;

    std::stringstream ret;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char* sval = sqlite3_column_text(stmt, 0);
        ret << sval;
    }
    else
    {
        ret << m_sNodata;
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret.str();


    //	const int& tidx = m_vPosition[colidx];
    //	std::stringstream ret;
    //	switch(m_vTypes[colidx])
    //	{
    //		case ATTYPE_STRING:
    //			ret << this->m_mStringCols.at(tidx)->at(idx);
    //			break;
    //		case ATTYPE_INT:
    //			ret << this->m_mIntCols.at(tidx)->at(idx);
    //			break;
    //		case ATTYPE_DOUBLE:
    //			ret << this->m_mDoubleCols.at(tidx)->at(idx);
    //			break;
    //		default:
    //			ret << this->m_sNodata;
    //			break;
    //	}

    //	return ret.str();
}

void*
AttributeTable::GetColumnPointer(int col)
{
	void* ret = 0;

	if (col < 0 || col >= m_vNames.size())
		return ret;

	if (m_iNumRows == 0)
		return ret;

	const int& tidx = m_vPosition[col];
	switch(m_vTypes[col])
	{
	case ATTYPE_STRING:
		ret = (void*)&this->m_mStringCols.at(tidx)->at(0);
		break;
	case ATTYPE_INT:
		ret = (void*)&this->m_mIntCols.at(tidx)->at(0);
		break;
	case ATTYPE_DOUBLE:
		ret = (void*)&this->m_mDoubleCols.at(tidx)->at(0);
		break;
	default:
		break;
	}

	return ret;
}

long
AttributeTable::GetRowIdx(const std::string& column, void* value)
{
	long idx = -1;

	int colidx = ColumnExists(column);
	if (colidx < 0)
		return idx;

    sqlite3_stmt* stmt = m_vStmtGetRowidx.at(colidx);
    int rc;
    switch(m_vTypes[colidx])
    {
    case ATTYPE_DOUBLE:
        rc = sqlite3_bind_double(stmt, 1, *(static_cast<double*>(value)));
        break;
    case ATTYPE_INT:
        rc = sqlite3_bind_int(stmt, 1, *(static_cast<long*>(value)));
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
        idx = sqlite3_column_int(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return idx;


    //	switch(m_vTypes[colidx])
    //	{
    //	case ATTYPE_STRING:
    //		{
    //			std::string* strings = static_cast<std::string*>(GetColumnPointer(colidx));
    //			std::string* strVal = static_cast<std::string*>(value);
    //			for (int r=0; r < m_iNumRows; ++r)
    //			{
    //				if (*strVal == strings[r])
    //				{
    //					idx = r;
    //					break;
    //				}
    //			}
    //		}
    //		break;

    //	case ATTYPE_INT:
    //		{
    //			long* longValues = static_cast<long*>(GetColumnPointer(colidx));
    //			long* longVal = static_cast<long*>(value);
    //			for (int r=0; r < m_iNumRows; ++r)
    //			{
    //				if (*longVal == longValues[r])
    //				{
    //					idx = r;
    //					break;
    //				}
    //			}
    //		}
    //		break;

    //	case ATTYPE_DOUBLE:
    //		{
    //			double* doubleValues = static_cast<double*>(GetColumnPointer(colidx));
    //			double* doubleVal = static_cast<double*>(value);
    //			for (int r=0; r < m_iNumRows; ++r)
    //			{
    //				if (*doubleVal == doubleValues[r])
    //				{
    //					idx = r;
    //					break;
    //				}
    //			}
    //		}
    //		break;
    //	}

    //	return idx;
}


void AttributeTable::SetBandNumber(int iBand)
{
	if (iBand > 0)
		this->m_iBand = iBand;
}

void AttributeTable::SetImgFileName(const std::string& sFileName)
{
	this->m_sImgName = sFileName;
}

std::string
AttributeTable::GetColumnName(int idx)
{
	std::string ret = "";
	if (idx < 0 || idx > m_vNames.size()-1)
		return ret;
	return m_vNames.at(idx);
}

AttributeTable::TableColumnType
AttributeTable::GetColumnType(int idx)
{
	if (idx < 0 || idx > m_vNames.size()-1)
		return ATTYPE_UNKNOWN;
	return m_vTypes.at(idx);
}

void
AttributeTable::SetColumnName(int col, const std::string& name)
{
	if (	col < 0
		||  col > this->m_vNames.size()-1
		||  name.empty()
	   )
		return;

	this->m_vNames[col] = name;
}

bool
AttributeTable::RemoveColumn(int col)
{
	if (col < 0 || col > this->m_vNames.size()-1)
		return false;

    this->RemoveColumn(m_vNames[col]);

    //	int tidx = m_vPosition[col];
    //	switch(this->m_vTypes[col])
    //	{
    //	case ATTYPE_INT:
    //		delete this->m_mIntCols.at(tidx);
    //		this->m_mIntCols.erase(this->m_mIntCols.begin()+tidx);
    //		break;
    //	case ATTYPE_DOUBLE:
    //		delete this->m_mDoubleCols.at(tidx);
    //		this->m_mDoubleCols.erase(this->m_mDoubleCols.begin()+tidx);
    //		break;
    //	case ATTYPE_STRING:
    //		delete this->m_mStringCols.at(tidx);
    //		this->m_mStringCols.erase(this->m_mStringCols.begin()+tidx);
    //		break;
    //	default:
    //		return false;
    //		break;
    //	}

    //	// house keeping: adjust type specific array indices
    //	// for each column of the same type to the right of
    //	// the one being removed
    //	for (int c=col+1; c < this->m_vNames.size(); ++c)
    //	{
    //		if (m_vTypes[c] == m_vTypes[col])
    //		{
    //			--m_vPosition[c];
    //		}
    //	}

	// now remove any traces of the column in the admin arrays
    //	this->m_vNames.erase(this->m_vNames.begin() + col);
    //	this->m_vTypes.erase(this->m_vTypes.begin() + col);
    //	this->m_vPosition.erase(this->m_vPosition.begin() + col);

	return true;
}

bool
AttributeTable::RemoveColumn(const std::string& name)
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
        ssql << colsvec.at(c);
        if (c < colsvec.size()-1)
        {
            collist << ",";
        }
    }

    ssql << "BEGIN TRANSACTION;"
         << "CREATE TEMPORARY TABLE main.t1_backup(" << collist << ");"
         << "INSERT INTO main.t1_backup SELECT "     << collist << " FROM main.nmtab;"
         << "DROP TABLE main.nmtab;"
         << "CREATE TABLE main.nmtab(" << collist << ");"
         << "INSERT INTO main.nmtab SELECT " << collist << " FROM main.t1_backup;"
         << "DROP TABLE t1_backup"
         << "END TRANSACTION";

    int rc = sqlite3_exec(m_db, ssql.str().c_str(), 0, 0, 0);
    if (sqliteError(rc, 0))
    {
        return false;
    }

    this->m_vNames.erase(m_vNames.begin()+idx);
    this->m_vTypes.erase(m_vTypes.begin()+idx);

    return true;


//	int idx = this->ColumnExists(name);
//	if (idx < 0)
//		return false;

//	return this->RemoveColumn(idx);
}

void AttributeTable::SetValue(int col, int row, double value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_double(stmt, 1, value);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, row);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);


//	const int& tidx = m_vPosition[col];
//	switch (m_vTypes[col])
//	{
//		case ATTYPE_STRING:
//		{
//			std::stringstream sval;
//			sval << value;
//			this->m_mStringCols.at(tidx)->at(row) = sval.str();
//			break;
//		}
//		case ATTYPE_INT:
//		{
//			this->m_mIntCols.at(tidx)->at(row) = value;
//			break;
//		}
//		case ATTYPE_DOUBLE:
//		{
//			this->m_mDoubleCols.at(tidx)->at(row) = value;
//			break;
//		}
//		default:
//			break;
//	}
}

void AttributeTable::SetValue(int col, int row, long value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_int(stmt, 1, value);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, row);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);


    //	const int& tidx = m_vPosition[col];
    //	switch (m_vTypes[col])
    //	{
    //		case ATTYPE_STRING:
    //		{
    //			std::stringstream sval;
    //			sval << value;
    //			this->m_mStringCols.at(tidx)->at(row) = sval.str();
    //			break;
    //		}
    //		case ATTYPE_INT:
    //		{
    //			this->m_mIntCols.at(tidx)->at(row) = value;
    //			break;
    //		}
    //		case ATTYPE_DOUBLE:
    //		{
    //			this->m_mDoubleCols.at(tidx)->at(row) = value;
    //			break;
    //		}
    //		default:
    //			break;
    //	}
}

void AttributeTable::SetValue(int col, int row, std::string value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

    sqlite3_stmt* stmt = m_vStmtUpdate.at(col);

    int rc = sqlite3_bind_text(stmt, 1, value.c_str(), -1, 0);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_bind_int(stmt, 2, row);
    if (sqliteError(rc, &stmt)) return;

    rc = sqlite3_step(stmt);
    sqliteStepCheck(rc);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);


//	const int& tidx = m_vPosition[col];
//	switch (m_vTypes[col])
//	{
//		case ATTYPE_STRING:
//		{
//			this->m_mStringCols.at(tidx)->at(row) = value;
//			break;
//		}
//		case ATTYPE_INT:
//		{
//			this->m_mIntCols.at(tidx)->at(row) = ::strtol(value.c_str(), 0, 10);
//			break;
//		}
//		case ATTYPE_DOUBLE:
//		{
//			this->m_mDoubleCols.at(tidx)->at(row) = ::strtod(value.c_str(), 0);
//			break;
//		}
//		default:
//			break;
//	}
}

double AttributeTable::GetDblValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_dNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_dNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int(stmt, 1, row);
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


//	const int& tidx = m_vPosition[col];
//	double ret;
//	switch(m_vTypes[col])
//	{
//		case ATTYPE_STRING:
//			ret = ::strtod(this->m_mStringCols.at(tidx)->at(row).c_str(),0);
//			break;
//		case ATTYPE_INT:
//			ret = this->m_mIntCols.at(tidx)->at(row);
//			break;
//		case ATTYPE_DOUBLE:
//			ret = this->m_mDoubleCols.at(tidx)->at(row);
//			break;
//		default:
//			ret = this->m_dNodata;
//			break;
//	}

//	return ret;
}

long AttributeTable::GetIntValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_iNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_iNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int(stmt, 1, row);
    if (sqliteError(rc, &stmt)) return m_iNodata;

    long ret = m_iNodata;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int(stmt, 0);
    }
    else
    {
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret;


    //	const int& tidx = m_vPosition[col];
    //	long ret;
    //	switch(m_vTypes[col])
    //	{
    //		case ATTYPE_STRING:
    //			ret = ::strtol(this->m_mStringCols.at(tidx)->at(row).c_str(),0,10);
    //			break;
    //		case ATTYPE_INT:
    //			ret = this->m_mIntCols.at(tidx)->at(row);
    //			break;
    //		case ATTYPE_DOUBLE:
    //			ret = this->m_mDoubleCols.at(tidx)->at(row);
    //			break;
    //		default:
    //			ret = this->m_iNodata;
    //			break;
    //	}

    //	return ret;

}

std::string AttributeTable::GetStrValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_sNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_sNodata;

    sqlite3_stmt* stmt = m_vStmtSelect.at(col);
    int rc = sqlite3_bind_int(stmt, 1, row);
    if (sqliteError(rc, &stmt)) return m_sNodata;

    std::stringstream ret;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char* sval = sqlite3_column_text(stmt, 0);
        ret << sval;
    }
    else
    {
        ret << m_sNodata;
        sqliteStepCheck(rc);
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return ret.str();


//	const int& tidx = m_vPosition[col];
//	std::stringstream ret;
//	switch(m_vTypes[col])
//	{
//		case ATTYPE_STRING:
//			ret << this->m_mStringCols.at(tidx)->at(row);
//			break;
//		case ATTYPE_INT:
//			ret << this->m_mIntCols.at(tidx)->at(row);
//			break;
//		case ATTYPE_DOUBLE:
//			ret << this->m_mDoubleCols.at(tidx)->at(row);
//			break;
//		default:
//			ret << this->m_sNodata;
//			break;
//	}

//	return ret.str();

}

int AttributeTable::GetBandNumber(void)
{
	return this->m_iBand;
}

std::string AttributeTable::GetImgFileName(void)
{
	return this->m_sImgName;
}

// ------------------------------------------------------- other useful public functions
void AttributeTable::Print(std::ostream& os, itk::Indent indent, int nrows)
{
	os << indent << "\n";
	os << indent << "OTB raster attribute table print-out\n";
	os << indent << "File: " <<	this->m_sImgName << "\n";
	os << indent << "Band: " << this->m_iBand << "\n";
	os << indent << "Columns: " << this->m_vNames.size() << "\n";
	os << indent << "Rows: " << this->m_iNumRows << "\n";
	os << "\n";

	itk::Indent ii = indent.GetNextIndent();
	itk::Indent iii = ii.GetNextIndent();

	os << indent << "Columns:\n";
	for (int c = 0; c < this->m_vNames.size(); c++)
		os << ii << c << ": " << this->m_vNames[c] << " (" << typestr(this->m_vTypes[c]) << ")\n";

	os << indent << "Rows:\n";
	char val[256];
	nrows = nrows < this->m_iNumRows ? nrows : this->m_iNumRows;
	for (int r=0; r < nrows; ++r)
	{
		os << ii << "#" << r << " - ROW\n";
		for (int c=0; c < this->m_vNames.size(); c++)
		{
			switch (this->GetColumnType(c))
			{
			case ATTYPE_DOUBLE:
				::sprintf(val, "%g", this->GetDblValue(c, r));
				break;
			case ATTYPE_INT:
				::sprintf(val, "%ld", this->GetIntValue(c, r));
				break;
			case ATTYPE_STRING:
				::sprintf(val, "%s", this->GetStrValue(c, r).c_str());
				break;
			default:
				::sprintf(val, "%s", this->m_sNodata.c_str());
			}
			os << iii << this->m_vNames[c] << ": " << val << "\n";
		}
	}
}

void AttributeTable::PrintStructure(std::ostream& os, itk::Indent indent)
{
	  itk::Indent i = indent;
	  itk::Indent ii = i.GetNextIndent();
	  itk::Indent iii = ii.GetNextIndent();
	  os << i << "Raster Attribute Table, band #" << this->GetBandNumber() << " ('" <<
			  this->GetImgFileName() << "')\n";
	  os << ii << "#columns: " << this->GetNumCols() << ", #rows: " << this->GetNumRows() << "\n";
	  os << ii << "ColumnIndex: ColumnName (ColumnType):\n";

	  for (int c = 0; c < m_vNames.size(); c++)
		  os << iii << c << ": " << m_vNames[c] << " (" << typestr(m_vTypes[c]) << ")\n";
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ---------------------  PROTECTED FUNCTIONS -----------------------------------------------

std::string AttributeTable::typestr(TableColumnType type)
{
	switch(type)
	{
	case ATTYPE_DOUBLE:
		return "double";
		break;
	case ATTYPE_INT:
		return "integer";
		break;
	case ATTYPE_STRING:
		return "string";
		break;
	default:
		return "unknown";
	}
}

int
AttributeTable::valid(const std::string& sColName, int idx)
{
	//check if the column exists -> column exists returns the zero-based index of the
	//particular column within the set of columns or -1 if the column does not exist
	int colidx = ColumnExists(sColName);
	if (colidx < 0)
		return -1;

	//check for the index
	if (idx > m_iNumRows - 1 || idx < 0)
		return -1;

	return colidx;
}

void
AttributeTable::createTable(std::string filename)
{
    this->DebugOn();

    std::stringstream uri;
    m_dbFileName = std::tmpnam(0);
    m_dbFileName += ".db";

    itkDebugMacro(<< "temp database: " << m_dbFileName);

    // ============================================================
    // create the host data base
    // ============================================================
    int rc = ::sqlite3_open_v2(m_dbFileName.c_str(),
                               &m_db,
                               SQLITE_OPEN_URI |
                               SQLITE_OPEN_READWRITE |
                               SQLITE_OPEN_CREATE |
                               SQLITE_OPEN_SHAREDCACHE,
                               0);
    if (rc != SQLITE_OK)
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        itkDebugMacro(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
        //itkExceptionMacro(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
        m_dbFileName.clear();
        ::sqlite3_close(m_db);
        m_db = 0;
        return;
    }

    // ============================================================
    // create the nmtab table
    // ============================================================
    uri.str("");
    uri << "begin transaction;";
    uri << "CREATE TABLE nmtab (rowidx INTEGER PRIMARY KEY);";
    uri << "commit;";

    char* errMsg = 0;
    rc = ::sqlite3_exec(m_db, uri.str().c_str(), 0, 0, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::string errmsg = sqlite3_errmsg(m_db);
        itkDebugMacro(<< "SQLite3 ERROR #" << rc << ": " << errmsg);
        m_dbFileName.clear();
        ::sqlite3_close(m_db);
        m_db = 0;
    }

    // add rowidx column to list of columns
    m_vNames.push_back("rowidx");
    m_vTypes.push_back(AttributeTable::ATTYPE_INT);

    //==================================================
    // 'dummy' statements for rowidx set/get
    //==================================================

    // we prepare those  statements simply for synchorinsing
    // column index with vector indices for the prepared
    // statements (update & select); actually it doesn't really
    // make sense to use those, however you never know what the
    // user wants and this ways we avoid any segfaults

    // prepare an update statement for this column
    sqlite3_stmt* stmt_upd;
    std::stringstream ssql;
    ssql <<  "UPDATE main.nmtab SET rowidx = "
         <<  "@VAL WHERE rowidx = @IDX ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            1024, &stmt_upd, 0);
    sqliteError(rc, &stmt_upd);
    this->m_vStmtUpdate.push_back(stmt_upd);

    // prepare a get value statement for this column
    sqlite3_stmt* stmt_sel;
    ssql.str("");
    ssql <<  "SELECT rowidx from main.nmtab"
         <<  " WHERE rowidx = @IDX ;";
    rc = sqlite3_prepare_v2(m_db, ssql.str().c_str(),
                            1024, &stmt_sel, 0);
    sqliteError(rc, &stmt_sel);
    this->m_vStmtSelect.push_back(stmt_sel);


    // ============================================================
    // prepare statements for recurring tasks
    // ============================================================
    //    rc = sqlite3_prepare_v2(m_db, "ALTER TABLE main.nmtab ADD @COL @TYP ;",
    //                                256, &m_StmtAddColl, 0);
    //    sqliteError(rc, &m_StmtAddColl);

    //    rc = sqlite3_prepare_v2(m_db, "UPDATE main.nmtab SET @COL = @VAL WHERE rowidx = @IDX ;",
    //                            1024, &m_StmtUpdate, 0);
    //    sqliteError(rc, &m_StmtUpdate);

    rc = sqlite3_prepare_v2(m_db, "BEGIN TRANSACTION;", 100, &m_StmtBegin, 0);
    sqliteError(rc, &m_StmtBegin);

    rc = sqlite3_prepare_v2(m_db, "END TRANSACTION;", 100, &m_StmtEnd, 0);
    sqliteError(rc, &m_StmtEnd);

    rc = sqlite3_prepare_v2(m_db, "ROLLBACK TRANSACTION;", 100, &m_StmtRollback, 0);
    sqliteError(rc, &m_StmtRollback);

}

AttributeTable::AttributeTable()
	: m_iNumRows(0),
	  m_iBand(1),
	  m_iNodata(-std::numeric_limits<long>::max()),
	  m_dNodata(-std::numeric_limits<double>::max()),
      m_sNodata("NULL"),
      m_db(0),
      m_InTransaction(false),
      m_StmtBegin(0),
      m_StmtEnd(0),
      m_StmtRollback(0),
      m_StmtBulkSet(0),
      m_StmtBulkGet(0)
{
    this->createTable("");
}

// clean up
AttributeTable::~AttributeTable()
{
    //sqlite3_finalize(m_StmtAddColl);
    //sqlite3_finalize(m_StmtUpdate);
    sqlite3_finalize(m_StmtBegin);
    sqlite3_finalize(m_StmtEnd);
    sqlite3_finalize(m_StmtRollback);
    sqlite3_finalize(m_StmtBulkSet);
    sqlite3_finalize(m_StmtBulkGet);

    for (int v=0; v < m_vStmtUpdate.size(); ++v)
    {
        sqlite3_finalize(m_vStmtUpdate.at(v));
        sqlite3_finalize(m_vStmtSelect.at(v));
    }

    if (m_db != 0)
    {
        ::sqlite3_close(m_db);
    }
    m_db = 0;


	for (int v=0; v < m_mStringCols.size(); ++v)
		delete m_mStringCols[v];

	for (int v=0; v < m_mIntCols.size(); ++v)
		delete m_mIntCols[v];

	for (int v=0; v < m_mDoubleCols.size(); ++v)
		delete m_mDoubleCols[v];
}


} // end of namespace otb
