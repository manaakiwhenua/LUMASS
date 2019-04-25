/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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
 * NMMosraFilterWrapper.h
 *
 *  Created on: 2017-03-03
 *      Author: Alexander Herzig
 */

#ifndef NMMosraFilterWrapper_H_
#define NMMosraFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmmosrafilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMMosraFilterWrapper_Internal;

class NMMOSRAFILTERWRAPPER_EXPORT
NMMosraFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    Q_PROPERTY(QStringList LosFileName READ getLosFileName WRITE setLosFileName)
    Q_PROPERTY(QStringList TimeOut READ getTimeOut WRITE setTimeOut)

public:

    
    NMPropertyGetSet( LosFileName, QStringList )
    NMPropertyGetSet( TimeOut, QStringList )

public:
    NMMosraFilterWrapper(QObject* parent=0);
    virtual ~NMMosraFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMMosraFilterWrapper_Internal;

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

    
    QStringList mLosFileName;
    QStringList mTimeOut;

};

#endif /* NMMosraFilterWrapper_H_ */
