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
 * NMDEMSlopeAspectFilterWrapper.h
 *
 *  Created on: 2019-03-22
 *      Author: Alexander Herzig
 */

#ifndef NMDEMSlopeAspectFilterWrapper_H_
#define NMDEMSlopeAspectFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmdemslopeaspectfilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMDEMSlopeAspectFilterWrapper_Internal;

class NMDEMSLOPEASPECTFILTERWRAPPER_EXPORT
NMDEMSlopeAspectFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QString TerrainAttributeType READ getTerrainAttributeType WRITE setTerrainAttributeType)
    Q_PROPERTY(QString TerrainAlgorithmType READ getTerrainAlgorithmType WRITE setTerrainAlgorithmType)
    Q_PROPERTY(QString AttributeUnitType READ getAttributeUnitType WRITE setAttributeUnitType)
    Q_PROPERTY(QStringList TerrainAttributeEnum READ getTerrainAttributeEnum)
    Q_PROPERTY(QStringList TerrainAlgorithmEnum READ getTerrainAlgorithmEnum)
    Q_PROPERTY(QStringList AttributeUnitEnum READ getAttributeUnitEnum)
    Q_PROPERTY(QStringList Nodata READ getNodata WRITE setNodata)

public:

    
    NMPropertyGetSet( TerrainAttributeType, QString )
    NMPropertyGetSet( TerrainAlgorithmType, QString )
    NMPropertyGetSet( AttributeUnitType, QString )
    NMPropertyGetSet( TerrainAttributeEnum, QStringList )
    NMPropertyGetSet( TerrainAlgorithmEnum, QStringList )
    NMPropertyGetSet( AttributeUnitEnum, QStringList )
    NMPropertyGetSet( Nodata, QStringList )

public:
    NMDEMSlopeAspectFilterWrapper(QObject* parent=0);
    virtual ~NMDEMSlopeAspectFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMDEMSlopeAspectFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    QString mTerrainAttributeType;
    QString mTerrainAlgorithmType;
    QString mAttributeUnitType;

    QStringList mTerrainAttributeEnum;
    QStringList mTerrainAlgorithmEnum;
    QStringList mAttributeUnitEnum;
    QStringList mNodata;

};

#endif /* NMDEMSlopeAspectFilterWrapper_H_ */
