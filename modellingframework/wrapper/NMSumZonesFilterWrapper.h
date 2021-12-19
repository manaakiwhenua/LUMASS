/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014, 2015 Landcare Research New Zealand Ltd
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
 * NMSumZonesFilterWrapper.h
 *
 *  Created on: 2014-03-27, 2015-08-27
 *      Author: Alexander Herzig
 */

#ifndef NMSumZonesFilterWrapper_H_
#define NMSumZonesFilterWrapper_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include "nmsumzonesfilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMSumZonesFilterWrapper_Internal;

class NMSUMZONESFILTERWRAPPER_EXPORT
NMSumZonesFilterWrapper
        : public NMProcess
{
    Q_OBJECT


    Q_PROPERTY(QStringList IgnoreNodataValue READ getIgnoreNodataValue WRITE setIgnoreNodataValue)
    Q_PROPERTY(QStringList NodataValue READ getNodataValue WRITE setNodataValue)
    Q_PROPERTY(QStringList HaveMaxKeyRows READ getHaveMaxKeyRows WRITE setHaveMaxKeyRows)
    Q_PROPERTY(QStringList ZoneTableFileName READ getZoneTableFileName WRITE setZoneTableFileName)

public:


    NMPropertyGetSet( IgnoreNodataValue, QStringList )
    NMPropertyGetSet( NodataValue, QStringList )
    NMPropertyGetSet( HaveMaxKeyRows, QStringList )
    NMPropertyGetSet( ZoneTableFileName, QStringList )

public:
    NMSumZonesFilterWrapper(QObject* parent=0);
    virtual ~NMSumZonesFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMSumZonesFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper, const QString& name);

    QSharedPointer<NMItkDataObjectWrapper> getRAT(unsigned int idx);


    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);


    QStringList mIgnoreNodataValue;
    QStringList mNodataValue;
    QStringList mHaveMaxKeyRows;
    QStringList mZoneTableFileName;

};

#endif /* NMSumZonesFilterWrapper_H_ */
