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
 *  NMItkCastImageFilterWrapper.cpp
 *
 *  Created on: 2014-04-02
 *      Author: Alexander Herzig
 */

#include "NMItkCastImageFilterWrapper.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "itkCastImageFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMItkCastImageFilterWrapper_Internal
{
public:
	typedef otb::Image<TInputImage, Dimension> InImgType;
	typedef otb::Image<TOutputImage, Dimension> OutImgType;
	typedef typename itk::CastImageFilter<InImgType, OutImgType> FilterType;
	typedef typename FilterType::Pointer FilterTypePointer;

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
		NMDebugCtx("NMItkCastImageFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMItkCastImageFilterWrapper* p =
				dynamic_cast<NMItkCastImageFilterWrapper*>(proc);

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

		

		NMDebugCtx("NMItkCastImageFilterWrapper_Internal", << "done!");
	}
};

InstantiateObjectWrap( NMItkCastImageFilterWrapper, NMItkCastImageFilterWrapper_Internal )
SetNthInputWrap( NMItkCastImageFilterWrapper, NMItkCastImageFilterWrapper_Internal )
GetOutputWrap( NMItkCastImageFilterWrapper, NMItkCastImageFilterWrapper_Internal )
LinkInternalParametersWrap( NMItkCastImageFilterWrapper, NMItkCastImageFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMItkCastImageFilterWrapper
::NMItkCastImageFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMItkCastImageFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

NMItkCastImageFilterWrapper
::~NMItkCastImageFilterWrapper()
{
}