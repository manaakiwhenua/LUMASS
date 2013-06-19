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

#include "nmlog.h"
#include <QObject>
#include <QString>
#include <QStringList>

#include "NMTableView.h"

#include "vtkType.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkFunctionParser.h"
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"

#include "otbMultiParser.h"

class NMTableCalculator : public QObject
{
	Q_OBJECT
	Q_ENUMS(NMStrOperator)

public:
	NMTableCalculator(QObject* parent=0);
	NMTableCalculator(vtkTable* tab, QObject* parent=0);
	virtual ~NMTableCalculator();

	// supported string comparison operators
	enum NMStrOperator   {NM_STR_GT,		 // >
						  NM_STR_GTEQ,		 // >=
						  NM_STR_LT,		 // <
						  NM_STR_LTEQ,		 // <=
						  NM_STR_NEQ,		 // !=
						  NM_STR_IN,         // in
						  NM_STR_NOTIN,		 // !in
						  NM_STR_CONTAINS,   // contains
						  NM_STR_EQ,		 // ==
						  NM_STR_STARTSWITH, // startsWith
						  NM_STR_ENDSWITH,   // endsWith
						  NM_STR_UNKNOWN
	};

	inline NMStrOperator getStrOperator(const QString& strOp);

	bool setRowFilterModeOn(QString filterColumn);
	bool setResultColumn(const QString& name);
	void setFunction(const QString& function);
	bool setSelectionModeOn(const QString& selColumn,
			NMTableView* tabView);

	bool calculate(void);

	long getSelectionCount() {return  this->mNumSelRecs;};
	std::vector<double> calcColumnStats(QString column);
	QStringList normaliseColumns(QStringList columnNames, bool bCostCriterion);

protected:
	//vtkSmartPointer<vtkFunctionParser> mParser;
	otb::MultiParser::Pointer mParser;
	vtkTable* mTab;

	QString mFunction;
	QStringList mFuncVars;
	QList<vtkDataArray*> mFuncFields;
	QString mResultColumn;
	int mResultColumnIndex;
	int mResultColumnType;

	bool mbRowFilter;
	vtkDataArray* mFilterArray;

	NMTableView* mTabView;
	bool mSelectionMode;
	long mNumSelRecs;
	QStringList mslStrTerms;
	QList<QStringList> mLstStrLeftRight;
	QList<QList<vtkStringArray*> > mLstLstStrFields;
	QList<NMStrOperator> mLstNMStrOperator;

	void initCalculator();
	void clearLists();
	virtual	bool parseFunction();
	virtual	void doNumericCalcSelection();
	virtual	void doStringCalculation();

	int getColumnIndex(QString name);

};

#endif // ifndef NMTABLECALCULATOR_H_
