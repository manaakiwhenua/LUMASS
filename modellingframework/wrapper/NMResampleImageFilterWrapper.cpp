/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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
 *  NMResampleImageFilterWrapper.cpp
 *
 *  Created on: 2014-04-03
 *      Author: Alexander Herzig
 */

#include "NMResampleImageFilterWrapper.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "itkProcessObject.h"
#include "otbImage.h"
#include "itkNMResampleImageFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMResampleImageFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename itk::NMResampleImageFilter<InImgType, OutImgType>      FilterType;
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
        if (idx == 1)
        {
            OutImgType* img = dynamic_cast<OutImgType*>(dataObj);
            filter->SetReferenceImage(img);
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

/*$<InternalRATGetSupport>$*/

/*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMResampleImageFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMResampleImageFilterWrapper* p =
				dynamic_cast<NMResampleImageFilterWrapper*>(proc);

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

		
        QVariant curInterpolationMethodVar = p->getParameter("InterpolationMethod");
        std::string curInterpolationMethod;
        if (curInterpolationMethodVar.isValid())
        {
           curInterpolationMethod = curInterpolationMethodVar.toString().toStdString();
            f->SetInterpolationMethod(curInterpolationMethod);
        }

        QVariant curDefaultPixelValueVar = p->getParameter("DefaultPixelValue");
        double curDefaultPixelValue;
        if (curDefaultPixelValueVar.isValid())
        {
           curDefaultPixelValue = curDefaultPixelValueVar.toDouble(&bok);
            if (bok)
            {
                f->SetDefaultPixelValue((curDefaultPixelValue));
            }
            else
            {
                NMErr("NMResampleImageFilterWrapper_Internal", << "Invalid value for 'DefaultPixelValue'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setMsg("Invalid value for 'DefaultPixelValue'!");
                throw e;
            }
        }

        QVariant curOutputSpacingVar = p->getParameter("OutputSpacing");
        if (curOutputSpacingVar.isValid())
        {
           std::vector<OutSpacingValueType> vecOutputSpacing;
           QStringList curValVarList = curOutputSpacingVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                double curOutputSpacing = vStr.toDouble(&bok);
                if (bok)
                {
                    vecOutputSpacing.push_back(static_cast<OutSpacingValueType>(curOutputSpacing));
                }
                else
                {
                    NMErr("NMResampleImageFilterWrapper_Internal", << "Invalid value for 'OutputSpacing'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setMsg("Invalid value for 'OutputSpacing'!");
                    throw e;
                }
            }
            if (vecOutputSpacing.size() > 0)
            {
                f->SetOutputSpacing(static_cast<OutSpacingValueType*>(&vecOutputSpacing[0]));
            }
            else
            {
                f->SetOutputSpacing(0);
            }
        }

        QVariant curOutputOriginVar = p->getParameter("OutputOrigin");
        if (curOutputOriginVar.isValid())
        {
           std::vector<OutPointValueType> vecOutputOrigin;
           QStringList curValVarList = curOutputOriginVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                double curOutputOrigin = vStr.toDouble(&bok);
                if (bok)
                {
                    vecOutputOrigin.push_back(static_cast<OutPointValueType>(curOutputOrigin));
                }
                else
                {
                    NMErr("NMResampleImageFilterWrapper_Internal", << "Invalid value for 'OutputOrigin'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setMsg("Invalid value for 'OutputOrigin'!");
                    throw e;
                }
            }
            if (vecOutputOrigin.size() > 0)
            {
                f->SetOutputOrigin(static_cast<OutPointValueType*>(&vecOutputOrigin[0]));
            }
            else
            {
                f->SetOutputOrigin(0);
            }
        }

        QVariant curSizeVar = p->getParameter("Size");
        if (curSizeVar.isValid())
        {
           std::vector<SizeValueType> vecSize;
           QStringList curValVarList = curSizeVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                long curSize = vStr.toLong(&bok);
                if (bok)
                {
                    vecSize.push_back(static_cast<SizeValueType>(curSize));
                }
                else
                {
                    NMErr("NMResampleImageFilterWrapper_Internal", << "Invalid value for 'Size'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setMsg("Invalid value for 'Size'!");
                    throw e;
                }
            }
            if (vecSize.size() > 0)
            {
                f->SetSize(static_cast<SizeValueType*>(&vecSize[0]));
            }
            else
            {
                f->SetSize(0);
            }
        }


                /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMResampleImageFilterWrapper_Internal", << "done!");
	}
};

InstantiateObjectWrap( NMResampleImageFilterWrapper, NMResampleImageFilterWrapper_Internal )
SetNthInputWrap( NMResampleImageFilterWrapper, NMResampleImageFilterWrapper_Internal )
GetOutputWrap( NMResampleImageFilterWrapper, NMResampleImageFilterWrapper_Internal )
LinkInternalParametersWrap( NMResampleImageFilterWrapper, NMResampleImageFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMResampleImageFilterWrapper
::NMResampleImageFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMResampleImageFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMResampleImageFilterWrapper
::~NMResampleImageFilterWrapper()
{
}
