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
 * NMUniqueCombinationFilterWrapper.h
 *
 *  Created on: 2015-09-07
 *      Author: Alexander Herzig
 */

#ifndef NMUniqueCombinationFilterWrapper_H_
#define NMUniqueCombinationFilterWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include "nmuniquecombinationfilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMUniqueCombinationFilterWrapper_Internal;

class NMUNIQUECOMBINATIONFILTERWRAPPER_EXPORT
NMUniqueCombinationFilterWrapper
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(QStringList OutputImageFileName READ getOutputImageFileName WRITE setOutputImageFileName)
    Q_PROPERTY(QList<QStringList> InputNodata READ getInputNodata WRITE setInputNodata)

public:


    NMPropertyGetSet( OutputImageFileName, QStringList )
    NMPropertyGetSet( InputNodata, QList<QStringList> )

public:
    NMUniqueCombinationFilterWrapper(QObject* parent=0);
    virtual ~NMUniqueCombinationFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMUniqueCombinationFilterWrapper_Internal;

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


    QStringList mOutputImageFileName;
    QList<QStringList> mInputNodata;

};

#endif /* NMUniqueCombinationFilterWrapper_H_ */
