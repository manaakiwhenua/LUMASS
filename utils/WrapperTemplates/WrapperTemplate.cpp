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
#include "/*$<FilterClassName>$*/.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
/*$<WrapperTemplateTypePamphlet>$*/
class /*$<WrapperClassName>$*/_Internal
{
public:
	 /*$<InternalInImgTypedef>$*/ InImgType;
	 /*$<InternalOutImgTypedef>$*/ OutImgType;
	 /*$<InternalFilterTypedef>$*/ FilterType;
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
		filter->SetInput(img);
	}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast</*$<GetOutputImgTypename>$*/*>(filter->GetOutput(idx));
	}

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
		int givenStep = step;

		/*$<InternalFilterParamSetter>$*/

		// DEBUG
		// let's print the filter's self information
		f->Print(std::cout, itk::Indent(2));

		NMDebugCtx("/*$<WrapperClassName>$*/_Internal", << "done!");
	}
};

InstantiateObjectWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
SetNthInputWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
GetOutputWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )
LinkInternalParametersWrap( /*$<WrapperClassName>$*/, /*$<WrapperClassName>$*/_Internal )

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
