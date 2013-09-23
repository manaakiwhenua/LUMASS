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
 * NMItk2VtkConnector.cpp
 *
 *  Created on: 26/01/2012
 *      Author: alex
 *
 *  Source of Inspiration: vtkKWImage class, http://hdl.handle.net/1926/495
 */

#ifndef NMItk2VtkConnector_H_
#define NMItk2VtkConnector_H_

#define ctxNMItk2VtkConnector "NMItk2VtkConnector"
#include "nmlog.h"
#include "NMProcess.h"

#include <QObject>
#include "NMItkDataObjectWrapper.h"
#include "vtkImageImport.h"
#include "vtkSmartPointer.h"
#include "itkDataObject.h"
#include "otbImageIOBase.h"
#include "itkImageBase.h"
#include "itkVTKImageExportBase.h"
#include "vtkImageChangeInformation.h"

class NMItk2VtkConnector : public NMProcess
{
	Q_OBJECT
public:

	NMItk2VtkConnector(QObject* parent=0);
	virtual ~NMItk2VtkConnector();

	void setNthInput(unsigned int numInput, NMItkDataObjectWrapper* imgWrapper);
	NMItkDataObjectWrapper* getOutput(unsigned int idx)
		{return 0;}

	vtkImageData *getVtkImage(void);
	vtkAlgorithmOutput *getVtkAlgorithmOutput(void);

	void instantiateObject(void);

private:
	NMItk2VtkConnector & operator=(const NMItk2VtkConnector&);

	vtkSmartPointer<vtkImageImport> mVtkImgImp;
	vtkSmartPointer<vtkImageChangeInformation> mVtkImgChangeInfo;
	itk::VTKImageExportBase::Pointer mVtkImgExp;

};

#endif // NMPipelineConnector_H_
