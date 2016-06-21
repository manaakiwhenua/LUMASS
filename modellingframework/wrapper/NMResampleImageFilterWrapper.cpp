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
#include "itkNMResampleImageFilter_ExplicitInst.h"

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

template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, float, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, double, 1>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<char, char, 1>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<char, short, 1>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<char, int, 1>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<char, long, 1>;
template class NMResampleImageFilterWrapper_Internal<char, float, 1>;
template class NMResampleImageFilterWrapper_Internal<char, double, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, float, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, double, 1>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<short, char, 1>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<short, short, 1>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<short, int, 1>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<short, long, 1>;
template class NMResampleImageFilterWrapper_Internal<short, float, 1>;
template class NMResampleImageFilterWrapper_Internal<short, double, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, float, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, double, 1>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<int, char, 1>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<int, short, 1>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<int, int, 1>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<int, long, 1>;
template class NMResampleImageFilterWrapper_Internal<int, float, 1>;
template class NMResampleImageFilterWrapper_Internal<int, double, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, char, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, short, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, int, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, long, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, float, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, double, 1>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<long, char, 1>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<long, short, 1>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<long, int, 1>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<long, long, 1>;
template class NMResampleImageFilterWrapper_Internal<long, float, 1>;
template class NMResampleImageFilterWrapper_Internal<long, double, 1>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<float, char, 1>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<float, short, 1>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<float, int, 1>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<float, long, 1>;
template class NMResampleImageFilterWrapper_Internal<float, float, 1>;
template class NMResampleImageFilterWrapper_Internal<float, double, 1>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned char, 1>;
template class NMResampleImageFilterWrapper_Internal<double, char, 1>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned short, 1>;
template class NMResampleImageFilterWrapper_Internal<double, short, 1>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned int, 1>;
template class NMResampleImageFilterWrapper_Internal<double, int, 1>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned long, 1>;
template class NMResampleImageFilterWrapper_Internal<double, long, 1>;
template class NMResampleImageFilterWrapper_Internal<double, float, 1>;
template class NMResampleImageFilterWrapper_Internal<double, double, 1>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, float, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, double, 2>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<char, char, 2>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<char, short, 2>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<char, int, 2>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<char, long, 2>;
template class NMResampleImageFilterWrapper_Internal<char, float, 2>;
template class NMResampleImageFilterWrapper_Internal<char, double, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, float, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, double, 2>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<short, char, 2>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<short, short, 2>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<short, int, 2>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<short, long, 2>;
template class NMResampleImageFilterWrapper_Internal<short, float, 2>;
template class NMResampleImageFilterWrapper_Internal<short, double, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, float, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, double, 2>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<int, char, 2>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<int, short, 2>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<int, int, 2>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<int, long, 2>;
template class NMResampleImageFilterWrapper_Internal<int, float, 2>;
template class NMResampleImageFilterWrapper_Internal<int, double, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, char, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, short, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, int, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, long, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, float, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, double, 2>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<long, char, 2>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<long, short, 2>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<long, int, 2>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<long, long, 2>;
template class NMResampleImageFilterWrapper_Internal<long, float, 2>;
template class NMResampleImageFilterWrapper_Internal<long, double, 2>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<float, char, 2>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<float, short, 2>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<float, int, 2>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<float, long, 2>;
template class NMResampleImageFilterWrapper_Internal<float, float, 2>;
template class NMResampleImageFilterWrapper_Internal<float, double, 2>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned char, 2>;
template class NMResampleImageFilterWrapper_Internal<double, char, 2>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned short, 2>;
template class NMResampleImageFilterWrapper_Internal<double, short, 2>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned int, 2>;
template class NMResampleImageFilterWrapper_Internal<double, int, 2>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned long, 2>;
template class NMResampleImageFilterWrapper_Internal<double, long, 2>;
template class NMResampleImageFilterWrapper_Internal<double, float, 2>;
template class NMResampleImageFilterWrapper_Internal<double, double, 2>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, float, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned char, double, 3>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<char, char, 3>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<char, short, 3>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<char, int, 3>;
template class NMResampleImageFilterWrapper_Internal<char, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<char, long, 3>;
template class NMResampleImageFilterWrapper_Internal<char, float, 3>;
template class NMResampleImageFilterWrapper_Internal<char, double, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, float, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned short, double, 3>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<short, char, 3>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<short, short, 3>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<short, int, 3>;
template class NMResampleImageFilterWrapper_Internal<short, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<short, long, 3>;
template class NMResampleImageFilterWrapper_Internal<short, float, 3>;
template class NMResampleImageFilterWrapper_Internal<short, double, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, float, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned int, double, 3>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<int, char, 3>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<int, short, 3>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<int, int, 3>;
template class NMResampleImageFilterWrapper_Internal<int, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<int, long, 3>;
template class NMResampleImageFilterWrapper_Internal<int, float, 3>;
template class NMResampleImageFilterWrapper_Internal<int, double, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, char, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, short, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, int, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, long, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, float, 3>;
template class NMResampleImageFilterWrapper_Internal<unsigned long, double, 3>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<long, char, 3>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<long, short, 3>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<long, int, 3>;
template class NMResampleImageFilterWrapper_Internal<long, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<long, long, 3>;
template class NMResampleImageFilterWrapper_Internal<long, float, 3>;
template class NMResampleImageFilterWrapper_Internal<long, double, 3>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<float, char, 3>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<float, short, 3>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<float, int, 3>;
template class NMResampleImageFilterWrapper_Internal<float, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<float, long, 3>;
template class NMResampleImageFilterWrapper_Internal<float, float, 3>;
template class NMResampleImageFilterWrapper_Internal<float, double, 3>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned char, 3>;
template class NMResampleImageFilterWrapper_Internal<double, char, 3>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned short, 3>;
template class NMResampleImageFilterWrapper_Internal<double, short, 3>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned int, 3>;
template class NMResampleImageFilterWrapper_Internal<double, int, 3>;
template class NMResampleImageFilterWrapper_Internal<double, unsigned long, 3>;
template class NMResampleImageFilterWrapper_Internal<double, long, 3>;
template class NMResampleImageFilterWrapper_Internal<double, float, 3>;
template class NMResampleImageFilterWrapper_Internal<double, double, 3>;

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
