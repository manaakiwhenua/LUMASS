/******************************************************************************
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

#include <limits>
#include <algorithm>

#include <QString>
#include <QStack>
#include <QModelIndex>
#include <QDateTime>

//#include "vtkVariant.h"
//#include "vtkStdString.h"
//#include "vtkDoubleArray.h"
#include "itkExceptionObject.h"

#include "valgrind/callgrind.h"


//NMTableCalculator::NMTableCalculator(QObject* parent)
//	: QObject(parent), mModel(0)
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
	this->mResultColumnType = QVariant::String;
	this->mResultColumnIndex = -1;
	this->mSelectionModeOn = false;
	this->mbRowFilter = false;
	this->mInputSelection.clear();
	this->mOutputSelection.clear();
	this->clearLists();
}

NMTableCalculator::~NMTableCalculator()
{
}

const QItemSelection*
NMTableCalculator::getSelection(void)
{
	//std::stable_sort(mOutputSrcSelIndices.begin(), mOutputSrcSelIndices.end());

	QItemSelection& isel = mOutputSelection;
	//const int& nrows = mOutputSrcSelIndices.size();
	//if (nrows == 0)
	//	return isel;
    //
    //
	//NMDebugAI(<< "TableCalc::getSelection(): processing #" << nrows << " selections ... ");
    //
	//int start = mOutputSrcSelIndices.at(0);
	//int end   = start;
	//for (int r=0; r < nrows; ++r)
	//{
	//	// expansion of selection range
	//	if (mOutputSrcSelIndices.at(r) == end + 1)
	//	{
	//		++end;
    //
	//		if (r == nrows -1)
	//		{
	//			QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
	//			QModelIndex eidx = this->mModel->index(end, 0, QModelIndex());
	//			isel.append(QItemSelectionRange(sidx, eidx));
	//		}
	//	}
	//	else
	//	{
	//		QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
	//		QModelIndex eidx = this->mModel->index(end, 0, QModelIndex());
	//		isel.append(QItemSelectionRange(sidx, eidx));
    //
	//		start = mOutputSrcSelIndices.at(r);
	//		end   = start;
    //
	//		// end of selection range
	//		if (r == nrows -1 && nrows > 1)
	//		{
	//			QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
	//			QModelIndex eidx = this->mModel->index(end, 0, QModelIndex());
	//			isel.append(QItemSelectionRange(sidx, eidx));
	//		}
	//	}
	//}
	//NMDebug(<< "done!" << std::endl);
	return &isel;
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

void
NMTableCalculator::setSelectionMode(bool mode)
{
	this->mSelectionModeOn = mode;
}

bool
NMTableCalculator::setRowFilter(const QItemSelection& inputSelection)
{

	if (inputSelection.size() == 0)
	{
		NMWarn(ctxTabCalc, << "Selection is empty! Row filter is turned off!");
		this->mInputSelection.clear();
		this->mbRowFilter = false;
		return false;
	}

	this->mInputSelection = inputSelection;
	this->mbRowFilter = true;

	//this->printSelRanges(mInputSelection, "TabCalc input selection");

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
	NMDebugAI(<< __FUNCTION__ << "variant type: " << QVariant::typeToName(type) << std::endl);
	if (	type == QVariant::Int
		||  type == QVariant::UInt
		||  type == QVariant::LongLong
		||  type == QVariant::ULongLong
		||  type == QVariant::Double
		||  type == QVariant::Char
	   )
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
	if (	this->mFunction.isEmpty()
		||  (this->mResultColumn.isEmpty() && !this->mSelectionModeOn)
	   )
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
		NMDebugInd(2, << "group term: " << grp.toStdString() << std::endl);

		QStringList logTerms = grp.split(exprSep, QString::SkipEmptyParts);
		foreach(const QString& lt, logTerms)
		{
			//if (lt.simplified().isEmpty())
			//	continue;
			NMDebugInd(4, << "logical term: " << lt.toStdString() << std::endl);
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
				//		<< "expression again? " << guts[0].toStdString() << endl);
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
					int idxright = this->getColumnIndex(guts.at(3));
					QStringList strNames;
					QList<int> strFields;
					if (idxleft >= 0)
					{
						if (this->isStringColumn(idxleft))
						{
							bLeftStr = true;
							strFields.append(idxleft);
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(1).toStdString() << std::endl);
						}
						else
						{
							strFields.append(-1);
							this->mFuncVars.append(guts.at(1));
							this->mFuncFields.append(idxleft);
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(1).toStdString() << std::endl);
						}
					}
					else
						strFields << -1;

					if (idxright >= 0)
					{
						if (this->isStringColumn(idxright))
						{
							bRightStr = true;
							strFields.append(idxright);
							NMDebugAI(<< "detected STRING field: "
									<< guts.at(3).toStdString() << std::endl);
						}
						else
						{
							strFields.append(-1);
							this->mFuncVars.append(guts.at(3));
							this->mFuncFields.append(idxright);
							NMDebugAI(<< "detected NUMERIC field: "
									<< guts.at(3).toStdString() << std::endl);
						}
					}
					else
						strFields << -1;


					// analyse string expression
					if (bLeftStr || bRightStr)
					{
						NMDebugAI(<< "detected string term: " << lt.toStdString() << std::endl);
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
								<< strNames.at(1).toStdString() << std::endl);
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
									<< item.toStdString() << std::endl);
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
									<< egg.toStdString() << std::endl);
						}
					}
					else
					{
						NMDebugAI(<< "Couldn't do anything with this egg: "
								<< item.toStdString() << std::endl
								<< "We leave it in the capable hands of the muParser!"
								<< std::endl);
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

	if (	this->mResultColumnType == QVariant::String
		&&  !this->mSelectionModeOn
	   )
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

void
NMTableCalculator::doNumericCalcSelection()
{
	NMDebugCtx(ctxTabCalc, << "...");

	if (this->mSelectionModeOn)
	{
		mOutputSelection.clear();
		mNumSel = 0;
	}

	QDateTime started = QDateTime::currentDateTime();
	QItemSelection& isel = mOutputSelection;
	const int maxcolidx = 0;
	if (this->mbRowFilter)
	{
		foreach(const QItemSelectionRange& range, this->mInputSelection)
		{
			const int top = range.top();
			const int bottom = range.bottom();
			const int nrows = bottom - top + 1;
			int start = -1;
			int end = -1;
			bool selected;
			for (int row=top; row <= bottom; ++row)
			{
				if (mRaw2Source->at(row) == -1)
					continue;

				this->processNumericCalcSelection(row, &selected);
				if (selected)
				{
					if (start == -1)
					{
						start = row;
						end   = start;

						if (row == bottom)
						{
							QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
							QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
							isel.append(QItemSelectionRange(sidx, eidx));
						}
					}
					// expansion of selection range
					else if (row == end + 1)
					{
						++end;

						if (row == bottom)
						{
							QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
							QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
							isel.append(QItemSelectionRange(sidx, eidx));
						}
					}
				}
				else
				{
					if (start != -1 && end != -1)
					{
						QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
						QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
						isel.append(QItemSelectionRange(sidx, eidx));

						start = -1;
						end   = -1;
					}
				}
			}
		}
	}
	else
	{
		const int& nrows = this->mModel->rowCount();
		const int& bottom = nrows-1;
		int start = -1;
		int end = -1;
		bool selected;
		for (int row=0; row < nrows; ++row)
		{
			if (mRaw2Source->at(row) == -1)
				continue;

			this->processNumericCalcSelection(row, &selected);
			if (selected)
			{
				if (start == -1)
				{
					start = row;
					end   = start;

					if (row == bottom)
					{
						QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
						QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
						isel.append(QItemSelectionRange(sidx, eidx));
					}
				}
				// expansion of selection range
				else if (row == end + 1)
				{
					++end;

					if (row == bottom)
					{
						QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
						QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
						isel.append(QItemSelectionRange(sidx, eidx));
					}
				}
			}

			else
			{
				if (start != -1 && end != -1)
				{
					QModelIndex sidx = this->mModel->index(start, 0, QModelIndex());
					QModelIndex eidx = this->mModel->index(end, maxcolidx, QModelIndex());
					isel.append(QItemSelectionRange(sidx, eidx));

					start = -1;
					end   = -1;
				}
			}
		}
	}

	// get some feedback as to how long the calculation took
	QDateTime stopped = QDateTime::currentDateTime();
	int msec = started.msecsTo(stopped);
	int min = msec / 60000;
	double sec = (msec % 60000) / 1000.0;

	QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
	NMMsg(<< "Table calculation took (min:sec): " << elapsedTime.toStdString() << std::endl);

	NMDebugCtx(ctxTabCalc, << "done!");
}

void
NMTableCalculator::processNumericCalcSelection(int row, bool* selected)
{
	double res;
	QString newFunc;
	QString strFieldVal;
	int strExpRes;
	*selected = false;

	//NMDebug(<< "row #" << row << " ");

	// feed the parser with numeric variables and values
	bool bok = true;
	int fcnt=0;
	std::vector<double> funcvals(this->mFuncVars.size());
	foreach(const QString &colName, this->mFuncVars)
	{
		QModelIndex fieldidx = this->mModel->index(row, mFuncFields.at(fcnt), QModelIndex());
		funcvals[fcnt] = this->mModel->data(fieldidx, Qt::DisplayRole).toDouble(&bok);
		if (bok)
		{
			this->mParser->DefineVar(this->mFuncVars.at(fcnt).toStdString(), &funcvals[fcnt]);
		}
		else
		{
			NMErr(ctxTabCalc, << "Encountered invalid numeric value in column "
					<< mFuncFields.at(fcnt) << " at row " << row
					<< "!");
			break;
		}

		//if (row==0) {NMDebugAI(<< "setting numeric variable: "
		//		<< this->mFuncVars.at(fcnt).toStdString() << std::endl);}

		//NMDebug(<< colName.toStdString() << "=" << funcvals[fcnt] << " ");
		++fcnt;
	}
	//if (!bok)
	//{
	//	NMErr(ctxTabCalc, << "Skipping rest of invalid calculation in row " << row << "!");
	//	CALLGRIND_STOP_INSTRUMENTATION;
	//	CALLGRIND_DUMP_STATS;
	//	return;
	//}

	// evaluate string expressions and replace them with numeric results (0 or 1)
	// in function
	newFunc = this->mFunction;
	for (int t=0; t < this->mslStrTerms.size(); ++t)
	{
		//if (row==0 && t==0) {NMDebugAI(<< "processing string expressions ..." << endl);}

		QString origTerm = this->mslStrTerms.at(t);
		//if (row==0) {NMDebugAI(<< "... term: " << origTerm.toStdString() << endl);}

		int idxleft = this->mLstLstStrFields.at(t).at(0);
		int idxright = this->mLstLstStrFields.at(t).at(1);
		QString left;
		QString right;
		if (idxleft >= 0)
		{
			QModelIndex fidx = this->mModel->index(row, idxleft, QModelIndex());
			left = this->mModel->data(fidx, Qt::DisplayRole).toString().trimmed();
		}
		else
		{
			left = this->mLstStrLeftRight.at(t).at(0);
		}

		if (idxright >= 0)
		{
			QModelIndex fidx = this->mModel->index(row, idxright, QModelIndex());
			right = this->mModel->data(fidx, Qt::DisplayRole).toString().trimmed();
		}
		else
		{
			right = this->mLstStrLeftRight.at(t).at(1);
		}

		// debug
		//if (row == 0)
		//{
		//	NMDebugAI(<< "... evaluating: "
		//			<< left.toStdString() << " "
		//			<< mLstNMStrOperator.at(t) << " "
		//			<< right.toStdString() << endl);
		//}

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

				QStringList rl = right.simplified().split(" ");
				strExpRes = rl.contains(left, Qt::CaseInsensitive) ? 1 : 0;
			}
			break;
		case NM_STR_NOTIN:
			{
				QStringList rl = right.simplified().split(" ");
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

	try
	{
		this->mParser->SetExpr(newFunc.toStdString());
		res = this->mParser->Eval();
	}
	catch(itk::ExceptionObject& err)
	{
		NMErr(ctxTabCalc, << "Invalid expression detected!");
		NMDebugCtx(ctxTabCalc, << "done!");

		throw err;
		return;
	}


	if (this->mSelectionModeOn)
	{
		if (res != 0)
		{
			//mOutputSrcSelIndices.push_back(row);
			++mNumSel;

			//NMDebug(<< " -> = " << res << std::endl);

			*selected = true;
		}
	}
	else
	{
		QModelIndex resIdx = this->mModel->index(row, this->mResultColumnIndex, QModelIndex());
		this->mModel->setData(resIdx, QVariant(res));
	}
	//NMDebug(<< std::endl);

	//CALLGRIND_STOP_INSTRUMENTATION;
	//CALLGRIND_DUMP_STATS;
}

void
NMTableCalculator::doStringCalculation()
{
	if (this->mbRowFilter)
	{
		foreach(const QItemSelectionRange& range, this->mInputSelection)
		{
			const int top = range.top();
			const int bottom = range.bottom();
			for (int row=top; row <= bottom; ++row)
			{
				this->processStringCalc(row);
			}
		}
	}
	else
	{
		const int nrows = this->mModel->rowCount(QModelIndex());
		for (int row=0; row < nrows; ++row)
		{
			this->processStringCalc(row);
		}
	}
}

void
NMTableCalculator::processStringCalc(int row)
{
	QString res;
	QString value;

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

std::vector<double>
NMTableCalculator::calcColumnStats(const QString& column)
{
	// result vector containing min, max, mean, sum
	std::vector<double> res;

	if (mRaw2Source == 0)
	{
		NMErr(ctxTabCalc, << "mRaw2Source not set! Bail out!" << std::endl);
		return res;
	}

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

	int nrows = 0;
	int nranges = 0;
	if (this->mbRowFilter)
	{
		nranges = this->mInputSelection.size();
		foreach(const QItemSelectionRange& sr, mInputSelection)
		{
			nrows += sr.bottom() - sr.top() + 1;
		}
	}
	else
	{
		nrows = mRaw2Source->size();//this->mModel->rowCount(QModelIndex());
	}

	// get the first valid value
	bool bok;
	double val;
	int range = 0;
	int row = 0;
	bool bGotInitial = false;

	if (this->mbRowFilter)
	{
		for (; range < nranges; ++range)
		{
			const int top = mInputSelection.at(range).top();
			const int bottom = mInputSelection.at(range).bottom();
			for (row=top; row <= bottom; ++row)
			{
				const QModelIndex valIdx = this->mModel->index(row, colidx, QModelIndex());
				val = this->mModel->data(valIdx, Qt::DisplayRole).toDouble(&bok);
				if (!bok)
				{
					NMErr(ctxTabCalc, << "Calc Column Stats for '" << column.toStdString()
							<< "': Disregarding invalid value at row " << row << "!");
					continue;
				}
				bGotInitial = true;
				break;
			}

			if (bGotInitial)
				break;
		}
	}
	else
	{
		while(row < nrows && !bGotInitial)
		{
			if (mRaw2Source->at(row) < 0)
			{
				++row;
				continue;
			}

			const QModelIndex valIdx = this->mModel->index(mRaw2Source->at(row), colidx, QModelIndex());
			val = this->mModel->data(valIdx, Qt::DisplayRole).toDouble(&bok);
			if (!bok)
			{
				NMErr(ctxTabCalc, << "Calc Column Stats for '" << column.toStdString()
						<< "': Disregarding invalid value at row " << mRaw2Source->at(row) << "!");
				++row;
				continue;
			}
			bGotInitial = true;
			break;
			++row;
		}
	}

	// need to advance row, since we broke out of the above before
	// moving on to the the next following one
	++row;

	double min = val;
	double max = val;
	double sum = val;
	double mean;


	if (this->mbRowFilter)
	{
		for (; range < nranges; ++range)
		{
			int top = mInputSelection.at(range).top();
			// this is just to finish off the remaining selected rows in this range
			// which were left over from the 'GET INITIAL VALUE' THING
			top = top < row ? row : top;
			const int bottom = mInputSelection.at(range).bottom();
			for (row=top; row <= bottom; ++row)
			{
				const QModelIndex valIdx = this->mModel->index(row, colidx, QModelIndex());
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
							<< "': Disregarding invalid value at row " << row << "!");
				}
			}
		}
	}
	else
	{
		while(row < nrows)
		{
			if (mRaw2Source->at(row) < 0)
			{
				++row;
				continue;
			}

			const QModelIndex valIdx = this->mModel->index(mRaw2Source->at(row), colidx, QModelIndex());
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
						<< "': Disregarding invalid value at row " << row << "!");
			}
			++row;
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

	// check whether we've got the mRaw2Source filter set
	if (mRaw2Source == 0)
	{
		NMErr(ctxTabCalc, << "mRaw2Source not set! Bail out!" << std::endl);
		return normalisedCols;
	}

	// --------------------------------------------------------------------------
	// check input fields for numeric data type
	// --------------------------------------------------------------------------
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
					<< name.toStdString() << "'!" << std::endl);
			NMDebugCtx(ctxTabCalc, << "done!");
			return normalisedCols;
		}
	}

	// --------------------------------------------------------------------------
	// work out overall max and min of involved columns
	// --------------------------------------------------------------------------
	double min = std::numeric_limits<double>::max();
	double max = -std::numeric_limits<double>::max();

	for (int field=0; field < fieldVec.size(); ++field)
	{
		std::vector<double> stats = this->calcColumnStats(columnNames.at(field));
		if (stats.size() == 0)
			return normalisedCols;

		min = stats[0] < min ? stats[0] : min;
		max = stats[1] > max ? stats[1] : max;
	}

	NMDebugAI(<< "min: " << min << " | max: " << max << std::endl);


	// --------------------------------------------------------------------------
	// add output columns to take normalised values "<field name>_N"
	// --------------------------------------------------------------------------

	// type variable to denote the column type
	bool bSomethingWrong = false;
	QVariant::Type ftype = QVariant::Double;
	QList<int> nfIdx; // the new field indices
	int nfields = this->mModel->columnCount(QModelIndex());
	for (int field=0; field < fieldVec.size(); ++field)
	{
		QString name = QString("%1_N").arg(columnNames.at(field));

		QModelIndex indexType = this->mModel->index(0, 0, QModelIndex());
		void* typepointer = indexType.internalPointer();
		typepointer = (void*)(&ftype);

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

	const long nrows = mRaw2Source->size();//this->mModel->rowCount(QModelIndex());
	int srcIdx, tarIdx;
	bool bok;
	for(int field=0; field < fieldVec.size(); ++field)
	{
		srcIdx = fieldVec.at(field);
		tarIdx = nfIdx.at(field);

		// get the selected model indices to not have to go through the whole table
		if (this->mbRowFilter)
		{
			foreach(const QItemSelectionRange& idx, this->mInputSelection)
			{
				const int top=idx.top();
				const int bottom=idx.bottom();
				for (int i=top; i <= bottom; ++i)
				{
					// TODO: we need to check, whether actually there's only one index for each row!!!
					QModelIndex misrc = this->mModel->index(i, srcIdx, QModelIndex());
					QModelIndex mitar = this->mModel->index(i, tarIdx, QModelIndex());

					double val = this->mModel->data(misrc, Qt::DisplayRole).toDouble(&bok);
					if (!bok)
					{
						NMErr(ctxTabCalc, << "Failed getting input value for '"
							<< this->mModel->headerData(srcIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
							<< "', row " << i << "!");
						continue;
					}

					double normval = bCostCriterion ? ((max - val) / diff) : ((val - min) / diff);

					if (!this->mModel->setData(mitar, QVariant(normval), Qt::EditRole))
					{
						NMErr(ctxTabCalc, << "Failed setting normalised value for '"
							<< this->mModel->headerData(tarIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
							<< "', row " << i << "!");
						continue;
					}
				}
			}
		}
		else // we iterate over the whole table
		{
			for (int row=0; row < nrows; ++row)
			{
				if (mRaw2Source->at(row) < 0)
				{
					continue;
				}

				QModelIndex misrc = this->mModel->index(mRaw2Source->at(row), srcIdx, QModelIndex());
				QModelIndex mitar = this->mModel->index(mRaw2Source->at(row), tarIdx, QModelIndex());

				double val = this->mModel->data(misrc, Qt::DisplayRole).toDouble(&bok);
				if (!bok)
				{
					NMErr(ctxTabCalc, << "Failed getting input value for '"
						<< this->mModel->headerData(srcIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << mRaw2Source->at(row) << "!");
					continue;
				}

				double normval = bCostCriterion ? ((max - val) / diff) : ((val - min) / diff);

				if (!this->mModel->setData(mitar, QVariant(normval), Qt::EditRole))
				{
					NMErr(ctxTabCalc, << "Failed setting normalised value for '"
						<< this->mModel->headerData(tarIdx, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()
						<< "', row " << mRaw2Source->at(row) << "!");
					continue;
				}
			}
		}
	}
	return normalisedCols;
}

void
NMTableCalculator::printSelRanges(const QItemSelection& selection, const QString& msg)
{
	int total = selection.count();
	NMDebugAI(<< msg.toStdString() << std::endl);
	int rcnt = 1;
	int numidx = 0;
	foreach(const QItemSelectionRange& range, selection)
	{
		NMDebugAI(<< "   range #" << rcnt << ":  " << range.width()
				                          << " x " << range.height() << std::endl);
		NMDebugAI(<< "     rows: " << range.top() << " - " << range.bottom() << std::endl);
		++rcnt;
		numidx += range.bottom() - range.top() + 1;
	}
	NMDebugAI(<< numidx << " selected rows in total" << std::endl);
}
