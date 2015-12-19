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


    itkNewMacro(Self);
    itkTypeMacro(SQLiteTable, Superclass);

	// getting info about the table
    long long GetNumRows();
    long long int GetRowIdx(const std::string& column, void* value);

	//long GetRowIdx(const std::string& column, const double& value);
	//long GetRowIdx(const std::string& column, const long& value);
	//long GetRowIdx(const std::string& column, const std::string& value);

	// managing the attribute table's content
    bool AddColumn(const std::string &sColName, TableColumnType type)
        {return AddConstrainedColumn(sColName, type, "");}
    bool AddConstrainedColumn(const std::string& sColName, TableColumnType type,
                   const std::string& sColConstraint="");
    void SetValue(const std::string& sColName, long long idx, double value);
    void SetValue(const std::string& sColName, long long idx, long long value);
    void SetValue(const std::string& sColName, long long idx, std::string value);
    double GetDblValue(const std::string& sColName, long long idx);
    long long GetIntValue(const std::string& sColName, long long idx);
    std::string GetStrValue(const std::string& sColName, long long idx);

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
    void SetOpenReadOnly(bool readonly) {m_bOpenReadOnly = readonly;}
    TableCreateStatus CreateTable(std::string filename, std::string tag="");
    bool CreateFromVirtual(const std::string& fileName, const std::string& encoding =
            "UTF-8", const int& srid = -1);
    void CloseTable(bool drop=false);
    bool SetRowIDColName(const std::string& name);
    bool DeleteDatabase(void);

    std::string GetRandomString(int len);
    std::string GetDbFileName() {return this->m_dbFileName;}
    std::string GetTableName() {return this->m_tableName;}
    std::vector<std::string> GetFilenameInfo(const std::string& fileName);

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
    bool CreateIndex(const std::vector<std::string>& colNames, bool unique);

    bool GreedyNumericFetch(const std::vector<std::string>& columns,
                            std::map<int, std::map<long, double> >& valstore);

    /// more 'free-style' sql support
    bool SetTableName(const std::string& tableName);
    bool SqlExec(const std::string& sqlstr);
    bool AttachDatabase(const std::string &fileName, const std::string& dbName);
    bool DetachDatabase(const std::string& dbName);
    bool DropTable(const std::string& tablename="");
    bool FindTable(const std::string& tableName);
    // repopulates the admin structures for the current table (e.g. SetTableName);
    bool PopulateTableAdmin();
    bool loadExtension(const std::string& lib, const std::string& entry);



    /// FAST INLINE ACCESS TO COLUMN VALUES
    bool PrepareColumnByIndex(const std::string& colname);//, const std::string& whereClause);

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

    const unsigned char* NextTextValue(const long long& row)
    {
        sqlite3_bind_int64(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            char* err = const_cast<char*>(m_sNodata.c_str());
            return reinterpret_cast<unsigned char*>(err);
        }
        const unsigned char* v = sqlite3_column_text(m_StmtColIter, 0);
        sqlite3_clear_bindings(m_StmtColIter);
        sqlite3_reset(m_StmtColIter);

        return v;
    }

protected:
        SQLiteTable();
    virtual ~SQLiteTable();

    /*! SQLite foundation of otbSQLiteTable
     *
     */

    bool openConnection();

    void createPreparedColumnStatements(const std::string& colname);
    void resetTableAdmin();
    void disconnectDB();
    inline bool sqliteError(const int& rc, sqlite3_stmt** stmt);
    inline void sqliteStepCheck(const int& rc);


    bool m_bUseSharedCache;
    bool m_bOpenReadOnly;
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
