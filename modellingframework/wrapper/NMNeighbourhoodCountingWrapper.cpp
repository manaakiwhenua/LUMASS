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
 * NMNeighbourhoodCountingWrapper.cpp
 *
 *  Created on: 8/08/2012
 *      Author: alex
 */

#include "NMNeighbourhoodCountingWrapper.h"
#include "NMMacros.h"

#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImage.h"
#include "otbNeighbourhoodCountingFilter.h"

#include <vector>
#include <string>

/** Helper Class for dealing with ugly templates */
template <class InPixelType, class OutPixelType, unsigned int Dimension>
class NMNeighbourhoodCountingWrapper_Internal
{
public:
	typedef otb::Image<InPixelType, Dimension> InImgType;
	typedef otb::Image<OutPixelType, Dimension> OutImgType;
	typedef otb::NeighbourhoodCountingFilter<InImgType, OutImgType> FilterType;
	typedef typename FilterType::Pointer FilterTypePointer;
	typedef typename InImgType::SizeType InImgSizeType;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		FilterTypePointer f = FilterType::New();
		otbFilter = f;
	}

	static void setInternalTestValue(itk::ProcessObject::Pointer& otbFilter,
			int testvalue)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetTestvalue(testvalue);
	}

	static void setInternalRadius(itk::ProcessObject::Pointer& otbFilter,
			std::vector<unsigned int> userradius)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		InImgSizeType radius;
		for (unsigned int r=0; r < Dimension; ++r)
		{
			radius[r] = userradius[r];
		}
		filter->SetRadius(radius);
	}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
	{
		InImgType* img = dynamic_cast<InImgType*>(dataObj);
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetInput(img);
	}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
	}
};

GetOutputWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )
InstantiateObjectWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )
SetNthInputWrap( NMNeighbourhoodCountingWrapper, NMNeighbourhoodCountingWrapper_Internal )

//// some more macros, we need to support all sorts of types and dimensions

/*!
 *  \brief set the radius of the kernel defining the neighbourhood
 */
#define callSetInternalRadius( inputType, outputType, wrapperName )			\
{	\
	if (this->mInputNumDimensions == 1)                                     \
	{                                                                       \
		wrapperName< inputType, outputType, 1>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 2)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 2>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 3)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 3>::setInternalRadius(       \
				this->mOtbProcess, kernelsize);                       \
	}                                                                       \
}


/*!
 * \brief set the value to test the neighbourhood pixels against
 */
#define callSetInternalTestValue( inputType, outputType, wrapperName )		\
{                                                                           \
	if (this->mInputNumDimensions == 1)                                     \
	{                                                                       \
		wrapperName< inputType, outputType, 1>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 2)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 2>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
	else if (this->mInputNumDimensions == 3)                                \
	{                                                                       \
		wrapperName< inputType, outputType, 3>::setInternalTestValue(       \
				this->mOtbProcess, this->mTestValue);                       \
	}                                                                       \
}


NMNeighbourhoodCountingWrapper::NMNeighbourhoodCountingWrapper(QObject* parent)
	: mTestValue(0), mKernelSizeX(1), mKernelSizeY(1), mKernelSizeZ(1)
{
	this->setParent(parent);
	this->ctx = "NMNeighbourhoodCountingWrapper";

	this->mInputComponentType = otb::ImageIOBase::SHORT;
	this->mOutputComponentType = otb::ImageIOBase::SHORT;
	this->mParameterHandling = NMProcess::NM_USE_UP;
	this->mParamPos = 0;
}

NMNeighbourhoodCountingWrapper::~NMNeighbourhoodCountingWrapper()
{
}


void
NMNeighbourhoodCountingWrapper::internalSetTestValue(void)
{
	if (!this->mbIsInitialised)
		return;

	switch (this->mInputComponentType)
	{
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callSetInternalTestValue, NMNeighbourhoodCountingWrapper_Internal );                                                \
		break;																	\
	default:
		break;
	}
}

void
NMNeighbourhoodCountingWrapper::internalSetRadius(void)
{
	if (!this->mbIsInitialised)
		return;

	std::vector<unsigned int> kernelsize;
	kernelsize.push_back(this->mKernelSizeX);
	kernelsize.push_back(this->mKernelSizeY);
	kernelsize.push_back(this->mKernelSizeZ);

	switch (this->mInputComponentType)
	{
	case otb::ImageIOBase::UCHAR:                                               \
		outputTypeSwitch( unsigned char, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::CHAR:                                                \
		outputTypeSwitch( char, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::USHORT:                                              \
		outputTypeSwitch( unsigned short, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                        \
		break;                                                                  \
	case otb::ImageIOBase::SHORT:                                               \
		outputTypeSwitch( short, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::UINT:                                                \
		outputTypeSwitch( unsigned int, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                          \
		break;                                                                  \
	case otb::ImageIOBase::INT:                                                 \
		outputTypeSwitch( int, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                   \
		break;                                                                  \
	case otb::ImageIOBase::ULONG:                                               \
		outputTypeSwitch( unsigned long, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                         \
		break;                                                                  \
	case otb::ImageIOBase::LONG:                                                \
		outputTypeSwitch( long, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                  \
		break;                                                                  \
	case otb::ImageIOBase::FLOAT:                                               \
		outputTypeSwitch( float, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                 \
		break;                                                                  \
	case otb::ImageIOBase::DOUBLE:                                              \
		outputTypeSwitch( double, callSetInternalRadius, NMNeighbourhoodCountingWrapper_Internal );                                                \
		break;																	\
	default:
		break;
	}
}


void
NMNeighbourhoodCountingWrapper::linkParameters(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctx, << "...");

    //if (step > this->mTestValueList.size()-1)
    //	step = 0;

    if (this->mTestValueList.size())
	{
        int pos = this->mapHostIndexToPolicyIndex(step, mTestValueList.size());

		bool bok;
        int val = this->mTestValueList.at(pos).toInt(&bok);
		if (bok)
			this->mTestValue = val;
	}
	this->internalSetTestValue();
	this->internalSetRadius();

	NMDebugCtx(ctx, << "done!");
}


