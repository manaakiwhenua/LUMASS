/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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
 * NMScriptableKernelFilterWrapper.h
 *
 *  Created on: 2016-05-22
 *      Author: Alexander Herzig
 */

#ifndef NMScriptableKernelFilterWrapper_H_
#define NMScriptableKernelFilterWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include "nmmodframe_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMScriptableKernelFilterWrapper_Internal;

class
NMScriptableKernelFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QList<QStringList> Radius READ getRadius WRITE setRadius)
    Q_PROPERTY(QStringList KernelScript READ getKernelScript WRITE setKernelScript)
    Q_PROPERTY(QStringList OutputVarName READ getOutputVarName WRITE setOutputVarName)
    Q_PROPERTY(QStringList Nodata READ getNodata WRITE setNodata)
    Q_PROPERTY(QString KernelShapeType READ getKernelShapeType WRITE setKernelShapeType)
    Q_PROPERTY(QStringList KernelShapeEnum READ getKernelShapeEnum)
    Q_PROPERTY(QString NumThreads READ getNumThreads WRITE setNumThreads)

public:

    
    NMPropertyGetSet( Radius, QList<QStringList> )
    NMPropertyGetSet( KernelScript, QStringList )
    NMPropertyGetSet( OutputVarName, QStringList )
    NMPropertyGetSet( Nodata, QStringList )
    NMPropertyGetSet( KernelShapeType, QString )
    NMPropertyGetSet( KernelShapeEnum, QStringList )
    NMPropertyGetSet( NumThreads, QString )

public:
    NMScriptableKernelFilterWrapper(QObject* parent=0);
    virtual ~NMScriptableKernelFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMScriptableKernelFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    QString mNumThreads;
    
    QList<QStringList> mRadius;
    QStringList mKernelScript;
    QStringList mOutputVarName;
    QStringList mNodata;
    QString mKernelShapeType;
    QStringList mKernelShapeEnum;

};

#endif /* NMScriptableKernelFilterWrapper_H_ */
