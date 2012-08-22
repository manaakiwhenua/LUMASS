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
 * NMTableCalculator.cpp
 *
 *  Created on: 21/12/2011
 *      Author: alex
 */

#include "NMTableCalculator.h"

#include <QString>
#include "vtkTable.h"
#include "vtkStringArray.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkVariant.h"
#include "vtkStdString.h"


NMTableCalculator::NMTableCalculator(QObject* parent)
	: QObject(parent), mTab(0)
{
	this->initCalculator();
}

NMTableCalculator::NMTableCalculator(vtkTable* tab, QObject* parent)
	: QObject(parent), mTab(tab)
{
	this->initCalculator();
}

void NMTableCalculator::initCalculator()
{
	this->mParser = vtkSmartPointer<vtkFunctionParser>::New();
	this->mTabView = 0;
	this->mFunction.clear();
	this->mResultColumn.clear();
	this->mResultColumnType = VTK_STRING;
	this->mResultColumnIndex = -1;
	this->mSelectionMode = false;
	this->mbRowFilter = false;
	this->mFilterArray = 0;
	this->mNumSelRecs = 0;
	this->clearLists();
}

NMTableCalculator::~NMTableCalculator()
{

}

bool NMTableCalculator::setResultColumn(const QString& name)
{

	int colidx = this->getColumnIndex(name);

	// check, whether the column is available in the table
	if (colidx < 0)
	{
		NMErr(ctxTabCalc, << "Specified Column couldn't be found in the table!");
		return false;
	}

	this->mResultColumn = name;
	this->mResultColumnIndex = colidx;
	this->mResultColumnType = this->mTab->GetColumn(colidx)->GetDataType();
	return true;
}

void NMTableCalculator::setFunction(const QString& function)
{
	this->mParser->SetFunction(function.toStdString().c_str());
	this->mFunction = function;
}

int NMTableCalculator::getColumnIndex(QString name)
{
	int idx = -1;

	for (int col=0; col < this->mTab->GetNumberOfColumns(); ++col)
	{
		if (name.toLower() == QString(this->mTab->GetColumnName(col)).toLower())
		{
			idx = col;
			break;
		}
	}

	return idx;
}

bool NMTableCalculator::setSelectionModeOn(const QString& selColumn,
		NMTableView* tabView)
{
	NMDebugCtx(ctxTabCalc, << "...");
	if (!this->setResultColumn(selColumn) || tabView == 0)
	{
		NMErr(ctxTabCalc, << "Invalid Selection Column and/or NMTableView!");
		return false;
	}

	this->mSelectionMode = true;
	this->mTabView = tabView;

	NMDebugAI(<< "selection mode on? " << this->mSelectionMode << endl);
	NMDebugAI(<< "table view set?    " << this->mTabView << endl);

	NMDebugCtx(ctxTabCalc, << "done!");
	return true;
}

bool NMTableCalculator::setRowFilterModeOn(QString filterColumn)
{
	// check, whether the column is available in the table
	vtkAbstractArray* aa = this->mTab->GetColumnByName(filterColumn.toStdString().c_str());
	if (aa == 0)
	{
		NMErr(ctxTabCalc, << "Specified Filter Column couldn't be found in the table!");
		return false;
	}

	if (aa->GetDataType() < VTK_VOID || aa->GetDataType() >= VTK_STRING)
	{
		NMErr(ctxTabCalc, << "Ivalid Filter Column Type!");
		return false;
	}

	this->mFilterArray = vtkDataArray::SafeDownCast(aa);
	this->mbRowFilter = true;

	return true;
}


bool NMTableCalculator::parseFunction()
{
	NMDebugCtx(ctxTabCalc, << "...");
	if (this->mFunction.isEmpty() || this->mResultColumn.isEmpty())
	{
		NMErr(ctxTabCalc, << "Function or result column has not been set!");
		NMDebugCtx(ctxTabCalc, << "done!");
		return false;
	}

	QString name;
	QStringList funcTerms = this->mFunction.split(QRegExp("[&|,]"));
	foreach(QString term, funcTerms)
	{
		NMDebug(<< endl);
		NMDebugAI(<< "analysing term '" << term.toStdString() << "' ..." << endl);

		int type;
		QStringList strFieldNames;
		QList<int> strFieldIndices;

		NMDebugAI(<< "look for table columns ... " << endl);
		for (int col=0; col < this->mTab->GetNumberOfColumns(); ++col)
		{
			name = this->mTab->GetColumnName(col);
			type = this->mTab->GetColumn(col)->GetDataType();
			if (term.contains(name, Qt::CaseInsensitive))
			{
				if (type == VTK_STRING)
				{
					strFieldNames.append(name);
					strFieldIndices.append(col);
					NMDebugAI(<< "detected string column: " << col
							<< "-" << name.toStdString() << endl);
				}
				else
				{
					this->mFuncVars.append(name);
					this->mLstFuncVarColumnIndex.append(col);
					NMDebugAI(<< "detected numeric column: " << col
							<< "-" << name.toStdString() << endl);
				}
			}
		}

		QRegExp rxOrig("('[\\w\\W]*'|[A-Za-z_]+[\\d\\w]*)\\s*(=|!=|>|<|>=|<=|in|startsWith|endsWith|contains)\\s*([A-Za-z_]+[\\d\\w]*|'[\\w\\W]*')",
						Qt::CaseInsensitive);
		int posOrig = rxOrig.indexIn(term);

		// if the current term is no string expression, we can skip the rest
		if (strFieldIndices.size() == 0 && posOrig == -1)
			continue;

		// add the field and col index list to the member vars
		this->mLstStrFieldNames.append(strFieldNames);
		this->mLstStrColumnIndices.append(strFieldIndices);

		// get captured parts of the term
		QStringList parts = rxOrig.capturedTexts();
		this->mslStrTerms.append(parts[0]);
		NMDebugAI(<< "captured string expression: " << parts[0].toStdString() << endl);

		NMStrOperator sop;
		if (parts[2] == ">")
			sop = NM_STR_GT;
		else if (parts[2] == ">=")
			sop = NM_STR_GTEQ;
		else if (parts[2] == "<")
			sop = NM_STR_LT;
		else if (parts[2] == "<=")
			sop = NM_STR_LTEQ;
		else if (parts[2] == "=")
			sop = NM_STR_EQ;
		else if (parts[2] == "!=")
			sop = NM_STR_NEQ;
		else if (parts[2].toLower() == "in")
			sop = NM_STR_IN;
		else if (parts[2].toLower() == "contains")
			sop = NM_STR_CONTAINS;
		else if (parts[2].toLower() == "startswith")
			sop = NM_STR_STARTSWITH;
		else if (parts[2].toLower() == "endswith")
			sop = NM_STR_ENDSWITH;

		this->mLstNMStrOperator.append(sop);
		NMDebugAI(<< "string comparison operator: " << parts[2].toStdString()
				<< " -> " << sop << endl);
	}

	NMDebugCtx(ctxTabCalc, << "done!");
	return true;
}

bool NMTableCalculator::calculate(void)
{
	NMDebugCtx(ctxTabCalc, << "...");

	this->clearLists();

	// check if we've got everything we need
	if (!this->parseFunction())
	{
		return false;
	}

	if (this->mResultColumnType == VTK_STRING)
	{
		this->doStringCalculation();
	}
	else
	{
		this->doNumericCalcSelection();
	}

	NMDebugCtx(ctxTabCalc, << "done!");
	return true;
}

void NMTableCalculator::clearLists()
{
	this->mFuncVars.clear();
	this->mslStrTerms.clear();
	this->mLstFuncVarColumnIndex.clear();
	this->mLstStrFieldNames.clear();
	this->mLstStrColumnIndices.clear();
	this->mLstNMStrOperator.clear();
	this->mslStrOperators.clear();
}

void NMTableCalculator::doNumericCalcSelection()
{
	NMDebugCtx(ctxTabCalc, << "...");
	// get the res array
	vtkDataArray* resAr = vtkDataArray::SafeDownCast(
			this->mTab->GetColumn(this->mResultColumnIndex));

	if (this->mSelectionMode)
		this->mNumSelRecs = 0;

	double res;
	QString newFunc;
	QString strFieldVal;
	int strExpRes;
	for (int row=0; row < this->mTab->GetNumberOfRows(); ++row)
	{
		if (mbRowFilter)
		{
			if (this->mFilterArray->GetTuple1(row) == 0)
				continue;
		}

		// feed the parser with numeric variables and values
		int fcnt=0;
		foreach(const int &idx, this->mLstFuncVarColumnIndex)
		{
			vtkDataArray* va = vtkDataArray::SafeDownCast(
					this->mTab->GetColumn(idx));

			this->mParser->SetScalarVariableValue(
					this->mFuncVars.at(fcnt).toStdString().c_str(),
					va->GetTuple1(row));

			if (row==0) {NMDebugAI(<< "setting numeric variable: "
					<< this->mFuncVars.at(fcnt).toStdString() << endl);}
			++fcnt;
		}


		if (row==0) {NMDebugAI(<< "processing string expressions ..." << endl);}
		// evaluate string expressions and replace them with numeric results (0 or 1)
		// in function
		newFunc = this->mFunction;
		for (int t=0; t < this->mslStrTerms.size(); ++t)
		{
			QString term = this->mslStrTerms[t];
			if (row==0) {NMDebugAI(<< "... term: " << term.toStdString() << endl);}
			QString origTerm = term;
			QList<int> idxs = this->mLstStrColumnIndices[t];
			QStringList names = this->mLstStrFieldNames[t];
			NMStrOperator op = this->mLstNMStrOperator[t];

			if (row==0) {NMDebugAI(<< "setting string variables in term " << term.toStdString() << endl);}
			for (int f=0; f < idxs.size(); ++f)
			{
				const int &idx = idxs[f];
				QString &name = names[f];

				vtkStringArray* sa = vtkStringArray::SafeDownCast(
						this->mTab->GetColumn(idx));
				term = term.replace(name, QString("'%1'").arg(sa->GetValue(row).c_str()),
						Qt::CaseInsensitive);
			}

			QRegExp rx("'([\\w\\W]*)'\\s*(=|!=|>|<|>=|<=|in|startsWith|endsWith|contains)\\s*'([\\w\\W]*)'",
					Qt::CaseInsensitive);
			int pos = rx.indexIn(term);
			if (pos == -1)
			{
				NMErr(ctxTabCalc, << "This should have never happened: String expression: "
						  << term.toStdString() << " not recognised" << endl);
				return;
			}

			// get captured parts of the term
			QStringList parts = rx.capturedTexts();

			QString left = parts[1];
			left = left.trimmed();
			QString right = parts[3];
			right = right.trimmed();

			// eval expression
			switch (op)
			{
			case NM_STR_GT:
				strExpRes = QString::localeAwareCompare(left, right) > 0 ? 1 : 0;
				break;
			case NM_STR_GTEQ:
				strExpRes = (QString::localeAwareCompare(left, right) > 0) ||
							(QString::localeAwareCompare(left, right) == 0)    ? 1 : 0;
				break;
			case NM_STR_LT:
				strExpRes = QString::localeAwareCompare(left, right) < 0 ? 1 : 0;
				break;
			case NM_STR_LTEQ:
				strExpRes = (QString::localeAwareCompare(left, right) < 0) ||
							(QString::localeAwareCompare(left, right) == 0)    ? 1 : 0;
				break;
			case NM_STR_EQ:
				strExpRes = QString::localeAwareCompare(left, right) == 0 ? 1 : 0;
				break;
			case NM_STR_NEQ:
				strExpRes = QString::localeAwareCompare(left, right) != 0 ? 1 : 0;
				break;
			case NM_STR_IN:
				strExpRes = right.contains(left, Qt::CaseInsensitive) ? 1 : 0;
				break;
			case NM_STR_CONTAINS:
				strExpRes = left.contains(right, Qt::CaseInsensitive) ? 1 : 0;
				break;
			case NM_STR_STARTSWITH:
				strExpRes = left.startsWith(right, Qt::CaseInsensitive) ? 1 : 0;
				break;
			case NM_STR_ENDSWITH:
				strExpRes = left.endsWith(right, Qt::CaseInsensitive) ? 1 : 0;
				break;
			}

			newFunc = newFunc.replace(origTerm, QString("%1").arg(strExpRes),
					Qt::CaseInsensitive);
		}

		// write result into the result column
		this->mParser->SetFunction(newFunc.toStdString().c_str());
		double res = this->mParser->GetScalarResult();

		if (this->mSelectionMode)
		{
			if (res != 0)
			{
				resAr->SetTuple1(row, 1);
				this->mTabView->selectRow(row);
				++this->mNumSelRecs;
			}
			else
			{
				resAr->SetTuple1(row, 0);
				this->mTabView->deselectRow(row);
			}
		}
		else
			resAr->SetTuple1(row, res);

		// clear all variables
		this->mParser->RemoveAllVariables();
	}
	NMDebugCtx(ctxTabCalc, << "done!");
}

void NMTableCalculator::doStringCalculation()
{
	// get the res array
	vtkStringArray* resAr = vtkStringArray::SafeDownCast(
			this->mTab->GetColumn(this->mResultColumnIndex));

	QString res;
	QString value;
	for (int row=0; row < this->mTab->GetNumberOfRows(); ++row)
	{
		if (mbRowFilter)
		{
			if (this->mFilterArray->GetTuple1(row) == 0)
				continue;
		}

		// replace numeric columns with string version of the actual value
		res = this->mFunction;
		int cnt = 0;
		foreach(const int &idx, this->mLstFuncVarColumnIndex)
		{
			vtkAbstractArray* va = this->mTab->GetColumn(idx);
			value = QString(va->GetVariantValue(row).ToString().c_str());
			res = res.replace(this->mFuncVars.at(cnt), value, Qt::CaseInsensitive);
			++cnt;
		}

		int termcnt=0;
		int fieldcnt;
		foreach(const QList<int> &ilst, this->mLstStrColumnIndices)
		{
			fieldcnt=0;
			foreach(const int& idx, ilst)
			{
				vtkStringArray* sa = vtkStringArray::SafeDownCast(
						this->mTab->GetColumn(idx));
				res = res.replace(this->mLstStrFieldNames.at(termcnt).at(fieldcnt),
						sa->GetValue(row).c_str(), Qt::CaseInsensitive);
				++fieldcnt;
			}
			++termcnt;
		}

		// write result into the result column
		resAr->SetValue(row, res.toStdString());
	}
}

std::vector<double> NMTableCalculator::calcColumnStats(QString column)
{
	// result vector containing min, max, mean, sum
	std::vector<double> res;

	// check, whether the column is available in the table
	int colidx = this->getColumnIndex(column);
	if (colidx < 0)
	{
		NMErr(ctxTabCalc, << "Specified Column couldn't be found in the table!");
		return res;
	}

	if (!this->mTab->GetColumn(colidx)->IsNumeric())
	{
		NMErr(ctxTabCalc, << "Data Type is not suitable for this kind of operation!");
		return res;
	}

	vtkDataArray* da = vtkDataArray::SafeDownCast(this->mTab->GetColumn(colidx));
	int nrows = da->GetNumberOfTuples();

	// get the first valid value
	double val;
	int r = 0;
	bool bGotInitial = false;
	while (!bGotInitial && r < nrows)
	{
		if (this->mbRowFilter)
		{
			if (this->mFilterArray->GetTuple1(r) == 0)
			{
				r = r + 1;
				continue;
			}
		}

		val = da->GetTuple1(r);
		bGotInitial = true;
		r = r + 1;
	}

	double min = val;
	double max = val;
	double sum = val;
	double mean;


	for (; r < nrows; ++r)
	{
		if (this->mbRowFilter)
		{
			if (this->mFilterArray->GetTuple1(r) == 0)
				continue;
		}

		val = da->GetTuple1(r);
		sum += val;
		min = val < min ? val : min;
		max = val > max ? val : max;
	}
	mean = sum / nrows;

	res.push_back(min);
	res.push_back(max);
	res.push_back(mean);
	res.push_back(sum);

	return res;
}

QStringList NMTableCalculator::normaliseColumns(QStringList columnNames, bool bCostCriterion)
{
	// return value
	QStringList normalisedCols;

	//check, whether all given fields are indeed in the data base
	vtkTable* tab = this->mTab;

	QList<vtkDataArray*> fieldVec;
	for (int f=0; f < columnNames.count(); ++f)
	{
		QString name = columnNames.at(f);
		int colidx = this->getColumnIndex(name);
		if (colidx >= 0)
		{
			vtkDataArray* da = vtkDataArray::SafeDownCast(
					tab->GetColumn(colidx));
			fieldVec.push_back(da);
		}
		else
		{
			NMErr(ctxTabCalc, << "Array '" << name.toStdString() << "' not found!" << endl);
			NMDebugCtx(ctxTabCalc, << "done!");
			return normalisedCols;
		}
	}

	// getting the maximum and minimum of the range of fields
	double min;
	double max;

	for (int ar=0; ar < fieldVec.size(); ++ar)
	{
		std::vector<double> stats = this->calcColumnStats(columnNames.at(ar));
		if (stats.size() == 0)
			return normalisedCols;

		if (ar == 0)
		{
			min = stats[0];
			max = stats[1];
		}
		min = stats[0] < min ? stats[0] : min;
		max = stats[1] > max ? stats[1] : max;
	}

	NMDebugAI(<< "min: " << min << " | max: " << max << endl);

	// now create a new 'normalised' array for each of the input arrays
	int nrows = fieldVec.at(0)->GetNumberOfTuples();
	for (int ar=0; ar < fieldVec.size(); ++ar)
	{
		vtkSmartPointer<vtkDoubleArray> na = vtkSmartPointer<vtkDoubleArray>::New();
		na->SetNumberOfComponents(1);
		na->Allocate(nrows);
		QString newName = QString(tr("%1_N")).arg(fieldVec.at(ar)->GetName());
		na->SetName(newName.toStdString().c_str());

		vtkDataArray* da = fieldVec.at(ar);
		double val;
		// calc normalised values
		if (bCostCriterion)
		{
			for (int r=0; r < nrows; ++r)
			{
				if (this->mbRowFilter)
				{
					if (this->mFilterArray->GetTuple1(r) == 0)
					{
						na->InsertNextTuple1(-1);
						continue;
					}
				}
				val = da->GetTuple1(r);
				na->InsertNextTuple1((max - val) / (max-min));
			}
		}
		else // must be benefit now
		{
			for (int r=0; r < nrows; ++r)
			{
				if (this->mbRowFilter)
				{
					if (this->mFilterArray->GetTuple1(r) == 0)
					{
						na->InsertNextTuple1(-1);
						continue;
					}
				}
				val = da->GetTuple1(r);
				na->InsertNextTuple1((val - min) / (max-min));
			}
		}

		tab->AddColumn(na);
		normalisedCols.append(newName);
	}

	return normalisedCols;
}
