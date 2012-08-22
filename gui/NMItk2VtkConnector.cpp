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

#include "NMItk2VtkConnector.h"

#include "otbImage.h"
#include "otbVectorImage.h"
#include "NMitkVTKImageExport.h"
//#include "NMitkVTKImageExport.txx"

/** some private classes facilitating pipeline connection
 *  for templated image exporter
 */
template <class PixelType, unsigned int ImageDimension>
class PipelineConnector
{
public:
	typedef otb::Image< PixelType, ImageDimension > ImgType;
	typedef itk::NMVTKImageExport< ImgType >	    ExporterType;
	typedef typename ExporterType::Pointer	        ExporterTypePointer;

//	typedef otb::VectorImage< PixelType, ImageDimension > VecImgType;
//	typedef itk::NMVTKImageExport< VecImgType >		      VecExporterType;
//	typedef typename VecExporterType::Pointer		      VecExporterTypePointer;

	static void connectPipeline(
			itk::VTKImageExportBase::Pointer& vtkImgExp,
			vtkSmartPointer<vtkImageImport>& vtkImgImp,
			itk::DataObject::Pointer& imgObj,
			unsigned int numBands)
	{
		NMDebugCtx(ctxNMItk2VtkConnector, << "...");

		if (numBands == 1)
		{
			ImgType *img = dynamic_cast<ImgType*>(imgObj.GetPointer());
			ExporterTypePointer vtkExp = ExporterType::New();

			vtkExp->SetInput(img);
			vtkImgImp->SetBufferPointerCallback(vtkExp->GetBufferPointerCallback());
			vtkImgImp->SetCallbackUserData(vtkExp->GetCallbackUserData());
			vtkImgImp->SetDataExtentCallback(vtkExp->GetDataExtentCallback());
			vtkImgImp->SetNumberOfComponentsCallback(vtkExp->GetNumberOfComponentsCallback());
			vtkImgImp->SetOriginCallback(vtkExp->GetOriginCallback());
			vtkImgImp->SetPipelineModifiedCallback(vtkExp->GetPipelineModifiedCallback());
			vtkImgImp->SetPropagateUpdateExtentCallback(vtkExp->GetPropagateUpdateExtentCallback());
			vtkImgImp->SetScalarTypeCallback(vtkExp->GetScalarTypeCallback());
			vtkImgImp->SetSpacingCallback(vtkExp->GetSpacingCallback());
			vtkImgImp->SetUpdateDataCallback(vtkExp->GetUpdateDataCallback());
			vtkImgImp->SetUpdateInformationCallback(vtkExp->GetUpdateInformationCallback());
			vtkImgImp->SetWholeExtentCallback(vtkExp->GetWholeExtentCallback());
	//		vtkImgImp->SetReleaseDataFlag(1);

			// keep references to the exporter
			vtkImgExp = vtkExp;
		}
//		else
//		{
//			VecImgType *img = dynamic_cast<VecImgType*>(imgObj);
//			VecExporterTypePointer vtkExp = VecExporterType::New();
//
//			vtkExp->SetInput(img);
//			vtkImgImp->SetBufferPointerCallback(vtkExp->GetBufferPointerCallback());
//			vtkImgImp->SetCallbackUserData(vtkExp->GetCallbackUserData());
//			vtkImgImp->SetDataExtentCallback(vtkExp->GetDataExtentCallback());
//			vtkImgImp->SetNumberOfComponentsCallback(vtkExp->GetNumberOfComponentsCallback());
//			vtkImgImp->SetOriginCallback(vtkExp->GetOriginCallback());
//			vtkImgImp->SetPipelineModifiedCallback(vtkExp->GetPipelineModifiedCallback());
//			vtkImgImp->SetPropagateUpdateExtentCallback(vtkExp->GetPropagateUpdateExtentCallback());
//			vtkImgImp->SetScalarTypeCallback(vtkExp->GetScalarTypeCallback());
//			vtkImgImp->SetSpacingCallback(vtkExp->GetSpacingCallback());
//			vtkImgImp->SetUpdateDataCallback(vtkExp->GetUpdateDataCallback());
//			vtkImgImp->SetUpdateInformationCallback(vtkExp->GetUpdateInformationCallback());
//			vtkImgImp->SetWholeExtentCallback(vtkExp->GetWholeExtentCallback());
//	//		vtkImgImp->SetReleaseDataFlag(1);
//
//			// keep references to the exporter
//			vtkImgExp = vtkExp;
//
//		}

		NMDebugCtx(ctxNMItk2VtkConnector, << "done!");
	}
};


#define ConnectPipelines( PixelType ) \
{ \
	switch (this->mInputNumDimensions) \
	{ \
	case 3: \
		PipelineConnector< PixelType, 3 >::connectPipeline( \
			this->mVtkImgExp, this->mVtkImgImp, this->mInputImg, \
			this->mInputNumBands); \
		break; \
	default: \
		PipelineConnector< PixelType, 2 >::connectPipeline( \
			this->mVtkImgExp, this->mVtkImgImp, this->mInputImg, \
			this->mInputNumBands); \
	} \
}

NMItk2VtkConnector::NMItk2VtkConnector(QObject* parent)
{
	this->setParent(parent);
}

NMItk2VtkConnector::~NMItk2VtkConnector()
{
}

void NMItk2VtkConnector::setNthInput(unsigned int numInput, NMItkDataObjectWrapper* imgWrapper)
{
	NMDebugCtx(ctxNMItk2VtkConnector, << "...");

	this->mInputImg = imgWrapper->getDataObject();

//	this->mItkImgObj = imgObj;
	this->mInputComponentType = imgWrapper->getItkComponentType();
	this->mInputNumDimensions = imgWrapper->getNumDimensions();
	this->mInputNumBands = imgWrapper->getNumBands();

	this->mVtkImgImp = vtkSmartPointer<vtkImageImport>::New();

	switch(this->mInputComponentType)
	{
	case itk::ImageIOBase::UCHAR:
		ConnectPipelines( unsigned char );
		break;
	case itk::ImageIOBase::CHAR:
		ConnectPipelines( char );
		break;
	case itk::ImageIOBase::USHORT:
		ConnectPipelines( unsigned short );
		break;
	case itk::ImageIOBase::SHORT:
		ConnectPipelines( short );
		break;
	case itk::ImageIOBase::UINT:
		ConnectPipelines( unsigned int );
		break;
	case itk::ImageIOBase::INT:
		ConnectPipelines( int );
		break;
	case itk::ImageIOBase::ULONG:
		ConnectPipelines( unsigned long );
		break;
	case itk::ImageIOBase::LONG:
		ConnectPipelines( long );
		break;
	case itk::ImageIOBase::FLOAT:
		ConnectPipelines( float );
		break;
	case itk::ImageIOBase::DOUBLE:
		ConnectPipelines( double );
		break;
	default:
		NMErr(ctxNMItk2VtkConnector,
				<< "UNKNOWN DATA TYPE, couldn't connect pipelines!");
		this->mInputComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
		break;
	}
	NMDebugCtx(ctxNMItk2VtkConnector, << "done!");
}

vtkImageData * NMItk2VtkConnector::getVtkImage()
{
	return this->mVtkImgImp->GetOutput();
}

vtkAlgorithmOutput * NMItk2VtkConnector::getVtkAlgorithmOutput()
{
	return this->mVtkImgImp->GetOutputPort();
}

void NMItk2VtkConnector::instantiateObject(void)
{

}

NMItkDataObjectWrapper* NMItk2VtkConnector::getOutput(void)
{
	return 0;
}
