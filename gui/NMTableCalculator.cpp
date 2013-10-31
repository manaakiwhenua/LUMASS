/*****************************h*******A***********dx*******************************
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
#include "nmlog.h"

#include <QString>
#include <QStack>
#include <QModelIndex>

#include "vtkVariant.h"
#include "vtkStdString.h"
#include "vtkDoubleArray.h"
#include "itkExceptionObject.h"


NMTableCalculator::NMTableCalculator(QObject* parent)
	: QObject(parent), mModel(0)
{
	this->initCalculator();
}

//NMTableCalculator::NMTableCalculator(vtkTable* tab, QObject* parent)
//	: QObject(parent), mTab(tab)
//{
//	this->initCalculator();
//}

NMTableCalculator::NMTableCalculator(QAbstractItemModel* model,
		QObject* parent)
	: QObject(parent), mModel(model)
{
	this->initCalculator();
}

void NMTableCalculator::initCalculator()
{
	this->mParser = otb::MultiParser::New();
	this->mFunction.clear();
	this->mResultColumn.clear();
	this->mFuncFields.clear();
	//this->mInputSelection = 0;
	this->mInputSelection.clear();
	this->mOutputSelection = 0;
	//this->mSelection.clear();
	this->mResultColumnType = QVariant::String;
	this->mResultColumnIndex = -1;
	this->mSelectionMode = false;
	this->mbRowFilter = false;
	//this->mFilterArray = 0;
	this->mNumSelRecs = 0;
	this->clearLists();
}

NMTableCalculator::~NMTableCalculator()
{

}

const QItemSelection*
NMTableCalculator::getSelection(void)
{
	return this->mOutputSelection;
}

bool NMTableCalculator::setResultColumn(const QString& name)
{
	// check, whether the column is available in the table
	int colidx = this->getColumnIndex(name);
	if (colidx < 0)
	{
		NMErr(ctxTabCalc, << "Specified Column couldn't be found in the table!");
		return false;
	}

	QModelIndex dummy = this->mModel->index(0, colidx, QModelIndex());

	this->mResultColumn = name;
	this->mResultColumnIndex = colidx;
	this->mResultColumnType = this->mModel->data(dummy, Qt::DisplayRole).type();
	return true;
}

void NMTableCalculator::setFunction(const QString& function)
{
	this->mFunction = function;
}

int NMTableCalculator::getColumnIndex(const QString& name)
{
	int colidx = -1;
	int ncols = this->mModel->columnCount(QModelIndex());
	QString colname;
	for (int c=0; c < ncols; ++c)
	{
		colname = this->mModel->headerData(c, Qt::Horizontal).toString();
		if (colname.compare(name, Qt::CaseInsensitive) == 0)
		{
			colidx = c;
			break;
		}
	}

	return colidx;
}

bool
NMTableCalculator::setSelectionModeOn(QItemSelection* outputSelection)
{
	//NMDebugCtx(ctxTabCalc, << "...");
	//if (!this->setResultColumn(selColumn))// || tabView == 0)
	//{
	//	NMErr(ctxTabCalc, << "Invalid selection column and/or NMTableView!");
	//	return false;
	//}

	if (outputSelection == 0)
	{
		NMErr(ctxTabCalc, << "Please provide pre-allocated QItemSelection!");
		return false;
	}

	this->mOutputSelection = outputSelection;
	this->mSelectionMode = true;

	NMDebugAI(<< "selection mode on? " << this->mSelectionMode << endl);

	//NMDebugCtx(ctxTabCalc, << "done!");
	return true;
}

bool
NMTableCalculator::setRowFilterModeOn(QItemSelection* inputSelection)
{

	if (inputSelection == 0)
	{
		NMErr(ctxTabCalc, << "Please provided preallocated Item Selection!");
		return false;
	}

	this->mInputSelection = inputSelection;
	this->mbRowFilter = true;

	return true;
}

QVariant::Type
NMTableCalculator::getColumnType(int colidx)
{
	QModelIndex midx = this->mModel->index(0, colidx, QModelIndex());
	return this->mModel->data(midx, Qt::DisplayRole).type();
}

bool
NMTableCalculator::isNumericColumn(int colidx)
{
	QVariant::Type type = this->getColumnType(colidx);
	if (	type == QVariant::Int
		||  type == QVariant::UInt
		||  type == QVariant::Double)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool
NMTableCalculator::isStringColumn(int colidx)
{
	QVariant::Type type = this->getColumnType(colidx);
	if (type == QVariant::String)
		return true;
	else
		return false;
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
					int idxleft = this->getColumnIndex(guts.at(1));
					int idxright = this->getColumnIndex(guts.at(2));
					QStringList strNames;
					QList<int> strFields;
					if (idxleft >= 0)
					{
						if (this->isStringColumn(idxleft))
						{
							bLeftStr = true;
							strFields.append(idxleft);
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(1).toStdString() << endl);
						}
						else
						{
							strFields.append(-1);
							this->mFuncVars.append(guts.at(1));
							this->mFuncFields.append(idxleft);
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(1).toStdString() << endl);
						}
					}
					else
					{
						// we thought we've got a column here, but don't!
						// Let's get out of here!
						NMErr(ctxTabCalc, << "Expected " << guts.at(1).toStdString()
								<< " to be a table column, but that's not the case!");
						NMDebugCtx(ctxTabCalc, << "done!");
						return false;
					}

					if (idxright >= 0)
					{
						if (this->isStringColumn(idxright))
						{
							bRightStr = true;
							strFields.append(idxright);
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(3).toStdString() << endl);
						}
						else
						{
							strFields.append(-1);
							this->mFuncVars.append(guts.at(3));
							this->mFuncFields.append(idxright);
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(3).toStdString() << endl);
						}
					}
					else
					{
						// we thought we've got a column here, but don't!
						// Let's get out of here!
						NMErr(ctxTabCalc, << "Expected " << guts.at(2).toStdString()
								<< " to be a table column, but that's not the case!");
						NMDebugCtx(ctxTabCalc, << "done!");
						return false;
					}

					// analyse string expression
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
					QString item = egg.trimmed();
					//vtkAbstractArray* ar = this->mTab->GetColumnByName(item.toStdString().c_str());
					int aridx = this->getColumnIndex(item);

					if (aridx >= 0)
					{
						if (!this->isStringColumn(aridx))
						{
							this->mFuncVars.append(item);
							this->mFuncFields.append(aridx);
							NMDebugAI(<< "detected NUMERIC field: "
									<< item.toStdString() << endl);
						}
						else
						{
							//ToDo:: does this actually make sense?
							this->mslStrTerms.append(item);
							QList<int> strFields;
							strFields.append(aridx);
							strFields.append(-1);
							this->mLstLstStrFields.append(strFields);
							this->mLstNMStrOperator.append(NMTableCalculator::NM_STR_UNKNOWN);
							NMDebugAI(<< "detected STRING field: "
									<< egg.toStdString() << endl);
						}
					}
					else
					{
						NMDebugAI(<< "couldn't do anything with this egg: "
								<< item.toStdString() << endl);
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

	if (this->mResultColumnType == QVariant::String)
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
	//vtkDataArray* resAr = vtkDataArray::SafeDownCast(
	//		this->mTab->GetColumn(this->mResultColumnIndex));

	if (this->mSelectionMode)
		this->mNumSelRecs = 0;

	double res;
	QString newFunc;
	QString strFieldVal;
	int strExpRes;
	for (int row=0; row < this->mModel->rowCount(QModelIndex()); ++row)
	{
		if (mbRowFilter)
		{
			QModelIndex rowidx = this->mModel->index(row, 0, QModelIndex());
			if (!this->mInputSelection->contains(rowidx))
				continue;
		}

		// feed the parser with numeric variables and values
		bool bok;
		int fcnt=0;
		foreach(const QString &colName, this->mFuncVars)
		{
			//this->mParser->SetScalarVariableValue(
			//		this->mFuncVars.at(fcnt).toStdString().c_str(),
			//		this->mFuncFields.at(fcnt)->GetTuple1(row));

			QModelIndex fieldidx = this->mModel->index(row, mFuncFields.at(fcnt), QModelIndex());
			double value = this->mModel->data(fieldidx, Qt::DisplayRole).toDouble(&bok);
			if (bok)
			{
				this->mParser->DefineVar(this->mFuncVars.at(fcnt).toStdString(), &value);
			}
			else
			{
				NMErr(ctxTabCalc, << "Encountered invalid numeric value in column "
						<< mFuncFields.at(fcnt) << " at row " << row
						<< "!");
				break;
			}

			if (row==0) {NMDebugAI(<< "setting numeric variable: "
					<< this->mFuncVars.at(fcnt).toStdString() << endl);}
			++fcnt;
		}
		if (!bok)
		{
			NMErr(ctxTabCalc, << "Skipping rest of invalid calculation in row " << row << "!");
			continue;
		}


		// evaluate string expressions and replace them with numeric results (0 or 1)
		// in function
		newFunc = this->mFunction;
		for (int t=0; t < this->mslStrTerms.size(); ++t)
		{
			if (row==0 && t==0) {NMDebugAI(<< "processing string expressions ..." << endl);}

			QString origTerm = this->mslStrTerms.at(t);
			if (row==0) {NMDebugAI(<< "... term: " << origTerm.toStdString() << endl);}

			//vtkStringArray* leftField = this->mLstLstStrFields.at(t).at(0);
			//vtkStringArray* rightField = this->mLstLstStrFields.at(t).at(1);
			int idxleft = this->mLstLstStrFields.at(t).at(0);
			int idxright = this->mLstLstStrFields.at(t).at(1);
			QString left;
			QString right;
			if (idxleft >= 0)
			{
				QModelIndex fidx = this->mModel->index(row, idxleft, QModelIndex());
				left = this->mModel->data(fidx, Qt::DisplayRole).toString();
			}
			else
			{
				left = this->mLstStrLeftRight.at(t).at(0);
			}

			if (idxright >= 0)
			{
				QModelIndex fidx = this->mModel->index(row, idxright, QModelIndex());
				right = this->mModel->data(fidx, Qt::DisplayRole).toString();
			}
			else
			{
				right = this->mLstStrLeftRight.at(t).at(1);
			}

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

		double res;
		try
		{
			this->mParser->SetExpr(newFunc.toStdString());
			res = this->mParser->Eval();
		}
		catch(itk::ExceptionObject& err)
		{
			//NMDebugAI(<< res << "oops - functions parser threw an exception!" << endl);
			//QMessageBox msgBox;
			//msgBox.setText(tr("Invalid Where Clause!\nPlease check syntax and try again."));
			//msgBox.setIcon(QMessageBox::Critical);
			//msgBox.exec();

			NMErr(ctxTabCalc, << "Invalid expression detected!");
			NMDebugCtx(ctxTabCalc, << "done!");

			throw err;
			return;
		}

		if (this->mSelectionMode)
		{
			QModelIndex left = this->mModel->index(row, 0, QModelIndex());
			QModelIndex right = this->mModel->index(row,
					this->mModel->columnCount(QModelIndex())-1, QModelIndex());
			QItemSelection rowsel(left, right);
			if (res != 0)
			{
				this->mOutputSelection->merge(rowsel, QItemSelectionModel::Select);
				//resAr->SetTuple1(row, 1);
				//this->mTabView->selectRow(row);
				++this->mNumSelRecs;
			}
			else
			{
				this->mOutputSelection->merge(rowsel, QItemSelectionModel::Deselect);
				//resAr->SetTuple1(row, 0);
				//this->mTabView->deselectRow(row);
			}
		}
		else
		{
			QModelIndex resIdx = this->mModel->index(row, this->mResultColumnIndex, QModelIndex());
			this->mModel->setData(resIdx, QVariant(res));
			//resAr->SetTuple1(row, res);
		}
	}
	NMDebugCtx(ctxTabCalc, << "done!");
}

void NMTableCalculator::doStringCalculation()
{
	// get the res array
	//vtkStringArray* resAr = vtkStringArray::SafeDownCast(
	//		this->mTab->GetColumn(this->mResultColumnIndex));

	QString res;
	QString value;
	for (int row=0; row < this->mModel->rowCount(QModelIndex()); ++row)
	{
		if (mbRowFilter)
		{
			QModelIndex rowidx = this->mModel->index(row, 0, QModelIndex());
			if (!this->mInputSelection->contains(rowidx))
				continue;
		}

		// replace numeric columns with string version of the actual value
		res = this->mFunction;
		int cnt = 0;
		foreach(const QString& fieldName, this->mFuncVars)
		{
			// get value from the model
			QModelIndex fidx = this->mModel->index(row, this->mFuncFields.at(cnt), QModelIndex());
			value = this->mModel->data(fidx, Qt::DisplayRole).toString();

			// we make sure, we've got a proper field name here and
			// don't just replace the middle of a fieldname
			QString rx = QString("\\b%1\\b").arg(fieldName);
			res = res.replace(QRegExp(rx), value);

			++cnt;
		}

		int termcnt=0;
		int fieldcnt;
		foreach(const QString& st, this->mslStrTerms)
		{
			// get value from the model
			QModelIndex fidx = this->mModel->index(row,
					this->mLstLstStrFields.at(termcnt).at(0), QModelIndex());
			value = this->mModel->data(fidx, Qt::DisplayRole).toString();

			QString rx = QString("\\b%1\\b").arg(this->mLstStrLeftRight.at(termcnt).at(0));
			//value = QString(this->mLstLstStrFields.at(termcnt).at(0)->GetValue(row).c_str());
			res = res.replace(QRegExp(rx), value);
			++termcnt;
		}

		// write result into the result column
		QModelIndex resIdx = this->mModel->index(row, this->mResultColumnIndex, QModelIndex());
		this->mModel->setData(resIdx, QVariant(res));
		//resAr->SetValue(row, res.toStdString());
	}
}

std::vector<double>
NMTableCalculator::calcColumnStats(const QString& column)
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

	if (!this->isNumericColumn(colidx))
	{
		NMErr(ctxTabCalc, << "Data Type is not suitable for this kind of operation!");
		return res;
	}

	//vtkDataArray* da = vtkDataArray::SafeDownCast(this->mTab->GetColumn(colidx));
	int nrows = this->mModel->rowCount(QModelIndex());

	// get the first valid value
	bool bok;
	double val;
	int r = 0;
	bool bGotInitial = false;
	while (!bGotInitial && r < nrows)
	{
		if (this->mbRowFilter)
		{
			QModelIndex rowidx = this->mModel->index(r, 0, QModelIndex());
			if (!this->mInputSelection->contains(rowidx))
			//if (this->mFilterArray->GetTuple1(r) == 0)
			{
				r = r + 1;
				continue;
			}
		}

		QModelIndex valIdx = this->mModel->index(r, colidx, QModelIndex());
		val = this->mModel->data(valIdx, Qt::DisplayRole).toDouble(&bok);
		if (!bok)
		{
			NMErr(ctxTabCalc, << "Calc Column Stats for '" << column.toStdString()
					<< "': Disregarding invalid value at row " << r << "!");
			r = r + 1;
			continue;
		}

		//val = da->GetTuple1(r);
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
			QModelIndex rowidx = this->mModel->index(r, 0, QModelIndex());
			if (!this->mInputSelection->contains(rowidx))
				continue;
		}

		//val = da->GetTuple1(r);

		QModelIndex valIdx = this->mModel->index(r, colidx, QModelIndex());
		val = this->mModel->data(valIdx, Qt::DisplayRole).toDouble(&bok);
		if (bok)
		{
			sum += val;
			min = val < min ? val : min;
			max = val > max ? val : max;
		}
		else
		{
			NMErr(ctxTabCalc, << "Calc Column Stats for '" << column.toStdString()
					<< "': Disregarding invalid value at row " << r << "!");
		}
	}
	mean = sum / nrows;

	res.push_back(min);
	res.push_back(max);
	res.push_back(mean);
	res.push_back(sum);

	return res;
}

QStringList NMTableCalculator::normaliseColumns(const QStringList& columnNames,
		bool bCostCriterion)
{
	// return value
	QStringList normalisedCols;

	QList<int> fieldVec;
	for (int f=0; f < columnNames.count(); ++f)
	{
		QString name = columnNames.at(f);
		int colidx = this->getColumnIndex(name);
		if (colidx >= 0 && this->isNumericColumn(colidx))
		{
			fieldVec.push_back(colidx);
		}
		else
		{
			NMErr(ctxTabCalc, << "Could't find any numeric array '"
					<< name.toStdString() << "'!" << endl);
			NMDebugCtx(ctxTabCalc, << "done!");
			return normalisedCols;
		}
	}

	// getting the maximum and minimum of the range of fields
	double min = std::numeric_limits<double>::max();
	double max = -std::numeric_limits<double>::max();;

	for (int field=0; field < fieldVec.size(); ++field)
	{
		std::vector<double> stats = this->calcColumnStats(columnNames.at(field));
		if (stats.size() == 0)
			return normalisedCols;

		min = stats[0] < min ? stats[0] : min;
		max = stats[1] > max ? stats[1] : max;
	}

	NMDebugAI(<< "min: " << min << " | max: " << max << endl);

	// now create a new 'normalised' "<field name>_N" array for each of the input arrays

	// type variable to denote the column type
	bool bSomethingWrong = false;
	QVariant::Type ftype = QVariant::Double;
	QList<int> nfIdx; // the new field indices
	int nfields = this->mModel->columnCount(QModelIndex());
	for (int field=0; field < fieldVec.size(); ++field)
	{
		QString name = QString("%1_N").arg(columnNames.at(field));

		QModelIndex indexType = this->mModel->index(0, 0, QModelIndex());
		indexType.internalPointer() = (void*)(&ftype);

		if (!this->mModel->insertColumns(0, 0, indexType))
		{
			NMErr(ctxTabCalc, << "Failed to add a column to the model!");
			bSomethingWrong = true;
			break;
		}

		// give the column a name
		if (!this->mModel->setHeaderData(nfields+field, Qt::Horizontal, QVariant(name), Qt::EditRole))
		{
			NMErr(ctxTabCalc, << "Failed to set name for column '" << name.toStdString() << "'!");
			bSomethingWrong = true;
			break;
		}

		// add new column index to list
		if (!bSomethingWrong)
		{
			nfIdx.push_back(nfields + field);
			normalisedCols.push_back(name);
		}
	}

	// in case bSomethingWrong, we delete the successful added columns ("roll back")
	if (bSomethingWrong)
	{
		foreach(const int& idx, nfIdx)
		{
			this->mModel->removeColumns(idx, 0, QModelIndex());
		}

		normalisedCols.clear();
		NMDebugCtx(ctxTabCalc, << "done!");
		return normalisedCols;
	}

	// SO, HERE IS WHAT WE ACTUALLY DO
	// val = getValue(row, column)
	// cost formula:    val = (max - val) / (max-min)
	// benefit formula: val = (val - min) / (max-min)
	double diff = max-min;

	long nrows = this->mModel->rowCount(QModelIndex());
	int srcIdx, tarIdx;
	bool bok;
	for(int field=0; field < fieldVec.size(); ++field)
	{
		srcIdx = fieldVec.at(field);
		tarIdx = nfIdx.at(field);

		// get the selected model indices to not have to go through the whole table
		if (this->mbRowFilter && this->mInputSelection.size() > 0)
		{
			//QModelIndexList indices = this->mInputSelection->indexes();
			foreach(const QModelIndex& idx, this->mInputSelection)
			{
				// TODO: we need to check, whether actually there's only one index for each row!!!
				QModelIndex misrc = this->mModel->index(idx.row(), srcIdx, QModelIndex());
				QModelIndex mitar = this->mModel->index(idx.row(), tarIdx, QModelIndex());

				double val = this->mModel->data(misrc, Qt::DisplayRole).toDouble(&bok);
				if (!bok)
				{
					NMErr(ctxTabCalc, << "Failed getting input value for '"
						<< this->mModel->headerData(srcIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << idx.row() << "!");
					continue;
				}

				double normval = (max - val) / diff;

				if (!this->mModel->setData(mitar, QVariant(normval), Qt::EditRole))
				{
					NMErr(ctxTabCalc, << "Failed setting normalised value for '"
						<< this->mModel->headerData(tarIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << idx.row() << "!");
					continue;
				}
			}
		}
		else // we iterate over the whole table
		{
			for (int row=0; row < nrows; ++row)
			{
				QModelIndex misrc = this->mModel->index(row, srcIdx, QModelIndex());
				QModelIndex mitar = this->mModel->index(row, tarIdx, QModelIndex());

				double val = this->mModel->data(misrc, Qt::DisplayRole).toDouble(&bok);
				if (!bok)
				{
					NMErr(ctxTabCalc, << "Failed getting input value for '"
						<< this->mModel->headerData(srcIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << row << "!");
					continue;
				}

				double normval = (max - val) / diff;

				if (!this->mModel->setData(mitar, QVariant(normval), Qt::EditRole))
				{
					NMErr(ctxTabCalc, << "Failed setting normalised value for '"
						<< this->mModel->headerData(tarIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << row << "!");
					continue;
				}
			}
		}
	}
	return normalisedCols;
}
