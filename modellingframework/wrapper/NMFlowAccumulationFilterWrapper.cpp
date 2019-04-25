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
 *  NMFlowAccumulationFilterWrapper.cpp
 *
 *  Created on: 2019-03-19
 *      Author: Alexander Herzig
 */

#include "NMFlowAccumulationFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbFlowAccumulationFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMFlowAccumulationFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::FlowAccumulationFilter<InImgType, OutImgType>      FilterType;
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

/*$<InternalRATGetSupport>$*/

/*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMFlowAccumulationFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMFlowAccumulationFilterWrapper* p =
				dynamic_cast<NMFlowAccumulationFilterWrapper*>(proc);

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


        QVariant curAlgorithmTypeVar = p->getParameter("AlgorithmType");
        std::string curAlgorithmType;
        if (curAlgorithmTypeVar.isValid())
        {
           curAlgorithmType = curAlgorithmTypeVar.toString().simplified().toStdString();
            f->SetFlowAccAlgorithm(curAlgorithmType);
        }

        QVariant curNodataVar = p->getParameter("Nodata");
        double curNodata;
        if (curNodataVar.isValid())
        {
           curNodata = curNodataVar.toDouble(&bok);
            if (bok)
            {
                f->Setnodata(static_cast<InImgPixelType>(curNodata));
            }
            else
            {
                NMLogError(<< "NMFlowAccumulationFilterWrapper_Internal: " << "Invalid value for 'Nodata'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(p->parent()->objectName().toStdString());
                e.setDescription("Invalid value for 'Nodata'!");
                throw e;
            }
        }

        QVariant curFlowExponentVar = p->getParameter("FlowExponent");
        int curFlowExponent;
        if (curFlowExponentVar.isValid())
        {
           curFlowExponent = curFlowExponentVar.toInt(&bok);
            if (bok)
            {
                f->SetFlowExponent((curFlowExponent));
            }
            else
            {
                NMLogError(<< "NMFlowAccumulationFilterWrapper_Internal: " << "Invalid value for 'FlowExponent'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(p->parent()->objectName().toStdString());
                e.setDescription("Invalid value for 'FlowExponent'!");
                throw e;
            }
        }

        QVariant curFlowLengthTypeVar = p->getParameter("FlowLengthType");
        std::string curFlowLengthType;
        if (curFlowLengthTypeVar.isValid())
        {
           curFlowLengthType = curFlowLengthTypeVar.toString().simplified().toStdString();
           if (curFlowLengthType.compare("NO_FLOWLENGTH") == 0)
           {
                f->SetFlowLength(false);
           }
           else if (curFlowLengthType.compare("UPSTREAM") == 0)
           {
               f->SetFlowLength(true);
               f->SetFlowLengthUpstream(false); // <-- looks wrong, but is correct!
           }
           else if (curFlowLengthType.compare("DOWNSTREAM") == 0)
           {
               f->SetFlowLength(true);
               f->SetFlowLengthUpstream(true); // <-- looks wrong, but is correct!
           }
        }

        /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMFlowAccumulationFilterWrapper_Internal", << "done!");
	}
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMFlowAccumulationFilterWrapper, NMFlowAccumulationFilterWrapper_Internal )
SetNthInputWrap( NMFlowAccumulationFilterWrapper, NMFlowAccumulationFilterWrapper_Internal )
GetOutputWrap( NMFlowAccumulationFilterWrapper, NMFlowAccumulationFilterWrapper_Internal )
LinkInternalParametersWrap( NMFlowAccumulationFilterWrapper, NMFlowAccumulationFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMFlowAccumulationFilterWrapper
::NMFlowAccumulationFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMFlowAccumulationFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;

    this->mAlgorithmType = QString(tr("MFD"));

    mAlgorithmEnum.clear();
    mAlgorithmEnum << "MFD" << "Dinf" << "MFDw";

    mFlowLengthType = "NO_FLOWLENGTH";
    mFlowLengthEnum.clear();
    mFlowLengthEnum << "NO_FLOWLENGTH" << "DOWNSTREAM" << "UPSTREAM";
}

NMFlowAccumulationFilterWrapper
::~NMFlowAccumulationFilterWrapper()
{
}
