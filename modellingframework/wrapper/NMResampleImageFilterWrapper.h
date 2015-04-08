/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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
 * NMResampleImageFilterWrapper.h
 *
 *  Created on: 2014-04-03
 *      Author: Alexander Herzig
 */

#ifndef NMResampleImageFilterWrapper_H_
#define NMResampleImageFilterWrapper_H_

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
class NMResampleImageFilterWrapper_Internal;

class
NMMODFRAME_EXPORT NMResampleImageFilterWrapper
		: public NMProcess
{
	Q_OBJECT

    
    Q_PROPERTY(QStringList InterpolationMethod READ getInterpolationMethod WRITE setInterpolationMethod)
    Q_PROPERTY(QStringList DefaultPixelValue READ getDefaultPixelValue WRITE setDefaultPixelValue)
    Q_PROPERTY(QList<QStringList> OutputSpacing READ getOutputSpacing WRITE setOutputSpacing)
    Q_PROPERTY(QList<QStringList> OutputOrigin READ getOutputOrigin WRITE setOutputOrigin)
    Q_PROPERTY(QList<QStringList> Size READ getSize WRITE setSize)

public:

    
    NMPropertyGetSet( InterpolationMethod, QStringList )
    NMPropertyGetSet( DefaultPixelValue, QStringList )
    NMPropertyGetSet( OutputSpacing, QList<QStringList> )
    NMPropertyGetSet( OutputOrigin, QList<QStringList> )
    NMPropertyGetSet( Size, QList<QStringList> )

public:
    NMResampleImageFilterWrapper(QObject* parent=0);
    virtual ~NMResampleImageFilterWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMResampleImageFilterWrapper_Internal;

	QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    /*$<RATGetSupportDecl>$*/

    /*$<RATSetSupportDecl>$*/

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    
    QStringList mInterpolationMethod;
    QStringList mDefaultPixelValue;
    QList<QStringList> mOutputSpacing;
    QList<QStringList> mOutputOrigin;
    QList<QStringList> mSize;

};

#endif /* NMResampleImageFilterWrapper_H_ */
