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

#include "itkDataObject.h"
#include "otbImage.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "otbImageIOBase.h"
#include "vtkImageHistogram.h"

#ifdef BUILD_RASSUPPORT
  #include "RasdamanConnector.hh"
#endif

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

	void world2pixel(double world[3], int pixel[3]);
	void getBBox(double bbox[6]);
	otb::AttributeTable::Pointer getRasterAttributeTable(int band);

	void setImage(NMItkDataObjectWrapper* imgWrapper);

	itk::DataObject *getITKImage(void);
	NMItkDataObjectWrapper* getImage(void);
	NMProcess* getProcess(void)
		{return this->mReader;}

	bool isRasLayer(void) {return this->mReader->isRasMode();};
	int mapUniqueValues(QString fieldName);

	void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);
	NMItkDataObjectWrapper* getOutput(unsigned int idx);

	const double* getStatistics(void);


public slots:
	void writeDataSet(void);
	void computeStats(void);
	void selectionChanged(const QItemSelection& newSel,
			const QItemSelection& oldSel);
	//void selectCell(int cellID)

	void test(void);


protected:

	void createTableView(void);
	void updateStats(void);

	NMImageReader* mReader;
	NMItk2VtkConnector* mPipeconn;

#ifdef BUILD_RASSUPPORT	
	RasdamanConnector* mpRasconn;
#endif	

	//std::vector<otb::AttributeTable::Pointer> mRATVec;
	otb::AttributeTable::Pointer mOtbRAT;

	itk::DataObject::Pointer mImage;
	vtkSmartPointer<vtkImageProperty> mImgProp;
	otb::ImageIOBase::IOComponentType mComponentType;
	unsigned int mNumDimensions;
	unsigned int mNumBands;

	bool mbStatsAvailable;

	/* Image stats
	 * 0: min
	 * 1: max
	 * 2: mean
	 * 3: median
	 * 4: standard deviation
	 */
	double mImgStats[5];

	//void fetchRATs(void);

protected slots:
	int updateAttributeTable(void);
	void windowLevelReset(vtkObject* obj);
	void windowLevelChanged(vtkObject* obj);


	//virtual void updateDataSet(QStringList& slAlteredColumns,
	//		QStringList& slDeletedColumns);
};

#endif // ifndef NMIMAGELAYER_H_
