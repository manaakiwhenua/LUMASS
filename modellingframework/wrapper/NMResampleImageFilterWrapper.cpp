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
#include "otbNMGridResampleImageFilter.h"
//#include "itkNMResampleImageFilter.h"
//#include "itkNMResampleImageFilter_ExplicitInst.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMResampleImageFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMGridResampleImageFilter<InImgType, OutImgType>  FilterType;
    typedef typename FilterType::Pointer           FilterTypePointer;

    typedef typename FilterType::SizeType          FilterSizeType;
    typedef typename FilterSizeType::SizeValueType FilterSizeValueType;

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
//        if (idx == 1)
//        {
//            OutImgType* img = dynamic_cast<OutImgType*>(dataObj);
//            filter->SetReferenceImage(img);
//        }
//        else
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
            e.setSource(p->parent()->objectName().toStdString());
			e.setDescription("We're trying to link, but the filter doesn't seem to be initialised properly!");
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
            QString interpolMethodProvN = QString("nm:InterpolationMethod=\"%1\"").arg(curInterpolationMethod.c_str());
            p->addRunTimeParaProvN(interpolMethodProvN);
        }

        QVariant curDefaultPixelValueVar = p->getParameter("DefaultPixelValue");
        double curDefaultPixelValue;
        if (curDefaultPixelValueVar.isValid())
        {
            curDefaultPixelValue = curDefaultPixelValueVar.toDouble(&bok);
            if (bok)
            {
                f->SetDefaultPixelValue((curDefaultPixelValue));
                QString defaultPixValProvN = QString("nm:DefaultPixelValue=\"%1\"").arg(curDefaultPixelValue);
                p->addRunTimeParaProvN(defaultPixValProvN);
            }
            else
            {
                NMLogError(<< "NMResampleImageFilterWrapper_Internal: " << "Invalid value for 'DefaultPixelValue'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(p->parent()->objectName().toStdString());
                e.setDescription("Invalid value for 'DefaultPixelValue'!");
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
                    NMLogError(<< "NMResampleImageFilterWrapper_Internal: " << "Invalid value for 'OutputSpacing'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setSource(p->parent()->objectName().toStdString());
                    e.setDescription("Invalid value for 'OutputSpacing'!");
                    throw e;
                }
            }
            if (vecOutputSpacing.size() > 0)
            {
                f->SetOutputSpacing(static_cast<OutSpacingValueType*>(&vecOutputSpacing[0]));
            }
            else
            {
                for (int d=0; d < Dimension; ++d)
                {
                    vecOutputSpacing.push_back(0);
                }
                f->SetOutputSpacing(static_cast<OutSpacingValueType*>(&vecOutputSpacing[0]));
            }
            QString outputSpacingProvN = QString("nm:OutputSpacing=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(outputSpacingProvN);
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
                    NMLogError(<< "NMResampleImageFilterWrapper_Internal: " << "Invalid value for 'OutputOrigin'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setSource(p->parent()->objectName().toStdString());
                    e.setDescription("Invalid value for 'OutputOrigin'!");
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
            QString outputOriginProvN = QString("nm:OutputOrigin=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(outputOriginProvN);
        }

        QVariant curSizeVar = p->getParameter("Size");
        if (curSizeVar.isValid())
        {
           std::vector<FilterSizeValueType> vecSize;
           QStringList curValVarList = curSizeVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                // since ...
                // using itk::SizeValueType = unsigned long
                unsigned long curSize = vStr.toULong(&bok);

                if (bok)
                {
                    vecSize.push_back(static_cast<FilterSizeValueType>(curSize));
                }
                else
                {
                    NMLogError(<< "NMResampleImageFilterWrapper_Internal: " << "Invalid value for 'Size'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'Size'!");
                    e.setSource(p->parent()->objectName().toStdString());
                    throw e;
                }
            }
            if (vecSize.size() > 0)
            {
                f->SetSize(static_cast<FilterSizeValueType*>(&vecSize[0]));
            }
            else
            {
                for (int d=0; d < Dimension; ++d)
                {
                    vecSize.push_back(0);
                }
                f->SetSize(static_cast<FilterSizeValueType*>(&vecSize[0]));
            }
            QString sizeProvN = QString("nm:Size=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(sizeProvN);
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
