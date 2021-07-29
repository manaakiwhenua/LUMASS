/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2021 Manaaki Whenua - Landcare Research New Zealand Ltd
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
 *  DataBufferFilter.h
 *
 *  Created on 2021-07-21
 *  Author: Alex Herzig
 *
 */

#ifndef DataBufferFilter_H_
#define DataBufferFilter_H_

#include "nmlog.h"
#include "itkImageToImageFilter.h"
#include <itkIndent.h>

#include "otbsupplfilters_export.h"

/**
 *   This filter is part of LUMASS' NMDataComponent (DataBuffer) that keeps
 *   an image or table in RAM. This filter allows a DataBuffer to
 *
 *   i) provide data into a streaming processing pipeline by providing just
 *      the portion of buffered data that is requested by the downstream
 *      processing object;
 *
 *
 *
 *   Note: the downstream proecessing object must not be an in-place processing
 *   filter.
 */

namespace otb {

template <class TInputImage >
class OTBSUPPLFILTERS_EXPORT DataBufferFilter : public itk::ImageToImageFilter<TInputImage, TInputImage>
{
public:

    using Self          = DataBufferFilter;
    using Superclass    = itk::ImageToImageFilter<TInputImage, TInputImage>;
    using Pointer       = itk::SmartPointer<Self>;
    using ConstPointer  = itk::SmartPointer<const Self>;

    itkNewMacro(Self)
    itkTypeMacro(DataBufferFilter, itk::ImageToImageFilter)

    using InputImageType    = TInputImage;

    void SetNthInput(unsigned int num,
                     itk::DataObject *input);

protected:
    DataBufferFilter();
    virtual ~DataBufferFilter();

    void PrintSelf(std::ostream &os, itk::Indent indent) const;
    void GenerateData();
    //void GenerateOutputInformation();
    //void GenerateInputRequestedRegion();
    //void AllocateOutputs(){}

};     // end of class

}      // end namespace otb

#ifndef ITK_MANUAL_INSTANTIATION
#include "nmDataBufferFilter.txx"
#endif

#endif // end of include guard
