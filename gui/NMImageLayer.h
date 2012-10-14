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
#include "itkImageIOBase.h"

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

	itk::ImageIOBase::IOComponentType getITKComponentType(void);
	unsigned int getNumDimensions(void)
		{return this->mNumDimensions;};
	unsigned int getNumBands(void)
			{return this->mNumBands;};

	void setFileName(QString filename);

#ifdef BUILD_RASSUPPORT	
	void setRasdamanConnector(RasdamanConnector* rasconn)
		{this->mpRasconn = rasconn;};
#endif

	const vtkDataSet* getDataSet(void);

	void getBBox(double bbox[6]);
	otb::AttributeTable* getRasterAttributeTable(int band);

//	void setITKImage(itk::DataObject* img,
//			itk::ImageIOBase::IOComponentType pixType,
//			unsigned int numDims,
//			unsigned int numBands);
	void setImage(NMItkDataObjectWrapper* imgWrapper);

	itk::DataObject *getITKImage(void);
	NMItkDataObjectWrapper* getImage(void);
	NMProcess* getProcess(void)
		{return this->mReader;}

	bool isRasLayer(void) {return this->mReader->isRasMode();};
	int mapUniqueValues(QString fieldName);

	void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);
	NMItkDataObjectWrapper* getOutput(void);

protected:

	void createTableView(void);

	NMImageReader* mReader;
	NMItk2VtkConnector* mPipeconn;

#ifdef BUILD_RASSUPPORT	
	RasdamanConnector* mpRasconn;
#endif	

	std::vector<otb::AttributeTable::Pointer> mRATVec;

	itk::DataObject::Pointer mImage;
	vtkSmartPointer<vtkImageProperty> mImgProp;
	itk::ImageIOBase::IOComponentType mComponentType;
	unsigned int mNumDimensions;
	unsigned int mNumBands;

	void fetchRATs(void);

protected slots:
	void updateAttributeTable(void);
};

#endif // ifndef NMIMAGELAYER_H_
