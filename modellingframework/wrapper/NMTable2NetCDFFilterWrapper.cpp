/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2023 Landcare Research New Zealand Ltd
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
 *  NMTable2NetCDFFilterWrapper.cpp
 *
 *  Created on: 2023-06-22
 *      Author: Alexander Herzig
 */

#include "NMTable2NetCDFFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "nmTable2NetCDFFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMTable2NetCDFFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, 2>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename nm::Table2NetCDFFilter<InImgType, OutImgType>      FilterType;
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
		NMDebugCtx("NMTable2NetCDFFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMTable2NetCDFFilterWrapper* p =
				dynamic_cast<NMTable2NetCDFFilterWrapper*>(proc);

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
                    NMErr("NMTable2NetCDFFilterWrapper_Internal", << "Invalid value for 'DimMapping'!");
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
                    NMErr("NMTable2NetCDFFilterWrapper_Internal", << "Invalid value for 'OutputOrigin'!");
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
                    NMErr("NMTable2NetCDFFilterWrapper_Internal", << "Invalid value for 'OutputSpacing'!");
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
                    NMErr("NMTable2NetCDFFilterWrapper_Internal", << "Invalid value for 'OutputSize'!");
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
                    NMErr("NMTable2NetCDFFilterWrapper_Internal", << "Invalid value for 'OutputIndex'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'OutputIndex'!");
                    throw e;
                }
            }
            f->SetOutputIndex(vecOutputIndex);
        }

        QVariant curInputTableNameVar = p->getParameter("InputTableName");
        std::string curInputTableName;
        if (curInputTableNameVar.isValid())
        {
            curInputTableName = curInputTableNameVar.toString().toStdString();
            f->SetInputTableName(curInputTableName);
        }

        QVariant curSQLWhereClauseVar = p->getParameter("SQLWhereClause");
        std::string curSQLWhereClause;
        if (curSQLWhereClauseVar.isValid())
        {
            curSQLWhereClause = curSQLWhereClauseVar.toString().toStdString();
            f->SetSQLWhereClause(curSQLWhereClause);
        }

        QVariant curImageVarNameVar = p->getParameter("ImageVarName");
        std::string curImageVarName;
        if (curImageVarNameVar.isValid())
        {
            curImageVarName = curImageVarNameVar.toString().toStdString();
            f->SetImageVarName(curImageVarName);
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

        QVariant curVarAndDimDescriptorsVar = p->getParameter("VarAndDimDescriptors");
        if (curVarAndDimDescriptorsVar.isValid())
        {
           std::vector<string> vecVarAndDimDescriptors;
           QStringList curValVarList = curVarAndDimDescriptorsVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                std::string curVarAndDimDescriptors = vStr.toStdString();
                vecVarAndDimDescriptors.push_back(curVarAndDimDescriptors);
           }
           f->SetVarAndDimDescriptors(vecVarAndDimDescriptors);
        }


                /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMTable2NetCDFFilterWrapper_Internal", << "done!");
	}
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMTable2NetCDFFilterWrapper, NMTable2NetCDFFilterWrapper_Internal )
SetNthInputWrap( NMTable2NetCDFFilterWrapper, NMTable2NetCDFFilterWrapper_Internal )
GetOutputWrap( NMTable2NetCDFFilterWrapper, NMTable2NetCDFFilterWrapper_Internal )
LinkInternalParametersWrap( NMTable2NetCDFFilterWrapper, NMTable2NetCDFFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
SetRATWrap( NMTable2NetCDFFilterWrapper, NMTable2NetCDFFilterWrapper_Internal )

NMTable2NetCDFFilterWrapper
::NMTable2NetCDFFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMTable2NetCDFFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputNumDimensions = 2;
    this->mOutputNumDimensions = 3;
    this->mInputComponentType = otb::ImageIOBase::INT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("OutputDimensions"));
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("InputTableName"), QStringLiteral("InputTableName"));
    mUserProperties.insert(QStringLiteral("SQLWhereClause"), QStringLiteral("SQLWhereClause"));
    mUserProperties.insert(QStringLiteral("NcImageContainer"), QStringLiteral("NetCDFFileName"));
    mUserProperties.insert(QStringLiteral("NcGroupName"), QStringLiteral("NcGroupName"));
    mUserProperties.insert(QStringLiteral("ImageVarName"), QStringLiteral("ImageVarName"));
    mUserProperties.insert(QStringLiteral("DimMapping"), QStringLiteral("DimMapping"));
    mUserProperties.insert(QStringLiteral("DimVarNames"), QStringLiteral("DimVariableNames"));
    mUserProperties.insert(QStringLiteral("VarAndDimDescriptors"), QStringLiteral("VarAndDimDescription"));
    mUserProperties.insert(QStringLiteral("OutputOrigin"), QStringLiteral("OutputOrigin"));
    mUserProperties.insert(QStringLiteral("OutputSpacing"), QStringLiteral("OutputSpacing"));
    mUserProperties.insert(QStringLiteral("OutputSize"), QStringLiteral("OutputSize"));
    mUserProperties.insert(QStringLiteral("OutputIndex"), QStringLiteral("OutputIndex"));
}

NMTable2NetCDFFilterWrapper
::~NMTable2NetCDFFilterWrapper()
{
}
