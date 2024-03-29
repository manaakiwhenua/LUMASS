/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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
 * NMCombineTwoFilterWrapper.h
 *
 *  Created on: 2015-09-16
 *      Author: Alexander Herzig
 */

#ifndef NMCombineTwoFilterWrapper_H_
#define NMCombineTwoFilterWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include "nmcombinetwofilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMCombineTwoFilterWrapper_Internal;

class NMCOMBINETWOFILTERWRAPPER_EXPORT NMCombineTwoFilterWrapper
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(QList<QStringList> InputNodata READ getInputNodata WRITE setInputNodata)
    Q_PROPERTY(QStringList OutputTableFileName READ getOutputTableFileName WRITE setOutputTableFileName)

public:


    NMPropertyGetSet( InputNodata, QList<QStringList> )
    NMPropertyGetSet( OutputTableFileName, QStringList )

public:
    NMCombineTwoFilterWrapper(QObject* parent=0);
    virtual ~NMCombineTwoFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMCombineTwoFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    QSharedPointer<NMItkDataObjectWrapper> getRAT(unsigned int idx);


    void setRAT(unsigned int idx,
        QSharedPointer<NMItkDataObjectWrapper> imgWrapper);


protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);


    QList<QStringList> mInputNodata;
    QStringList mOutputTableFileName;

};

#endif /* NMCombineTwoFilterWrapper_H_ */
