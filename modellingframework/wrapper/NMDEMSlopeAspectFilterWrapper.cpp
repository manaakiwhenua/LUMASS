/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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
 *  NMDEMSlopeAspectFilterWrapper.cpp
 *
 *  Created on: 2019-03-22
 *      Author: Alexander Herzig
 */

#include "NMDEMSlopeAspectFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"

#include "otbDEMSlopeAspectFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, unsigned int Dimension>
class NMDEMSlopeAspectFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TInputImage, Dimension> OutImgType;
    typedef typename otb::DEMSlopeAspectFilter<InImgType, OutImgType>      FilterType;
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
        //InImgType* img = dynamic_cast<InImgType*>(dataObj);
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        filter->SetNthInput(idx, dataObj);
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
        NMDebugCtx("NMDEMSlopeAspectFilterWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMDEMSlopeAspectFilterWrapper* p =
                dynamic_cast<NMDEMSlopeAspectFilterWrapper*>(proc);

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


        QVariant curTerrainAttributeVar = p->getParameter("TerrainAttributeType");
        std::string curTerrainAttribute;
        if (curTerrainAttributeVar.isValid())
        {
            curTerrainAttribute = curTerrainAttributeVar.toString().toStdString();
            f->SetTerrainAttribute(curTerrainAttribute);
            QString provN = QString("nm:TerrainAttributeType=\"%1\"").arg(curTerrainAttribute.c_str());
            p->addRunTimeParaProvN(provN);
        }

        QVariant curTerrainAlgorithmVar = p->getParameter("TerrainAlgorithmType");
        std::string curTerrainAlgorithm;
        if (curTerrainAlgorithmVar.isValid())
        {
            curTerrainAlgorithm = curTerrainAlgorithmVar.toString().toStdString();
            f->SetTerrainAlgorithm(curTerrainAlgorithm);
            QString provN = QString("nm:TerrainAlgorithmType=\"%1\"").arg(curTerrainAlgorithm.c_str());
            p->addRunTimeParaProvN(provN);
        }

        QVariant curAttributeUnitVar = p->getParameter("AttributeUnitType");
        std::string curAttributeUnit;
        if (curAttributeUnitVar.isValid())
        {
            curAttributeUnit = curAttributeUnitVar.toString().toStdString();
            f->SetAttributeUnit(curAttributeUnit);
            QString provN = QString("nm:AttributeUnitType=\"%1\"").arg(curAttributeUnit.c_str());
            p->addRunTimeParaProvN(provN);
        }

        QVariant curNodataVar = p->getParameter("Nodata");
        double curNodata;
        if (curNodataVar.isValid())
        {
            curNodata = curNodataVar.toDouble(&bok);
            if (bok)
            {
                f->SetNodata((curNodata));
                QString provN = QString("nm:Nodata=\"%1\"").arg(curNodata);
                p->addRunTimeParaProvN(provN);
            }
            else
            {
                NMErr("NMDEMSlopeAspectFilterWrapper_Internal", << "Invalid value for 'Nodata'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setDescription("Invalid value for 'Nodata'!");
                throw e;
            }
        }


        step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());
        std::vector<std::string> userIDs;
        QStringList currentInputs;
        QStringList userInputIDProvVal;
        if (step < p->mInputComponents.size())
        {
            currentInputs = p->mInputComponents.at(step);
            int cnt=0;
            foreach (const QString& input, currentInputs)
            {
                std::stringstream uid;
                uid << "L" << cnt;
                QString inputCompName = p->getModelController()->getComponentNameFromInputSpec(input);
                NMModelComponent* comp = p->getModelController()->getComponent(inputCompName);
                if (comp != 0)
                {
                    if (comp->getUserID().isEmpty())
                    {
                        userIDs.push_back(uid.str());
                    }
                    else
                    {
                        userIDs.push_back(comp->getUserID().toStdString());
                    }
                }
                else
                {
                    userIDs.push_back(uid.str());
                }
                userInputIDProvVal << uid.str().c_str();
                ++cnt;
            }
        }
        f->SetInputNames(userIDs);
        QString inputIDsProvN = QString("nm:InputUserIDs=\"%1\"").arg(userInputIDProvVal.join(' '));
        p->addRunTimeParaProvN(inputIDsProvN);

        NMDebugCtx("NMDEMSlopeAspectFilterWrapper_Internal", << "done!");
    }
};

/*$<HelperClassInstantiation>$*/

InstantiateInputTypeObjectWrap( NMDEMSlopeAspectFilterWrapper, NMDEMSlopeAspectFilterWrapper_Internal )
SetInputTypeNthInputWrap( NMDEMSlopeAspectFilterWrapper, NMDEMSlopeAspectFilterWrapper_Internal )
GetInputTypeOutputWrap( NMDEMSlopeAspectFilterWrapper, NMDEMSlopeAspectFilterWrapper_Internal )
LinkInputTypeInternalParametersWrap( NMDEMSlopeAspectFilterWrapper, NMDEMSlopeAspectFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMDEMSlopeAspectFilterWrapper
::NMDEMSlopeAspectFilterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMDEMSlopeAspectFilterWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;

    this->mAttributeUnitEnum.clear();
    this->mAttributeUnitEnum << "Dim.less" << "Degree" << "Percent" << "Aspect";
    this->mAttributeUnitType = QString(tr("Degree"));

    this->mTerrainAttributeEnum.clear();
    this->mTerrainAttributeEnum << "Slope" << "LS" << "Wetness" << "SedTransport";
    this->mTerrainAttributeType = QString(tr("Slope"));

    this->mTerrainAlgorithmEnum.clear();
    this->mTerrainAlgorithmEnum << "Horn" << "Zevenbergen";
    this->mTerrainAlgorithmType = QString(tr("Zevenbergen"));

    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 2;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("PixelType"));
    mUserProperties.insert(QStringLiteral("TerrainAttributeType"), QStringLiteral("Attribute"));
    mUserProperties.insert(QStringLiteral("AttributeUnitType"), QStringLiteral("Unit"));
    mUserProperties.insert(QStringLiteral("TerrainAlgorithmType"), QStringLiteral("Algorithm"));
    mUserProperties.insert(QStringLiteral("Nodata"), QStringLiteral("NodataValue"));

}

NMDEMSlopeAspectFilterWrapper
::~NMDEMSlopeAspectFilterWrapper()
{
}
