/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014, 2015 Landcare Research New Zealand Ltd
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
 *  NMSumZonesFilterWrapper.cpp
 *
 *  Created on: 2014-03-27, 2015-08-27
 *      Author: Alexander Herzig
 */

#include "NMSumZonesFilterWrapper.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbSumZonesFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMSumZonesFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::SumZonesFilter<InImgType, OutImgType>      FilterType;
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
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        if (idx == 0)
        {
            OutImgType* img = dynamic_cast<OutImgType*>(dataObj);
            filter->SetZoneImage(img);
        }
        else if (idx == 1)
        {
            InImgType* img = dynamic_cast<InImgType*>(dataObj);
            filter->SetValueImage(img);
        }
        else
        {
            InImgType* img = dynamic_cast<InImgType*>(dataObj);
            filter->SetInput(idx, img);
        }
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


/*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMSumZonesFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMSumZonesFilterWrapper* p =
				dynamic_cast<NMSumZonesFilterWrapper*>(proc);

		// make sure we've got a valid filter object
		if (f == 0)
		{
			NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
			e.setMsg("We're trying to link, but the filter doesn't seem to be initialised properly!");
			throw e;
			return;
		}

		/* do something reasonable here */
		bool bok;
		int givenStep = step;

		
        step = p->mapHostIndexToPolicyIndex(givenStep, p->mIgnoreNodataValue.size());
        bool curIgnoreNodataValue;
        if (step < p->mIgnoreNodataValue.size())
        {
            curIgnoreNodataValue = p->mIgnoreNodataValue.at(step).toInt(&bok);
            if (bok)
            {
                f->SetIgnoreNodataValue((curIgnoreNodataValue));
            }
            else
            {
                NMErr("NMSumZonesFilterWrapper_Internal", << "Invalid value for 'IgnoreNodataValue'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setMsg("Invalid value for 'IgnoreNodataValue'!");
                throw e;
            }
        }

        step = p->mapHostIndexToPolicyIndex(givenStep, p->mNodataValue.size());
        double curNodataValue;
        if (step < p->mNodataValue.size())
        {
            curNodataValue = p->mNodataValue.at(step).toDouble(&bok);
            if (bok)
            {
                f->SetNodataValue((curNodataValue));
            }
            else
            {
                NMErr("NMSumZonesFilterWrapper_Internal", << "Invalid value for 'NodataValue'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setMsg("Invalid value for 'NodataValue'!");
                throw e;
            }
        }

        step = p->mapHostIndexToPolicyIndex(givenStep, p->mHaveMaxKeyRows.size());
        bool curHaveMaxKeyRows;
        if (step < p->mHaveMaxKeyRows.size())
        {
            curHaveMaxKeyRows = p->mHaveMaxKeyRows.at(step).toInt(&bok);
            if (bok)
            {
                f->SetHaveMaxKeyRows((curHaveMaxKeyRows));
            }
            else
            {
                NMErr("NMSumZonesFilterWrapper_Internal", << "Invalid value for 'HaveMaxKeyRows'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setMsg("Invalid value for 'HaveMaxKeyRows'!");
                throw e;
            }
        }

//        step = p->mapHostIndexToPolicyIndex(givenStep, p->mZoneTableFileName.size());
//        std::string curZoneTableFileName;
//        if (step < p->mZoneTableFileName.size())
//        {
//            curZoneTableFileName = p->mZoneTableFileName.at(step).toStdString().c_str();
//            f->SetZoneTableFileName(curZoneTableFileName);
//        }

        QVariant ztparam = p->getParameter("ZoneTableFileName");
        if (ztparam.isValid())
        {
            f->SetZoneTableFileName(ztparam.toString().toStdString());
        }

		NMDebugCtx("NMSumZonesFilterWrapper_Internal", << "done!");
	}
};

InstantiateObjectWrap( NMSumZonesFilterWrapper, NMSumZonesFilterWrapper_Internal )
SetNthInputWrap( NMSumZonesFilterWrapper, NMSumZonesFilterWrapper_Internal )
GetOutputRATWrap( NMSumZonesFilterWrapper, NMSumZonesFilterWrapper_Internal )
LinkInternalParametersWrap( NMSumZonesFilterWrapper, NMSumZonesFilterWrapper_Internal )
GetRATWrap( NMSumZonesFilterWrapper, NMSumZonesFilterWrapper_Internal )

/*$<RATSetSupportWrap>$*/

NMSumZonesFilterWrapper
::NMSumZonesFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMSumZonesFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMSumZonesFilterWrapper
::~NMSumZonesFilterWrapper()
{
}
