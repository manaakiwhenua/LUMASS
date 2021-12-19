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
 *  NMCubeSliceToImage2DFilterWrapper.cpp
 *
 *  Created on: 2020-12-07
 *      Author: Alex Herzig
 */

#include "NMCubeSliceToImage2DFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbCubeSliceToImage2DFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMCubeSliceToImage2DFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, 2> OutImgType;
    typedef typename otb::CubeSliceToImage2DFilter<InImgType, OutImgType>      FilterType;
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
        NMDebugCtx("NMCubeSliceToImage2DFilterWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMCubeSliceToImage2DFilterWrapper* p =
                dynamic_cast<NMCubeSliceToImage2DFilterWrapper*>(proc);

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
                    NMErr("NMCubeSliceToImage2DFilterWrapper_Internal", << "Invalid value for 'DimMapping'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'DimMapping'!");
                    throw e;
                }
            }
            f->SetDimMapping(vecDimMapping);
        }

        QVariant curInputOriginVar = p->getParameter("InputOrigin");
        if (curInputOriginVar.isValid())
        {
           std::vector<double> vecInputOrigin;
           QStringList curValVarList = curInputOriginVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                double curInputOrigin = vStr.toDouble(&bok);
                if (bok)
                {
                    vecInputOrigin.push_back(static_cast<double>(curInputOrigin));
                }
                else
                {
                    NMErr("NMCubeSliceToImage2DFilterWrapper_Internal", << "Invalid value for 'InputOrigin'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'InputOrigin'!");
                    throw e;
                }
            }
            f->SetInputOrigin(vecInputOrigin);
        }

        QVariant curInputSizeVar = p->getParameter("InputSize");
        if (curInputSizeVar.isValid())
        {
           std::vector<long long> vecInputSize;
           QStringList curValVarList = curInputSizeVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                long long curInputSize = vStr.toLongLong(&bok);
                if (bok)
                {
                    vecInputSize.push_back(static_cast<long long>(curInputSize));
                }
                else
                {
                    NMErr("NMCubeSliceToImage2DFilterWrapper_Internal", << "Invalid value for 'InputSize'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'InputSize'!");
                    throw e;
                }
            }
            f->SetInputSize(vecInputSize);
        }

        QVariant curInputIndexVar = p->getParameter("InputIndex");
        if (curInputIndexVar.isValid())
        {
           std::vector<long long> vecInputIndex;
           QStringList curValVarList = curInputIndexVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                long long curInputIndex = vStr.toLongLong(&bok);
                if (bok)
                {
                    vecInputIndex.push_back(static_cast<long long>(curInputIndex));
                }
                else
                {
                    NMErr("NMCubeSliceToImage2DFilterWrapper_Internal", << "Invalid value for 'InputIndex'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'InputIndex'!");
                    throw e;
                }
            }
            f->SetInputIndex(vecInputIndex);
        }


                /*$<ForwardInputUserIDs_Body>$*/


        NMDebugCtx("NMCubeSliceToImage2DFilterWrapper_Internal", << "done!");
    }
};

/*$<HelperClassInstantiation>$*/
WrapFlexiInstantiation( NMCubeSliceToImage2DFilterWrapper, NMCubeSliceToImage2DFilterWrapper_Internal, callInputDimCreator )
WrapFlexiSetNthInput( NMCubeSliceToImage2DFilterWrapper, NMCubeSliceToImage2DFilterWrapper_Internal, callInputDimSetInput )
WrapFlexiGetOutput( NMCubeSliceToImage2DFilterWrapper, NMCubeSliceToImage2DFilterWrapper_Internal, callInputDimGetOutput )
WrapFlexiLinkInternal( NMCubeSliceToImage2DFilterWrapper, NMCubeSliceToImage2DFilterWrapper_Internal, callInputDimInternalLinkParameters )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMCubeSliceToImage2DFilterWrapper
::NMCubeSliceToImage2DFilterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMCubeSliceToImage2DFilterWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputNumDimensions = 3;
    this->mOutputNumDimensions = 2;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("DimMapping"), QStringLiteral("DimMapping"));
    mUserProperties.insert(QStringLiteral("InputOrigin"), QStringLiteral("InputOrigin"));
    mUserProperties.insert(QStringLiteral("InputSize"), QStringLiteral("InputSize"));
    mUserProperties.insert(QStringLiteral("InputIndex"), QStringLiteral("InputIndex"));

}

NMCubeSliceToImage2DFilterWrapper
::~NMCubeSliceToImage2DFilterWrapper()
{
}
