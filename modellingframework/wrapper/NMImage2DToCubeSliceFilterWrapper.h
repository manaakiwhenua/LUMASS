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
 * NMImage2DToCubeSliceFilterWrapper.h
 *
 *  Created on: 2020-12-05
 *      Author: Alexander Herzig
 */

#ifndef NMImage2DToCubeSliceFilterWrapper_H_
#define NMImage2DToCubeSliceFilterWrapper_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmimage2dtocubeslicefilterwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>
class NMImage2DToCubeSliceFilterWrapper_Internal;

class
NMImage2DToCubeSliceFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QList<QStringList> DimMapping READ getDimMapping WRITE setDimMapping)
    Q_PROPERTY(QList<QStringList> OutputOrigin READ getOutputOrigin WRITE setOutputOrigin)
    Q_PROPERTY(QList<QStringList> OutputSpacing READ getOutputSpacing WRITE setOutputSpacing)
    Q_PROPERTY(QList<QStringList> OutputSize READ getOutputSize WRITE setOutputSize)
    Q_PROPERTY(QList<QStringList> OutputIndex READ getOutputIndex WRITE setOutputIndex)

public:

    
    NMPropertyGetSet( DimMapping, QList<QStringList> )
    NMPropertyGetSet( OutputOrigin, QList<QStringList> )
    NMPropertyGetSet( OutputSpacing, QList<QStringList> )
    NMPropertyGetSet( OutputSize, QList<QStringList> )
    NMPropertyGetSet( OutputIndex, QList<QStringList> )

public:
    NMImage2DToCubeSliceFilterWrapper(QObject* parent=0);
    virtual ~NMImage2DToCubeSliceFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMImage2DToCubeSliceFilterWrapper_Internal;

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
    QList<QStringList> mOutputOrigin;
    QList<QStringList> mOutputSpacing;
    QList<QStringList> mOutputSize;
    QList<QStringList> mOutputIndex;

};

#endif /* NMImage2DToCubeSliceFilterWrapper_H_ */
