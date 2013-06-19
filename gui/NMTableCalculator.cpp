/*****************************h*************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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

#include <QMessageBox>
#include <QString>
#include <QStack>
#include "vtkTable.h"
#include "vtkStringArray.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkVariant.h"
#include "vtkStdString.h"
#include "itkExceptionObject.h"


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
	//this->mParser = vtkSmartPointer<vtkFunctionParser>::New();
	this->mParser = otb::MultiParser::New();
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
		NMErr(ctxTabCalc, << "Invalid selection column and/or NMTableView!");
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
		NMErr(ctxTabCalc, << "Invalid Filter Column Type!");
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

	QString expr = this->mFunction;
	QStack<int> bstack;
	QStack<bool> instack;
	bool bin = false;

	// first we extract functional groups as defined by parentheses or
	// by commas to distinguish function input
	QStringList groups;
	std::stringstream term;
	int pos = 0;
	while (pos < expr.size())
	{
		QString c = expr.at(pos);
		if (c != "(" && c != ")" && c != ",")
		{
			term << c.toStdString();

			if (pos == expr.size() - 1)
			{
				if (term.str().size() > 0)
					groups.push_back(QString(term.str().c_str()));
			}
		}
		else
		{
			if (term.str().size() > 0)
			{
				groups.push_back(QString(term.str().c_str()));
				term.str("");
			}
		}

		++pos;
	}

	QRegExp exprSep("(\\&\\&|\\?|:|\\|\\|)");
	QRegExp termIdent("([_a-zA-Z]+[_a-zA-Z\\d]*|'[\\w\\d\\s\\W]*'|\\d*\\.*\\d+)"
			"\\s*(=|!=|>|<|>=|<=|==|\\+|-|\\*|/|\\^|in|!in|"
			"startsWith|endsWith|contains)\\s*"
			"([_a-zA-Z]+[_a-zA-Z\\d]*|'[\\w\\d\\s\\W]*'|\\d*\\.*\\d+)"); // right hand side
	QRegExp allOps("(?:=|!=|>|<|>=|<=|==|\\+|-|\\*|/|\\^|in|!in|startsWith|endsWith|contains)");

	// now we analyse the functional groups and break them down into parts of logical expressions
	foreach(const QString& grp, groups)
	{
		NMDebugInd(2, << "group term: " << grp.toStdString() << endl);

		QStringList logTerms = grp.split(exprSep, QString::SkipEmptyParts);
		foreach(const QString& lt, logTerms)
		{
			//if (lt.simplified().isEmpty())
			//	continue;
			NMDebugInd(4, << "logical term: " << lt.toStdString() << endl);
			int pos = termIdent.indexIn(lt);

			// ideally, we've got a proper term; however we could also have only 'half' a term
			// which is sitting outside of an expression in parenthesis or similar, so that's
			// why we then just try and identify field names
			if (pos >= 0)
			{
				QStringList guts = termIdent.capturedTexts();
				// if we haven't got 4 elements in the list, something went wrong
				if (guts.size() != 4)
				{
					NMErr(ctxTabCalc, << "Expected three element calculator expression!");
					NMDebugCtx(ctxTabCalc, << "done!");
					return false;
				}
				//NMDebugInd(6,
				//		<< "expression again? > " << guts[0].toStdString() << endl);
				//NMDebugInd(6,
				//		<< "left-hand-side: '" << guts[1].toStdString() << "'" << endl);
				//NMDebugInd(6,
				//		<< "operator: '" << guts[2].toStdString() << "'" << endl);
				//NMDebugInd(6,
				//		<< "right-hand-side: '" << guts[3].toStdString() << "'" << endl);

				// here, we check, whether the particular operator is a string operator or not
				// if yes, we add the whole expression to the list of string expressions,
				// if not, we check the left and right hand side of the expression of being
				// a numeric attribute of the table
				//if (guts.at(2).indexOf(allOps) != -1)
						//|| guts.at(2).indexOf(numOps) != -1)
				{
					bool bRightStr = false;
					bool bLeftStr = false;
					vtkAbstractArray* arLeft = 0;
					vtkAbstractArray* arRight = 0;
					arLeft = this->mTab->GetColumnByName(guts.at(1).toStdString().c_str());
					arRight = this->mTab->GetColumnByName(guts.at(3).toStdString().c_str());
					QStringList strNames;
					QList<vtkStringArray*> strFields;
					if (arLeft != 0)
					{
						if (arLeft->GetDataType() == VTK_STRING)
						{
							bLeftStr = true;
							strFields.append(vtkStringArray::SafeDownCast(arLeft));
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(1).toStdString() << endl);
						}
						else
						{
							strFields.append(0);
							this->mFuncVars.append(guts.at(1));
							this->mFuncFields.append(vtkDataArray::SafeDownCast(arLeft));
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(1).toStdString() << endl);
						}
					}
					else
						strFields.append(0);

					if (arRight != 0)
					{
						if (arRight->GetDataType() == VTK_STRING)
						{
							bRightStr = true;
							strFields.append(vtkStringArray::SafeDownCast(arRight));
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(3).toStdString() << endl);
						}
						else
						{
							strFields.append(0);
							this->mFuncVars.append(guts.at(3));
							this->mFuncFields.append(vtkDataArray::SafeDownCast(arRight));
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(3).toStdString() << endl);
						}
					}
					else
						strFields.append(0);

					// is this a string expression?
					if (bLeftStr || bRightStr)
					{
						NMDebugAI(<< "detected string term: " << lt.toStdString() << endl);
						this->mslStrTerms.append(lt);

						QRegExp stripHighComma("'([\\w\\d\\s\\W]*)'");
						int pos = stripHighComma.indexIn(guts.at(1));
						if (bLeftStr)
							strNames << guts.at(1);
						else
							strNames << stripHighComma.capturedTexts().at(1);

						pos = stripHighComma.indexIn(guts.at(3));
						if (bRightStr)
							strNames << guts.at(3);
						else
							strNames << stripHighComma.capturedTexts().at(1);

						this->mLstStrLeftRight.append(strNames);
						this->mLstLstStrFields.append(strFields);
						this->mLstNMStrOperator.append(this->getStrOperator(guts.at(2)));
						NMDebugAI(<< "eval string term: " << strNames.at(0).toStdString() << " "
								<< guts.at(2).toStdString() << " "
								<< strNames.at(1).toStdString() << endl);
					}
				}
			}
			else
			{
				QStringList scrambledEggs = lt.split(allOps,
						QString::SkipEmptyParts);
				//NMDebugInd(6,
				//		<< "are those fields? : " << scrambledEggs.join(" ").toStdString() << endl);
				// if we've got a numeric field here, we register the name with the
				// the parser; in the calc routine, we then look up the particular
				// value for each row and just set the value for the registered variable
				// muParser deals with the rest;
				foreach(const QString& egg, scrambledEggs)
				{
					vtkAbstractArray* ar = this->mTab->GetColumnByName(egg.toStdString().c_str());
					if (ar != 0)
					{
						if (ar->GetDataType() != VTK_STRING)
						{
							this->mFuncVars.append(egg);
							this->mFuncFields.append(vtkDataArray::SafeDownCast(ar));
							NMDebugAI(<< "detected NUMERIC field: "
									<< egg.toStdString() << endl);
						}
						else
						{
							//ToDo:: does this actually makes sense?
							this->mslStrTerms.append(egg);
							QList<vtkStringArray*> strFields;
							strFields.append(vtkStringArray::SafeDownCast(ar));
							strFields.append(0);
							this->mLstLstStrFields.append(strFields);
							this->mLstNMStrOperator.append(NMTableCalculator::NM_STR_UNKNOWN);
							NMDebugAI(<< "detected STRING field: "
									<< egg.toStdString() << endl);
						}
					}
				}
			}
		}
	}

	NMDebugCtx(ctxTabCalc, << "done!");
	return true;
}

NMTableCalculator::NMStrOperator
NMTableCalculator::getStrOperator(const QString& strOp)
{
	NMStrOperator sop;
	if (strOp == ">")
		sop = NM_STR_GT;
	else if (strOp == ">=")
		sop = NM_STR_GTEQ;
	else if (strOp == "<")
		sop = NM_STR_LT;
	else if (strOp == "<=")
		sop = NM_STR_LTEQ;
	else if (strOp == "==")
		sop = NM_STR_EQ;
	else if (strOp == "!=")
		sop = NM_STR_NEQ;
	else if (strOp.toLower() == "in")
		sop = NM_STR_IN;
	else if (strOp.toLower() == "!in")
		sop = NM_STR_NOTIN;
	else if (strOp.toLower() == "contains")
		sop = NM_STR_CONTAINS;
	else if (strOp.toLower() == "startswith")
		sop = NM_STR_STARTSWITH;
	else if (strOp.toLower() == "endswith")
		sop = NM_STR_ENDSWITH;
	else
		sop = NM_STR_UNKNOWN;

	return sop;
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
	this->mFuncFields.clear();
	this->mslStrTerms.clear();
	this->mLstStrLeftRight.clear();
	this->mLstLstStrFields.clear();
	this->mLstNMStrOperator.clear();
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
		foreach(const QString &colName, this->mFuncVars)
		{
			//this->mParser->SetScalarVariableValue(
			//		this->mFuncVars.at(fcnt).toStdString().c_str(),
			//		this->mFuncFields.at(fcnt)->GetTuple1(row));

			this->mParser->DefineVar(this->mFuncVars.at(fcnt).toStdString(),
					this->mFuncFields.at(fcnt)->GetTuple(row));

			if (row==0) {NMDebugAI(<< "setting numeric variable: "
					<< this->mFuncVars.at(fcnt).toStdString() << endl);}
			++fcnt;
		}

		// evaluate string expressions and replace them with numeric results (0 or 1)
		// in function
		newFunc = this->mFunction;
		for (int t=0; t < this->mslStrTerms.size(); ++t)
		{
			if (row==0 && t==0) {NMDebugAI(<< "processing string expressions ..." << endl);}

			QString origTerm = this->mslStrTerms.at(t);
			if (row==0) {NMDebugAI(<< "... term: " << origTerm.toStdString() << endl);}

			vtkStringArray* leftField = this->mLstLstStrFields.at(t).at(0);
			vtkStringArray* rightField = this->mLstLstStrFields.at(t).at(1);
			QString left;
			QString right;
			if (leftField != 0)
				left = leftField->GetValue(row).c_str();
			else
				left = this->mLstStrLeftRight.at(t).at(0);

			if (rightField != 0)
				right = rightField->GetValue(row).c_str();
			else
				right = this->mLstStrLeftRight.at(t).at(1);

			// debug
			if (row == 0)
			{
				NMDebugAI(<< "... evaluating: "
						<< left.toStdString() << " "
						<< mLstNMStrOperator.at(t) << " "
						<< right.toStdString() << endl);
			}

			// eval expression
			switch (this->mLstNMStrOperator.at(t))
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
				{
					QStringList rl = right.split(" ");
					strExpRes = rl.contains(left, Qt::CaseInsensitive) ? 1 : 0;
				}
				break;
			case NM_STR_NOTIN:
				{
					QStringList rl = right.split(" ");
					strExpRes = rl.contains(left, Qt::CaseInsensitive) ? 0 : 1;
				}
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

		this->mParser->SetExpr(newFunc.toStdString());
		double res;
		try
		{
			res = this->mParser->Eval();
		}
		catch(itk::ExceptionObject& err)
		{
			NMDebugAI(<< res << "oops - functions parser threw an exception!" << endl);
			QMessageBox msgBox;
			msgBox.setText(tr("Invalid Where Clause!\nPlease check syntax and try again."));
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.exec();

			NMErr(ctxTabCalc, << "Invalid expression!");
			NMDebugCtx(ctxTabCalc, << "done!");

			throw err;
			return;
		}

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
		foreach(const QString& fieldName, this->mFuncVars)
		{
			// we make sure, we've got a proper field name here and
			// don't just replace the middle of a fieldname
			QString rx = QString("\\b%1\\b").arg(fieldName);
			value = QString(this->mFuncFields.at(cnt)->GetVariantValue(row).ToString().c_str());
			res = res.replace(QRegExp(rx), value);
			++cnt;
		}

		int termcnt=0;
		int fieldcnt;
		foreach(const QString& st, this->mslStrTerms)
		{
			QString rx = QString("\\b%1\\b").arg(this->mLstStrLeftRight.at(termcnt).at(0));
			value = QString(this->mLstLstStrFields.at(termcnt).at(0)->GetValue(row).c_str());
			res = res.replace(QRegExp(rx), value);
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
