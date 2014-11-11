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
 * NMStreamingImageFileWriterWrapper.h
 *
 *  Created on: 17/05/2012
 *      Author: alex
 */

#ifndef NMSTREAMINGIMAGEFILEWRITERWRAPPER_H_
#define NMSTREAMINGIMAGEFILEWRITERWRAPPER_H_

#include "nmlog.h"
#define ctxNMStreamWriter "NMStreamingImageFileWriterWrapper"

#include <string>
#include <iostream>
#include <sstream>

#include <QScopedPointer>
#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>

#include "NMMacros.h"
#include "NMProcess.h"
#include "NMModelComponent.h"

#ifdef BUILD_RASSUPPORT
 #include "NMRasdamanConnectorWrapper.h"
 #include "otbRasdamanImageIO.h"
 #include "RasdamanConnector.hh"
#endif

class NMStreamingImageFileWriterWrapper: public NMProcess
{
	Q_OBJECT
#ifdef BUILD_RASSUPPORT	
	Q_PROPERTY(NMRasdamanConnectorWrapper* RasConnector READ getRasConnector WRITE setRasConnector)
#endif	
	Q_PROPERTY(QStringList FileNames READ getFileNames WRITE setFileNames)
    Q_PROPERTY(QStringList InputTables READ getInputTables WRITE setInputTables)
    Q_PROPERTY(bool UpdateMode READ getUpdateMode WRITE setUpdateMode)
    Q_PROPERTY(QString PyramidResamplingType READ getPyramidResamplingType WRITE setPyramidResamplingType)
    Q_PROPERTY(QStringList PyramidResamplingEnum READ getPyramidResamplingEnum)
    Q_PROPERTY(bool RGBMode READ getRGBMode WRITE setRGBMode)

public:
    NMPropertyGetSet( FileNames, QStringList )
    NMPropertyGetSet( InputTables, QStringList )
    NMPropertyGetSet( UpdateMode, bool )
    NMPropertyGetSet( PyramidResamplingType, QString )
    NMPropertyGetSet( PyramidResamplingEnum, QStringList )
    NMPropertyGetSet( RGBMode, bool)


#ifdef BUILD_RASSUPPORT
    void setRasConnector(NMRasdamanConnectorWrapper* rw);
    NMRasdamanConnectorWrapper* getRasConnector(void);

    //NMPropertyGetSet( RasConnector, NMRasdamanConnectorWrapper* )
#endif

	
signals:
	void FileNamesChanged(QStringList );
	
#ifdef BUILD_RASSUPPORT	
	void RasConnectorChanged(NMRasdamanConnectorWrapper* );
#endif

public:
	NMStreamingImageFileWriterWrapper(QObject* parent=0);
	NMStreamingImageFileWriterWrapper(QObject* parent,
			otb::ImageIOBase::IOComponentType componentType,
			unsigned int numDims, unsigned int numBands);
	virtual ~NMStreamingImageFileWriterWrapper();

	// NMModel relevant virtual methods
	void instantiateObject(void);

	// NMProcess interface methods
	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> imgWrapper);
    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);

protected:

    QString mPyramidResamplingType;
    QStringList mPyramidResamplingEnum;

	QStringList mFileNames;
    QStringList mInputTables;
    bool mUpdateMode;
    bool mRGBMode;

#ifdef BUILD_RASSUPPORT
	NMRasdamanConnectorWrapper* mRasConnector;
#endif	
	QString mRasConnectFile;

	//typedef itk::MemberCommand<NMStreamingImageFileWriterWrapper> WriteObserverType;

	//void UpdateProgressInfo(itk::Object* obj, const itk::EventObject& event);

	void setInternalFileName(QString fileName);
	void linkParameters(unsigned int step,
			const QMap<QString, NMModelComponent*>& repo);

    void setInternalUpdateMode();
    void setInternalResamplingType();
    void setInternalInputTable(const QString& tabelSpec,
                               const QMap<QString, NMModelComponent*>& repo);

};

#endif /* NMSTREAMINGIMAGEFILEWRITERWRAPPER_H_ */
