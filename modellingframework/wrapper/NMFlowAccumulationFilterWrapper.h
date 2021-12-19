/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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
 * NMFlowAccumulationFilterWrapper.h
 *
 *  Created on: 2019-03-19
 *      Author: Alexander Herzig
 */

#ifndef NMFlowAccumulationFilterWrapper_H_
#define NMFlowAccumulationFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmflowaccumulationfilterwrapper_export.h"

//template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
template<class TInputImage, unsigned int Dimension=2>
class NMFlowAccumulationFilterWrapper_Internal;

class NMFLOWACCUMULATIONFILTERWRAPPER_EXPORT
NMFlowAccumulationFilterWrapper
        : public NMProcess
{
    Q_OBJECT

    Q_PROPERTY(QStringList Nodata READ getNodata WRITE setNodata)
    Q_PROPERTY(QString AlgorithmType READ getAlgorithmType WRITE setAlgorithmType)
    Q_PROPERTY(QStringList AlgorithmEnum READ getAlgorithmEnum)
    Q_PROPERTY(QStringList FlowExponent READ getFlowExponent WRITE setFlowExponent)
    Q_PROPERTY(QString FlowLengthType READ getFlowLengthType WRITE setFlowLengthType)
    Q_PROPERTY(QStringList FlowLengthEnum READ getFlowLengthEnum)


public:

    NMPropertyGetSet( Nodata,         QStringList )
    NMPropertyGetSet( AlgorithmType,  QString     )
    NMPropertyGetSet( AlgorithmEnum,  QStringList )
    NMPropertyGetSet( FlowExponent,   QStringList )
    NMPropertyGetSet( FlowLengthType, QString     )
    NMPropertyGetSet( FlowLengthEnum, QStringList )

public:
    NMFlowAccumulationFilterWrapper(QObject* parent=0);
    virtual ~NMFlowAccumulationFilterWrapper();

    //template<class TInputImage, class TOutputImage, unsigned int Dimension>
    template<class TInputImage, unsigned int Dimension>
    friend class NMFlowAccumulationFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:

    QStringList mNodata;
    QString mAlgorithmType;
    QStringList mAlgorithmEnum;
    QStringList mFlowExponent;
    QString mFlowLengthType;
    QStringList mFlowLengthEnum;

    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);



};

#endif /* NMFlowAccumulationFilterWrapper_H_ */
