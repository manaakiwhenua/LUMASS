/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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
 * NMCubeSliceToImage2DFilterWrapper.h
 *
 *  Created on: 2020-12-07
 *      Author: Alex Herzig
 */

#ifndef NMCubeSliceToImage2DFilterWrapper_H_
#define NMCubeSliceToImage2DFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmcubeslicetoimage2dfilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMCubeSliceToImage2DFilterWrapper_Internal;

class
NMCubeSliceToImage2DFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QList<QStringList> DimMapping READ getDimMapping WRITE setDimMapping)
    Q_PROPERTY(QList<QStringList> InputOrigin READ getInputOrigin WRITE setInputOrigin)
    Q_PROPERTY(QList<QStringList> InputSize READ getInputSize WRITE setInputSize)
    Q_PROPERTY(QList<QStringList> InputIndex READ getInputIndex WRITE setInputIndex)

public:

    
    NMPropertyGetSet( DimMapping, QList<QStringList> )
    NMPropertyGetSet( InputOrigin, QList<QStringList> )
    NMPropertyGetSet( InputSize, QList<QStringList> )
    NMPropertyGetSet( InputIndex, QList<QStringList> )

public:
    NMCubeSliceToImage2DFilterWrapper(QObject* parent=0);
    virtual ~NMCubeSliceToImage2DFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMCubeSliceToImage2DFilterWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    
    QList<QStringList> mDimMapping;
    QList<QStringList> mInputOrigin;
    QList<QStringList> mInputSize;
    QList<QStringList> mInputIndex;

};

#endif /* NMCubeSliceToImage2DFilterWrapper_H_ */
