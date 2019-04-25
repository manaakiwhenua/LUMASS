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
 * NMCostDistanceBufferImageWrapper.h
 *
 *  Created on: 5/12/2012
 *      Author: alex
 */

#ifndef NMCOSTDISTANCEBUFFERIMAGEWRAPPER_H_
#define NMCOSTDISTANCEBUFFERIMAGEWRAPPER_H_

#include <string>
#include <iostream>
#include <QStringList>
#include <QList>
#include <QMap>

#include "NMProcess.h"

#include "itkObject.h"
#include "nmcostdistancebufferimagewrapper_export.h"

//#include "itkCommand.h"
//#include "itkEventObject.h"


#ifdef BUILD_RASSUPPORT
	#include "RasdamanConnector.hh"
	#include "NMRasdamanConnectorWrapper.h"
#endif

template<class InPixelType, unsigned int Dimension>
class NMCostDistanceBufferImageWrapper_Internal;

class NMCOSTDISTANCEBUFFERIMAGEWRAPPER_EXPORT NMCostDistanceBufferImageWrapper : public NMProcess
{
	Q_OBJECT
	//Q_PROPERTY(int MemoryMax READ getMemoryMax WRITE setMemoryMax)
	Q_PROPERTY(QStringList InputImageFileName READ getInputImageFileName WRITE setInputImageFileName);
	Q_PROPERTY(QStringList CostImageFileName READ getCostImageFileName WRITE setCostImageFileName);
	Q_PROPERTY(QStringList OutputImageFileName READ getOutputImageFileName WRITE setOutputImageFileName);
	Q_PROPERTY(QList<QStringList> ObjectValueList READ getObjectValueList WRITE setObjectValueList);
	Q_PROPERTY(QStringList MaxDistance READ getMaxDistance WRITE setMaxDistance);
	Q_PROPERTY(bool UseImageSpacing READ getUseImageSpacing WRITE setUseImageSpacing);
	Q_PROPERTY(bool CreateBuffer READ getCreateBuffer WRITE setCreateBuffer);
	Q_PROPERTY(QStringList BufferZoneIndicator READ getBufferZoneIndicator WRITE setBufferZoneIndicator);
#ifdef BUILD_RASSUPPORT
	Q_PROPERTY(NMRasdamanConnectorWrapper* RasConnector READ getRasConnector WRITE setRasConnector);
#endif


public:
	//NMPropertyGetSet( MemoryMax, int )
	NMPropertyGetSet( ObjectValueList, QList<QStringList> )
	NMPropertyGetSet( MaxDistance, QStringList )
	NMPropertyGetSet( UseImageSpacing, bool )
	NMPropertyGetSet( CreateBuffer, bool )
	NMPropertyGetSet( BufferZoneIndicator, QStringList)
	NMPropertyGetSet( InputImageFileName, QStringList)
	NMPropertyGetSet( OutputImageFileName, QStringList)
	NMPropertyGetSet( CostImageFileName, QStringList)
//#ifdef BUILD_RASSUPPORT
//	NMPropertyGetSet(RasConnector, NMRasdamanConnectorWrapper*)
//#endif


public:
	NMCostDistanceBufferImageWrapper(QObject* parent = 0);
	virtual ~NMCostDistanceBufferImageWrapper();

	template<class InPixelType, unsigned int Dimension>
	friend class NMCostDistanceBufferImageWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
	void instantiateObject(void);

	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

#ifdef BUILD_RASSUPPORT
    void setRasConnector(NMRasdamanConnectorWrapper* rw);
    NMRasdamanConnectorWrapper* getRasConnector(void);

    void setRasdamanConnector(RasdamanConnector * rasconn);
#endif

	void update(void);

protected:

	int mMemoryMax;
	bool mUseImageSpacing;
	bool mCreateBuffer;
	QStringList mBufferZoneIndicator;
	QStringList mMaxDistance;
	QStringList mInputImageFileName;
	QStringList mOutputImageFileName;
	QStringList mCostImageFileName;
	QList<QStringList> mObjectValueList;
	int mCurrentStep;

#ifdef BUILD_RASSUPPORT
	NMRasdamanConnectorWrapper* mRasConnector;
	RasdamanConnector * mRasconn;
#endif
	bool mbRasMode;


	typedef itk::MemberCommand<NMCostDistanceBufferImageWrapper> DistanceObserverType;

	void UpdateProgressInfo(itk::Object*, const itk::EventObject&);


	void linkParameters(unsigned int step,
			const QMap<QString, NMModelComponent*>& repo);

	void linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
};

#endif /* NMCOSTDISTANCEBUFFERIMAGEWRAPPER_H_ */
