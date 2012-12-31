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
 * NMStreamingImageFileWriterWrapper.cpp
 *
 *  Created on: 17/05/2012
 *      Author: alex
 */


#include "NMMacros.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkSmartPointer.h"
#include "itkProcessObject.h"
#include "otbStreamingRATImageFileWriter.h"

#ifdef BUILD_RASSUPPORT
	#include "RasdamanConnector.h"
	#include "otbRasdamanImageIO.h"
#endif


/** Helper Classes */
template <class PixelType, class PixelType2, unsigned int Dimension>
class NMStreamingImageFileWriterWrapper_Internal
{
public:
	typedef otb::Image<PixelType, Dimension> 			ImgType;
	typedef otb::VectorImage<PixelType, Dimension> 		VecImgType;
	typedef otb::StreamingRATImageFileWriter<ImgType> 		FilterType;
	typedef otb::StreamingRATImageFileWriter<VecImgType> 	VecFilterType;
	typedef typename FilterType::Pointer 				FilterTypePointer;
	typedef typename VecFilterType::Pointer 			VecFilterTypePointer;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
		{
			if (numBands == 1)
			{
				FilterTypePointer f = FilterType::New();
				otbFilter = f;
			}
			else
			{
				VecFilterTypePointer v = VecFilterType::New();
				otbFilter = v;
			}
		}

	static void setFileName(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, QString& fileName

#ifdef BUILD_RASSUPPORT
			,
			otb::RasdamanImageIO::Pointer& rio
#endif
			)
		{
			if (numBands == 1)
			{
				FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
				filter->SetFileName(fileName.toStdString());

#ifdef BUILD_RASSUPPORT
				if (rio.IsNotNull())
					filter->SetImageIO(rio);
#endif
			}
			else
			{
				VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
				filter->SetFileName(fileName.toStdString());

#ifdef BUILD_RASSUPPORT
				if (rio.IsNotNull())
					filter->SetImageIO(rio);
#endif
				}
		}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
		{
			if (numBands == 1)
			{
				ImgType* img = dynamic_cast<ImgType*>(dataObj);
				FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
				filter->SetInput(img);
			}
			else
			{
				VecImgType* img = dynamic_cast<VecImgType*>(dataObj);
				VecFilterType* filter = dynamic_cast<VecFilterType*>(otbFilter.GetPointer());
				filter->SetInput(img);
			}
		}

//	static void setNthAttributeTable(itk::ProcessObject::Pointer& otbFilter,
//			unsigned int idx, otb::AttributeTable* tab, std::vector<std::string> varNames)
//		{
//			FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
//			filter->SetNthAttributeTable(idx, tab, varNames);
//		}

};

#ifdef BUILD_RASSUPPORT
	#define callSetFileName( PixelType, wrapName ) \
		if (this->mOutputNumDimensions == 1)                                    \
		{                                                                   \
			wrapName<PixelType, PixelType, 1>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName, rio);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 2)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 2>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName, rio);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 3)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 3>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName, rio);                  \
		}
#else
	#define callSetFileName( PixelType, wrapName ) \
		if (this->mOutputNumDimensions == 1)                                    \
		{                                                                   \
			wrapName<PixelType, PixelType, 1>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 2)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 2>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName);                  \
		}                                                                   \
		else if (this->mOutputNumDimensions == 3)                               \
		{                                                                   \
			wrapName<PixelType, PixelType, 3>::setFileName(this->mOtbProcess,   \
					this->mOutputNumBands, fileName);                  \
		}
#endif // BUILD_RASSUPPORT


InstantiateObjectWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )
SetNthInputWrap( NMStreamingImageFileWriterWrapper, NMStreamingImageFileWriterWrapper_Internal )


NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
	this->mbIsInitialised = false;
	this->mInputComponentType = itk::ImageIOBase::FLOAT;
	this->mOutputComponentType = itk::ImageIOBase::FLOAT;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
#ifdef BUILD_RASSUPPORT
	this->mRasConnector = 0;
	this->mParameterHandling = NMProcess::NM_USE_UP;
	this->mParamPos = 0;
#endif
}

NMStreamingImageFileWriterWrapper
::NMStreamingImageFileWriterWrapper(QObject* parent,
			itk::ImageIOBase::IOComponentType componentType,
			unsigned int numDims, unsigned int numBands)
{
	this->setParent(parent);
	this->setObjectName(tr("NMStreamingImageFileWriterWrapper"));
	this->mbIsInitialised = false;
	this->mInputComponentType = componentType;
	this->mOutputComponentType = componentType;
	this->mInputNumBands = numBands;
	this->mOutputNumBands = numBands;
	this->mInputNumDimensions = numDims;
	this->mOutputNumDimensions = numDims;
#ifdef BUILD_RASSUPPORT
	this->mRasConnector = 0;
#endif
}

NMStreamingImageFileWriterWrapper::~NMStreamingImageFileWriterWrapper()
{
}

void
NMStreamingImageFileWriterWrapper
::setInternalFileName(QString fileName)
{
	if (!this->mbIsInitialised)
		return;

#ifdef BUILD_RASSUPPORT
	otb::RasdamanImageIO::Pointer rio;
	if (mRasConnector != 0)
	{
		rio = otb::RasdamanImageIO::New();
		rio->setRasdamanConnector(const_cast<RasdamanConnector*>(
				this->mRasConnector->getConnector()));
	}
#endif

	switch(this->mOutputComponentType)
	{
	MacroPerType( callSetFileName, NMStreamingImageFileWriterWrapper_Internal )
	default:
		break;
	}
}

void
NMStreamingImageFileWriterWrapper
::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	// we set our own progress observer here, to be able to filter
	// this particular event from being processed for this object
	// by the default NMProcess-based implementation
	//WriteObserverType::Pointer observer = WriteObserverType::New();
	//observer->SetCallbackFunction(this,
	//		&NMStreamingImageFileWriterWrapper::UpdateProgressInfo);
	//this->mOtbProcess->AddObserver(itk::ProgressEvent(), observer);
	//this->mOtbProcess->AddObserver(itk::StartEvent(), observer);
	//this->mOtbProcess->AddObserver(itk::EndEvent(), observer);
	//this->mOtbProcess->AddObserver(itk::AbortEvent(), observer);

	if (step > this->mFileNames.size()-1)
		step = 0;

	if (step < this->mFileNames.size())
	{
		this->setInternalFileName(this->mFileNames.at(step));
	}
}

//void
//NMStreamingImageFileWriterWrapper
//::UpdateProgressInfo(itk::Object* obj, const itk::EventObject& event)
//{
//	// we just filter the progress event, since it doesn't provide
//	// meaningful information for a writer (at the moment)
//	if (typeid(event) != typeid(itk::ProgressEvent()))
//		NMProcess::UpdateProgressInfo(obj, event);
//}




