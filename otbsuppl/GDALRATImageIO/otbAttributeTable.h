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
 * AttributeTable.h
 *
 *  Created on: 23/09/2015
 *      Author: Alexander Herzig
 */

#ifndef ATTRIBUTETABLE_H_
#define ATTRIBUTETABLE_H_

#include <string>
#include <map>
#include <vector>
#include <fstream>

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

    // table types
    typedef enum
    {
        ATTABLE_TYPE_RAM = 0,
        ATTABLE_TYPE_SQLITE
    } TableType;

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
            long long int ival;
            double    dval;
            char*     tval;
        };
    } ColumnValue;


    //itkNewMacro(Self);
	itkTypeMacro(AttributeTable, Superclass);

	// getting info about the table
    TableType GetTableType(void) {return m_ATType;}
    int GetNumCols();
    virtual long long GetNumRows();
    int ColumnExists(const std::string& sColName);

	std::string GetColumnName(int idx);
	TableColumnType GetColumnType(int idx);

    virtual long long GetRowIdx(const std::string& column, void* value) = 0;

	// managing the attribute table's content
    virtual bool AddColumn(const std::string& sColName, TableColumnType type) = 0;
    virtual void SetValue(const std::string& sColName, long long idx, double value) = 0;
    virtual void SetValue(const std::string& sColName, long long idx, long long value) = 0;
    virtual void SetValue(const std::string& sColName, long long idx, std::string value) = 0;
    virtual double GetDblValue(const std::string& sColName, long long idx) = 0;
    virtual long long GetIntValue(const std::string& sColName, long long idx) = 0;
    virtual std::string GetStrValue(const std::string& sColName, long long idx) = 0;

    virtual void SetValue(int col, long long row, double value) = 0;
    virtual void SetValue(int col, long long row, long long value) = 0;
    virtual void SetValue(int col, long long row, std::string value) = 0;

    virtual void SetColumnName(int col, const std::string& name) {}

    virtual double GetDblValue(int col, long long row) = 0;
    virtual long long GetIntValue(int col, long long row) = 0;
    virtual std::string GetStrValue(int col, long long row) = 0;

    double GetDblNodata(void) {return m_dNodata;}
    long long GetIntNodata(void) {return m_iNodata;}
    std::string GetStrNodata(void) {return m_sNodata;}
    std::string GetPrimaryKey() {return this->m_idColName;}
    virtual long long GetMinPKValue() = 0;
    virtual long long GetMaxPKValue() = 0;


    virtual bool RemoveColumn(int col) = 0;
    virtual bool RemoveColumn(const std::string& name) = 0;

	// manage table meta data
    void SetBandNumber(int iBand);
	void SetImgFileName(const std::string& sFileName);
    virtual bool SetRowIdColName(const std::string& idCol)
        {m_idColName = idCol; return true;}
    int GetBandNumber(void);
	std::string GetImgFileName(void);

	// print the table
	void Print(std::ostream& os, itk::Indent indent, int nrows);
	void PrintStructure(std::ostream& os, itk::Indent indent);


protected:
	AttributeTable();
	virtual
	~AttributeTable();

	// admin vectors holding header infos about columns

	/**
	 *  Hold the name and type of a column,
	 *  in the order as columns are added to
	 *  the table;
	 */
	std::vector<std::string> m_vNames;
	std::vector<TableColumnType> m_vTypes;

    TableType m_ATType;
    long long m_iNumRows;
	std::string m_sNodata;
    long long m_iNodata;
	double m_dNodata;
	int m_iBand;
	std::string m_sImgName;
    std::string m_idColName;

	// validate column name and row index; if
	// parameters are valid then the column index
	// is returned otherwise -1;
    int valid(const std::string& sColName, int idx);
	std::string typestr(TableColumnType type);

};

}

#endif /* ATTRIBUTETABLE_H_ */
