/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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
 *  NMVectorImageToImageFilterWrapper.cpp
 *
 *  Created on: 2016-08-03
 *      Author: Alexander Herzig
 */

#include "NMVectorImageToImageFilterWrapper.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbVectorImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
/*$<ForwardInputUserIDs_Include>$*/

#include "otbNMVectorImageToImageFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMVectorImageToImageFilterWrapper_Internal
{
public:
    typedef otb::VectorImage<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMVectorImageToImageFilter<InImgType, OutImgType>  FilterType;
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
                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, const QString& name)
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
        NMDebugCtx("NMVectorImageToImageFilterWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMVectorImageToImageFilterWrapper* p =
                dynamic_cast<NMVectorImageToImageFilterWrapper*>(proc);

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


        QVariant curBandVar = p->getParameter("Band");
        unsigned int curBand;
        QString bandProvVal;
        if (curBandVar.isValid())
        {
           curBand = curBandVar.toUInt(&bok);
            if (bok)
            {
                f->SetBand((curBand));
                bandProvVal = QString("%1").arg(curBand);
            }
            else
            {
                NMLogError(<< "NMVectorImageToImageFilterWrapper_Internal: " << "Invalid value for 'Band'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setSource(p->parent()->objectName().toStdString());
                e.setDescription("Invalid value for 'Band'!");
                throw e;
            }
        }
        QString bandProvN = QString("nm:Band=\"%1\"").arg(bandProvVal);
        p->addRunTimeParaProvN(bandProvN);

        /*$<ForwardInputUserIDs_Body>$*/


        NMDebugCtx("NMVectorImageToImageFilterWrapper_Internal", << "done!");
    }
};

template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<char, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<short, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<int, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<long, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<float, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, char, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, short, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, int, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, long, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, float, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<double, double, 1>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<char, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<short, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<int, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<long, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<float, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, char, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, short, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, int, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, long, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, float, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<double, double, 2>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned char, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<char, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned short, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<short, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned int, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<int, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<unsigned long, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<long, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<float, double, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, char, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, short, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, int, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, unsigned long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, long, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, float, 3>;
template class NMVectorImageToImageFilterWrapper_Internal<double, double, 3>;


InstantiateObjectWrap( NMVectorImageToImageFilterWrapper, NMVectorImageToImageFilterWrapper_Internal )
SetNthInputWrap( NMVectorImageToImageFilterWrapper, NMVectorImageToImageFilterWrapper_Internal )
GetOutputWrap( NMVectorImageToImageFilterWrapper, NMVectorImageToImageFilterWrapper_Internal )
LinkInternalParametersWrap( NMVectorImageToImageFilterWrapper, NMVectorImageToImageFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMVectorImageToImageFilterWrapper
::NMVectorImageToImageFilterWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMVectorImageToImageFilterWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;
    this->mOutputNumBands = 1;
    this->mOutputNumDimensions = 2;
    this->mInputComponentType = otb::ImageIOBase::USHORT;
    this->mOutputComponentType = otb::ImageIOBase::USHORT;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    mUserProperties.insert(QStringLiteral("Band"), QStringLiteral("Band"));
}

NMVectorImageToImageFilterWrapper
::~NMVectorImageToImageFilterWrapper()
{
}
