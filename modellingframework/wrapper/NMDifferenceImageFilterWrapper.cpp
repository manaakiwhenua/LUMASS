/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2025 Landcare Research New Zealand Ltd
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
 *  NMDifferenceImageFilterWrapper.cpp
 *
 *  Created on: 2025-02-04
 *      Author: makeWrapperConfigFile, Alex Herzig
 */

#include "NMDifferenceImageFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbNMDifferenceImageFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMDifferenceImageFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMDifferenceImageFilter<InImgType,OutImgType>      FilterType;
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
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        if (name.contains(QStringLiteral("valid"), Qt::CaseInsensitive))
        {
            idx = 0;
        }
        else if (name.contains(QStringLiteral("test"), Qt::CaseInsensitive))
        {
            idx = 1;
        }

        if (idx == 0)
        {
            InImgType* img = dynamic_cast<InImgType*>(dataObj);
            if (img != nullptr)
            {
                filter->SetValidInput(img);
            }
            SetNthInputStandardTypeError
        }
        else if (idx == 1)
        {
            InImgType* img = dynamic_cast<InImgType*>(dataObj);
            if (img != nullptr)
            {
                filter->SetTestInput(img);
            }
            SetNthInputStandardTypeError

        }
        else
        {
            NMMfwException e(NMMfwException::NMProcess_InvalidInput);
            e.setDescription("This component does not accept more than two inputs!");

            throw e;
            return;
        }
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
		NMDebugCtx("NMDifferenceImageFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMDifferenceImageFilterWrapper* p =
				dynamic_cast<NMDifferenceImageFilterWrapper*>(proc);

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

		
        QVariant curToleranceRadiusVar = p->getParameter("ToleranceRadius");
        int curToleranceRadius;
        if (curToleranceRadiusVar.isValid())
        {
            curToleranceRadius = curToleranceRadiusVar.toInt(&bok);
            if (bok)
            {
                f->SetToleranceRadius((curToleranceRadius));
            }
            else
            {
                NMErr("NMDifferenceImageFilterWrapper_Internal", << "Invalid value for 'ToleranceRadius'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setDescription("Invalid value for 'ToleranceRadius'!");
                throw e;
            }
        }

        QVariant curDifferenceThresholdVar = p->getParameter("DifferenceThreshold");
        double curDifferenceThreshold;
        if (curDifferenceThresholdVar.isValid())
        {
            curDifferenceThreshold = curDifferenceThresholdVar.toDouble(&bok);
            if (bok)
            {
                f->SetDifferenceThreshold((curDifferenceThreshold));
            }
            else
            {
                NMErr("NMDifferenceImageFilterWrapper_Internal", << "Invalid value for 'DifferenceThreshold'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setDescription("Invalid value for 'DifferenceThreshold'!");
                throw e;
            }
        }

        QVariant curWSVar = p->getSetting("Workspace").toString();
        std::string curWS;
        if (curWSVar.isValid())
        {
            curWS = curWSVar.toString().toStdString();
            f->SetWorkspace(curWS);
        }


        QVariant curResultFileNameVar = p->getParameter("ResultFileName");
        std::string curResultFileName;
        if (curResultFileNameVar.isValid())
        {

            curResultFileName = curResultFileNameVar.toString().toStdString();
            f->SetResultFileName(curResultFileName);
        }


        QVariant curPrintResultsVar = p->getParameter("PrintResults");
        bool curPrintResults;
        if (curPrintResultsVar.isValid())
        {
            curPrintResults = curPrintResultsVar.toBool();
            f->SetPrintResults((curPrintResults));
        }
        else
        {
            NMErr("NMDifferenceImageFilterWrapper_Internal", << "Invalid value for 'PrintResults'!");
            NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
            e.setDescription("Invalid value for 'PrintResults'!");
            throw e;
        }


        /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMDifferenceImageFilterWrapper_Internal", << "done!");
	}

    static std::vector<double> getCompareResults(itk::ProcessObject::Pointer& otbFilter)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        std::vector<double> results;
        results.push_back(filter->GetToleranceRadius());
        results.push_back(filter->GetDifferenceThreshold());
        results.push_back(filter->GetMeanDifference());
        results.push_back(filter->GetTotalDifference());
        results.push_back(filter->GetNumberOfPixelsWithDifferences());
        return results;
    }
};

#define callGetResults( inputType, outputType, wrapName ) \
{ \
    if (this->mInputNumDimensions == 1)		\
    {		\
         results = wrapName< inputType, outputType, 1>::getCompareResults(this->mOtbProcess);\
    }		\
    else if (this->mInputNumDimensions == 2) \
    { \
        results = wrapName< inputType, outputType, 2 >::getCompareResults(this->mOtbProcess);\
    } \
    else if (this->mInputNumDimensions == 3) \
    { \
        results = wrapName< inputType, outputType, 3 >::getCompareResults(this->mOtbProcess);	\
    }\
}

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMDifferenceImageFilterWrapper, NMDifferenceImageFilterWrapper_Internal )
SetNthInputWrap( NMDifferenceImageFilterWrapper, NMDifferenceImageFilterWrapper_Internal )
GetOutputWrap( NMDifferenceImageFilterWrapper, NMDifferenceImageFilterWrapper_Internal )
LinkInternalParametersWrap( NMDifferenceImageFilterWrapper, NMDifferenceImageFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMDifferenceImageFilterWrapper
::NMDifferenceImageFilterWrapper(QObject* parent)
    : mHasCompleted(false)
{
	this->setParent(parent);
	this->setObjectName("NMDifferenceImageFilterWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;    this->mInputNumBands = 1;
    this->mOutputNumBands = 1;
    this->mInputComponentType = otb::ImageIOBase::FLOAT;
    this->mOutputComponentType = otb::ImageIOBase::FLOAT;
    this->mInputNumDimensions = 2;
    this->mInputNumBands = 1;
    this->mPrintResults = false;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("InputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("ToleranceRadius"), QStringLiteral("ToleranceRadius"));
    mUserProperties.insert(QStringLiteral("DifferenceThreshold"), QStringLiteral("DifferenceThreshold"));
    mUserProperties.insert(QStringLiteral("ResultFileName"), QStringLiteral("ResultFileName"));
    mUserProperties.insert(QStringLiteral("PrintResults"), QStringLiteral("PrintResults"));

    //mUserProperties.insert(QStringLiteral("NumberOfThreads"), QStringLiteral("NumThreads"));

}

NMDifferenceImageFilterWrapper
::~NMDifferenceImageFilterWrapper()
{
}

void NMDifferenceImageFilterWrapper::update()
{
    mHasCompleted = false;
    NMProcess::update();
    mHasCompleted = true;
    printResults();
}

void NMDifferenceImageFilterWrapper
::printResults()
{
    if (!this->mHasCompleted)
    {
        return;
    }

    std::vector<double> results;
    otb::ImageIOBase::IOComponentType outerType;
    otb::ImageIOBase::IOComponentType innerType;
    switch (this->mInputComponentType)
    {
        UserMacroPerTypeOuter( callGetResults, NMDifferenceImageFilterWrapper_Internal );
        default: break;
    }

    std::vector<std::string> names = {"ToleranceRadius", "DifferenceThreshold",
                                      "MeanDifference", "TotalDifference",
                                      "NumberOfPixelsWithDifferences"};

    NMLogInfo(<< "Test Results:");
    for (int r=0; r < results.size(); ++r)
    {
        NMLogInfo(<< names[r] << ": " << results[r]);
    }
}

