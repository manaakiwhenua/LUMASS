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

#include "itkObject.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"

namespace otb
{

class ITK_EXPORT AttributeTable : public itk::DataObject
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
		ATTYPE_DOUBLE,
		ATTYPE_UNKNOWN
	} TableColumnType;

	itkNewMacro(Self);
	itkTypeMacro(AttributeTable, Superclass);

	// getting info about the table
	int GetNumCols();
	int GetNumRows();
	int ColumnExists(const std::string& sColName);

	std::string GetColumnName(int idx);
	TableColumnType GetColumnType(int idx);


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



protected:
	AttributeTable();
	virtual
	~AttributeTable();

	// admin vectors holding header infos about columns
	std::vector<std::string> m_vNames;
	std::vector<TableColumnType> m_vTypes;

	// maps holding table columns
	std::map<int, std::vector<std::string> > m_mStringCols;
	std::map<int, std::vector<long> > m_mIntCols;
	std::map<int, std::vector<double> > m_mDoubleCols;

	// scalar members
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
