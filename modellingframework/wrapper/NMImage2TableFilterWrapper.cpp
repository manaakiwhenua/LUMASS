/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2021 Landcare Research New Zealand Ltd
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
 *  NMImage2TableFilterWrapper.cpp
 *
 *  Created on: 2020-12-05
 *      Author: Alexander Herzig
 */

#include "NMImage2TableFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbImage2TableFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMImage2TableFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef InImgType OutImgType;
    typedef typename otb::Image2TableFilter<InImgType>      FilterType;
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
                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
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

    static otb::AttributeTable::Pointer getRAT(
        itk::ProcessObject::Pointer& procObj,
        unsigned int numBands, unsigned int idx)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->getRAT(idx);
    }


    static void setRAT(
        itk::ProcessObject::Pointer& procObj,
        unsigned int numBands, unsigned int idx,
        otb::AttributeTable::Pointer& rat)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->setRAT(idx, rat);
    }

    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
                        unsigned int numBands, NMProcess* proc,
                        unsigned int step, const QMap<QString, NMModelComponent*>& repo)
        {
                NMDebugCtx("NMImage2TableFilterWrapper_Internal", << "...");

                FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
                NMImage2TableFilterWrapper* p =
                                dynamic_cast<NMImage2TableFilterWrapper*>(proc);

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


        QVariant curTableFileNameVar = p->getParameter("TableFileName");
        std::string curTableFileName;
        if (curTableFileNameVar.isValid())
        {
            curTableFileName = curTableFileNameVar.toString().toStdString();
            f->SetTableFileName(curTableFileName);
        }

        QVariant curTableNameVar = p->getParameter("TableName");
        std::string curTableName;
        if (curTableNameVar.isValid())
        {
            curTableName = curTableNameVar.toString().toStdString();
            f->SetTableName(curTableName);
        }

        QVariant curImageVarNameVar = p->getParameter("ImageVarName");
        std::string curImageVarName;
        if (curImageVarNameVar.isValid())
        {
            curImageVarName = curImageVarNameVar.toString().toStdString();
            f->SetImageVarName(curImageVarName);
        }

        QVariant curUpdateModeVar = p->getParameter("UpdateMode");
        int curUpdateMode;
        if (curUpdateModeVar.isValid())
        {
            curUpdateMode = curUpdateModeVar.toInt(&bok);
            if (bok)
            {
                f->SetUpdateMode(curUpdateMode);
            }
            else
            {
                f->SetUpdateMode(0);
                //NMErr("NMImage2TableFilterWrapper_Internal", << "Invalid value for 'UpdateMode'!");
                //NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                //e.setDescription("Invalid value for 'UpdateMode'!");
                //throw e;
            }
        }

        QVariant curNcImageContainerVar = p->getParameter("NcImageContainer");
        std::string curNcImageContainer;
        if (curNcImageContainerVar.isValid())
        {
            curNcImageContainer = curNcImageContainerVar.toString().toStdString();
            f->SetNcImageContainer(curNcImageContainer);
        }

        QVariant curNcGroupNameVar = p->getParameter("NcGroupName");
        std::string curNcGroupName;
        if (curNcGroupNameVar.isValid())
        {
            curNcGroupName = curNcGroupNameVar.toString().toStdString();
            f->SetNcGroupName(curNcGroupName);
        }

        QVariant curDimVarNamesVar = p->getParameter("DimVarNames");
        if (curDimVarNamesVar.isValid())
        {
           std::vector<string> vecDimVarNames;
           QStringList curValVarList = curDimVarNamesVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                std::string curDimVarNames = vStr.toStdString();
            vecDimVarNames.push_back(curDimVarNames);
            }
            f->SetDimVarNames(vecDimVarNames);
        }

        QVariant curAuxVarNamesVar = p->getParameter("AuxVarNames");
        if (curAuxVarNamesVar.isValid())
        {
           std::vector<string> vecAuxVarNames;
           QStringList curValVarList = curAuxVarNamesVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                std::string curAuxVarNames = vStr.toStdString();
            vecAuxVarNames.push_back(curAuxVarNames);
            }
            f->SetAuxVarNames(vecAuxVarNames);
        }


        QVariant curStartIndexVar = p->getParameter("StartIndex");
        if (curStartIndexVar.isValid())
        {
           std::vector<int> vecStartIndex;
           QStringList curValVarList = curStartIndexVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
                int curStartIndex = vStr.toInt(&bok);
                if (bok)
                {
                    vecStartIndex.push_back(static_cast<int>(curStartIndex));
                }
                else
                {
                    NMErr("NMImage2TableFilterWrapper_Internal", << "Invalid value for 'StartIndex'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'StartIndex'!");
                    throw e;
                }
            }
            f->SetStartIndex(vecStartIndex);
        }

        QVariant curSizeVar = p->getParameter("Size");
        if (curSizeVar.isValid())
        {
           std::vector<int> vecSize;
           QStringList curValVarList = curSizeVar.toStringList();
           foreach(const QString& vStr, curValVarList)
           {
               int curSize = vStr.toInt(&bok);
               if (bok)
               {
                   vecSize.push_back(static_cast<int>(curSize));
               }
               else
               {
                   NMErr("NMImage2TableFilterWrapper_Internal", << "Invalid value for 'Size'!");
                   NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                   e.setDescription("Invalid value for 'Size'!");
                   throw e;
               }
           }
           f->SetSize(vecSize);
        }

        /*$<ForwardInputUserIDs_Body>$*/


                NMDebugCtx("NMImage2TableFilterWrapper_Internal", << "done!");
        }
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )
SetNthInputWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )
GetOutputWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )
LinkInternalParametersWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )
GetRATWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )
SetRATWrap( NMImage2TableFilterWrapper, NMImage2TableFilterWrapper_Internal )

NMImage2TableFilterWrapper
::NMImage2TableFilterWrapper(QObject* parent)
{
        this->setParent(parent);
        this->setObjectName("NMImage2TableFilterWrapper");
        this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMImage2TableFilterWrapper
::~NMImage2TableFilterWrapper()
{
}
