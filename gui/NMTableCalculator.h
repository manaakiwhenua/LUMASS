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
 * NMTableCalculator.h
 *
 *  Created on: 21/12/2011
 *      Author: alex
 */

#ifndef NMTABLECALCULATOR_H_
#define NMTABLECALCULATOR_H_
#define ctxTabCalc "NMTableCalculator"

#include <list>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QAbstractItemModel>
#include <QModelIndexList>
#include <QItemSelection>

//#include "vtkType.h"
//#include "vtkSmartPointer.h"
//#include "vtkTable.h"
//#include "vtkAbstractArray.h"
//#include "vtkDataArray.h"
//#include "vtkStringArray.h"

#include "otbMultiParser.h"

class NMTableCalculator : public QObject
{
	Q_OBJECT
	Q_ENUMS(NMStrOperator)

public:
	//NMTableCalculator(QObject* parent=0);
	//NMTableCalculator(vtkTable* tab, QObject* parent=0);
	NMTableCalculator(QAbstractItemModel* model, QObject* parent=0);
	virtual ~NMTableCalculator();

	// supported string comparison operators
	enum NMStrOperator   {NM_STR_GT,		 //  0 >
						  NM_STR_GTEQ,		 //  1 >=
						  NM_STR_LT,		 //  2 <
						  NM_STR_LTEQ,		 //  3 <=
						  NM_STR_NEQ,		 //  4 !=
						  NM_STR_IN,         //  5 in
						  NM_STR_NOTIN,		 //  6 !in
						  NM_STR_CONTAINS,   //  7 contains
						  NM_STR_EQ,		 //  8 ==
						  NM_STR_STARTSWITH, //  9 startsWith
						  NM_STR_ENDSWITH,   // 10 endsWith
						  NM_STR_UNKNOWN
	};

	inline NMStrOperator getStrOperator(const QString& strOp);

	bool setRowFilter(const QItemSelection& inputSelection);
	bool setResultColumn(const QString& name);
	void setFunction(const QString& function);
	void setSelectionMode(bool selmode);

	void setRaw2Source(QList<int>* raw2Source)
		{mRaw2Source = raw2Source;}

	bool calculate(void);

	//const QItemSelection& getSource
	const QItemSelection* getSelection(void);
	long getSelectionCount(void)
		{return mNumSel;}
	std::vector<double> calcColumnStats(const QString& column);
	QStringList normaliseColumns(const QStringList& columnNames, bool bCostCriterion);

protected:
	otb::MultiParser::Pointer mParser;
	QAbstractItemModel* mModel;
	QList<int>* mRaw2Source;


	bool mbRowFilter;
	bool mSelectionModeOn;
	//QList<QPair<int, int> > mOutputSelection;
	//QList<QPair<int, int> >	mInputSelection;
	std::vector<int> mOutputSrcSelIndices;

	long mNumSel;
	//QList<int> mOutputProxySelIndices;
	QItemSelection mOutputSelection;
	QItemSelection mInputSelection;

	QString mFunction;
	QString mResultColumn;
	int mResultColumnIndex;
	QVariant::Type mResultColumnType;

	QStringList mFuncVars;
	QList<int> mFuncFields;
	QStringList mslStrTerms;
	QList<QStringList> mLstStrLeftRight;
	// note the inner list might contain '-1' denoting
	// non-index fields!
	QList<QList<int> > mLstLstStrFields;
	QList<NMStrOperator> mLstNMStrOperator;

	void initCalculator();
	void clearLists();
	bool parseFunction();
	void doNumericCalcSelection();
	void doStringCalculation();
	void processNumericCalcSelection(int row, bool* selected);
	void processStringCalc(int row);


	QModelIndexList getInputRows();
	int getColumnIndex(const QString& name);
	QVariant::Type getColumnType(int colidx);
	bool isNumericColumn(int colidx);
	bool isStringColumn(int colidx);

	// DEBUG ONLY
	void printSelRanges(const QItemSelection& selection,
			const QString& msg);

};

#endif // ifndef NMTABLECALCULATOR_H_
