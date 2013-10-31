 /*****************************h*************************************************
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
#include "otbAttributeTable.h"
#include <limits>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <algorithm>

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

int
AttributeTable::ColumnExists(const std::string& sColName)
{
    //std::string colname = sColName;
    //std::transform(sColName.begin(), sColName.end(), colname.begin(), ::tolower);
	int idx = -1;
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
	if (eType != ATTYPE_STRING && eType != ATTYPE_INT
			&& eType != ATTYPE_DOUBLE)
	{
		return false;
	}

	m_vNames.push_back(sColName);
	int idx = m_vNames.size()-1;

	// create a new vector for the column's values
	switch(eType)
	{
	case ATTYPE_STRING:
		{
		std::vector<std::string> vstr;
		for (int i=0; i < m_iNumRows; ++i)
			vstr.push_back(this->m_sNodata);
		//std::pair<int, std::vector<std::string> > rec(idx, vstr);
		//this->m_mStringCols.insert(rec);
		this->m_mStringCols[idx] = vstr;
		//NMDebugAI(<< "new number of string cols: "
		//		<< m_mStringCols.size() << std::endl);
		break;
		}
	case ATTYPE_INT:
		{
		std::vector<long> vint;
		for (int i=0; i < m_iNumRows; ++i)
			vint.push_back(this->m_iNodata);
		//std::pair<int, std::vector<long> > rec(idx, vint);
		//this->m_mIntCols.insert(rec);
		this->m_mIntCols[idx] = vint;
		//NMDebugAI(<< "new number of integer cols: "
		//		<< m_mIntCols.size() << std::endl);
		break;
		}
	case ATTYPE_DOUBLE:
		{
		std::vector<double> vdbl;
		for (int i=0; i < m_iNumRows; ++i)
			vdbl.push_back(this->m_dNodata);
		//std::pair<int, std::vector<double> > rec(idx, vdbl);
		//this->m_mDoubleCols.insert(rec);
		this->m_mDoubleCols[idx] = vdbl;
		//NMDebugAI(<< "new number of double cols: "
		//		<< m_mDoubleCols.size() << std::endl);
		break;
		}
	default:
		return false;
	}

	// update admin infos
	this->m_vTypes.push_back(eType);

	return true;
}

bool AttributeTable::AddRows(long numRows)
{
	// check for presence of columns
	if (this->m_vNames.size() == 0)
		return false;

	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
	{
		switch (this->m_vTypes[colidx])
		{
		case ATTYPE_STRING:
			{
			m_mStringCols.at(colidx).resize(m_iNumRows+numRows, m_sNodata);
			break;
			}
		case ATTYPE_INT:
			{
			m_mIntCols.at(colidx).resize(m_iNumRows+numRows, m_iNodata);
			break;
			}
		case ATTYPE_DOUBLE:
			{
			m_mDoubleCols.at(colidx).resize(m_iNumRows+numRows, m_dNodata);
			break;
			}
		default:
			return false;
		}
	}

	// increase the row number counter
	this->m_iNumRows += numRows;

	return true;
}

bool AttributeTable::AddRow()
{
	// check for presence of columns
	if (this->m_vNames.size() == 0)
		return false;

	for (int colidx = 0; colidx < this->m_vNames.size(); ++colidx)
	{
		switch (this->m_vTypes[colidx])
		{
		case ATTYPE_STRING:
			{
			m_mStringCols.at(colidx).push_back(this->m_sNodata);
			break;
			}
		case ATTYPE_INT:
			{
			m_mIntCols.at(colidx).push_back(this->m_iNodata);
			break;
			}
		case ATTYPE_DOUBLE:
			{
			m_mDoubleCols.at(colidx).push_back(this->m_dNodata);
			break;
			}
		default:
			return false;
		}
	}

	// increase the row number counter
	++this->m_iNumRows;

	return true;
}

void
AttributeTable::SetValue(const std::string& sColName, int idx, double value)
{
	// get the column index
	int colIdx = this->valid(sColName, idx);
	if (colIdx < 0)
		return;

	switch (m_vTypes[colIdx])
	{
		case ATTYPE_STRING:
		{
			std::stringstream sval;
			sval << value;
			this->m_mStringCols.at(colIdx).at(idx) = sval.str();
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(colIdx).at(idx) = value;
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(colIdx).at(idx) = value;
			break;
		}
		default:
			break;
	}
}

void
AttributeTable::SetValue(const std::string& sColName, int idx, long value)
{
	int colIdx = this->valid(sColName, idx);
	if (colIdx < 0)
		return;

	switch (m_vTypes[colIdx])
	{
		case ATTYPE_STRING:
		{
			std::stringstream sval;
			sval << value;
			this->m_mStringCols.at(colIdx).at(idx) = sval.str();
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(colIdx).at(idx) = value;
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(colIdx).at(idx) = value;
			break;
		}
		default:
			break;
	}
}

void
AttributeTable::SetValue(const std::string& sColName, int idx, std::string value)
{
	int colIdx = this->valid(sColName, idx);
	if (colIdx < 0)
		return;

	switch (m_vTypes[colIdx])
	{
		case ATTYPE_STRING:
		{
			this->m_mStringCols.at(colIdx).at(idx) = value;
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(colIdx).at(idx) = ::strtol(value.c_str(),0,10);
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(colIdx).at(idx) = ::strtod(value.c_str(),0);
			break;
		}
		default:
			break;
	}
}

double AttributeTable::GetDblValue(const std::string& sColName, int idx)
{
	//check for valid name and index parameters
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_dNodata;

	double ret;
	switch(m_vTypes[colidx])
	{
		case ATTYPE_STRING:
			ret = ::strtod(this->m_mStringCols.at(colidx).at(idx).c_str(),0);
			break;
		case ATTYPE_INT:
			ret = this->m_mIntCols.at(colidx).at(idx);
			break;
		case ATTYPE_DOUBLE:
			ret = this->m_mDoubleCols.at(colidx).at(idx);
			break;
		default:
			ret = this->m_dNodata;
			break;
	}

	return ret;
}

long
AttributeTable::GetIntValue(const std::string& sColName, int idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_iNodata;

	long ret;
	switch(m_vTypes[colidx])
	{
		case ATTYPE_STRING:
			ret = ::strtol(this->m_mStringCols.at(colidx).at(idx).c_str(),0,10);
			break;
		case ATTYPE_INT:
			ret = this->m_mIntCols.at(colidx).at(idx);
			break;
		case ATTYPE_DOUBLE:
			ret = this->m_mDoubleCols.at(colidx).at(idx);
			break;
		default:
			ret = this->m_iNodata;
			break;
	}

	return ret;
}

std::string AttributeTable::GetStrValue(const std::string& sColName, int idx)
{
	// check given index and column name
	int colidx = this->ColumnExists(sColName);
	if (colidx < 0 || idx < 0 || idx > m_iNumRows)
		return m_sNodata;

	std::stringstream ret;
	switch(m_vTypes[colidx])
	{
		case ATTYPE_STRING:
			ret << this->m_mStringCols.at(colidx).at(idx);
			break;
		case ATTYPE_INT:
			ret << this->m_mIntCols.at(colidx).at(idx);
			break;
		case ATTYPE_DOUBLE:
			ret << this->m_mDoubleCols.at(colidx).at(idx);
			break;
		default:
			ret << this->m_sNodata;
			break;
	}

	return ret.str();
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
	return m_vNames.at(idx);
}

AttributeTable::TableColumnType AttributeTable::GetColumnType(int idx)
{
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

	switch(this->m_vTypes[col])
	{
	case ATTYPE_INT:
		this->m_mIntCols.erase(col);
		break;
	case ATTYPE_DOUBLE:
		this->m_mDoubleCols.erase(col);
		break;
	case ATTYPE_STRING:
		this->m_mStringCols.erase(col);
		break;
	default:
		return false;
		break;
	}

	// now remove any traces of the column in the admin arrays
	this->m_vNames.erase(this->m_vNames.begin() + col);
	this->m_vTypes.erase(this->m_vTypes.begin() + col);

	return true;
}

bool
AttributeTable::RemoveColumn(const std::string& name)
{
	int idx = this->ColumnExists(name);
	if (idx < 0)
		return false;

	return this->RemoveColumn(idx);
}

void AttributeTable::SetValue(int col, int row, double value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

	switch (m_vTypes[col])
	{
		case ATTYPE_STRING:
		{
			std::stringstream sval;
			sval << value;
			this->m_mStringCols.at(col).at(row) = sval.str();
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(col).at(row) = value;
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(col).at(row) = value;
			break;
		}
		default:
			break;
	}
}

void AttributeTable::SetValue(int col, int row, long value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

	switch (m_vTypes[col])
	{
		case ATTYPE_STRING:
		{
			std::stringstream sval;
			sval << value;
			this->m_mStringCols.at(col).at(row) = sval.str();
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(col).at(row) = value;
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(col).at(row) = value;
			break;
		}
		default:
			break;
	}
}

void AttributeTable::SetValue(int col, int row, std::string value)
{
	if (col < 0 || col >= m_vNames.size())
		return;

	if (row < 0 || row >= m_iNumRows)
		return;

	switch (m_vTypes[col])
	{
		case ATTYPE_STRING:
		{
			this->m_mStringCols.at(col).at(row) = value;
			break;
		}
		case ATTYPE_INT:
		{
			this->m_mIntCols.at(col).at(row) = ::strtol(value.c_str(), 0, 10);
			break;
		}
		case ATTYPE_DOUBLE:
		{
			this->m_mDoubleCols.at(col).at(row) = ::strtod(value.c_str(), 0);
			break;
		}
		default:
			break;
	}
}

double AttributeTable::GetDblValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_dNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_dNodata;

	double ret;
	switch(m_vTypes[col])
	{
		case ATTYPE_STRING:
			ret = ::strtod(this->m_mStringCols.at(col).at(row).c_str(),0);
			break;
		case ATTYPE_INT:
			ret = this->m_mIntCols.at(col).at(row);
			break;
		case ATTYPE_DOUBLE:
			ret = this->m_mDoubleCols.at(col).at(row);
			break;
		default:
			ret = this->m_dNodata;
			break;
	}

	return ret;
}

long AttributeTable::GetIntValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_iNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_iNodata;

	long ret;
	switch(m_vTypes[col])
	{
		case ATTYPE_STRING:
			ret = ::strtol(this->m_mStringCols.at(col).at(row).c_str(),0,10);
			break;
		case ATTYPE_INT:
			ret = this->m_mIntCols.at(col).at(row);
			break;
		case ATTYPE_DOUBLE:
			ret = this->m_mDoubleCols.at(col).at(row);
			break;
		default:
			ret = this->m_iNodata;
			break;
	}

	return ret;

}

std::string AttributeTable::GetStrValue(int col, int row)
{
	if (col < 0 || col >= m_vNames.size())
		return m_sNodata;

	if (row < 0 || row >= m_iNumRows)
		return m_sNodata;

	std::stringstream ret;
	switch(m_vTypes[col])
	{
		case ATTYPE_STRING:
			ret << this->m_mStringCols.at(col).at(row);
			break;
		case ATTYPE_INT:
			ret << this->m_mIntCols.at(col).at(row);
			break;
		case ATTYPE_DOUBLE:
			ret << this->m_mDoubleCols.at(col).at(row);
			break;
		default:
			ret << this->m_sNodata;
			break;
	}

	return ret.str();

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


AttributeTable::AttributeTable()
{
	this->m_iNumRows = 0;
	this->m_iNodata = -std::numeric_limits<long>::max();
	this->m_dNodata = -std::numeric_limits<double>::max();
	this->m_sNodata = "<nodata>";
}

// not implemented
AttributeTable::~AttributeTable()
{
}


} // end of namespace otb
