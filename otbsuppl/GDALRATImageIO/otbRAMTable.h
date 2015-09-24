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
 * RAMTable.h
 *
 *  Created on: 23/09/2015
 *      Author: Alexander Herzig
 */

#ifndef RAMTABLE_H_
#define RAMTABLE_H_

#include <string>
#include <map>
#include <vector>
#include <fstream>

#include "otbAttributeTable.h"
#include "itkObject.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "otbgdalratimageio_export.h"

namespace otb
{

class OTBGDALRATIMAGEIO_EXPORT RAMTable : public AttributeTable
{
public:
	/** Standard class typedefs. */
        typedef RAMTable				Self;
	typedef itk::DataObject					Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	// supported column types
//	typedef enum
//	{
//		ATTYPE_STRING = 0,
//		ATTYPE_INT,
//		ATTYPE_DOUBLE ,
//		ATTYPE_UNKNOWN
//	} TableColumnType;

	itkNewMacro(Self);
        itkTypeMacro(RAMTable, Superclass);

	// getting info about the table
        //int GetNumCols();
        //int GetNumRows();
        //int ColumnExists(const std::string& sColName);

        //	std::string GetColumnName(int idx);
        //	TableColumnType GetColumnType(int idx);

	void* GetColumnPointer(int idx);

        long long GetRowIdx(const std::string& column, void* value);

	//long GetRowIdx(const std::string& column, const double& value);
	//long GetRowIdx(const std::string& column, const long& value);
	//long GetRowIdx(const std::string& column, const std::string& value);

	// managing the attribute table's content
	bool AddColumn(const std::string& sColName, TableColumnType type);
	bool AddRow();
        bool AddRows(long long numRows);
        void SetValue(const std::string& sColName, long long idx, double value);
        void SetValue(const std::string& sColName, long long idx, long long value);
        void SetValue(const std::string& sColName, long long idx, std::string value);
        double GetDblValue(const std::string& sColName, long long idx);
        long long GetIntValue(const std::string& sColName, long long idx);
        std::string GetStrValue(const std::string& sColName, long long idx);

        void SetValue(int col, long long row, double value);
        void SetValue(int col, long long row, long long value);
        void SetValue(int col, long long row, std::string value);

	void SetColumnName(int col, const std::string& name);

    double GetDblValue(int col, long long row);
    long long GetIntValue(int col, long long row);
    std::string GetStrValue(int col, long long row);

	bool RemoveColumn(int col);
	bool RemoveColumn(const std::string& name);

//	// print the table
//	void Print(std::ostream& os, itk::Indent indent, int nrows);
//	void PrintStructure(std::ostream& os, itk::Indent indent);


protected:
        RAMTable();
	virtual
        ~RAMTable();

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
	std::vector<int> m_vPosition;

	// maps holding table columns
	//std::map<int, std::vector<std::string> > m_mStringCols;
	//std::map<int, std::vector<long> > m_mIntCols;
	//std::map<int, std::vector<double> > m_mDoubleCols;
	std::vector<std::vector<std::string>* > m_mStringCols;
        std::vector<std::vector<long long>* > m_mIntCols;
	std::vector<std::vector<double>* > m_mDoubleCols;


        //int m_iNumRows;
        //std::string m_sNodata;
        //long long m_iNodata;
        //double m_dNodata;
        //int m_iBand;
        //std::string m_sImgName;

	// validate column name and row index; if
	// parameters are valid then the column index
	// is returned otherwise -1;
    //inline int valid(const std::string& sColName, int idx);
    //std::string typestr(TableColumnType type);

};

}

#endif /* RAMTABLE_H_ */
