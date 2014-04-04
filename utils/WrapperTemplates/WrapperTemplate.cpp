/******************************************************************************
 * Created by Alexander Herzig
 * Copyright /*$<Year>$*/ Landcare Research New Zealand Ltd
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
 *  /*$<WrapperClassName>$*/.cpp
 *
 *  Created on: /*$<FileDate>$*/
 *      Author: /*$<Author>$*/
 */

#include "/*$<WrapperClassName>$*/.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "/*$<FilterClassFileName>$*/.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class /*$<WrapperClassName>$*/_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename /*$<FilterTypeDef>$*/      FilterType;
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

/*$<InternalSetNthInput>$*/

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
		NMDebugCtx("/*$<WrapperClassName>$*/_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		/*$<WrapperClassName>$*/* p =
				dynamic_cast</*$<WrapperClassName>$*/*>(proc);

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

		/*$<InternalFilterParamSetter>$*/

		NMDebugCtx("/*$<WrapperClassName>$*/_Internal", << "done!");
	}
};

InstantiateObjectWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
SetNthInputWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
/*$<GetOutPutWrap>$*/( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
LinkInternalParametersWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

/*$<WrapperClassName>$*/
::/*$<WrapperClassName>$*/(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("/*$<WrapperClassName>$*/");
	this->mParameterHandling = NMProcess::NM_USE_UP;
}

/*$<WrapperClassName>$*/
::~/*$<WrapperClassName>$*/()
{
}
