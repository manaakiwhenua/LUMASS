/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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
 *  NMVirtualStreamWriter.cpp
 *
 *  Created on: 2017-03-28
 *      Author: Alexander Herzig
 */

#include "NMVirtualStreamWriter.h"

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbNMStreamingImageVirtualWriter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMVirtualStreamWriter_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMStreamingImageVirtualWriter<InImgType> FilterType;
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

    static otb::AttributeTable::Pointer getRAT(
        itk::ProcessObject::Pointer& procObj, 
        unsigned int numBands, unsigned int idx)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->getRAT(idx);
    }


    static void setRAT(
        itk::ProcessObject::Pointer& procObj, 
        unsigned int numBands, unsigned int idx,
        otb::AttributeTable::Pointer& rat)
    {
        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());
        return f->setRAT(idx, rat);
    }



    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMVirtualStreamWriter_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMVirtualStreamWriter* p =
				dynamic_cast<NMVirtualStreamWriter*>(proc);

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

		
        f->SetStreamingSize(p->getStreamingSize());

        f->SetStreamingMethod(p->getStreamingMethodType().toStdString());

        f->SetNumThreads(p->getNumThreads());


                /*$<ForwardInputUserIDs_Body>$*/


		NMDebugCtx("NMVirtualStreamWriter_Internal", << "done!");
	}
};


InstantiateObjectWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )
SetNthInputWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )
GetOutputRATWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )
LinkInternalParametersWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )
GetRATWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )

SetRATWrap( NMVirtualStreamWriter, NMVirtualStreamWriter_Internal )


NMVirtualStreamWriter
::NMVirtualStreamWriter(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMVirtualStreamWriter");
    this->mParameterHandling = NMProcess::NM_USE_UP;

    this->mStreamingMethodType = QString(tr("STRIPPED"));
    this->mStreamingMethodEnum.clear();
    this->mStreamingMethodEnum << "STRIPPED" << "TILED";

    this->mStreamingSize = 512;
}

NMVirtualStreamWriter
::~NMVirtualStreamWriter()
{
}
