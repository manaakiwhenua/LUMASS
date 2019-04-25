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
 * NMVirtualStreamWriter.h
 *
 *  Created on: 2017-03-28
 *      Author: Alexander Herzig
 */

#ifndef NMVirtualStreamWriter_H_
#define NMVirtualStreamWriter_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"


template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMVirtualStreamWriter_Internal;

class
NMVirtualStreamWriter
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(int StreamingSize READ getStreamingSize WRITE setStreamingSize)
    Q_PROPERTY(QString StreamingMethodType READ getStreamingMethodType WRITE setStreamingMethodType)
    Q_PROPERTY(QStringList StreamingMethodEnum READ getStreamingMethodEnum)
    Q_PROPERTY(unsigned int NumThreads READ getNumThreads WRITE setNumThreads)

public:

    
    NMPropertyGetSet( StreamingSize, int )
    NMPropertyGetSet( StreamingMethodType, QString )
    NMPropertyGetSet( StreamingMethodEnum, QStringList)
    NMPropertyGetSet( NumThreads, unsigned int )

public:
    NMVirtualStreamWriter(QObject* parent=0);
    virtual ~NMVirtualStreamWriter();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMVirtualStreamWriter_Internal;

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

    
    int mStreamingSize;
    QString mStreamingMethodType;
    QStringList mStreamingMethodEnum;
    unsigned int mNumThreads;

};

#endif /* NMVirtualStreamWriter_H_ */
