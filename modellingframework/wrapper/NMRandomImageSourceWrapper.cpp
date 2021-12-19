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
 * NMRandomImageSourceWrapper.cpp
 *
 *  Created on: 19/08/2012
 *      Author: alex
 */

#include "itkRandomImageSource.h"
#include "NMRandomImageSourceWrapper.h"

#include <vector>
#include <cstdlib>
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImage.h"
#include "NMItkDataObjectWrapper.h"
#include "NMMfwException.h"

#include <QVariant>


/** Helper Class */
template <class outputType, unsigned int Dimension>
class NMRandomImageSourceWrapper_Internal
{
public:
    typedef otb::Image<outputType, Dimension> ImgType;
    typedef itk::RandomImageSource<ImgType> FilterType;
    typedef typename FilterType::Pointer FilterTypePointer;
    typedef typename FilterType::PointType FilterPointType;
    typedef typename FilterType::SizeType FilterSizeType;
    typedef typename FilterType::SpacingType FilterSpacingType;


    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands)
        {
            FilterTypePointer f = FilterType::New();
            otbFilter = f;
        }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            return dynamic_cast<ImgType*>(filter->GetOutput(idx));
        }

    static void setSize(itk::ProcessObject::Pointer& otbFilter,
            std::vector<double>& insize)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            FilterSizeType size;
            for (unsigned int u=0; u < insize.size(); ++u)
                size[u] = insize[u];

            filter->SetSize(size);
        }

    static void setSpacing(itk::ProcessObject::Pointer& otbFilter,
            std::vector<double>& inspacing)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            FilterSpacingType spacing;
            for (unsigned int u=0; u < inspacing.size(); ++u)
                spacing[u] = inspacing[u];

            filter->SetSpacing(spacing);
        }

    static void setOrigin(itk::ProcessObject::Pointer& otbFilter,
            std::vector<double>& inorigin)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            FilterPointType origin;
            for (unsigned int u=0; u < inorigin.size(); ++u)
                origin[u] = inorigin[u];

            filter->SetOrigin(origin);
        }

    static void setMax(itk::ProcessObject::Pointer& otbFilter, double inmax)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetMax((outputType)inmax);
        }

    static void setMin(itk::ProcessObject::Pointer& otbFilter, double inmin)
        {
            FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
            filter->SetMin((outputType)inmin);
        }
};

InstantiateOutputTypeObjectWrap( NMRandomImageSourceWrapper, NMRandomImageSourceWrapper_Internal )
GetOutputTypeOutputWrap( NMRandomImageSourceWrapper, NMRandomImageSourceWrapper_Internal )

#define callInternalSetSize( outputType, wrapName )					\
if (this->mOutputNumDimensions == 1)                                \
    wrapName< outputType, 1>::setSize(this->mOtbProcess, insize);   \
else if (this->mOutputNumDimensions == 2)                           \
    wrapName< outputType, 2>::setSize(this->mOtbProcess, insize);   \
else if (this->mOutputNumDimensions == 3)                           \
    wrapName< outputType, 3>::setSize(this->mOtbProcess, insize);

#define callInternalSetSpacing( outputType, wrapName )					\
if (this->mOutputNumDimensions == 1)                                \
    wrapName< outputType, 1>::setSpacing(this->mOtbProcess, inspacing);   \
else if (this->mOutputNumDimensions == 2)                           \
    wrapName< outputType, 2>::setSpacing(this->mOtbProcess, inspacing);   \
else if (this->mOutputNumDimensions == 3)                           \
    wrapName< outputType, 3>::setSpacing(this->mOtbProcess, inspacing);

#define callInternalSetOrigin( outputType, wrapName )					\
if (this->mOutputNumDimensions == 1)                                \
    wrapName< outputType, 1>::setOrigin(this->mOtbProcess, inorigin);   \
else if (this->mOutputNumDimensions == 2)                           \
    wrapName< outputType, 2>::setOrigin(this->mOtbProcess, inorigin);   \
else if (this->mOutputNumDimensions == 3)                           \
    wrapName< outputType, 3>::setOrigin(this->mOtbProcess, inorigin);

#define callInternalSetMax( outputType, wrapName )					\
if (this->mOutputNumDimensions == 1)                                \
    wrapName< outputType, 1>::setMax(this->mOtbProcess, inmax);   \
else if (this->mOutputNumDimensions == 2)                           \
    wrapName< outputType, 2>::setMax(this->mOtbProcess, inmax);   \
else if (this->mOutputNumDimensions == 3)                           \
    wrapName< outputType, 3>::setMax(this->mOtbProcess, inmax);

#define callInternalSetMin( outputType, wrapName )					\
if (this->mOutputNumDimensions == 1)                                \
    wrapName< outputType, 1>::setMin(this->mOtbProcess, inmin);   \
else if (this->mOutputNumDimensions == 2)                           \
    wrapName< outputType, 2>::setMin(this->mOtbProcess, inmin);   \
else if (this->mOutputNumDimensions == 3)                           \
    wrapName< outputType, 3>::setMin(this->mOtbProcess, inmin);


NMRandomImageSourceWrapper::NMRandomImageSourceWrapper(QObject* parent)
{
    this->setParent(parent);
    this->ctx = "NMRandomImageSourceWrapper";
    this->setObjectName("NMRandomImageSourceWrapper");
    this->mbIsInitialised = false;
    this->mInputComponentType = otb::ImageIOBase::FLOAT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 2;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mParamPos = 0;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("ImageSize"), QStringLiteral("Size"));
    mUserProperties.insert(QStringLiteral("ImageSpacing"), QStringLiteral("Spacing"));
    mUserProperties.insert(QStringLiteral("ImageOrigin"), QStringLiteral("Origin"));
    mUserProperties.insert(QStringLiteral("MaxValue"), QStringLiteral("MaxValue"));
    mUserProperties.insert(QStringLiteral("MinValue"), QStringLiteral("MinValue"));
}

NMRandomImageSourceWrapper::~NMRandomImageSourceWrapper()
{
}

void NMRandomImageSourceWrapper::extractNumericVector(unsigned int step,
        QList<QStringList> instring, std::vector<double>& outvec)
{
    if (instring.size() == 0)
        return;

    outvec.clear();

    //if (step < 0 || step >= instring.size())
    //	step = 0;
    step = this->mapHostIndexToPolicyIndex(step, instring.size());

    QStringList vallist = instring.at(step);
    if (vallist.size() != this->mOutputNumDimensions)
        return;

    bool bok;
    for (unsigned int i=0; i < this->mOutputNumDimensions; ++i)
    {
        double val = vallist.at(i).toDouble(&bok);
        if (bok)
            outvec.push_back(val);
    }

    if (outvec.size() != this->mOutputNumDimensions)
        outvec.clear();
}

bool NMRandomImageSourceWrapper::extractNumericValue(unsigned int step,
        QStringList instring, double *outval)
{
    if (instring.size() == 0)
        return false;

    //if (step < 0 || step >= instring.size())
    //	step = 0;
    step = this->mapHostIndexToPolicyIndex(step, instring.size());

    bool bok;
    double val = instring.at(step).toDouble(&bok);
    if (bok)
        *outval = val;

    return bok;
}

void NMRandomImageSourceWrapper::linkParameters(
        unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");

    bool bok;

    QStringList vecparams;
    vecparams << "ImageSize" << "ImageSpacing" << "ImageOrigin";

    for (int p=0; p < 3; ++p)
    {
        QVariant valVecVar = this->getParameter(vecparams.at(p));
        if (valVecVar.isValid())
        {
            QStringList strValVec = valVecVar.toStringList();
            std::vector<double> vals;
            for (int v=0; v < strValVec.size(); ++v)
            {
                double val = strValVec.at(v).toDouble(&bok);
                if (bok)
                {
                    vals.push_back(val);
                }
                else
                {
                    NMLogError(<< "NMRandomImageSourceWrapper: " << "Invalid value for '"
                               << vecparams.at(p) << "'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setSource(this->objectName().toStdString());
                    QString descr = QString("Invalid value for '%1'").arg(vecparams.at(p));
                    e.setDescription(descr.toStdString());
                    throw e;
                }

                if (vals.size() == strValVec.size())
                {
                    this->setVecParam(vecparams.at(p), vals);
                    QString provN = QString("nm:%1=\"%2\"").arg(vecparams.at(p)).arg(strValVec.join(' '));
                    this->addRunTimeParaProvN(provN);
                }
            }
        }
    }


    QStringList singleparams;
    singleparams << "MaxValue" << "MinValue";

    for (int k=0; k < singleparams.size(); ++k)
    {
        QVariant svalVar = this->getParameter(singleparams.at(k));
        if (svalVar.isValid())
        {
            QString svalStr = svalVar.toString();
            double val = svalStr.toDouble(&bok);
            if (!bok)
            {
                NMLogError(<< "NMRandomImageSourceWrapper: " << "Invalid value for '"
                           << singleparams.at(k) << "'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(this->objectName().toStdString());
                QString descr = QString("Invalid value for '%1'").arg(singleparams.at(k));
                e.setDescription(descr.toStdString());
                throw e;
            }

            this->setParam(singleparams.at(k), val);
            QString singleProvN = QString("nm:%1=\"%2\"").arg(singleparams.at(k)).arg(svalStr);
            this->addRunTimeParaProvN(singleProvN);
        }
    }

    NMDebugCtx(ctx, << "done!");
}

void NMRandomImageSourceWrapper::setParam(QString param, double val)
{
    if(!this->mbIsInitialised)
        return;

    if (param.compare("MaxValue") == 0)
    {
        double& inmax = val;
        switch (this->mOutputComponentType)
        {
        MacroPerType( callInternalSetMax, NMRandomImageSourceWrapper_Internal)
        default:
            break;
        }
    }
    else if (param.compare("MinValue") == 0)
    {
        double& inmin = val;
        switch (this->mOutputComponentType)
        {
        MacroPerType( callInternalSetMin, NMRandomImageSourceWrapper_Internal)
        default:
            break;
        }
    }
}

void NMRandomImageSourceWrapper::setVecParam(QString param, std::vector<double>& vec)
{
    if(!this->mbIsInitialised)
        return;

    if (param.compare("ImageSize") == 0)
    {
        std::vector<double>& insize = vec;
        switch (this->mOutputComponentType)
        {
        MacroPerType( callInternalSetSize, NMRandomImageSourceWrapper_Internal)
        default:
            break;
        }
    }
    else if (param.compare("ImageSpacing") == 0)
    {
        std::vector<double>& inspacing = vec;
        switch (this->mOutputComponentType)
        {
        MacroPerType( callInternalSetSpacing, NMRandomImageSourceWrapper_Internal)
        default:
            break;
        }
    }
    else if (param.compare("ImageOrigin") == 0)
    {
        std::vector<double>& inorigin = vec;
        switch (this->mOutputComponentType)
        {
        MacroPerType( callInternalSetOrigin, NMRandomImageSourceWrapper_Internal)
        default:
            break;
        }
    }
}











