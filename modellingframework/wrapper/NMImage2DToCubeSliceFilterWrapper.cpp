/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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
 *  NMImage2DToCubeSliceFilterWrapper.cpp
 *
 *  Created on: 2020-12-05
 *      Author: Alexander Herzig
 */

#include "NMImage2DToCubeSliceFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbImage2DToCubeSliceFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMImage2DToCubeSliceFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, 2>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::Image2DToCubeSliceFilter<InImgType, OutImgType>      FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands)
    {
        FilterTypePointer f = FilterType::New();
        otbFilter = f;
    }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, const QString& name)
    {
        InImgType* img = dynamic_cast<InImgType*>(dataObj);
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        filter->SetInput(idx, img);
    }


    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, unsigned int idx)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
    }

/*$<InternalRATGetSupport>$*/

/*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
            unsigned int numBands, NMProcess* proc,
            unsigned int step, const QMap<QString, NMModelComponent*>& repo)
    {
        NMDebugCtx("NMImage2DToCubeSliceFilterWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMImage2DToCubeSliceFilterWrapper* p =
                dynamic_cast<NMImage2DToCubeSliceFilterWrapper*>(proc);

        // make sure we've got a valid filter object
        if (f == 0)
        {
            NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
                        e.setDescription("We're trying to link, but the filter doesn't seem to be initialised properly!");
            throw e;
            return;
        }

        /* do something reasonable here */
        bool bok;
        int givenStep = step;


        QVariant curDimMappingVar = p->getParameter("DimMapping");
        if (curDimMappingVar.isValid())
        {
           std::vector<int> vecDimMapping;
           QStringList curValVarList = curDimMappingVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                int curDimMapping = vStr.toInt(&bok);
                if (bok)
                {
                    vecDimMapping.push_back(static_cast<int>(curDimMapping));
                }
                else
                {
                    NMErr("NMImage2DToCubeSliceFilterWrapper_Internal", << "Invalid value for 'DimMapping'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'DimMapping'!");
                    throw e;
                }
            }
            f->SetDimMapping(vecDimMapping);
        }

        QVariant curOutputOriginVar = p->getParameter("OutputOrigin");
        if (curOutputOriginVar.isValid())
        {
           std::vector<double> vecOutputOrigin;
           QStringList curValVarList = curOutputOriginVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                double curOutputOrigin = vStr.toDouble(&bok);
                if (bok)
                {
                    vecOutputOrigin.push_back(static_cast<double>(curOutputOrigin));
                }
                else
                {
                    NMErr("NMImage2DToCubeSliceFilterWrapper_Internal", << "Invalid value for 'OutputOrigin'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'OutputOrigin'!");
                    throw e;
                }
            }
            f->SetOutputOrigin(vecOutputOrigin);
        }

        QVariant curOutputSpacingVar = p->getParameter("OutputSpacing");
        if (curOutputSpacingVar.isValid())
        {
           std::vector<double> vecOutputSpacing;
           QStringList curValVarList = curOutputSpacingVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                double curOutputSpacing = vStr.toDouble(&bok);
                if (bok)
                {
                    vecOutputSpacing.push_back(static_cast<double>(curOutputSpacing));
                }
                else
                {
                    NMErr("NMImage2DToCubeSliceFilterWrapper_Internal", << "Invalid value for 'OutputSpacing'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'OutputSpacing'!");
                    throw e;
                }
            }
            f->SetOutputSpacing(vecOutputSpacing);
        }

        QVariant curOutputSizeVar = p->getParameter("OutputSize");
        if (curOutputSizeVar.isValid())
        {
           std::vector<long long> vecOutputSize;
           QStringList curValVarList = curOutputSizeVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                long long curOutputSize = vStr.toLongLong(&bok);
                if (bok)
                {
                    vecOutputSize.push_back(static_cast<long long>(curOutputSize));
                }
                else
                {
                    NMErr("NMImage2DToCubeSliceFilterWrapper_Internal", << "Invalid value for 'OutputSize'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'OutputSize'!");
                    throw e;
                }
            }
            f->SetOutputSize(vecOutputSize);
        }

        QVariant curOutputIndexVar = p->getParameter("OutputIndex");
        if (curOutputIndexVar.isValid())
        {
           std::vector<long long> vecOutputIndex;
           QStringList curValVarList = curOutputIndexVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                long long curOutputIndex = vStr.toLongLong(&bok);
                if (bok)
                {
                    vecOutputIndex.push_back(static_cast<long long>(curOutputIndex));
                }
                else
                {
                    NMErr("NMImage2DToCubeSliceFilterWrapper_Internal", << "Invalid value for 'OutputIndex'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'OutputIndex'!");
                    throw e;
                }
            }
            f->SetOutputIndex(vecOutputIndex);
        }


                /*$<ForwardInputUserIDs_Body>$*/


        NMDebugCtx("NMImage2DToCubeSliceFilterWrapper_Internal", << "done!");
    }
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMImage2DToCubeSliceFilterWrapper, NMImage2DToCubeSliceFilterWrapper_Internal )
SetNthInputWrap( NMImage2DToCubeSliceFilterWrapper, NMImage2DToCubeSliceFilterWrapper_Internal )
GetOutputWrap( NMImage2DToCubeSliceFilterWrapper, NMImage2DToCubeSliceFilterWrapper_Internal )
LinkInternalParametersWrap( NMImage2DToCubeSliceFilterWrapper, NMImage2DToCubeSliceFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMImage2DToCubeSliceFilterWrapper
::NMImage2DToCubeSliceFilterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMImage2DToCubeSliceFilterWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 3;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("DimMapping"), QStringLiteral("DimMapping"));
    mUserProperties.insert(QStringLiteral("OutputOrigin"), QStringLiteral("OutputOrigin"));
    mUserProperties.insert(QStringLiteral("OutputSpacing"), QStringLiteral("OutputSpacing"));
    mUserProperties.insert(QStringLiteral("OutputSize"), QStringLiteral("OutputSize"));
    mUserProperties.insert(QStringLiteral("OutputIndex"), QStringLiteral("OutputIndex"));
}

NMImage2DToCubeSliceFilterWrapper
::~NMImage2DToCubeSliceFilterWrapper()
{
}
