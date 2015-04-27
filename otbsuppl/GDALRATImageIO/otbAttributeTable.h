 /******************************************************************************
 * Created by Alexander Herzig
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
 * AttributeTable.h
 *
 *  Created on: 20/08/2010
 *      Author: Alexander Herzig
 */

#ifndef ATTRIBUTETABLE_H_
#define ATTRIBUTETABLE_H_

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sqlite3.h>

#include "itkObject.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "otbgdalratimageio_export.h"

namespace otb
{

class OTBGDALRATIMAGEIO_EXPORT AttributeTable : public itk::DataObject
{
public:
	/** Standard class typedefs. */
	typedef AttributeTable				Self;
	typedef itk::DataObject					Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	// supported column types
	typedef enum
	{
		ATTYPE_STRING = 0,
		ATTYPE_INT,
		ATTYPE_DOUBLE ,
		ATTYPE_UNKNOWN
	} TableColumnType;

    // field value data structure
    typedef struct
    {
        TableColumnType type;
        union
        {
            int    ival;
            double dval;
            char*  tval;
        };
    } ColumnValue;


	itkNewMacro(Self);
	itkTypeMacro(AttributeTable, Superclass);

	// getting info about the table
	int GetNumCols();
	int GetNumRows();
	int ColumnExists(const std::string& sColName);

	std::string GetColumnName(int idx);
	TableColumnType GetColumnType(int idx);

	void* GetColumnPointer(int idx);

	long GetRowIdx(const std::string& column, void* value);

	//long GetRowIdx(const std::string& column, const double& value);
	//long GetRowIdx(const std::string& column, const long& value);
	//long GetRowIdx(const std::string& column, const std::string& value);

	// managing the attribute table's content
	bool AddColumn(const std::string& sColName, TableColumnType type);
	bool AddRow();
	bool AddRows(long numRows);
	void SetValue(const std::string& sColName, int idx, double value);
	void SetValue(const std::string& sColName, int idx, long value);
	void SetValue(const std::string& sColName, int idx, std::string value);
	double GetDblValue(const std::string& sColName, int idx);
	long GetIntValue(const std::string& sColName, int idx);
	std::string GetStrValue(const std::string& sColName, int idx);

	void SetValue(int col, int row, double value);
	void SetValue(int col, int row, long value);
	void SetValue(int col, int row, std::string value);

	void SetColumnName(int col, const std::string& name);

	double GetDblValue(int col, int row);
	long GetIntValue(int col, int row);
	std::string GetStrValue(int col, int row);

	bool RemoveColumn(int col);
	bool RemoveColumn(const std::string& name);

	// manage table meta data
	void SetBandNumber(int iBand);
	void SetImgFileName(const std::string& sFileName);
	int GetBandNumber(void);
	std::string GetImgFileName(void);

	// print the table
	void Print(std::ostream& os, itk::Indent indent, int nrows);
	void PrintStructure(std::ostream& os, itk::Indent indent);

    /// SQLite support functions
    std::string getDbFileName() {return this->m_dbFileName;}
    sqlite3* getDbConnection() {return this->m_db;}
    bool prepareBulkGet(const std::vector<std::string>& colNames, const std::string& whereClause="");
    bool prepareBulkSet(const std::vector<std::string>& colNames, const bool& bInsert=true);
    bool doBulkSet(std::vector< ColumnValue >& values, const int& row=-1);
    bool doBulkGet(std::vector< ColumnValue >& values);
    bool beginTransaction();
    bool endTransaction();
    bool createIndex(const std::vector<std::string>& colNames);


    /// FAST INLINE ACCESS TO COLUMN VALUES
    bool prepareColumnByIndex(const std::string& colname);//, const std::string& whereClause);

    double nextDoubleValue(const int& row)
    {
        sqlite3_bind_int(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            return m_dNodata;
        }
        double v = sqlite3_column_double(m_StmtColIter, 0);
        sqlite3_reset(m_StmtColIter);

        return v;

    }

    int nextIntValue(const int& row)
    {
        sqlite3_bind_int(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            return m_iNodata;
        }
        int v = sqlite3_column_int(m_StmtColIter, 0);
        sqlite3_reset(m_StmtColIter);

        return v;
    }

    const unsigned char* nextTextValue(const int& row)
    {
        sqlite3_bind_int(m_StmtColIter, 1, row);
        if (sqlite3_step(m_StmtColIter) != SQLITE_ROW)
        {
            sqlite3_reset(m_StmtColIter);
            char* err = const_cast<char*>(m_sNodata.c_str());
            return reinterpret_cast<unsigned char*>(err);
        }
        const unsigned char* v = sqlite3_column_text(m_StmtColIter, 0);
        sqlite3_reset(m_StmtColIter);

        return v;
    }

protected:
	AttributeTable();
	virtual
	~AttributeTable();


    /*! SQLite foundation of otbAttributeTable
     *
     */
    sqlite3* m_db;
    std::string m_dbFileName;

    std::vector<sqlite3_stmt*> m_vStmtUpdate;
    std::vector<sqlite3_stmt*> m_vStmtSelect;
    std::vector<sqlite3_stmt*> m_vStmtGetRowidx;
    sqlite3_stmt* m_StmtBulkSet;
    sqlite3_stmt* m_StmtBulkGet;
    std::vector<otb::AttributeTable::TableColumnType> m_vTypesBulkSet;
    std::vector<otb::AttributeTable::TableColumnType> m_vTypesBulkGet;
    std::vector<std::string> m_IndexNames;

    sqlite3_stmt* m_StmtColIter;

    sqlite3_stmt* m_StmtBegin;
    sqlite3_stmt* m_StmtEnd;
    sqlite3_stmt* m_StmtRollback;
    bool m_InTransaction;

    const char* m_CurPrepStmt;

    void createTable(std::string filename);
    inline bool sqliteError(const int& rc, sqlite3_stmt** stmt);
    inline void sqliteStepCheck(const int& rc);

    //    inline int getSqlIntValue(const std::string& col, int row);
    //    inline double getSqlDblValue(const std::string& col, int row);
    //    inline std::string getSqlStrValue(const std::string& col, int row);
    //    inline void setSqlValue(const std::string& col, int row);




	// admin vectors holding header infos about columns

	/**
	 *  Hold the name and type of a column,
	 *  in the order as columns are added to
	 *  the table;
	 */
	std::vector<std::string> m_vNames;
	std::vector<TableColumnType> m_vTypes;

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
	std::vector<int> m_vPosition;

	// maps holding table columns
	//std::map<int, std::vector<std::string> > m_mStringCols;
	//std::map<int, std::vector<long> > m_mIntCols;
	//std::map<int, std::vector<double> > m_mDoubleCols;
	std::vector<std::vector<std::string>* > m_mStringCols;
	std::vector<std::vector<long>* > m_mIntCols;
	std::vector<std::vector<double>* > m_mDoubleCols;


	int m_iNumRows;
	std::string m_sNodata;
	long m_iNodata;
	double m_dNodata;
	int m_iBand;
	std::string m_sImgName;

	// validate column name and row index; if
	// parameters are valid then the column index
	// is returned otherwise -1;
	inline int valid(const std::string& sColName, int idx);
	std::string typestr(TableColumnType type);

};

}

#endif /* ATTRIBUTETABLE_H_ */
