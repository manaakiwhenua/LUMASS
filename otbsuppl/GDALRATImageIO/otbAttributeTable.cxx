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
 * AttributeTable.cxx
 *
 *  Created on: 20/08/2010
 *      Author: Alexander Herzig
 */

#include <otbAttributeTable.h>
#include <cstring>
#include <cstdio>
#include <sstream>

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

int AttributeTable::ColumnExists(std::string sColName)
{
	int idx = -1;
	for (int c=0; c < this->m_vNames.size(); ++c)
	{
		std::string n = this->m_vNames[c];
		if (::strcmp(n.c_str(), sColName.c_str()) == 0)
		{
			idx = c;
			break;
		}
	}
	return idx;
}

bool AttributeTable::AddColumn(std::string sColName, TableColumnType eType)
{
	// create a new vector for the column's values
	switch(eType)
	{
	case ATTYPE_STRING:
		{
		std::vector<std::string> vstr;
		for (int i=0; i < m_iNumRows; ++i)
			vstr.push_back(this->m_sNodata);
		this->m_mStringCols[sColName] = vstr;
		break;
		}
	case ATTYPE_INT:
		{
		std::vector<int> vint;
		for (int i=0; i < m_iNumRows; ++i)
			vint.push_back(this->m_iNodata);
		this->m_mIntCols[sColName] = vint;
		break;
		}
	case ATTYPE_DOUBLE:
		{
		std::vector<double> vdbl;
		for (int i=0; i < m_iNumRows; ++i)
			vdbl.push_back(this->m_dNodata);
		this->m_mDoubleCols[sColName] = vdbl;
		break;
		}
	default:
		return false;
	}

	// update admin infos
	this->m_vNames.push_back(sColName);
	this->m_vTypes.push_back(eType);

	return true;
}

bool AttributeTable::AddRow()
{
	// check for presence of columns
	if (this->m_vNames.size() == 0)
		return false;

	std::vector<std::string>::iterator iterName;
	std::vector<TableColumnType>::iterator iterType;
	for (iterName = m_vNames.begin(), iterType = m_vTypes.begin();
			iterName != m_vNames.end(); ++iterName, ++iterType)
	{
		switch (*iterType)
		{
		case ATTYPE_STRING:
			{
			std::map<std::string, std::vector<std::string> >::iterator vsiter = m_mStringCols.find(*iterName);
			if (vsiter != m_mStringCols.end())
				vsiter->second.push_back(this->m_sNodata);
			else
				return false;
			break;
			}
		case ATTYPE_INT:
			{
			std::map<std::string, std::vector<int> >::iterator viiter = m_mIntCols.find(*iterName);
			if (viiter != m_mIntCols.end())
				viiter->second.push_back(this->m_iNodata);
			else
				return false;
			break;
			}
		case ATTYPE_DOUBLE:
			{
			std::map<std::string, std::vector<double> >::iterator vditer = m_mDoubleCols.find(*iterName);
			if (vditer != m_mDoubleCols.end())
				vditer->second.push_back(this->m_dNodata);
			else
				return false;
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

bool AttributeTable::SetValue(std::string sColName, int idx, double value)
{
	//check for valid name and index parameters
	if (this->valid(sColName, idx) < 0)
		return false;

	//get a pointer to the column values
	std::map<std::string, std::vector<double> >::iterator vditer = this->m_mDoubleCols.find(sColName);
	vditer->second[idx] = value;

	return true;
}

bool AttributeTable::SetValue(std::string sColName, int idx, int value)
{
	//check for valid name and index parameters
	if (this->valid(sColName, idx) < 0)
		return false;

	//get a pointer to the column values
	std::map<std::string, std::vector<int> >::iterator viiter = this->m_mIntCols.find(sColName);
	viiter->second[idx] = value;

	return true;
}

bool AttributeTable::SetValue(std::string sColName, int idx, std::string value)
{
	//check for valid name and index parameters
	if (this->valid(sColName, idx) < 0)
		return false;

	//get a pointer to the column values
	std::map<std::string, std::vector<std::string> >::iterator vsiter = this->m_mStringCols.find(sColName);
	vsiter->second[idx] = value;

	return true;
}

double AttributeTable::GetDblValue(std::string sColName, int idx)
{
	//check for valid name and index parameters
	int colidx = this->valid(sColName, idx);
	if (colidx < 0)
		return this->m_dNodata;

	double ret;
	std::map<std::string, std::vector<std::string> >::const_iterator vsiter;
	std::map<std::string, std::vector<int> >::const_iterator viiter;
	std::map<std::string, std::vector<double> >::const_iterator vditer;

	switch(this->GetColumnType(colidx))
	{
		case ATTYPE_STRING:
			vsiter = this->m_mStringCols.find(sColName);
			ret = ::atof(vsiter->second[idx].c_str());
			break;
		case ATTYPE_INT:
			viiter = this->m_mIntCols.find(sColName);
			ret = viiter->second[idx];
			break;
		case ATTYPE_DOUBLE:
			 vditer = this->m_mDoubleCols.find(sColName);
			ret = vditer->second[idx];
			break;
		default:
			ret = this->m_dNodata;
	}

	return ret;
}

int AttributeTable::GetIntValue(std::string sColName, int idx)
{
	// check given index and column name
	int colidx = this->valid(sColName, idx);
	if (colidx < 0)
		return this->m_iNodata;

	int ret;
	std::map<std::string, std::vector<std::string> >::const_iterator vsiter;
	std::map<std::string, std::vector<int> >::const_iterator viiter;
	std::map<std::string, std::vector<double> >::const_iterator vditer;

	// query value and convert into integer if necessary
	switch(this->GetColumnType(colidx))
	{
		case ATTYPE_STRING:
			vsiter = this->m_mStringCols.find(sColName);
			ret = ::atoi(vsiter->second[idx].c_str());
			break;
		case ATTYPE_INT:
			viiter = this->m_mIntCols.find(sColName);
			ret = viiter->second[idx];
			break;
		case ATTYPE_DOUBLE:
			 vditer = this->m_mDoubleCols.find(sColName);
			ret = (int) vditer->second[idx];
			break;
		default:
			ret = this->m_iNodata;
	}

	return ret;
}

std::string AttributeTable::GetStrValue(std::string sColName, int idx)
{
	// check given index and column name
	int colidx = this->valid(sColName, idx);
	if (colidx < 0)
		return this->m_sNodata;

	std::stringstream ret;
	std::map<std::string, std::vector<std::string> >::const_iterator vsiter;
	std::map<std::string, std::vector<int> >::const_iterator viiter;
	std::map<std::string, std::vector<double> >::const_iterator vditer;

	switch(this->GetColumnType(colidx))
	{
		case ATTYPE_STRING:
			vsiter = this->m_mStringCols.find(sColName);
			ret << vsiter->second[idx];
			break;
		case ATTYPE_INT:
			viiter = this->m_mIntCols.find(sColName);
			ret << viiter->second[idx];
			break;
		case ATTYPE_DOUBLE:
			vditer = this->m_mDoubleCols.find(sColName);
			ret << vditer->second[idx];
			break;
		default:
			ret << this->m_sNodata;
	}

	return ret.str();
}

void AttributeTable::SetBandNumber(int iBand)
{
	if (iBand > 0)
		this->m_iBand = iBand;
}

void AttributeTable::SetImgFileName(std::string sFileName)
{
	this->m_sImgName = sFileName;
}

std::string AttributeTable::GetColumnName(int idx)
{
	if (idx >= m_vNames.size())
		return this->m_sNodata;

	return m_vNames[idx];
}

AttributeTable::TableColumnType AttributeTable::GetColumnType(int idx)
{
	if (idx >= m_vNames.size())
		return ATTYPE_UNKNOWN;

	return m_vTypes[idx];
}


bool AttributeTable::SetValue(int col, int row, double value)
{
	return this->SetValue(this->GetColumnName(col), row, value);
}

bool AttributeTable::SetValue(int col, int row, int value)
{
	return this->SetValue(this->GetColumnName(col), row, value);
}

bool AttributeTable::SetValue(int col, int row, std::string value)
{
	return this->SetValue(this->GetColumnName(col), row, value);
}

double AttributeTable::GetDblValue(int col, int row)
{
	return this->GetDblValue(this->GetColumnName(col), row);
}

int AttributeTable::GetIntValue(int col, int row)
{
	return this->GetIntValue(this->GetColumnName(col), row);
}

std::string AttributeTable::GetStrValue(int col, int row)
{
	return this->GetStrValue(this->GetColumnName(col), row);
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
				::sprintf(val, "%d", this->GetIntValue(c, r));
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

int AttributeTable::valid(std::string sColName, int idx)
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
}

// not implemented
AttributeTable::~AttributeTable()
{
}


} // end of namespace otb
