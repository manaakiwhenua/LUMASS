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
	typedef typename ImgType::SpacingType			ImgSpacing;
	typedef typename ImgType::PointType				ImgOrigin;
	typedef typename ImgType::SizeType				ImgSize;

//	typedef otb::VectorImage< PixelType, ImageDimension > VecImgType;
//	typedef itk::NMVTKImageExport< VecImgType >		      VecExporterType;
//	typedef typename VecExporterType::Pointer		      VecExporterTypePointer;

    static void updateInput(
            itk::VTKImageExportBase::Pointer& vtkImgExp,
            vtkSmartPointer<vtkImageChangeInformation>& vtkImgChangeInfo,
            itk::DataObject::Pointer& imgObj, unsigned int numBands
            )
    {
        if (numBands == 1)
        {
            ImgType* img = dynamic_cast<ImgType*>(imgObj.GetPointer());
            ExporterType* vtkExp = dynamic_cast<ExporterType*>(vtkImgExp.GetPointer());
            vtkExp->SetInput(img);

//            img->UpdateOutputInformation();
//            const ImgSize& size = img->GetRequestedRegion().GetSize();
//            const ImgSpacing& spacing = img->GetSpacing();
//            const ImgOrigin& origin = img->GetOrigin();
//            double neworigin[3] = {0,0,0};
//            double newspacing[3] = {0,0,0};

//            std::cout.precision(9);
//            NMDebugAI(<< "VTK image origin: ");
//            for (unsigned int d=0; d < ImageDimension; ++d)
//            {
//                newspacing[d] = spacing[d];
//                if (d==1)
//                    neworigin[d] = origin[d];// - (spacing[d] / 2.0);
//                else
//                    neworigin[d] = origin[d] + (spacing[d] / 2.0);

//                NMDebug( << origin[d] << " ");
//            }
//            NMDebug(<< endl);


//            vtkImgChangeInfo->SetOutputOrigin(neworigin);
//            vtkImgChangeInfo->SetOutputSpacing(newspacing);
        }
    }

    static void connectPipeline(
			itk::VTKImageExportBase::Pointer& vtkImgExp,
			vtkSmartPointer<vtkImageImport>& vtkImgImp,
			vtkSmartPointer<vtkImageChangeInformation>& vtkImgChangeInfo,
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
			//vtkImgImp->SetReleaseDataFlag(1);

			// we shift the origin of the vtk image by half a pixel length in each
			// dimension, since vtk uses corner/edge-based coordinates
			// whereas itk/otb are using pixel-centered coordinates
//			img->UpdateOutputInformation();
//			const ImgSize& size = img->GetRequestedRegion().GetSize();
//			const ImgSpacing& spacing = img->GetSpacing();
//			const ImgOrigin& origin = img->GetOrigin();
//			double neworigin[3] = {0,0,0};
//			double newspacing[3] = {0,0,0};

//			std::cout.precision(9);
//			NMDebugAI(<< "VTK image origin: ");
//			for (unsigned int d=0; d < ImageDimension; ++d)
//			{
//				newspacing[d] = spacing[d];
//				if (d==1)
//					neworigin[d] = origin[d];// - (spacing[d] / 2.0);
//				else
//					neworigin[d] = origin[d] + (spacing[d] / 2.0);

//				NMDebug( << origin[d] << " ");
//			}
//			NMDebug(<< endl);



//			vtkImgChangeInfo->SetInputConnection(vtkImgImp->GetOutputPort());
//			vtkImgChangeInfo->SetOutputOrigin(neworigin);
//			vtkImgChangeInfo->SetOutputSpacing(newspacing);

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
			this->mVtkImgExp, this->mVtkImgImp, this->mVtkImgChangeInfo, this->mInputImg, \
			this->mInputNumBands); \
		break; \
	case 1: \
		PipelineConnector< PixelType, 1 >::connectPipeline( \
			this->mVtkImgExp, this->mVtkImgImp, this->mVtkImgChangeInfo, this->mInputImg, \
			this->mInputNumBands); \
		break; \
	default: \
		PipelineConnector< PixelType, 2 >::connectPipeline( \
			this->mVtkImgExp, this->mVtkImgImp, this->mVtkImgChangeInfo, this->mInputImg, \
			this->mInputNumBands); \
	} \
}

#define UpdateInput( PixelType ) \
{ \
    switch (this->mInputNumDimensions) \
    { \
    case 3: \
        PipelineConnector< PixelType, 3 >::updateInput( \
            this->mVtkImgExp, this->mVtkImgChangeInfo, this->mInputImg, this->mInputNumBands); \
        break; \
    case 1: \
        PipelineConnector< PixelType, 1 >::updateInput( \
            this->mVtkImgExp, this->mVtkImgChangeInfo, this->mInputImg, this->mInputNumBands); \
        break; \
    default: \
        PipelineConnector< PixelType, 2 >::updateInput( \
            this->mVtkImgExp, this->mVtkImgChangeInfo, this->mInputImg, this->mInputNumBands); \
    } \
}


NMItk2VtkConnector::NMItk2VtkConnector(QObject* parent)
    : mbIsConnected(false)
{
	this->setParent(parent);
}

NMItk2VtkConnector::~NMItk2VtkConnector()
{
}

void NMItk2VtkConnector::setNthInput(unsigned int numInput, NMItkDataObjectWrapper* imgWrapper)
{
    //NMDebugCtx(ctxNMItk2VtkConnector, << "...");

    if (mbIsConnected && this->mInputImg.IsNotNull())
    {
        this->updateInput(imgWrapper);
        //        NMDebugCtx(ctxNMItk2VtkConnector, << "done!");
        return;
    }

	this->mInputImg = imgWrapper->getDataObject();
	this->mInputComponentType = imgWrapper->getItkComponentType();
	this->mInputNumDimensions = imgWrapper->getNumDimensions();
	this->mInputNumBands = imgWrapper->getNumBands();

	this->mVtkImgImp = vtkSmartPointer<vtkImageImport>::New();
	this->mVtkImgChangeInfo = vtkSmartPointer<vtkImageChangeInformation>::New();

	bool connect = true;
	switch(this->mInputComponentType)
	{
    LocalMacroPerSingleType( ConnectPipelines )
	default:
		NMErr(ctxNMItk2VtkConnector,
				<< "UNKNOWN DATA TYPE, couldn't connect pipelines!");
		this->mInputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
        connect = false;
		break;
	}

    this->mbIsConnected = connect;
    //	NMDebugCtx(ctxNMItk2VtkConnector, << "done!");
}

void
NMItk2VtkConnector::updateInput(NMItkDataObjectWrapper* imgWrapper)
{
    if (imgWrapper)
    {
        if (    imgWrapper->getDataObject()
            &&  this->mInputComponentType == imgWrapper->getItkComponentType()
           )
        {
            this->mInputImg = imgWrapper->getDataObject();
            this->mInputComponentType = imgWrapper->getItkComponentType();
            this->mInputNumDimensions = imgWrapper->getNumDimensions();
            this->mInputNumBands = imgWrapper->getNumBands();

            switch(this->mInputComponentType)
            {
            LocalMacroPerSingleType( UpdateInput )
            default:
                NMErr(ctxNMItk2VtkConnector,
                        << "UNKNOWN DATA TYPE, couldn't connect pipelines!");
                this->mInputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
                break;
            }
        }
        else
        {
            QString parentName = "NMItk2VtkConnector";
            if (this->parent())
            {
                if (this->parent()->objectName().isEmpty())
                {
                    parentName = this->parent()->metaObject()->className();
                }
                else
                {
                    parentName = this->parent()->objectName();
                }
            }

            NMErr(ctxNMItk2VtkConnector,
                  << "Data type mismatch! Update of "
                  << parentName.toStdString() << "'s data failed!");
        }
    }
}

void
NMItk2VtkConnector::update()
{
    // this class is just a container for the ITK-VTK bridge
    // whose involved classes are updated by other classes, so
    // no need for an update implementation here
}

vtkImageData * NMItk2VtkConnector::getVtkImage()
{
    //return this->mVtkImgChangeInfo->GetOutput();
    return this->mVtkImgImp->GetOutput();
}

vtkAlgorithmOutput * NMItk2VtkConnector::getVtkAlgorithmOutput()
{
    //return this->mVtkImgChangeInfo->GetOutputPort();
    return this->mVtkImgImp->GetOutputPort();
}

vtkImageImport*
NMItk2VtkConnector::getVtkImageImport(void)
{
    return this->mVtkImgImp.Get();
}

void NMItk2VtkConnector::instantiateObject(void)
{
}

