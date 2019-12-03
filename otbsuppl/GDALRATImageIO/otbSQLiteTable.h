 /******************************************************************************
 * Created by Alexander Herzig
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
 * SQLiteTable.h
 *
 *  Created on: 23/09/2015
 *      Author: Alexander Herzig
 */

#ifndef SQLITETABLE_H_
#define SQLITETABLE_H_

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sqlite3.h>

#include "otbAttributeTable.h"
#include "itkObject.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"

#include "otbgdalratimageio_export.h"

/** \brief Attribute table implementation based on
 *         sqlite database.
 *
 *
 *
 *
 *
 */

namespace otb
{

class OTBGDALRATIMAGEIO_EXPORT SQLiteTable : public AttributeTable
{
public:
	/** Standard class typedefs. */
    typedef SQLiteTable             		Self;
	typedef itk::DataObject					Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

    /** Indicates table status after open/create */
    typedef enum
    {
        ATCREATE_CREATED = 0,
        ATCREATE_READ,
        ATCREATE_ERROR
    } TableCreateStatus;


    //itkNewMacro(Self);
    static Pointer New();
    itkTypeMacro(SQLiteTable, Superclass);

	// getting info about the table
    long long GetNumRows();
    long long int GetRowIdx(const std::string& column, void* value);

	//long GetRowIdx(const std::string& column, const double& value);
	//long GetRowIdx(const std::string& column, const long& value);
	//long GetRowIdx(const std::string& column, const std::string& value);

	// managing the attribute table's content
    bool isRowidColumn(const std::string& sColName);
    bool AddColumn(const std::string &sColName, TableColumnType type)
        {return AddConstrainedColumn(sColName, type, "");}
    bool AddConstrainedColumn(const std::string& sColName, TableColumnType type,
                   const std::string& sColConstraint="");
    void SetValue(const std::string& sColName, long long idx, double value);
    void SetValue(const std::string& sColName, long long idx, long long value);
    void SetValue(const std::string& sColName, long long idx, std::string value);

    void SetValue(const std::string& sColName, const std::string& whereClause, double value);
    void SetValue(const std::string& sColName, const std::string& whereClause, long long value);
    void SetValue(const std::string& sColName, const std::string& whereClause, std::string value);


    double GetDblValue(const std::string& sColName, long long idx);
    long long GetIntValue(const std::string& sColName, long long idx);
    std::string GetStrValue(const std::string& sColName, long long idx);

    double GetDblValue(const std::string& sColName, const std::string& whereClause);
    long long GetIntValue(const std::string& sColName, const std::string& whereClause);
    std::string GetStrValue(const std::string& sColName, const std::string& whereClause);

    void SetValue(int col, long long row, double value);
    void SetValue(int col, long long row, long long value);
    void SetValue(int col, long long row, std::string value);

    //void SetColumnName(int col, const std::string& name);

    double GetDblValue(int col, long long row);
    long long GetIntValue(int col, long long row);
    std::string GetStrValue(int col, long long row);

	bool RemoveColumn(int col);
	bool RemoveColumn(const std::string& name);

//	// print the table
//	void Print(std::ostream& os, itk::Indent indent, int nrows);
//	void PrintStructure(std::ostream& os, itk::Indent indent);

    /// SQLite support functions
    void SetUseSharedCache(bool shared) {m_bUseSharedCache = shared;}
    bool GetUseSharedCache(void){return m_bUseSharedCache;}
    void SetOpenReadOnly(bool readonly) {m_bOpenReadOnly = readonly;}
    bool GetOpenReadOnlyFlag(void){return m_bOpenReadOnly;}
    bool SetTableName(const std::string& tableName);
    bool SetDbFileName(const std::string& dbFileName);

    void SetRowIdColNameIsPersistent(bool bPersistent)
    {m_bPersistentRowIdColName = bPersistent;}

    bool openConnection();
    void disconnectDB();
    TableCreateStatus CreateTable(std::string filename, std::string tag="");
    bool CreateFromVirtual(const std::string& fileName, const std::string& encoding =
            "UTF-8", const int& srid = -1);
    void CloseTable(bool drop=false);
    bool SetRowIDColName(const std::string& name);
    bool DeleteDatabase(void);

    static std::string GetRandomString(int len);
    std::string GetDbFileName() {return this->m_dbFileName;}
    std::string GetTableName() {return this->m_tableName;}
    std::vector<std::string> GetFilenameInfo(const std::string& fileName);
    long long GetMinPKValue();
    long long GetMaxPKValue();

    sqlite3* GetDbConnection() {return this->m_db;}

    bool PrepareBulkGet(const std::vector<std::string>& colNames, const std::string& whereClause="");

    bool PrepareAutoBulkSet(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& autoValue,
                        const std::vector<std::vector<TableColumnType> >& autoTypes,
                        const bool& bInsert=true);

    bool PrepareBulkSet(const std::vector<std::string>& colNames,
                        const bool& bInsert=true);

    bool DoPtrBulkSet(std::vector< int* >& intVals,
                      std::vector< double* >& dblVals,
                      std::vector< char** >& chrVals,
                      std::vector< int >& colpos,
                      const int & chunkrow,
                      const long long int &row=-1);

    bool DoBulkSet(std::vector< ColumnValue >& values, const long long int& row=-1);

    bool DoBulkGet(std::vector< ColumnValue >& values);
    bool BeginTransaction();
    bool EndTransaction();
    bool CreateIndex(const std::vector<std::string>& colNames, bool unique,
                     const std::string& table="", const std::string& db = "main");

    bool GreedyNumericFetch(const std::vector<std::string>& columns,
                            std::map<int, std::map<long, double> >& valstore);

    /// more 'free-style' sql support
    std::vector<std::string> GetTableList(void);
    bool SqlExec(const std::string& sqlstr);
    bool AttachDatabase(const std::string &fileName, const std::string& dbName);
    bool DetachDatabase(const std::string& dbName);
    bool DropTable(const std::string& tablename="");
    bool FindTable(const std::string& tableName);
    bool JoinAttributes(const std::string& targetTable,
                        const std::string& targetJoinField,
                        const std::string& sourceDbFileName,
                        const std::string& sourceTable,
                        const std::string& sourceJoinField,
                        std::vector<std::string> sourceFields);
    // repopulates the admin structures for the current table (e.g. SetTableName);
    bool PopulateTableAdmin();
    bool loadExtension(const std::string& lib, const std::string& entry);

    bool openAsInMemDb(const std::string& dbName, const std::string& tablename);




    /// FAST INLINE ACCESS TO COLUMN VALUES
    bool PrepareColumnByIndex(const std::string& colname);//, const std::string& whereClause);
    bool PrepareColumnIterator(const std::string& colname, const std::string& whereClause="");

    double NextDoubleValue(const long long& row)
    {
        sqlite3_bind_int64(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            return m_dNodata;
        }
        double v = sqlite3_column_double(m_StmtColIter, 0);
        sqlite3_clear_bindings(m_StmtColIter);
        sqlite3_reset(m_StmtColIter);

        return v;

    }

    double NextDoubleValue(bool& bOK)
    {
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            bOK = false;
            return m_dNodata;
        }
        bOK = true;
        return sqlite3_column_double(m_StmtColIter, 0);
    }


    long long NextIntValue(const long long& row)
    {
        sqlite3_bind_int64(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            return m_iNodata;
        }
        long long v = sqlite3_column_int(m_StmtColIter, 0);
        sqlite3_clear_bindings(m_StmtColIter);
        sqlite3_reset(m_StmtColIter);

        return v;
    }

    long long NextIntValue(bool& bOK)
    {
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            bOK = false;
            return m_iNodata;
        }
        bOK = true;
        return sqlite3_column_int(m_StmtColIter, 0);
    }

    const char* NextTextValue(const long long& row)
    {
        sqlite3_bind_int64(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            return m_sNodata.c_str();
        }
        const char* v = reinterpret_cast<const char*>(
                        sqlite3_column_text(m_StmtColIter, 0));
        sqlite3_clear_bindings(m_StmtColIter);
        sqlite3_reset(m_StmtColIter);

        return v;
    }

    const char* NextTextValue(bool& bOK)
    {
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            bOK = false;
            return m_sNodata.c_str();
        }
        bOK = true;
        return reinterpret_cast<const char*>(
                    sqlite3_column_text(m_StmtColIter, 0));
    }

    std::string getLastLogMsg(void){return m_lastLogMsg;}

protected:
        SQLiteTable();
    virtual ~SQLiteTable();

    /*! SQLite foundation of otbSQLiteTable
     */

    void createPreparedColumnStatements(const std::string& colname);
    void resetTableAdmin();

	/*! deletes the ldb table if the ldb file has a more recent modified data; 
	 *  returns 1 when ldb is deleted or did not exist 
	 *  returns 0 when existing ldb is kept
	 *  returns -1 when provided 'vt' is not accessible
	 */
	int deleteOldLDB(const std::string& vt, const std::string& ldb);


	/*! replace any char in 
	 *	{ '-', '.', '+', '*', '/', '%', '|', '<', '>', '=', '!', '~'},
	 *  i.e. operator characters or a leading digit with '_' or double
	 *  quote any keyword
	 */
	// awesome function - we're actually not using it, we just double quote
	// any table name identifier!
	//std::string formatTableName(const std::string& tableName);
    long long GetMinMaxPKValue(bool bmax);

    inline bool sqliteError(const int& rc, sqlite3_stmt** stmt);
    inline void sqliteStepCheck(const int& rc);

    std::string m_lastLogMsg;

    bool m_bUseSharedCache;
    bool m_bOpenReadOnly;

    bool m_bPersistentRowIdColName;

    sqlite3* m_db;
    std::string m_dbFileName;
    std::string m_tableName;

    std::vector<sqlite3_stmt*> m_vStmtUpdate;
    std::vector<sqlite3_stmt*> m_vStmtSelect;
    std::vector<sqlite3_stmt*> m_vStmtGetRowidx;

    sqlite3_stmt* m_StmtBulkSet;
    sqlite3_stmt* m_StmtBulkGet;
    int m_iStmtBulkGetNumParam;

    std::vector<otb::SQLiteTable::TableColumnType> m_vTypesBulkSet;
    std::vector<otb::SQLiteTable::TableColumnType> m_vTypesBulkGet;
    std::vector<std::string> m_vIndexNames;

    sqlite3_stmt* m_StmtColIter;
    sqlite3_stmt* m_StmtRowCount;

    sqlite3_stmt* m_StmtBegin;
    sqlite3_stmt* m_StmtEnd;
    sqlite3_stmt* m_StmtRollback;

    const char* m_CurPrepStmt;
	void* m_SpatialiteCache;

};

}

#endif /* SQLITETABLE_H_ */
