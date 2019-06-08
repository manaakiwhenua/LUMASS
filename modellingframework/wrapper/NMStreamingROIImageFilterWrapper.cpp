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
 *  NMStreamingROIImageFilterWrapper.cpp
 *
 *  Created on: 2019-05-16
 *      Author: Alexander Herzig
 */

#include "NMStreamingROIImageFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "nmStreamingROIImageFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMStreamingROIImageFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename nm::StreamingROIImageFilter<InImgType, OutImgType> FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

    typedef typename OutImgType::IndexValueType   IndexValueType;
    typedef typename OutImgType::PointValueType   PointValueType;

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
		NMDebugCtx("NMStreamingROIImageFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMStreamingROIImageFilterWrapper* p =
				dynamic_cast<NMStreamingROIImageFilterWrapper*>(proc);

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

		
        QVariant curROIIndexVar = p->getParameter("ROIIndex");
        if (curROIIndexVar.isValid())
        {
           std::vector<IndexValueType> vecROIIndex;
           QStringList curValVarList = curROIIndexVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                long curROIIndex = vStr.toLong(&bok);
                if (bok)
                {
                    vecROIIndex.push_back(static_cast<IndexValueType>(curROIIndex));
                }
                else
                {
                    NMErr("NMStreamingROIImageFilterWrapper_Internal", << "Invalid value for 'ROIIndex'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'ROIIndex'!");
                    throw e;
                }
            }
            f->SetROIIndex(vecROIIndex);
            QString roiIndexProvN = QString("nm:ROIIndex=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(roiIndexProvN);
        }

        QVariant curROISizeVar = p->getParameter("ROISize");
        if (curROISizeVar.isValid())
        {
           std::vector<IndexValueType> vecROISize;
           QStringList curValVarList = curROISizeVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                long curROISize = vStr.toLong(&bok);
                if (bok)
                {
                    vecROISize.push_back(static_cast<IndexValueType>(curROISize));
                }
                else
                {
                    NMErr("NMStreamingROIImageFilterWrapper_Internal", << "Invalid value for 'ROISize'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'ROISize'!");
                    throw e;
                }
            }
            f->SetROISize(vecROISize);
            QString roiSizeProvN = QString("nm:ROISize=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(roiSizeProvN);
        }

        QVariant curROIOriginVar = p->getParameter("ROIOrigin");
        if (curROIOriginVar.isValid())
        {
           std::vector<PointValueType> vecROIOrigin;
           QStringList curValVarList = curROIOriginVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                double curROIOrigin = vStr.toDouble(&bok);
                if (bok)
                {
                    vecROIOrigin.push_back(static_cast<PointValueType>(curROIOrigin));
                }
                else
                {
                    NMErr("NMStreamingROIImageFilterWrapper_Internal", << "Invalid value for 'ROIOrigin'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'ROIOrigin'!");
                    throw e;
                }
            }
            f->SetROIOrigin(vecROIOrigin);
            QString roiOriginProvN = QString("nm:ROIOrigin=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(roiOriginProvN);
        }

        QVariant curROILengthVar = p->getParameter("ROILength");
        if (curROILengthVar.isValid())
        {
           std::vector<PointValueType> vecROILength;
           QStringList curValVarList = curROILengthVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                double curROILength = vStr.toDouble(&bok);
                if (bok)
                {
                    vecROILength.push_back(static_cast<PointValueType>(curROILength));
                }
                else
                {
                    NMErr("NMStreamingROIImageFilterWrapper_Internal", << "Invalid value for 'ROILength'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setDescription("Invalid value for 'ROILength'!");
                    throw e;
                }
            }
            f->SetROILength(vecROILength);
            QString roiLengthProvN = QString("nm:ROILength=\"%1\"").arg(curValVarList.join(' '));
            p->addRunTimeParaProvN(roiLengthProvN);
        }


                /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMStreamingROIImageFilterWrapper_Internal", << "done!");
	}
};

/*$<HelperClassInstantiation>$*/

InstantiateObjectWrap( NMStreamingROIImageFilterWrapper, NMStreamingROIImageFilterWrapper_Internal )
SetNthInputWrap( NMStreamingROIImageFilterWrapper, NMStreamingROIImageFilterWrapper_Internal )
GetOutputWrap( NMStreamingROIImageFilterWrapper, NMStreamingROIImageFilterWrapper_Internal )
LinkInternalParametersWrap( NMStreamingROIImageFilterWrapper, NMStreamingROIImageFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMStreamingROIImageFilterWrapper
::NMStreamingROIImageFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMStreamingROIImageFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMStreamingROIImageFilterWrapper
::~NMStreamingROIImageFilterWrapper()
{
}
