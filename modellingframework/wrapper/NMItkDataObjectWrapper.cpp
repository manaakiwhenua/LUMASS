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
 *
 *  last updated: 2021-07-21
 */

#include "NMItkDataObjectWrapper.h"

#include <array>

#include "itkDataObject.h"
#include "itkImageRegion.h"
#include "otbImage.h"
#include "otbVectorImage.h"
#include "itkRGBPixel.h"
#include "nmDataBufferFilter.h"
#include "NMMacros.h"
#include "nmtypeinfo.h"

template <class PixelType, unsigned int ImageDimension>
class NMItkDataObjectWrapper_Internal
{
public:
    typedef itk::RGBPixel< PixelType >                  RGBPixelType;
    typedef otb::Image< RGBPixelType, ImageDimension >  RGBImgType;
    typedef typename RGBImgType::RegionType             RGBImgRegType;
    typedef typename RGBImgType::PointType              RGBImgPointType;
    typedef typename RGBImgType::SpacingType            RGBImgSpacingType;

    using RGBImgBufferFilterType    = typename otb::DataBufferFilter<RGBImgType>;//, RGBImgType>;
    using RGBImgBufferFilterPointer = typename RGBImgBufferFilterType::Pointer;

    typedef otb::Image<PixelType, ImageDimension> ImgType;
    typedef typename ImgType::RegionType ImgRegType;
    typedef typename ImgType::PointType ImgPointType;
    typedef typename ImgType::SpacingType ImgSpacingType;

    using ImgBufferFilterType    = typename otb::DataBufferFilter<ImgType>;//, ImgType>;
    using ImgBufferFilterPointer = typename ImgBufferFilterType::Pointer;

    typedef otb::VectorImage<PixelType, ImageDimension> VecType;
    typedef typename VecType::RegionType VecRegType;
    typedef typename VecType::PointType VecPointType;
    typedef typename VecType::SpacingType VecSpacingType;

    using VecImgBufferFilterType    = typename otb::DataBufferFilter<VecType>;//, VecType>;
    using VecImgBufferFilterPointer = typename VecImgBufferFilterType::Pointer;


    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
                               unsigned int numBands, bool rgbMode)
    {
        if (numBands == 1)
        {
            ImgBufferFilterPointer f = ImgBufferFilterType::New();
            otbFilter = f;
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgBufferFilterPointer f = RGBImgBufferFilterType::New();
            otbFilter = f;
        }
        else
        {
            VecImgBufferFilterPointer f = VecImgBufferFilterType::New();
            otbFilter = f;
        }
    }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                    unsigned int numBands, unsigned int idx, bool rgbMode, itk::DataObject* dataObj)
    {
        if (numBands == 1)
        {
            ImgBufferFilterPointer f = dynamic_cast<ImgBufferFilterType*>(otbFilter.GetPointer());
            f->SetNthInput(idx, dataObj);
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgBufferFilterPointer f = dynamic_cast<RGBImgBufferFilterType*>(
                                            otbFilter.GetPointer());
            f->SetNthInput(idx, dataObj);
        }
        else
        {
            VecImgBufferFilterPointer f = dynamic_cast<VecImgBufferFilterType*>(
                                            otbFilter.GetPointer());
            f->SetNthInput(idx, dataObj);
        }
    }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
                    unsigned int numBands, unsigned int idx, bool rgbMode)
    {
        itk::DataObject* dbj = nullptr;

        if (numBands == 1)
        {
            ImgBufferFilterPointer f = dynamic_cast<ImgBufferFilterType*>(otbFilter.GetPointer());
            return dynamic_cast<ImgType*>(f->GetOutput(idx));
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgBufferFilterPointer f = dynamic_cast<RGBImgBufferFilterType*>(
                                            otbFilter.GetPointer());
            return dynamic_cast<RGBImgType*>(f->GetOutput(idx));
        }
        else
        {
            VecImgBufferFilterPointer f = dynamic_cast<VecImgBufferFilterType*>(
                                            otbFilter.GetPointer());
            return dynamic_cast<VecType*>(f->GetOutput(idx));
        }

        return dbj;
    }

    static void getSignedSpacing(itk::DataObject::Pointer& dataObj,
                           unsigned int numBands, bool rgbMode, std::array<double, 3>& spacing)
    {
        if (numBands == 1)
        {
            ImgType* img = dynamic_cast<ImgType*>(dataObj.GetPointer());
            ImgSpacingType spac = img->GetSignedSpacing();
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                spacing[d] = spac[d];
            }
            for (unsigned int k=ImageDimension; k < 3; ++k)
            {
                spacing[k] = 0.0;
            }
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgType* img = dynamic_cast<RGBImgType*>(dataObj.GetPointer());
            RGBImgPointType spac = img->GetSignedSpacing();
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                spacing[d] = spac[d];
            }
            for (unsigned int k=ImageDimension; k < 3; ++k)
            {
                spacing[k] = 0.0;
            }
        }
        else
        {
            VecType* img = dynamic_cast<VecType*>(dataObj.GetPointer());
            VecPointType spac = img->GetSignedSpacing();
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                spacing[d] = spac[d];
            }
            for (unsigned int k=ImageDimension; k < 3; ++k)
            {
                spacing[k] = 0.0;
            }
        }
    }

    static void setOrigin(itk::DataObject::Pointer& dataObj,
                          unsigned int numBands, bool rgbMode, const double* origin)
    {
        if (numBands == 1)
        {
            ImgType* img = dynamic_cast<ImgType*>(dataObj.GetPointer());
            ImgPointType orig;
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                orig[d] = origin[d];
            }
            img->SetOrigin(orig);
        }
        else if (numBands == 3 && rgbMode)
        {
            RGBImgType* img = dynamic_cast<RGBImgType*>(dataObj.GetPointer());
            RGBImgPointType orig;
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                orig[d] = origin[d];
            }
            img->SetOrigin(orig);
        }
        else
        {
            VecType* img = dynamic_cast<VecType*>(dataObj.GetPointer());
            VecPointType orig;
            for (unsigned int d=0; d < ImageDimension; ++d)
            {
                orig[d] = origin[d];
            }
            img->SetOrigin(orig);
        }
    }

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

#define DWCreateBufferFilterInstance( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        NMItkDataObjectWrapper_Internal<comptype, 1>::createInstance( \
            mItkProcess, mNumBands, mIsRGBImage); \
        break; \
    case 2: \
        NMItkDataObjectWrapper_Internal<comptype, 2>::createInstance( \
            mItkProcess, mNumBands, mIsRGBImage); \
        break; \
    case 3: \
        NMItkDataObjectWrapper_Internal<comptype, 3>::createInstance( \
            mItkProcess, mNumBands, mIsRGBImage); \
        break; \
    }\
}

#define DWSetNthBufferFilterInput( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        NMItkDataObjectWrapper_Internal<comptype, 1>::setNthInput( \
            mItkProcess, mNumBands, idx, mIsRGBImage, dataObj); \
        break; \
    case 2: \
        NMItkDataObjectWrapper_Internal<comptype, 2>::setNthInput( \
            mItkProcess, mNumBands, idx, mIsRGBImage, dataObj); \
        break; \
    case 3: \
        NMItkDataObjectWrapper_Internal<comptype, 3>::setNthInput( \
            mItkProcess, mNumBands, idx, mIsRGBImage, dataObj); \
        break; \
    }\
}

#define DWGetBufferFilterOutput( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        bufout = NMItkDataObjectWrapper_Internal<comptype, 1>::getOutput( \
            mItkProcess, mNumBands, idx, mIsRGBImage); \
        break; \
    case 2: \
        bufout = NMItkDataObjectWrapper_Internal<comptype, 2>::getOutput( \
            mItkProcess, mNumBands, idx, mIsRGBImage); \
        break; \
    case 3: \
        bufout = NMItkDataObjectWrapper_Internal<comptype, 3>::getOutput( \
            mItkProcess, mNumBands, idx, mIsRGBImage); \
        break; \
    }\
}

#define DWGetSignedSpacing( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        NMItkDataObjectWrapper_Internal<comptype, 1>::getSignedSpacing( \
            mDataObject, mNumBands, mIsRGBImage, spacing); \
        break; \
    case 2: \
        NMItkDataObjectWrapper_Internal<comptype, 2>::getSignedSpacing( \
            mDataObject, mNumBands, mIsRGBImage, spacing); \
        break; \
    case 3: \
        NMItkDataObjectWrapper_Internal<comptype, 3>::getSignedSpacing( \
            mDataObject, mNumBands, mIsRGBImage, spacing); \
        break; \
    }\
}

#define DWSetOrigin( comptype ) \
{ \
    switch(mNumDimensions) \
    { \
    case 1: \
        NMItkDataObjectWrapper_Internal<comptype, 1>::setOrigin( \
            mDataObject, mNumBands, mIsRGBImage, origin); \
        break; \
    case 2: \
        NMItkDataObjectWrapper_Internal<comptype, 2>::setOrigin( \
            mDataObject, mNumBands, mIsRGBImage, origin); \
        break; \
    case 3: \
        NMItkDataObjectWrapper_Internal<comptype, 3>::setOrigin( \
            mDataObject, mNumBands, mIsRGBImage, origin); \
        break; \
    }\
}

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
    this->mDataObject = nullptr;
    this->mNumBands = 0;
    this->mNumDimensions = 1;
    this->mIsRGBImage = false;
    this->mbIsStreaming = false;
    this->mItkProcess = nullptr;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject* parent, QString str)
{
    this->mDataObject = nullptr;
    this->mStringObject = str;
    this->mNumBands = 0;
    this->mNumDimensions = 0;
    this->mIsRGBImage = false;
    this->mbIsStreaming = false;
    this->mItkProcess = nullptr;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(QObject *parent, itk::DataObject* obj,
        otb::ImageIOBase::IOComponentType type, unsigned int numDims, unsigned int numBands)
{
    this->mDataObject = obj;
    this->setItkComponentType(type);
    this->setNumDimensions(numDims);
    this->setNumBands(numBands);
    this->mIsRGBImage = false;
    this->mbIsStreaming = false;
    this->mItkProcess = nullptr;
}

NMItkDataObjectWrapper::NMItkDataObjectWrapper(
        const NMItkDataObjectWrapper& dataObjectWrapper)
{
    NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dataObjectWrapper);
    this->mDataObject = w->getDataObject();
    this->mOTBTab = w->getOTBTab();
    this->mNMComponentType = w->getNMComponentType();
    this->mNumDimensions = w->getNumDimensions();
    this->mNumBands = w->getNumBands();
    this->mIsRGBImage = w->getIsRGBImage();
    this->mbIsStreaming = w->getIsStreaming();
    this->mItkProcess = nullptr;

    if (mbIsStreaming)
    {
        this->setupBufferFilter();
    }
}

NMItkDataObjectWrapper::~NMItkDataObjectWrapper()
{
}

NMItkDataObjectWrapper& NMItkDataObjectWrapper::operator=(const NMItkDataObjectWrapper& dw)
{
    NMItkDataObjectWrapper* w = const_cast<NMItkDataObjectWrapper*>(&dw);
    this->mDataObject = w->getDataObject();
    this->mOTBTab = w->getOTBTab();
    this->mNMComponentType = w->getNMComponentType();
    this->mNumDimensions = w->getNumDimensions();
    this->mNumBands = w->getNumBands();
    this->mIsRGBImage = w->getIsRGBImage();
    this->mbIsStreaming = w->getIsStreaming();
    this->setupBufferFilter();

    if (mbIsStreaming)
    {
        this->setupBufferFilter();
    }

    return *this;
}

void
NMItkDataObjectWrapper::setIsStreaming(bool stream)
{
    this->mbIsStreaming = stream;
    if (this->mbIsStreaming)
    {
        this->setupBufferFilter();
    }
}

void
NMItkDataObjectWrapper::setupBufferFilter()
{
    if (mDataObject.IsNotNull())
    {
        this->createBufferFilterInstance();
        this->setBufferFilterInput();
        this->mDataObject = this->getBufferFilterOutput();
    }
}

void
NMItkDataObjectWrapper::createBufferFilterInstance()
{
    if (mDataObject.IsNull())
    {
        NMErr("NMItkDataObjectWrapper::createBufferFilterInstance()",
               << "itk::DataObject is NULL");
        return;
    }

    switch(this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWCreateBufferFilterInstance )
        default:
            break;
    }
}

void
NMItkDataObjectWrapper::setBufferFilterInput()
{
    if (mDataObject.IsNull())
    {
        NMErr("NMItkDataObjectWrapper::setBufferFilterInput()",
               << "itk::DataObject is NULL");
        return;
    }

    itk::DataObject* dataObj = mDataObject.GetPointer();
    unsigned int idx = 0;

    switch(this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWSetNthBufferFilterInput )
        default:
            break;
    }
}

itk::DataObject*
NMItkDataObjectWrapper::getBufferFilterOutput()
{
    itk::DataObject* dbj = nullptr;

    if (mDataObject.IsNull())
    {
        NMErr("NMItkDataObjectWrapper::getBufferFilterOutput()",
               << "itk::DataObject is NULL");
        return dbj;
    }

    itk::DataObject* bufout = nullptr;
    unsigned int idx = 0;

    switch(this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWGetBufferFilterOutput )
        default:
            break;
    }

    return bufout;
}

itk::DataObject*
NMItkDataObjectWrapper::getDataObject()
{
    return mDataObject;
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

    switch (this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWSetRegion )
        default:
            break;
    }
}

void
NMItkDataObjectWrapper::setImageOrigin(const double* origin)
{
    if (mDataObject.IsNull())
    {
        NMWarn("NMItkDataObjectWrapper::setImageOrigin()",
               << "itk::DataObject is NULL");
        return;
    }

    switch (this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWSetOrigin )
        default:
            break;
    }
}

void NMItkDataObjectWrapper::getSignedImageSpacing(std::array<double, 3>& spacing)
{
    if (mDataObject.IsNull())
    {
        NMWarn("NMItkDataObjectWrapper::getImageSpacing()",
               << "itk::DataObject is NULL");
        return;
    }

    switch (this->getItkComponentType())
    {
        LocalMacroPerSingleType( DWGetSignedSpacing )
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
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
        case NM_ULONGLONG: type = otb::ImageIOBase::ULONGLONG; break;
        case NM_LONGLONG: type = otb::ImageIOBase::LONGLONG; break;
#else
        case NM_ULONGLONG: type = otb::ImageIOBase::ULONG; break;
        case NM_LONGLONG: type = otb::ImageIOBase::LONG; break;
#endif
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
        case otb::ImageIOBase::UCHAR	 : this->mNMComponentType = NM_UCHAR	; break;
        case otb::ImageIOBase::CHAR		 : this->mNMComponentType = NM_CHAR	; break;
        case otb::ImageIOBase::USHORT	 : this->mNMComponentType = NM_USHORT; break;
        case otb::ImageIOBase::SHORT	 : this->mNMComponentType = NM_SHORT	; break;
        case otb::ImageIOBase::UINT		 : this->mNMComponentType = NM_UINT	; break;
        case otb::ImageIOBase::INT       : this->mNMComponentType =	NM_INT	; break;
        case otb::ImageIOBase::ULONG     : this->mNMComponentType =	NM_ULONG; break;
        case otb::ImageIOBase::LONG      : this->mNMComponentType =	NM_LONG	; break;
#if defined(_WIN32) && SIZEOF_LONGLONG >= 8
        case otb::ImageIOBase::ULONGLONG : this->mNMComponentType =	NM_ULONGLONG;   break;
        case otb::ImageIOBase::LONGLONG  : this->mNMComponentType =	NM_LONGLONG	;   break;
#endif
        case otb::ImageIOBase::FLOAT     : this->mNMComponentType =	NM_FLOAT; break;
        case otb::ImageIOBase::DOUBLE    : this->mNMComponentType =	NM_DOUBLE;	break;
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
    case NM_ULONGLONG: ret = "ulonglong"; break;
    case NM_LONGLONG: ret = "longlong"; break;
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
    else if (compType.compare("ulonglong", Qt::CaseInsensitive) == 0)
        type = NMItkDataObjectWrapper::NM_ULONGLONG;
    else if (compType.compare("longlong", Qt::CaseInsensitive) == 0)
        type = NMItkDataObjectWrapper::NM_LONGLONG;
    else if (compType.compare("float", Qt::CaseInsensitive) == 0)
        type = NMItkDataObjectWrapper::NM_FLOAT;
    else if (compType.compare("ushort", Qt::CaseInsensitive) == 0)
        type = NMItkDataObjectWrapper::NM_FLOAT;
    else if (compType.compare("double", Qt::CaseInsensitive) == 0)
        type = NMItkDataObjectWrapper::NM_DOUBLE;

    return type;
}

template class NMItkDataObjectWrapper_Internal<unsigned char, 1>;
template class NMItkDataObjectWrapper_Internal<char, 1>;
template class NMItkDataObjectWrapper_Internal<unsigned short, 1>;
template class NMItkDataObjectWrapper_Internal<short, 1>;
template class NMItkDataObjectWrapper_Internal<unsigned int, 1>;
template class NMItkDataObjectWrapper_Internal<int, 1>;
template class NMItkDataObjectWrapper_Internal<unsigned long, 1>;
template class NMItkDataObjectWrapper_Internal<long, 1>;
template class NMItkDataObjectWrapper_Internal<unsigned long long, 1>;
template class NMItkDataObjectWrapper_Internal<long long, 1>;
template class NMItkDataObjectWrapper_Internal<float, 1>;
template class NMItkDataObjectWrapper_Internal<double, 1>;
template class NMItkDataObjectWrapper_Internal<unsigned char, 2>;
template class NMItkDataObjectWrapper_Internal<char, 2>;
template class NMItkDataObjectWrapper_Internal<unsigned short, 2>;
template class NMItkDataObjectWrapper_Internal<short, 2>;
template class NMItkDataObjectWrapper_Internal<unsigned int, 2>;
template class NMItkDataObjectWrapper_Internal<int, 2>;
template class NMItkDataObjectWrapper_Internal<unsigned long, 2>;
template class NMItkDataObjectWrapper_Internal<long, 2>;
template class NMItkDataObjectWrapper_Internal<unsigned long long, 2>;
template class NMItkDataObjectWrapper_Internal<long long, 2>;
template class NMItkDataObjectWrapper_Internal<float, 2>;
template class NMItkDataObjectWrapper_Internal<double, 2>;
template class NMItkDataObjectWrapper_Internal<unsigned char, 3>;
template class NMItkDataObjectWrapper_Internal<char, 3>;
template class NMItkDataObjectWrapper_Internal<unsigned short, 3>;
template class NMItkDataObjectWrapper_Internal<short, 3>;
template class NMItkDataObjectWrapper_Internal<unsigned int, 3>;
template class NMItkDataObjectWrapper_Internal<int, 3>;
template class NMItkDataObjectWrapper_Internal<unsigned long, 3>;
template class NMItkDataObjectWrapper_Internal<long, 3>;
template class NMItkDataObjectWrapper_Internal<unsigned long long, 3>;
template class NMItkDataObjectWrapper_Internal<long long, 3>;
template class NMItkDataObjectWrapper_Internal<float, 3>;
template class NMItkDataObjectWrapper_Internal<double, 3>;

