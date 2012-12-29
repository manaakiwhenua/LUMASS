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
 * NMImageReader.h
 *
 *  Created on: 25/11/2010
 *  Heavily revised: 20/01/2012
 *      Author: alex
 *
 *  Source of Inspiration:
 */


#ifndef NMImageReader_H_
#define NMImageReader_H_

#include <QObject>
#include <QString>
#include "NMProcess.h"
#include "NMMacros.h"

#include "itkImageIOBase.h"
#include "otbImageFileReader.h"
#include "otbGDALRATImageFileReader.h"
#include "NMItkDataObjectWrapper.h"
#include "otbAttributeTable.h"
#include "otbImage.h"
#include "itkSmartPointer.h"
#include "vtkSmartPointer.h"

#ifdef BUILD_RASSUPPORT
  #include "otbRasdamanImageReader.h"
  #include "RasdamanConnector.h"
  #include "NMRasdamanConnectorWrapper.h"
#endif

/**
 *  \brief Type independent image reader class.
 *
 *  The NMImageReader class is meant to hide the complexity of handling the
 *  data type templated GDALRATImageFileReader class.
 */

class NMImageReader : public NMProcess
{
	Q_OBJECT
	Q_PROPERTY(QStringList FileNames READ getFileNames WRITE setFileNames)

#ifdef BUILD_RASSUPPORT	
	Q_PROPERTY(NMRasdamanConnectorWrapper* RasConnector READ getRasConnector WRITE setRasConnector)
#endif

public:
	NMPropertyGetSet(FileNames, QStringList)

#ifdef BUILD_RASSUPPORT
	NMPropertyGetSet(RasConnector, NMRasdamanConnectorWrapper*)
#endif

signals:
	void FileNamesChanged(QStringList);

#ifdef BUILD_RASSUPPORT	
	void RasConnectorChanged(NMRasdamanConnectorWrapper*);
#endif

public:

	NMImageReader(QObject *parent=0);
	virtual ~NMImageReader();

	void setNthInput(unsigned int numInput,
			NMItkDataObjectWrapper* img);//,

	// get the associated raster attribute table
	otb::AttributeTable::Pointer getRasterAttributeTable(int band);

	// get the ITK Image Base
	itk::DataObject* getItkImage(void);

	NMItkDataObjectWrapper* getOutput(void);

	// file name getter and setter
	void setFileName(QString filename);
	QString getFileName(void);

#ifdef BUILD_RASSUPPORT	
	void setRasdamanConnector(RasdamanConnector * rasconn);
#endif
	// initialise the reader: this will probe the given
	// image file,  and set up the first part of the image pipeline

	const itk::ImageIOBase* getImageIOBase();
	void getBBox(double bbox[6]);

	bool isRasMode(void) {return this->mbRasMode;};
	void instantiateObject(void);
//	void linkInPipeline(const QMap<QString, NMModelComponent*>& repo);
//	void advanceParameter(unsigned int step,
//			const QMap<QString, NMModelComponent*>& repo);

private:

#ifdef BUILD_RASSUPPORT
	bool initialise() throw(r_Error);
#else
	bool initialise();
#endif

	void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

	QString mFileName;
	QStringList mFileNames;

#ifdef BUILD_RASSUPPORT
	NMRasdamanConnectorWrapper* mRasConnector;
	RasdamanConnector * mRasconn;
#endif
	bool mbRasMode;
	itk::ImageIOBase::Pointer mItkImgIOBase;

	/** NMImageReader needs its own input parameter position indicator,
	 *  since it doesn't use the input components' path
	 */
	//unsigned int mFilePos;
};

#endif /* NMImageReader_H_ */
