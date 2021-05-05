/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2021 Landcare Research New Zealand Ltd
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
 * NMImage2TableFilterWrapper.h
 *
 *  Created on: 2020-12-05
 *      Author: Alexander Herzig
 */

#ifndef NMImage2TableFilterWrapper_H_
#define NMImage2TableFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmimage2tablefilterwrapper_export.h"

template<class TInputImage, class TOutputImage = TInputImage, unsigned int Dimension=2>
class NMImage2TableFilterWrapper_Internal;

class
NMImage2TableFilterWrapper
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(QStringList TableFileName READ getTableFileName WRITE setTableFileName)
    Q_PROPERTY(QStringList TableName READ getTableName WRITE setTableName)
    Q_PROPERTY(QStringList ImageVarName READ getImageVarName WRITE setImageVarName)
    Q_PROPERTY(QStringList UpdateMode READ getUpdateMode WRITE setUpdateMode)
    Q_PROPERTY(QStringList NcImageContainer READ getNcImageContainer WRITE setNcImageContainer)
    Q_PROPERTY(QStringList NcGroupName READ getNcGroupName WRITE setNcGroupName)
    Q_PROPERTY(QList<QStringList> StartIndex READ getStartIndex WRITE setStartIndex)
    Q_PROPERTY(QList<QStringList> Size READ getSize WRITE setSize)
    Q_PROPERTY(QList<QStringList> DimVarNames READ getDimVarNames WRITE setDimVarNames)
    Q_PROPERTY(QList<QStringList> AuxVarNames READ getAuxVarNames WRITE setAuxVarNames)

public:


    NMPropertyGetSet( TableFileName, QStringList )
    NMPropertyGetSet( TableName, QStringList )
    NMPropertyGetSet( ImageVarName, QStringList )
    NMPropertyGetSet( UpdateMode, QStringList )
    NMPropertyGetSet( NcImageContainer, QStringList )
    NMPropertyGetSet( NcGroupName, QStringList )
    NMPropertyGetSet( StartIndex, QList<QStringList> )
    NMPropertyGetSet( Size, QList<QStringList> )
    NMPropertyGetSet( DimVarNames, QList<QStringList> )
    NMPropertyGetSet( AuxVarNames, QList<QStringList> )

public:
    NMImage2TableFilterWrapper(QObject* parent=0);
    virtual ~NMImage2TableFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMImage2TableFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    QSharedPointer<NMItkDataObjectWrapper> getRAT(unsigned int idx);


    void setRAT(unsigned int idx,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);


    QStringList mTableFileName;
    QStringList mTableName;
    QStringList mImageVarName;
    QStringList mUpdateMode;
    QStringList mNcImageContainer;
    QStringList mNcGroupName;
    QList<QStringList> mStartIndex;
    QList<QStringList> mSize;
    QList<QStringList> mDimVarNames;
    QList<QStringList> mAuxVarNames;

};

#endif /* NMImage2TableFilterWrapper_H_ */
