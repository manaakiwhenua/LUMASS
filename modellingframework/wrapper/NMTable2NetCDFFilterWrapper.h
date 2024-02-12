/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2023 Landcare Research New Zealand Ltd
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
 * NMTable2NetCDFFilterWrapper.h
 *
 *  Created on: 2023-06-22
 *      Author: Alexander Herzig
 */

#ifndef NMTable2NetCDFFilterWrapper_H_
#define NMTable2NetCDFFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmtable2netcdffilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMTable2NetCDFFilterWrapper_Internal;

class
NMTable2NetCDFFilterWrapper
        : public NMProcess
{
    Q_OBJECT

    
    Q_PROPERTY(QList<QStringList> DimMapping READ getDimMapping WRITE setDimMapping)
    Q_PROPERTY(QList<QStringList> OutputOrigin READ getOutputOrigin WRITE setOutputOrigin)
    Q_PROPERTY(QList<QStringList> OutputSpacing READ getOutputSpacing WRITE setOutputSpacing)
    Q_PROPERTY(QList<QStringList> OutputSize READ getOutputSize WRITE setOutputSize)
    Q_PROPERTY(QList<QStringList> OutputIndex READ getOutputIndex WRITE setOutputIndex)
    Q_PROPERTY(QStringList InputTableName READ getInputTableName WRITE setInputTableName)
    Q_PROPERTY(QStringList SQLWhereClause READ getSQLWhereClause WRITE setSQLWhereClause)
    Q_PROPERTY(QStringList ImageVarName READ getImageVarName WRITE setImageVarName)
    Q_PROPERTY(QStringList NcImageContainer READ getNcImageContainer WRITE setNcImageContainer)
    Q_PROPERTY(QStringList NcGroupName READ getNcGroupName WRITE setNcGroupName)
    Q_PROPERTY(QList<QStringList> DimVarNames READ getDimVarNames WRITE setDimVarNames)
    Q_PROPERTY(QList<QStringList> VarAndDimDescriptors READ getVarAndDimDescriptors WRITE setVarAndDimDescriptors)

public:

    
    NMPropertyGetSet( DimMapping, QList<QStringList> )
    NMPropertyGetSet( OutputOrigin, QList<QStringList> )
    NMPropertyGetSet( OutputSpacing, QList<QStringList> )
    NMPropertyGetSet( OutputSize, QList<QStringList> )
    NMPropertyGetSet( OutputIndex, QList<QStringList> )
    NMPropertyGetSet( InputTableName, QStringList )
    NMPropertyGetSet( SQLWhereClause, QStringList )
    NMPropertyGetSet( ImageVarName, QStringList )
    NMPropertyGetSet( NcImageContainer, QStringList )
    NMPropertyGetSet( NcGroupName, QStringList )
    NMPropertyGetSet( DimVarNames, QList<QStringList> )
    NMPropertyGetSet( VarAndDimDescriptors, QList<QStringList> )

public:
    NMTable2NetCDFFilterWrapper(QObject* parent=0);
    virtual ~NMTable2NetCDFFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMTable2NetCDFFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    /*$<RATGetSupportDecl>$*/

    void setRAT(unsigned int idx,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);

    
    QList<QStringList> mDimMapping;
    QList<QStringList> mOutputOrigin;
    QList<QStringList> mOutputSpacing;
    QList<QStringList> mOutputSize;
    QList<QStringList> mOutputIndex;
    QStringList mInputTableName;
    QStringList mSQLWhereClause;
    QStringList mImageVarName;
    QStringList mNcImageContainer;
    QStringList mNcGroupName;
    QList<QStringList> mDimVarNames;
    QList<QStringList> mVarAndDimDescriptors;

};

#endif /* NMTable2NetCDFFilterWrapper_H_ */
