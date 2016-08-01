 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
 * NMNeighbourhoodCountingWrapper.h
 *
 *  Created on: 8/08/2012
 *      Author: alex
 */

#ifndef NMNEIGHBOURHOODCOUNTINGWRAPPER_H_
#define NMNEIGHBOURHOODCOUNTINGWRAPPER_H_

#include "nmlog.h"

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>

#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "otbImageIOBase.h"
#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMNeighbourhoodCountingWrapper: public NMProcess
{
	Q_OBJECT
	Q_PROPERTY(QStringList TestValueList READ getTestValueList WRITE setTestValueList)
	Q_PROPERTY(int TestValue READ getTestValue WRITE setTestValue)
	Q_PROPERTY(unsigned int KernelSizeX READ getKernelSizeX WRITE setKernelSizeX)
	Q_PROPERTY(unsigned int KernelSizeY READ getKernelSizeY WRITE setKernelSizeY)
	Q_PROPERTY(unsigned int KernelSizeZ READ getKernelSizeZ WRITE setKernelSizeZ)

public:
	NMPropertyGetSet( TestValueList, QStringList );
	NMPropertyGetSet( TestValue, int);
	NMPropertyGetSet( KernelSizeX, unsigned int );
	NMPropertyGetSet( KernelSizeY, unsigned int );
	NMPropertyGetSet( KernelSizeZ, unsigned int );

signals:


public:
	NMNeighbourhoodCountingWrapper(QObject* parent = 0);
	virtual ~NMNeighbourhoodCountingWrapper();

	QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

protected:
    void linkParameters(unsigned int step,
    		const QMap<QString, NMModelComponent*>& repo);
	void internalSetTestValue();
	void internalSetRadius();

	std::string ctx;

	QStringList mTestValueList;
	int mTestValue;
	//QStringListe mKernelSizeList;
	unsigned int mKernelSizeX;
	unsigned int mKernelSizeY;
	unsigned int mKernelSizeZ;
};

#endif /* NMNEIGHBOURHOODCOUNTINGWRAPPER_H_ */
