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


    // supported column types
//	typedef enum
//	{
//		ATTYPE_STRING = 0,
//		ATTYPE_INT,
//		ATTYPE_DOUBLE ,
//		ATTYPE_UNKNOWN
//	} TableColumnType;

//    // field value data structure
//    typedef struct
//    {
//        TableColumnType type;
//        union
//        {
//            long long int ival;
//            double    dval;
//            char*     tval;
//        };
//    } ColumnValue;

    itkNewMacro(Self);
    itkTypeMacro(SQLiteTable, Superclass);

	// getting info about the table
        //int GetNumCols();
        long long GetNumRows();
        //int ColumnExists(const std::string& sColName);

        //	std::string GetColumnName(int idx);
        //	TableColumnType GetColumnType(int idx);

    //void* GetColumnPointer(int idx);

    long long int GetRowIdx(const std::string& column, void* value);

	//long GetRowIdx(const std::string& column, const double& value);
	//long GetRowIdx(const std::string& column, const long& value);
	//long GetRowIdx(const std::string& column, const std::string& value);

	// managing the attribute table's content
    bool AddColumn(const std::string &sColName, TableColumnType type)
        {return AddConstrainedColumn(sColName, type, "");}
    bool AddConstrainedColumn(const std::string& sColName, TableColumnType type,
                   const std::string& sColConstraint="");
    //bool AddRow();
    //bool AddRows(long long int numRows);
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
    TableCreateStatus CreateTable(std::string filename, std::string tag="");
    void CloseTable(bool drop=false);
    bool SetRowIDColName(const std::string& name);
    bool DeleteDatabase(void);

    std::string GetDbFileName() {return this->m_dbFileName;}
    std::string GetTableName() {return this->m_tableName;}
    //std::string getPrimaryKey() {return this->m_idColName;}
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

    void createPreparedColumnStatements(const std::string& colname);
    bool dropTable(const std::string& tablename="");
    void resetTableAdmin();
    void disconnectDB();
    inline bool sqliteError(const int& rc, sqlite3_stmt** stmt);
    inline void sqliteStepCheck(const int& rc);

    //    inline int getSqlIntValue(const std::string& col, int row);
    //    inline double getSqlDblValue(const std::string& col, int row);
    //    inline std::string getSqlStrValue(const std::string& col, int row);
    //    inline void setSqlValue(const std::string& col, int row);



    sqlite3* m_db;
    std::string m_dbFileName;
    std::string m_tableName;
   // std::string m_idColName;

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

	// admin vectors holding header infos about columns

	/**
	 *  Hold the name and type of a column,
	 *  in the order as columns are added to
	 *  the table;
	 */
        //std::vector<std::string> m_vNames;
        //std::vector<TableColumnType> m_vTypes;

	/** Holds the index of the respective type specific
	 *  vector containing the data of the column
	 *  denoted by the index in this vector;
	 *  e.g.:
	 *  		m_vNames[2]    = "Column_3";
	 *  		m_vTypes[2]    = ATTYPE_INT;
	 *  		m_vPosition[2] = 3;
	 *
	 *  Column "Column_3" is the third column in the table
	 *  and its content is stored in m_mIntCols[3];
	 *
	 */
        //std::vector<int> m_vPosition;

	// maps holding table columns
	//std::map<int, std::vector<std::string> > m_mStringCols;
	//std::map<int, std::vector<long> > m_mIntCols;
	//std::map<int, std::vector<double> > m_mDoubleCols;
        //	std::vector<std::vector<std::string>* > m_mStringCols;
        //	std::vector<std::vector<long>* > m_mIntCols;
        //	std::vector<std::vector<double>* > m_mDoubleCols;


        //long long m_iNumRows;
        //	std::string m_sNodata;
        //      long long m_iNodata;
        //	double m_dNodata;
        //	int m_iBand;
        //	std::string m_sImgName;

	// validate column name and row index; if
	// parameters are valid then the column index
	// is returned otherwise -1;
    //inline int valid(const std::string& sColName, int idx);
    //std::string typestr(TableColumnType type);

};

}

#endif /* ATTRIBUTETABLE_H_ */
