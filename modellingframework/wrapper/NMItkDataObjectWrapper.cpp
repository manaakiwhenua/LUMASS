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
 * NMItkDataObjectWrapper.cpp
 *
 *  Created on: 22/04/2012
 *      Author: alex
 */

#include "NMItkDataObjectWrapper.h"
#include "itkDataObject.h"
#include "itkImageRegion.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkRGBPixel.h"
#include "NMMacros.h"

template <class PixelType, unsigned int ImageDimension>
class NMItkDataObjectWrapper_Internal
{
public:
    typedef itk::RGBPixel< PixelType >                  RGBPixelType;
    typedef otb::Image< RGBPixelType, ImageDimension >  RGBImgType;
    typedef typename RGBImgType::RegionType             RGBImgRegType;

    typedef otb::Image<PixelType, ImageDimension> ImgType;
    typedef typename ImgType::RegionType ImgRegType;
    typedef otb::VectorImage<PixelType, ImageDimension> VecType;
    typedef typename VecType::RegionType VecRegType;

    static void setRegion(itk::DataObject::Pointer& dataObj,
                          NMItkDataObjectWrapper::NMRegionType& regType,
                          void* regObj, unsigned int numBands, bool rgbMode)
    {
        if (numBands == 1)
        {
            ImgType* img = dynamic_cast<ImgType*>(dataObj.GetPointer());
            ImgRegType* reg = (ImgRegType*)regObj;

            switch(regType)
            {
                case NMItkDataObjectWrapper::NM_BUFFERED_REGION:
                    img->SetBufferedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_REQUESTED_REGION:
                    img->SetRequestedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_LARGESTPOSSIBLE_REGION:
                    img->SetLargestPossibleRegion(*reg);
                    break;
            }
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgType* img = dynamic_cast<RGBImgType*>(dataObj.GetPointer());
            RGBImgRegType* reg = (RGBImgRegType*)regObj;

            switch(regType)
            {
                case NMItkDataObjectWrapper::NM_BUFFERED_REGION:
                    img->SetBufferedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_REQUESTED_REGION:
                    img->SetRequestedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_LARGESTPOSSIBLE_REGION:
                    img->SetLargestPossibleRegion(*reg);
                    break;
            }
        }
        else
        {
            VecType* img = dynamic_cast<VecType*>(dataObj.GetPointer());
            VecRegType* reg = (VecRegType*)regObj;

            switch(regType)
            {
                case NMItkDataObjectWrapper::NM_BUFFERED_REGION:
                    img->SetBufferedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_REQUESTED_REGION:
                    img->SetRequestedRegion(*reg);
                    break;
                case NMItkDataObjectWrapper::NM_LARGESTPOSSIBLE_REGION:
                    img->SetLargestPossibleRegion(*reg);
                    break;
            }
        }
    }
};

#define DWSetRegion( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        NMItkDataObjectWrapper_Internal<comptype, 1>::setRegion( \
            mDataObject, regType, regObj, mNumBands, mIsRGBImage); \
        break; \
    case 2: \
        NMItkDataObjectWrapper_Internal<comptype, 2>::setRegion( \
            mDataObject, regType, regObj, mNumBands, mIsRGBImage); \
        break; \
    case 3: \
        NMItkDataObjectWrapper_Internal<comptype, 3>::setRegion( \
            mDataObject, regType, regObj, mNumBands, mIsRGBImage); \
        break; \
    }\
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject* parent)//, itk::DataObject* obj)
{
	this->setParent(parent);
	this->mDataObject = 0;
	this->mNumBands = 0;
	this->mNumDimensions = 1;
    this->mIsRGBImage = false;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject* parent, QString str)
{
	this->setParent(parent);
	this->mDataObject = 0;
	this->mStringObject = str;
	this->mNumBands = 0;
	this->mNumDimensions = 0;
    this->mIsRGBImage = false;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject *parent, itk::DataObject* obj,
		otb::ImageIOBase::IOComponentType type, unsigned int numDims, unsigned int numBands)
{
	this->setParent(parent);
	this->mDataObject = obj;
	this->setItkComponentType(type);
	this->setNumDimensions(numDims);
	this->setNumBands(numBands);
    this->mIsRGBImage = false;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(
		const NMItkDataObjectWrapper& dataObjectWrapper)
{
	NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dataObjectWrapper);
	this->setParent(w->parent());
	this->mDataObject = w->getDataObject();
    this->mOTBTab = w->getOTBTab();
	this->mNMComponentType = w->getNMComponentType();
	this->mNumDimensions = w->getNumDimensions();
	this->mNumBands = w->getNumBands();
    this->mIsRGBImage = w->getIsRGBImage();
}

NMItkDataObjectWrapper::~NMItkDataObjectWrapper()
{
}

NMItkDataObjectWrapper& NMItkDataObjectWrapper::operator=(const NMItkDataObjectWrapper& dw)
{
	this->setParent(dw.parent());
	NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dw);
	this->mDataObject = w->getDataObject();
    this->mOTBTab = w->getOTBTab();
	this->mNMComponentType = w->getNMComponentType();
	this->mNumDimensions = w->getNumDimensions();
	this->mNumBands = w->getNumBands();
    this->mIsRGBImage = w->getIsRGBImage();

	return *this;
}

void
NMItkDataObjectWrapper::setImageRegion(NMRegionType regType, void *regObj)
{
    if (mDataObject.IsNull())
    {
        NMWarn("NMItkDataObjectWrapper::setImageRegion()",
               << "itk::DataObject is NULL");
        return;
    }

    switch (this->mNMComponentType)
    {
        LocalMacroPerSingleType( DWSetRegion )
        default:
            break;
    }
}

otb::ImageIOBase::IOComponentType NMItkDataObjectWrapper::getItkComponentType()
{
	otb::ImageIOBase::IOComponentType type;
	switch (this->mNMComponentType)
	{
		case NM_UCHAR: type = otb::ImageIOBase::UCHAR; break;
		case NM_CHAR: type = otb::ImageIOBase::CHAR; break;
		case NM_USHORT: type = otb::ImageIOBase::USHORT; break;
		case NM_SHORT: type = otb::ImageIOBase::SHORT; break;
		case NM_UINT: type = otb::ImageIOBase::UINT; break;
		case NM_INT: type = otb::ImageIOBase::INT; break;
		case NM_ULONG: type = otb::ImageIOBase::ULONG; break;
		case NM_LONG: type = otb::ImageIOBase::LONG; break;
		case NM_FLOAT: type = otb::ImageIOBase::FLOAT; break;
		case NM_DOUBLE: type = otb::ImageIOBase::DOUBLE; break;
		default: type = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

	return type;
}

void NMItkDataObjectWrapper::setItkComponentType(otb::ImageIOBase::IOComponentType type)
{
	switch (type)
	{
		case otb::ImageIOBase::UCHAR	: this->mNMComponentType = NM_UCHAR	; break;
		case otb::ImageIOBase::CHAR		: this->mNMComponentType = NM_CHAR	; break;
		case otb::ImageIOBase::USHORT	: this->mNMComponentType = NM_USHORT; break;
		case otb::ImageIOBase::SHORT	: this->mNMComponentType = NM_SHORT	; break;
		case otb::ImageIOBase::UINT		: this->mNMComponentType = NM_UINT	; break;
		case otb::ImageIOBase::INT	 : this->mNMComponentType =	NM_INT	;	break;
		case otb::ImageIOBase::ULONG : this->mNMComponentType =	NM_ULONG;    break;
		case otb::ImageIOBase::LONG	 : this->mNMComponentType =	NM_LONG	;   break;
		case otb::ImageIOBase::FLOAT : this->mNMComponentType =	NM_FLOAT; break;
		case otb::ImageIOBase::DOUBLE: this->mNMComponentType =	NM_DOUBLE;	break;
		default: this->mNMComponentType = NM_UNKNOWN; break;
	}
}

const QString
NMItkDataObjectWrapper::getComponentTypeString(NMItkDataObjectWrapper::NMComponentType type)
{
	QString ret;
	switch (type)
	{
	case NM_UCHAR: ret = "uchar"; break;
	case NM_CHAR: ret = "char"; break;
	case NM_USHORT: ret = "ushort"; break;
	case NM_SHORT: ret = "short"; break;
	case NM_UINT: ret = "uint"; break;
	case NM_INT: ret = "int"; break;
	case NM_ULONG: ret = "ulong"; break;
	case NM_LONG: ret = "long"; break;
	case NM_FLOAT: ret = "float"; break;
	case NM_DOUBLE: ret = "double"; break;
	default: ret = "unknown"; break;
	}
	return ret;
}

const
NMItkDataObjectWrapper::NMComponentType
NMItkDataObjectWrapper::getComponentTypeFromString(const QString& compType)
{
	NMItkDataObjectWrapper::NMComponentType type = NMItkDataObjectWrapper::NM_UNKNOWN;
	if (compType.compare("uchar", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_UCHAR;
	else if (compType.compare("char", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_CHAR;
	else if (compType.compare("ushort", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_USHORT;
	else if (compType.compare("short", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_SHORT;
	else if (compType.compare("uint", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_UINT;
	else if (compType.compare("int", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_INT;
	else if (compType.compare("ulong", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_ULONG;
	else if (compType.compare("long", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_LONG;
	else if (compType.compare("float", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_FLOAT;
	else if (compType.compare("ushort", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_FLOAT;
	else if (compType.compare("double", Qt::CaseInsensitive) == 0)
		type = NMItkDataObjectWrapper::NM_DOUBLE;

	return type;
}
