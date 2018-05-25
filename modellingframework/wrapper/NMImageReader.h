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

#include "otbImageIOBase.h"
#include "otbImageFileReader.h"
#include "otbGDALRATImageFileReader.h"
#include "NMItkDataObjectWrapper.h"
#include "otbAttributeTable.h"
#include "otbImage.h"
#include "itkSmartPointer.h"

#ifdef BUILD_RASSUPPORT
  #include "otbRasdamanImageReader.h"
  #include "RasdamanConnector.hh"
  #include "NMRasdamanConnectorWrapper.h"
#endif

#include "nmmodframe_export.h"

/**
 *  \brief Type independent image reader class.
 *
 *  The NMImageReader class is meant to hide the complexity of handling the
 *  data type templated GDALRATImageFileReader class.
 */

class NMMODFRAME_EXPORT NMImageReader : public NMProcess
{
	Q_OBJECT
	Q_PROPERTY(QStringList FileNames READ getFileNames WRITE setFileNames)
    Q_PROPERTY(QString RATType READ getRATType WRITE setRATType)
    Q_PROPERTY(QStringList RATEnum READ getRATEnum)
    Q_PROPERTY(bool RGBMode READ getRGBMode WRITE setRGBMode)
    Q_PROPERTY(QList<QStringList> BandList READ getBandList WRITE setBandList)

#ifdef BUILD_RASSUPPORT	
	Q_PROPERTY(NMRasdamanConnectorWrapper* RasConnector READ getRasConnector WRITE setRasConnector)
#endif

public:
    NMPropertyGetSet(FileNames, QStringList)
    NMPropertyGetSet(RGBMode, bool)
    NMPropertyGetSet(RATType, QString)
    NMPropertyGetSet(RATEnum, QStringList)
    NMPropertyGetSet(BandList, QList<QStringList>)
    //NMPropertyGetSet(BandMap, std::vector<int>)

//#ifdef BUILD_RASSUPPORT
//	NMPropertyGetSet(RasConnector, NMRasdamanConnectorWrapper*);
//#endif

signals:
	void FileNamesChanged(QStringList);

#ifdef BUILD_RASSUPPORT	
	void RasConnectorChanged(NMRasdamanConnectorWrapper*);
#endif

public:

	NMImageReader(QObject *parent=0);
	virtual ~NMImageReader();

	void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> img);//,

	// get the associated raster attribute table
	otb::AttributeTable::Pointer getRasterAttributeTable(int band);

	// get the ITK Image Base
	itk::DataObject* getItkImage(void);
	QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);

    // file name getter and setter
	void setFileName(QString filename);
	QString getFileName(void);

    // enable the RGB mode, i.e. in case we've got 3 bands
    // they're going to be interpreted as RGBPixel


#ifdef BUILD_RASSUPPORT	
    void setRasConnector(NMRasdamanConnectorWrapper* rw);
    NMRasdamanConnectorWrapper* getRasConnector(void);

    void setRasdamanConnector(RasdamanConnector * rasconn);
#endif
	// initialise the reader: this will probe the given
	// image file,  and set up the first part of the image pipeline

	const otb::ImageIOBase* getImageIOBase();
	void getBBox(double bbox[6]);

    void getSpacing(double spacing[3]);
    void getOrigin(double origin[3]);
    std::vector<double> getImageStatistics(const int* index=0,
                                           const int* size=0);

    int getNumberOfOverviews(void);
    std::vector<unsigned int> getOverviewSize(int ovvidx);

    void buildOverviews(const std::string& resamplingType);

    std::vector<int> getBandMap() {return this->mBandMap;}
    void setBandMap(std::vector<int> map);

    bool isRasMode(void) {return this->mbRasMode;}
	void instantiateObject(void);

    void setOverviewIdx(int ovvidx, int* userLPR);

protected:
	void UpdateProgressInfo(itk::Object* obj,
			const itk::EventObject& event);

	typedef itk::MemberCommand<NMImageReader> ReaderObserverType;

private:

	bool initialise();
	void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    void setInternalRATType(void);

	QString mFileName;
	QStringList mFileNames;

#ifdef BUILD_RASSUPPORT
	NMRasdamanConnectorWrapper* mRasConnector;
	RasdamanConnector * mRasconn;
#endif
	bool mbRasMode;

    // support vector / RGB display
    QList<QStringList> mBandList;

    bool mRGBMode;
    std::vector<int> mBandMap;

    QString mRATType;
    QStringList mRATEnum;

    otb::ImageIOBase::Pointer mItkImgIOBase;

	/** NMImageReader needs its own input parameter position indicator,
	 *  since it doesn't use the input components' path
	 */
	//unsigned int mFilePos;
};

#endif /* NMImageReader_H_ */
