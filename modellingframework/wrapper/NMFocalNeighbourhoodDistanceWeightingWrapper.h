/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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
 * NMFocalNeighbourhoodDistanceWeightingWrapper.h
 *
 *  Created on: 12/01/2013
 *      Author: alex
 */

#ifndef NMFOCALNEIGHBOURHOODDISTANCEWEIGHTINGWRAPPER_H_
#define NMFOCALNEIGHBOURHOODDISTANCEWEIGHTINGWRAPPER_H_

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "nmfocalneighbourhooddistanceweightingwrapper_export.h"

template<class InPixelType, class OutPixelType, unsigned int Dimension>
class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal;

class
NMFOCALNEIGHBOURHOODDISTANCEWEIGHTINGWRAPPER_EXPORT NMFocalNeighbourhoodDistanceWeightingWrapper
		: public NMProcess
{
	Q_OBJECT
	Q_PROPERTY(QStringList RadiusList READ getRadiusList WRITE setRadiusList)
	Q_PROPERTY(QList<QList<QStringList> > Weights READ getWeights WRITE setWeights)
	Q_PROPERTY(QList<QStringList> Values READ getValues WRITE setValues)

public:

	NMPropertyGetSet ( RadiusList, QStringList)
	NMPropertyGetSet ( Weights, QList<QList<QStringList> >)
	NMPropertyGetSet ( Values, QList<QStringList>)

	NMFocalNeighbourhoodDistanceWeightingWrapper(QObject* parent=0);
	virtual ~NMFocalNeighbourhoodDistanceWeightingWrapper();

	template<class InPixelType, class OutPixelType, unsigned int Dimension>
	friend class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal;

	QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);

    QStringList 				mRadiusList;
    QList<QList<QStringList> >  mWeights;
    QList<QStringList> 			mValues;

};

#endif /* NMFOCALNEIGHBOURHOODDISTANCEWEIGHTINGWRAPPER_H_ */
