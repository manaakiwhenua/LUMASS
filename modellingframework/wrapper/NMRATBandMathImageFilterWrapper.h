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

#ifndef NMRATBANDMATHIMAGEFILTERWRAPPER_H
#define NMRATBANDMATHIMAGEFILTERWRAPPER_H

#include "nmlog.h"
#define ctxNMRATBandMathWrapper "NMRATBandMathImageFilterWrapper"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"
#include "NMOtbAttributeTableWrapper.h"

#include "otbImageIOBase.h"
#include "nmmodframe_export.h"

/**
  * class NMRATBandMathImageFilterWrapper
  * 
  * NMRATBandMathImageFilterWrapper wraps the RATBandMathImageFilter, which
  * permits simple mathematical (and conditional) operations on
  * a single image band.
  *
  * Parameters for filter configuration:
  * QMap<QString, QVariant>		//   name		value
  *
  * name: inputImages		  value (QStringList):  "inputComponentName1"
  *                                          		"inputComponentName2"
  *
  *	name: imageVarNames       value (QStringList):  "imgVarName1"
  *												    "imgVarName2"
  *
  * name: inputTables   	   value (QStringList): "inputComponentName1"
  *                                                 "inputComponentName2"
  *
  * name: tableVarNames	       value (QList<QStringList>): "col1a" "col1b" "col1c"
  *                                                        "col2a" "col2a" "col2c"
  *
  * name: Expression	       value: (QString):     "algebra expression"
  *
  *
  */

class NMMODFRAME_EXPORT NMRATBandMathImageFilterWrapper : public NMProcess
{
	Q_OBJECT

//	Q_PROPERTY(QList<QStringList> InputImgVarNames READ getInputImgVarNames WRITE setInputImgVarNames)
	Q_PROPERTY(QList<QStringList> InputTables READ getInputTables WRITE setInputTables)
	Q_PROPERTY(QList<QList<QStringList> > InputTableVarNames READ getInputTableVarNames WRITE setInputTableVarNames)
	Q_PROPERTY(QStringList MapExpressions READ getMapExpressions WRITE setMapExpressions )
	Q_PROPERTY(QStringList NumExpressions READ getNumExpressions WRITE setNumExpressions )
    Q_PROPERTY(bool UseTableColumnCache READ getUseTableColumnCache WRITE setUseTableColumnCache )

public:
//	NMPropertyGetSet( InputImgVarNames, QList<QStringList> )
    NMPropertyGetSet( InputTables, QList<QStringList> )
    NMPropertyGetSet( InputTableVarNames, QList<QList<QStringList> > )
    NMPropertyGetSet( MapExpressions, QStringList )
    NMPropertyGetSet( NumExpressions, QStringList )
    NMPropertyGetSet( UseTableColumnCache, bool )

signals:
//	void InputImgVarNamesChanged(QList<QStringList>);
	void InputTablesChanged(QList<QStringList>);
	void InputTableVarNamesChanged(QList<QList<QStringList> > );
	void MapExpressionsChanged(QStringList);


public:
    NMRATBandMathImageFilterWrapper(QObject *parent=0);
    ~NMRATBandMathImageFilterWrapper ( );

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
    		QSharedPointer<NMItkDataObjectWrapper> imgWrapper);
    void setNthAttributeTable(unsigned int idx,
    		otb::AttributeTable::Pointer table,
    		std::vector<std::string> tableColumns);

protected:

    void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    void setInternalExpression(QString expression);
    void setInternalNumExpression(unsigned int numExpr);
    void setInternalNthInputName(unsigned int idx, const QString& varName);
    void setInternalUseTableCache(bool useCache);

    std::string ctx;
    QList<QStringList> 		   mInputImgVarNames;
    QList<QStringList> 		   mInputTables;
    QList<QList<QStringList> > mInputTableVarNames;
    QStringList 			   mMapExpressions;
    QStringList				   mNumExpressions;
    bool                mUseTableColumnCache;

//    void setTableParams( const QMap<QString,
//    		NMModelComponent*>& repo);
};

#endif // NMRATBANDMATHIMAGEFILTERWRAPPER_H
