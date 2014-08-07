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
 * NMImageLayer.h
 *
 *  Created on: 18/01/2012
 *      Author: alex
 */

#ifndef NMIMAGELAYER_H_
#define NMIMAGELAYER_H_
#define ctxNMImageLayer "NMImageLayer"

#include "nmlog.h"

#include <NMLayer.h>
#include <NMImageReader.h>
#include <NMItk2VtkConnector.h>
#include <NMDataComponent.h>

#include "itkDataObject.h"
#include "otbImage.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "otbImageIOBase.h"
#include "vtkImageHistogram.h"


#ifdef BUILD_RASSUPPORT
  #include "RasdamanConnector.hh"
#endif

class NMLayer;

class NMImageLayer: public NMLayer
{
	Q_OBJECT

public:
	NMImageLayer(vtkRenderWindow* renWin,
			vtkRenderer* renderer=0,
			QObject* parent=0);
	virtual ~NMImageLayer();

	otb::ImageIOBase::IOComponentType getITKComponentType(void);
	unsigned int getNumDimensions(void)
		{return this->mNumDimensions;};
	unsigned int getNumBands(void)
			{return this->mNumBands;};

	bool setFileName(QString filename);

#ifdef BUILD_RASSUPPORT	
	void setRasdamanConnector(RasdamanConnector* rasconn)
		{this->mpRasconn = rasconn;};
#endif

	const vtkDataSet* getDataSet(void);

    /*!
     * \brief Calculates pixel indices for a given world coordinate on the
     *        displayed image. bOnLPR=true ensures that pixel indices of
     *        the largest possible region of the image are returned rather
     *        than pixel indices of an associated overview image (pyramid layer).
     * \param world
     * \param pixel
     * \param bOnLPR
     */
    void world2pixel(double world[3], int pixel[3],
        bool bOnLPR, bool bImgConstrained);

	void getBBox(double bbox[6]);
	otb::AttributeTable::Pointer getRasterAttributeTable(int band);

	void setImage(NMItkDataObjectWrapper* imgWrapper);

    void setUseOverviews(bool useOvv)
    {this->mbUseOverviews = useOvv;}
    bool getUseOverviews(void)
    {return this->mbUseOverviews;}

	itk::DataObject *getITKImage(void);
	NMItkDataObjectWrapper* getImage(void);
	NMProcess* getProcess(void)
		{return this->mReader;}

	bool isRasLayer(void) {return this->mReader->isRasMode();};
	//void mapUniqueValues();

	void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);
	NMItkDataObjectWrapper* getOutput(unsigned int idx);

	const double* getStatistics(void);
	const vtkImageProperty* getImageProperty(void)
		{return this->mImgProp;}

	double getDefaultNodata(void);


public slots:
    void updateSourceBuffer(void);
    void writeDataSet(void);
	void computeStats(void);
	void selectionChanged(const QItemSelection& newSel,
			const QItemSelection& oldSel);

	void test(void);

    void mapExtentChanged(void);

protected:

	void createTableView(void);
	void updateStats(void);

    NMDataComponent* mSourceBuffer;
	NMImageReader* mReader;
	NMItk2VtkConnector* mPipeconn;

#ifdef BUILD_RASSUPPORT	
	RasdamanConnector* mpRasconn;
#endif	

	//std::vector<otb::AttributeTable::Pointer> mRATVec;
	otb::AttributeTable::Pointer mOtbRAT;

	itk::DataObject::Pointer mImage;

    vtkSmartPointer<vtkImageChangeInformation> mImgInfoChange;

	vtkSmartPointer<vtkImageProperty> mImgProp;
	otb::ImageIOBase::IOComponentType mComponentType;
	unsigned int mNumDimensions;
	unsigned int mNumBands;

    double mSpacing[3];
    double mOrigin[3];

    int mOverviewIdx;

    /*!
     * \brief The buffered (current) image region
     *        (real world coordinates)
     *
     * \sa mLargestPossibleRegion
     *
     */
    double mBufferedBox[6];


	bool mbStatsAvailable;

    /*!
     * \brief Image stats
     *
	 * 0: min
	 * 1: max
	 * 2: mean
	 * 3: median
	 * 4: standard deviation
     *
	 */
	double mImgStats[5];

    bool mbUseOverviews;

	//void fetchRATs(void);

protected slots:
	int updateAttributeTable(void);
//	void windowLevelReset(vtkObject* obj);
//	void windowLevelChanged(vtkObject* obj);


	//virtual void updateDataSet(QStringList& slAlteredColumns,
	//		QStringList& slDeletedColumns);
};

#endif // ifndef NMIMAGELAYER_H_
