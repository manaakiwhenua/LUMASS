 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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
 * BMIModelFilter.txx
 *
 *  Created on: 30/04/2020
 *      Author: alex
 *
 *  Please note that portions of this file have been copied from
 *  ITK's itkImageSource.hxx, which is subject to the copyright
 *  notice reproduced below
 */
/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/*=========================================================================
 *
 *  Portions of this file are subject to the VTK Toolkit Version 3 copyright.
 *
 *  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 *
 *  For complete copyright, license and disclaimer of warranty information
 *  please refer to the NOTICE file at the top of the ITK source tree.
 *
 *=========================================================================*/


#ifndef __otbBMIModelFilter_txx
#define __otbBMIModelFilter_txx

#include "nmlog.h"
#include "otbBMIModelFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"
#include "itkOffset.h"
#include "itkImageRegionSplitterBase.h"
#include "itkImageRegionIterator.h"
#include "itkMultiThreader.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkExceptionObject.h"
#include "itkDataObject.h"
#include "itkImageSource.h"
#include "itkImportImageContainer.h"

#include "Python_wrapper.h"

#include <algorithm>
#include <typeindex>
#include <ctime>

class BMIModelException : public std::exception
{
public:
    BMIModelException(const char* msg)
    {
        _what = msg;
    }

    const char* what(void)
    {
        return _what.c_str();
    }

protected:
    std::string _what;
};

namespace otb {

template <class TInputImage, class TOutputImage>
BMIModelFilter<TInputImage, TOutputImage>
::BMIModelFilter()
     : m_IsStreamable(true),
       m_IsThreadable(false),
       m_NumOutputs(1),
       m_PixCount(0),
       m_NumNeighbourPixel(0),
       m_KernelShape("RECTANGULAR"),
       m_ImageBufferDimension(1),
       m_ImageRegionDimension(2),
       m_LPRName("LPR"),
       m_SRName("SR"),
       m_RegionValueType(0),
       m_ActiveNeighborhoodSize(0),
       m_AuxIntDataSize(0),
       m_AuxDoubleDataSize(0),
       m_AuxVarDataIndex(-1),
       m_AuxVarArLen(0),
       m_AuxVarAr_Name("auxVarAr"),
       m_AuxVarNames_Name("auxVarNames")
{
    this->SetNumberOfRequiredOutputs(m_NumOutputs);
    m_KernelRadius.Fill(0);
}

template <class TInputImage, class TOutputImage>
BMIModelFilter<TInputImage, TOutputImage>
::~BMIModelFilter()
{
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);
    os << indent << "Hello, I am the BMIModelFilter!" << std::endl;
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetInputNames(const std::vector<std::string> &inputNames)
{
    m_InputNames.clear();
    m_InputNames = inputNames;
    this->SetNumberOfIndexedInputs(m_InputNames.size());

    this->Modified();
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetAuxIntData(const std::vector<int64_t>& intData)
{
    m_AuxIntData = intData;
    m_AuxIntDataSize = intData.size();
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetAuxDoubleData(const std::vector<double_t>& doubleData)
{
    m_AuxDoubleData = doubleData;
    m_AuxDoubleDataSize = doubleData.size();
}


template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetBMIModule(const std::shared_ptr<bmi::PythonBMI>& bmiModule)
{
    if (bmiModule.get() == nullptr)
    {
        NMProcErr(<< "bmi::PythonBMI module is NULL!");
        return;
    }

    // determine name of kernel function associated with this class
    m_BMIModule = bmiModule;
    m_KernelFuncName = m_BMIModule->mBMIClass + "_kfunc";


    // set the number of outputs produced by
    // the configured BMI module
    m_NumOutputs = this->m_BMIModule->GetOutputItemCount();
    std::vector<std::string> outnames = this->m_BMIModule->GetOutputVarNames();
    if (m_NumOutputs < 1 || m_NumOutputs != outnames.size())
    {
        NMProcErr(<< "The number of output items does not match the number of "
                  << "output variables names!");
    }
    this->SetOutputNames(outnames);

    for (int oid=0; oid < m_NumOutputs; ++oid)
    {
        if (oid >= this->GetNumberOfIndexedOutputs())
        {
            this->SetNthOutput(oid, this->MakeOutput(oid));
        }
    }

    // init the number of output images
    this->m_NumOuputImages = m_NumOutputs;

    double_t* auxVar = static_cast<double_t*>(m_BMIModule->GetValuePtr(m_AuxVarAr_Name));
    if (auxVar != nullptr)
    {
        time_t timestamp;
        struct tm* timeinfo;
        time(&timestamp);
        timeinfo = localtime(&timestamp);
        static char curTime[128];
        sprintf(curTime, "%.2d-%.2d-%.2dT%.2d-%.2d-%.2d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec);
        std::string timeStr = curTime;

        // establish admin data structures
        const int gid = this->m_BMIModule->GetVarGrid(m_AuxVarAr_Name);
        if (gid >= 0)
        {
            m_AuxTable = otb::SQLiteTable::New();
            m_AuxTable->SetUseSharedCache(false);
            std::string tabName = m_WorkspacePath + "/" + this->m_BMIModule->mBMIClass + "_" +
                                  m_AuxVarAr_Name + "_stats_" + timeStr + ".ldb";
            if (m_AuxTable->CreateTable(tabName) == otb::SQLiteTable::ATCREATE_ERROR)
            {
                itkExceptionMacro(<< "Failed to create '" << tabName << "'!");
            }
            this->SetOutput("AuxTab", m_AuxTable);
            this->SetNthOutput(m_NumOutputs, m_AuxTable);
            this->m_AuxVarDataIndex = m_NumOutputs;
        }
    }
    else
    {
        this->m_AuxVarDataIndex = -1;
    }

    this->Modified();
}


template <class TInputImage, class TOutputImage>
TOutputImage* BMIModelFilter<TInputImage, TOutputImage>
::GetOutputByName(const std::string &name)
{
    OutputImageType* img = nullptr;
    for (int n=0; n < this->m_OutputNames.size(); ++n)
    {
        if (m_OutputNames[n].compare(name) == 0)
        {
            img = this->GetOutput(n);
            break;
        }
    }
    return img;
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input)
{
    InputImageType* img = dynamic_cast<InputImageType*>(input);

    if (img)
    {
        int idx = num >= this->GetNumberOfIndexedInputs() ? this->GetNumberOfIndexedInputs(): num;
        if (idx > m_InputNames.size()-1)
        {
            std::stringstream imgnamestr;
            imgnamestr << "img" << idx;
            NMProcWarn(<< "Input:" << idx << " has no name assigned to it, so we'll use '"
                       << imgnamestr.str() << "' instead!");
            m_InputNames.push_back(imgnamestr.str());
        }

        Superclass::SetNthInput(idx, input);

    }
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::CheckInputDataCongruence(void)
{
    int fstImg = 1;
    typename InputImageType::SizeValueType refSize[TInputImage::ImageDimension];
    for (int i=0; i < this->GetNumberOfIndexedInputs(); ++i)
    {
        InputImageType* img = dynamic_cast<InputImageType*>(this->GetIndexedInputs().at(i).GetPointer());
        if (img != 0)
        {
            if (fstImg == 1)
            {
                for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
                {
                    refSize[d] = img->GetLargestPossibleRegion().GetSize(d);
                }
                fstImg = 0;
            }
            else
            {
                for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
                {
                    if (refSize[d] != img->GetLargestPossibleRegion().GetSize(d))
                    {
                        NMProcErr(<< "Input images don't have the same size!");
                    }
                }
            }
        }
    }
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::PrepareNeighbourhoodProcessing(void)
{
    // if we've got a shaped neighbourhood iterator,
    // determine the active offsets
    m_ActiveKernelIndices.clear();
    m_ActiveKernelIndices.resize(m_NumNeighbourPixel);
    if (m_NumNeighbourPixel)
    {
        typedef itk::Neighborhood<int, TInputImage::ImageDimension> NeighbourhoodType;
        NeighbourhoodType neigh;
        neigh.SetRadius(m_KernelRadius);

        typename NeighbourhoodType::OffsetType offset;
        int circ = 0;
        for (int p=0; p < m_NumNeighbourPixel; ++p)
        {
            offset = neigh.GetOffset(static_cast<typename NeighbourhoodType::NeighborIndexType>(p));
            double d2 = 0;
            for (int d=0; d < offset.GetOffsetDimension(); ++d)
            {
                d2 += ((offset[d]*m_Spacing[d]) * (offset[d]*m_Spacing[d]));
            }

            // calucate distances to pixel centre (in pixel)
            // and define cicular neighbourhood, if applicable
            const double dist = ::sqrt(d2);

            // note: for circular neighborhood, we create a square
            // neighborhood first, s. ::GenerateInputRequestedRegion(),
            // so that's why we can just test against the radius of the
            // first dimension

            // NOTE: the circular neighbourhood assumes, we've got
            //       square shaped pixels!

            if (m_KernelShape == "CIRCULAR")
            {
                if  (dist <= (m_KernelRadius[0] * m_Spacing[0]))
                {
                    m_ActiveKernelIndices[circ++] = p;
                    m_NeighbourDistance.push_back(static_cast<OutputImagePixelType>(dist));
                }
            }
            else
            {
                m_ActiveKernelIndices[p] = p;
                m_NeighbourDistance.push_back(static_cast<OutputImagePixelType>(dist));
            }
        }

        if (circ > 0)
        {
            m_ActiveKernelIndices.resize(circ);
        }

        m_ActiveNeighborhoodSize = m_ActiveKernelIndices.size();
        m_CentrePixelIndex = (m_ActiveNeighborhoodSize-1) / 2;
    }
    else
    {
        m_NeighbourDistance.push_back(static_cast<OutputImagePixelType>(0));
        m_CentrePixelIndex = 0;
        m_ActiveNeighborhoodSize = 1;
    }

}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
    if (!this->m_IsStreamable)
    {
        InputImageType* img = nullptr;
        for (int i=0; i < this->GetNumberOfIndexedInputs(); ++i)
        {
            img = dynamic_cast<InputImageType*>(
                        this->GetIndexedInputs().at(i).GetPointer());
            if (img != nullptr)
            {
                img->SetRequestedRegionToLargestPossibleRegion();
            }
        }
    }
    else
    {
        Superclass::GenerateInputRequestedRegion();

        long long nPixels = -1;
        InputImageType* inputPtr = nullptr;

        int cnt = 0;
        while (nPixels < 0 && cnt < this->GetNumberOfIndexedInputs())
        {
            inputPtr = dynamic_cast<InputImageType*>(
                            this->GetIndexedInputs().at(cnt).GetPointer());
            if (inputPtr != nullptr)
            {
                nPixels = inputPtr->GetLargestPossibleRegion().GetNumberOfPixels();
                m_Spacing = inputPtr->GetSignedSpacing(); //GetGetSpacing();
                m_Origin = inputPtr->GetOrigin();
            }
            ++cnt;
        }

        // if we don't want to process pixel neighbourhoods,
        // we don't need to pad the input stream region
        if (m_KernelShape.compare("NO_KERNEL") == 0)
        {
            return;
        }

        // when we're working on a circular neighbourhood,
        // we make sure the fetched neighbourhood is square
        // rather than only rectangular; thereby, we're taking
        // the biggest radius across all dimension to determine
        // the size of the square;
        if (m_KernelShape.compare("CIRCULAR") == 0)
        {
            int maxRadius = 0;
            for (int d=0; d < m_KernelRadius.GetSizeDimension(); ++d)
            {
                maxRadius = m_KernelRadius[d] > maxRadius ? m_KernelRadius[d] : maxRadius;
            }
            for (int d=0; d < m_KernelRadius.GetSizeDimension(); ++d)
            {
                m_KernelRadius[d] = maxRadius;
            }
        }

        // determine kernel size
        m_NumNeighbourPixel = 1;
        for (int d=0; d < m_KernelRadius.GetSizeDimension(); ++d)
        {
            if (m_KernelRadius[d] > 0)
            {
                m_NumNeighbourPixel *= (m_KernelRadius[d] * 2 + 1);
            }
        }
        m_NumNeighbourPixel = m_NumNeighbourPixel == 1 ? 0 : m_NumNeighbourPixel;


        // no need to pad the input requested region,
        // if we're not operating on a kernel
        if (m_NumNeighbourPixel == 0)
        {
            return;
        }

        for (int ip=0; ip < this->GetNumberOfIndexedInputs(); ++ip)
        {
            inputPtr = dynamic_cast<InputImageType*>(
                        this->GetIndexedInputs().at(ip).GetPointer());

            if (inputPtr == 0)
            {
                continue;
            }

            // get a copy of the input requested region (should equal the output
            // requested region)
            typename TInputImage::RegionType inputRequestedRegion;
            inputRequestedRegion = inputPtr->GetRequestedRegion();

            // pad the input requested region by the operator radius
            SizeType radius;
            for (int r=0; r < m_KernelRadius.GetSizeDimension(); ++r)
            {
                radius[r] = m_KernelRadius[r];
            }
            inputRequestedRegion.PadByRadius( radius );

            // crop the input requested region at the input's largest possible region
            if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
            {
                inputPtr->SetRequestedRegion( inputRequestedRegion );
            }
            else
            {
                // Couldn't crop the region (requested region is outside the largest
                // possible region).  Throw an exception.

                // store what we tried to request (prior to trying to crop)
                inputPtr->SetRequestedRegion( inputRequestedRegion );

                // build an exception
                itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
                e.SetLocation(ITK_LOCATION);
                e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
                e.SetDataObject(inputPtr);
                throw e;
            }
        }
    }
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::SetBMIImageValue(const std::string &bmiName, const std::type_index typeInfo,
                   size_t *numPixel, size_t *rank, size_t* shape,
                   SpacingValueType* spacing, OriginValueType* origin, void* buf)
{
    const std::string bmiTypeName = bmiName + " type";
    const std::string bmiItemSizeName = bmiName + " itemsize";
    const std::string bmiGridSizeName = bmiName + " gridsize";
    const std::string bmiGridSpacingName = bmiName + " gridspacing";
    const std::string bmiGridRankName = bmiName + " gridrank";
    const std::string bmiGridShapeName = bmiName + " gridshape";
    const std::string bmiGridOriginName = bmiName + " gridorigin";

    // spciyfing the rank property first as PyBMIModel is using that info to
    // size the value arrays (ie whether for region or image buffer)
    this->m_BMIModule->SetValue(bmiGridRankName, static_cast<void*>(rank));

    // setting type info only for image buffers, not for processing region information
    if (buf != nullptr)
    {
        std::string tn;
        if (typeInfo.hash_code() == typeid(float).hash_code())
        {
            tn = "float";
            this->m_BMIModule->SetValue(bmiTypeName, static_cast<void*>(const_cast<char*>(tn.c_str())));

            size_t fsize = sizeof(float);
            this->m_BMIModule->SetValue(bmiItemSizeName, static_cast<void*>(&fsize));
        }
        else if (typeInfo.hash_code() == typeid(double).hash_code())
        {
            tn = "double";
            this->m_BMIModule->SetValue(bmiTypeName, static_cast<void*>(const_cast<char*>(tn.c_str())));

            size_t dsize = sizeof(double);
            this->m_BMIModule->SetValue(bmiItemSizeName, static_cast<void*>(&dsize));
        }
        else if (typeInfo.hash_code() == typeid(int).hash_code())
        {
            tn = "int";
            this->m_BMIModule->SetValue(bmiTypeName, static_cast<void*>(const_cast<char*>(tn.c_str())));

            size_t isize = sizeof(int);
            this->m_BMIModule->SetValue(bmiItemSizeName, static_cast<void*>(&isize));
        }
        else if (typeInfo.hash_code() == typeid(long).hash_code())
        {
            tn = "long";
            this->m_BMIModule->SetValue(bmiTypeName, static_cast<void*>(const_cast<char*>(tn.c_str())));

            size_t lsize = sizeof(long);
            this->m_BMIModule->SetValue(bmiItemSizeName, static_cast<void*>(&lsize));
        }
        else if (typeInfo.hash_code() == typeid(long long).hash_code())
        {
            tn = "long long";
            this->m_BMIModule->SetValue(bmiTypeName, static_cast<void*>(const_cast<char*>(tn.c_str())));

            size_t llsize = sizeof(long long);
            this->m_BMIModule->SetValue(bmiItemSizeName, static_cast<void*>(&llsize));
        }
    }

    this->m_BMIModule->SetValue(bmiGridSizeName, static_cast<void*>(numPixel));
    this->m_BMIModule->SetValue(bmiGridShapeName, static_cast<void*>(shape));
    this->m_BMIModule->SetValue(bmiGridSpacingName, static_cast<void*>(spacing));
    this->m_BMIModule->SetValue(bmiGridOriginName, static_cast<void*>(origin));
    this->m_BMIModule->SetValue(bmiName, buf);
}


template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::ConnectData(const OutputImageRegionType & outputWorkRegion)
{
    // populate settings for larget possible region and stream region
    this->SetBMIRegionValue("LPR", &m_NumLPRPixels, &m_ImageRegionDimension,
                            m_LPRSize, m_LPRIndex);

    this->SetBMIRegionValue("SR", &m_NumStreamRegPixels, &m_ImageRegionDimension,
                            m_StreamRegSize, m_StreamRegIndex);


    // process image admin info last as it relies in part on the
    // correct dimension information provided by the region admin info
    std::vector<std::string> bmiInputNames = this->m_BMIModule->GetInputVarNames();
    m_InputNumPix.clear();
    for (int in=0; in < m_InputNames.size(); ++in)
    {
        if (std::find(bmiInputNames.begin(), bmiInputNames.end(), m_InputNames.at(in)) != bmiInputNames.end())
        {
            InputImageType* inImg = const_cast<InputImageType*>(this->GetInput(in));
            InputImageRegionType bufReg = inImg->GetBufferedRegion();
            m_InputNumPix.push_back(bufReg.GetNumberOfPixels());
            InputImagePixelType* inbuf = inImg->GetBufferPointer();
            const std::type_index vtypeInfo = typeid(InputImagePixelType);

            /// ToDo: double check: do we need a static_cast<void*>(inbuf) here instead?
            this->SetBMIImageValue(m_InputNames.at(in), vtypeInfo,
                              &m_InputNumPix[in], &m_ImageBufferDimension,
                              &m_InputNumPix[in], m_LPRSpacing, m_LPROrigin,
                              static_cast<void*>(inbuf));
        }
        else
        {
            NMProcErr(<< "Sorry, but the BMI module is actually not looking for "
                      << "an input such as '" << m_InputNames.at(in) << "'!");
            return;
        }
    }

    std::vector<std::string> outNames = this->m_BMIModule->GetOutputVarNames();
    m_OutputNumPix.clear();
    for (int out=0; out < outNames.size(); ++out)
    {
        // only set output buffers for those variables that are not inputs
        // at the same time because have been set already in 'input loop' above
        if (std::find(bmiInputNames.begin(), bmiInputNames.end(), outNames.at(out)) == bmiInputNames.end())
        {
            if (out < this->GetNumberOfOutputs())
            {
                OutputImageType* outImg = this->GetOutput(out);
                OutputImageRegionType outReg = outImg->GetBufferedRegion();
                m_OutputNumPix.push_back(outReg.GetNumberOfPixels());
                OutputImagePixelType* outbuf = outImg->GetBufferPointer();
                const std::type_index outTypeInfo = typeid(OutputImagePixelType);

                this->SetBMIImageValue(outNames.at(out), outTypeInfo,
                                  &m_OutputNumPix[out], &m_ImageBufferDimension,
                                  &m_OutputNumPix[out], m_LPRSpacing, m_LPROrigin,
                                  static_cast<void*>(outbuf));
            }
        }
    }

    this->m_BMIModule->SetValue("kernel_int_data", static_cast<void*>(m_AuxIntData.data()));
    this->m_BMIModule->SetValue("kernel_int_data gridrank", static_cast<void*>(&m_ImageBufferDimension));
    this->m_BMIModule->SetValue("kernel_int_data gridsize", static_cast<void*>(&m_AuxIntDataSize));
    this->m_BMIModule->SetValue("kernel_int_data gridshape", static_cast<void*>(&m_AuxIntDataSize));

    this->m_BMIModule->SetValue("kernel_double_data", static_cast<void*>(m_AuxDoubleData.data()));
    this->m_BMIModule->SetValue("kernel_double_data gridrank", static_cast<void*>(&m_ImageBufferDimension));
    this->m_BMIModule->SetValue("kernel_double_data gridsize", static_cast<void*>(&m_AuxDoubleDataSize));
    this->m_BMIModule->SetValue("kernel_double_data gridshape", static_cast<void*>(&m_AuxDoubleDataSize));
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::SetBMIRegionValue(const std::string& bmiName, size_t* numPixel,
                    size_t* rank, uint64_t *shape, int64_t *index)
{
    const std::string bmiRegionRankName = bmiName  + " gridrank";
    const std::string bmiRegionSizeName = bmiName  + " gridsize";
    const std::string bmiRegionShapeName = bmiName + " gridshape";
    const std::string bmiRegionIndexName = bmiName + " gridindex";
    const std::string bmiRegionSpacingName = bmiName + " gridspacing";

    // add the 'value buffer' first to 'register' the region name
    // There's no real value array for the regions! We just set a dummy value here
    // so the number of items in all 'admin' dictionaries in the BMI class are the same
    // (as we this is important for retreiving the appropriate 'proper' image
    // value array through the BMI interface!
    this->m_BMIModule->SetValue(bmiName, static_cast<void*>(&m_RegionValueType));


    // we provide the rank information before other attributes as it will inform
    // the size of 1D arrays for the other properties to be set up
    this->m_BMIModule->SetValue(bmiRegionRankName, static_cast<void*>(rank));
    this->m_BMIModule->SetValue(bmiRegionSizeName, static_cast<void*>(numPixel));
    this->m_BMIModule->SetValue(bmiRegionShapeName, static_cast<void*>(shape));
    this->m_BMIModule->SetValue(bmiRegionIndexName, static_cast<void*>(index));
    this->m_BMIModule->SetValue(bmiRegionSpacingName, static_cast<void*>(m_LPRSpacing));
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::AllocateOutputs()
{
    // double check whether variables are input and output at the same time,
    // i.e. are going to be processed in-place (overwritten); those vars are
    // don't have to be allocated again and are grafted onto  the output
    // instead

    std::vector<std::string> bmiInputNames = this->m_BMIModule->GetInputVarNames();
    std::vector<std::string> outNames = this->m_BMIModule->GetOutputVarNames();
    for (int i=0; i < outNames.size(); ++i)
    {
        auto it = std::find(bmiInputNames.begin(), bmiInputNames.end(), outNames.at(i));
        if (it == bmiInputNames.end())
        {
            OutputImageType* outImg = this->GetOutput(i);
            outImg->SetBufferedRegion(outImg->GetRequestedRegion());
            outImg->Allocate();
        }
    }
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::GenerateData(void)
{
    // throws exception if input layers
    // don't have the same size
    this->CheckInputDataCongruence();
    this->AllocateOutputs();
    if (    m_KernelShape.compare("NO_KERNEL") != 0
         && m_NumNeighbourPixel >= 1
       )
    {
        this->PrepareNeighbourhoodProcessing();
    }

    // run the model
    if (this->m_BMIModule.get() == nullptr)
    {
        NMProcErr(<< "BMI module is NULL!");
        BMIModelException be("BMI module is NULL!");
        throw be;
        return;
    }


    // -----------------------------------------------
    // define some region and other info

    OutputImageType* out = this->GetOutput(0);
    m_StreamRegion = out->GetRequestedRegion();
    m_NumStreamRegPixels = m_StreamRegion.GetNumberOfPixels();
    m_LargestPossibleRegion = out->GetLargestPossibleRegion();
    m_NumLPRPixels = m_LargestPossibleRegion.GetNumberOfPixels();
    m_ImageRegionDimension = OutputImageType::ImageDimension;

    for (int d=0; d < m_ImageRegionDimension; ++d)
    {
        m_LPRSize[d] = m_LargestPossibleRegion.GetSize(d);
        m_LPRIndex[d] = m_LargestPossibleRegion.GetIndex(d);
        m_LPRSpacing[d] = out->GetSpacing()[d];
        m_LPROrigin[d] = out->GetOrigin()[d];

        m_StreamRegIndex[d] = m_StreamRegion.GetIndex(d);
        m_StreamRegSize[d] = m_StreamRegion.GetSize(d);
    }

    this->ConnectData(m_StreamRegion);

    try
    {
        // get a pointer to the kernel function, if required
        if (    m_KernelShape.compare("NO_KERNEL") != 0
             && m_NumNeighbourPixel > 0
           )
        {

            // get kernel function pointer
            py::module_ module = py::module_::import(m_BMIModule->mPyModuleName.c_str());
            py::object addr = module.attr(m_KernelFuncName.c_str()).attr("address");
            if (addr.is_none())
            {
                BMIModelException be((m_KernelFuncName + " callback function is not defined!").c_str());
                throw be;
            }
            m_KernelFunc = (kfunc_type)addr.cast<uint64_t>();
        }
    }
    catch (py::cast_error& ce)
    {
        NMProcErr(<< ce.what());
        itkExceptionMacro(<< ce.what());
    }
    catch (py::error_already_set& eas)
    {
        NMProcErr(<< eas.what());
        itkExceptionMacro(<< eas.what());
    }
    catch (std::exception& se)
    {
        NMProcErr(<< se.what());
        itkExceptionMacro(<< se.what());
    }
    catch(...)
    {
        NMProcErr(<< "Unknown error in PyBMIModel!");
        itkExceptionMacro(<< "Unknown error in PyBMIModel!");
    }

    // ----------------------------------------------
    // note this function prepares internal data used
    // for both sequential and/or multi-threaded processing
    this->BeforeThreadedGenerateData();

    // ----------------------------------------------
    //multi- or single-threaded implementation

    if (this->m_KernelFunc != nullptr && this->m_IsThreadable)
    {
        ThreadStruct str;
        str.Filter = this;

        // Get an output image pointer
        const OutputImageType *outputPtr = this->GetOutput();
        const itk::ImageRegionSplitterBase * splitter = this->GetImageRegionSplitter();
        const unsigned int validThreads = splitter->GetNumberOfSplits( outputPtr->GetRequestedRegion(), this->GetNumberOfThreads() );

        this->GetMultiThreader()->SetNumberOfThreads( validThreads );
        this->GetMultiThreader()->SetSingleMethod(this->ThreaderCallback, &str);

        // multithread the execution
        this->GetMultiThreader()->SingleMethodExecute();
    }
    else
    {
        this->SingleThreadedGenerateData();
    }

    // keep track of how much of the image(s) has been processed already
    m_PixCount += m_NumStreamRegPixels;

    // ----------------------------------------------
    // evaluate 'persistent' data generated during
    // sequential and/or multithreaded processing
    this->AfterThreadedGenerateData();
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SingleThreadedGenerateData(void)
{
    // here we rely on parallel processing implemented in the
    // model itself

    if (    m_KernelShape.compare("NO_KERNEL") == 0
         || m_NumNeighbourPixel <= 1
         || m_KernelFunc == nullptr
       )
    {
        NMProcInfo(<< "::Update() ...");
        this->m_BMIModule->Update();
        NMProcInfo(<< "::Update() - done!");
    }
    else if (m_KernelFunc != nullptr)
    {
        this->RunKernelFunc(m_StreamRegion, 0);
    }
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::RunKernelFunc(const OutputImageRegionType &outputRegionForThread, itk::ThreadIdType threadId)
{
    //NMProcInfo(<< "::RuKernelFunc() ...");
    try
    {
        // ----------------------------------
        // setup image/kernel iterators
        const uint64_t numPixel = outputRegionForThread.GetNumberOfPixels();
        const int numInputs = this->GetNumberOfIndexedInputs();
        const int numOutputs = m_NumOuputImages;

        const InputImageType* inImg = this->GetInput(0);

        itk::ProgressReporter progress(this, threadId, numPixel);

        std::vector<InputImageType*> inImgVec;
        std::vector<OutputImageType*> outImgVec;
        std::vector<InputShapedIterator> inIterVec;
        std::vector<OutputRegionIterator> outIterVec;

        itk::ZeroFluxNeumannBoundaryCondition<InputImageType> nbc;
        typedef typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType FaceListType;
        typedef typename FaceListType::iterator FaceListIteratorType;
        itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
        FaceListType faceList = bC(inImg, outputRegionForThread, m_KernelRadius);
        FaceListIteratorType fit;

        OutputImagePixelType outbuf[numOutputs];
        InputImagePixelType* nhbufCont[numInputs];
        NeighborhoodType nhCont[numInputs];
        int64_t outPixIndex[TInputImage::ImageDimension];
        for (int in=0; in < numInputs; ++in)
        {
            inImgVec.push_back(const_cast<InputImageType*>(this->GetInput(in)));
            inIterVec.push_back(InputShapedIterator());
        }
        for (int out=0; out < numOutputs; ++out)
        {
            outImgVec.push_back(const_cast<OutputImageType*>(this->GetOutput(out)));
            outIterVec.push_back(OutputRegionIterator());
        }

        // process boundary faces
        for (fit = faceList.begin(); fit != faceList.end() && !this->GetAbortGenerateData(); ++fit)
        {
            for (int in=0; in < numInputs; ++in)
            {
                inIterVec[in] = InputShapedIterator(m_KernelRadius, inImgVec[in], *fit);
                inIterVec[in].OverrideBoundaryCondition(&nbc);
                inIterVec[in].SetActiveIndexList(m_ActiveKernelIndices);
                inIterVec[in].GoToBegin();
            }
            for (int out=0; out < numOutputs; ++out)
            {
                outIterVec[out] = OutputRegionIterator(outImgVec[out], *fit);
                outIterVec[out].GoToBegin();
            }

            while (!(outIterVec[0].IsAtEnd()) && !this->GetAbortGenerateData())
            {
                for (int in=0; in < numInputs; ++in)
                {
                    nhCont[in] = inIterVec[in].GetNeighborhood();
                    nhbufCont[in] = &nhCont[in].GetBufferReference()[0];
                }
                for (int d=0; d < TInputImage::ImageDimension; ++d)
                {
                    outPixIndex[d] = static_cast<int64_t>(outIterVec[0].GetIndex()[d]);
                }

                // call kernel callback function
                m_KernelFunc(TInputImage::ImageDimension, numInputs, numOutputs, m_NumNeighbourPixel,
                             m_AuxIntData.size(), m_AuxDoubleData.size(), m_AuxVarArLen,
                             m_LPRSize, m_LPRSpacing, outPixIndex,
                             nhbufCont, outbuf,
                             m_AuxIntData.data(), m_AuxDoubleData.data(), m_vthAuxVarAr[threadId].data());

                // store aux variable stats
                for (int l=0; l < m_AuxVarArLen; ++l)
                {
                    m_vthAuxVarValMin[threadId][l] = std::min(m_vthAuxVarValMin[threadId][l], m_vthAuxVarAr[threadId][l]);
                    m_vthAuxVarValMax[threadId][l] = std::max(m_vthAuxVarValMax[threadId][l], m_vthAuxVarAr[threadId][l]);
                    m_vthAuxVarValSum[threadId][l] += m_vthAuxVarAr[threadId][l];
                    m_vthAuxVarValSum2[threadId][l] += (m_vthAuxVarAr[threadId][l] * m_vthAuxVarAr[threadId][l]);
                }

                for (int out=0; out < numOutputs; ++out)
                {
                    outIterVec[out].Set(outbuf[out]);
                    ++outIterVec[out];
                }

                for (int in=0; in < numInputs; ++in)
                {
                    ++inIterVec[in];
                }
                progress.CompletedPixel();
            }
        }
    }
    catch (std::exception& se)
    {
        NMProcErr(<< se.what());
        itkExceptionMacro(<< se.what());
    }
    catch(...)
    {
        NMProcErr(<< "Unknown error in PyBMIModel!");
        itkExceptionMacro(<< "Unknown error in PyBMIModel!");
    }
}


template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetKernelRadius(std::vector<int> radius)
{
    //if (m_KernelShape != "NO_KERNEL")
    {
        if (radius.size() == TInputImage::ImageDimension)
        {
            for (int r=0; r < radius.size(); ++r)
            {
                m_KernelRadius[r] = radius[r];
            }
        }
        else
        {
            NMProcWarn(<< "The provided kernel radius does not have the same number of dimensions as the input image(s)!");
        }
    }
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::ResetPipeline(void)
{
    m_PixCount = 0;
    m_NumNeighbourPixel = 0;
    m_BMIModule = nullptr;
    m_KernelFunc = nullptr;
    m_AuxDoubleData.clear();
    m_AuxDoubleDataSize = 0;
    m_AuxIntData.clear();
    m_AuxIntDataSize = 0;

    m_vthAuxVarAr.clear();
    m_vthAuxVarValMin.clear();
    m_vthAuxVarValMax.clear();
    m_vthAuxVarValSum.clear();

}

template <class TInputImage, class TOutputImage>
ITK_THREAD_RETURN_TYPE
BMIModelFilter<TInputImage, TOutputImage>
::ThreaderCallback( void *arg )
{
    ThreadStruct *str;
    itk::ThreadIdType  total, threadId, threadCount;

    threadId = ( (itk::MultiThreader::ThreadInfoStruct *)( arg ) )->ThreadID;
    threadCount = ( (itk::MultiThreader::ThreadInfoStruct *)( arg ) )->NumberOfThreads;

    str = (ThreadStruct *)( ( (itk::MultiThreader::ThreadInfoStruct *)( arg ) )->UserData );

    // execute the actual method with appropriate output region
    // first find out how many pieces extent can be split into.
    typename TOutputImage::RegionType splitRegion;
    total = str->Filter->SplitRequestedRegion(threadId, threadCount,
                                              splitRegion);

    if ( threadId < total )
    {
        str->Filter->ThreadedGenerateData(splitRegion, threadId);
    }

    return ITK_THREAD_RETURN_VALUE;
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData(void)
{
    // get a pointer to the user-defined array ('auxVarAr') storing persistent
    // ('global') auxilliary variables whose values are summarised for the
    // whole image (i.e. not just a kernel or image chunk). The summary values
    // vor those variables will be their max, min, mean, and standard deviation
    // statistics across kernels and processing regions
    if (m_BMIModule == nullptr)
    {
        return;
    }
    std::string pyListErrMsg =
            "Please provide 'auxVarNames' as Python List (e.g. settings['auxVarNames'] = ['var1', 'var2'])";
    double_t* auxVar = static_cast<double_t*>(m_BMIModule->GetValuePtr(m_AuxVarAr_Name));
    //if (auxVar != nullptr)
    //{
        // establish admin data structures
        const int gid = this->m_BMIModule->GetVarGrid(m_AuxVarAr_Name);
        m_AuxVarArLen = 0;
        if (gid >= 0)
        {
            m_AuxVarArLen = this->m_BMIModule->GetGridSize(gid);
        }

            //if (m_AuxVarArLen > 0)
            //{
                const int numThreads = this->GetNumberOfThreads();
                py::dict settings = this->m_BMIModule->mPyObject.attr("settings");
                if (    settings.is_none()
                     || settings.ptr() == nullptr
                   )
                {
                    itkExceptionMacro(<< pyListErrMsg);
                }

                for (auto item : settings)
                {
                    std::string key = item.first.cast<std::string>();
                    if (key.compare(m_AuxVarNames_Name) == 0)
                    {
                        if (py::isinstance<py::list>(item.second))
                        {
                            for (auto name : item.second)
                            {
                                if (py::isinstance<py::str>(name))
                                {
                                    m_AuxVarNames.push_back(name.cast<std::string>());
                                }
                            }
                        }
                        else if (py::isinstance<py::array>(item.second))
                        {
                            itkExceptionMacro(<< pyListErrMsg);
                        }
                        else if (py::isinstance<py::tuple>(item.second))
                        {
                            itkExceptionMacro(<< pyListErrMsg);
                        }
                        else if (py::isinstance<py::buffer>(item.second))
                        {
                            itkExceptionMacro(<< pyListErrMsg);
                        }
                    }
                }

                if (m_PixCount == 0)
                {
                    for (int t=0; t < numThreads + 1; ++t)
                    {
                        std::vector<double_t> val;
                        std::vector<double_t> minVal;
                        std::vector<double_t> maxVal;
                        std::vector<double_t> sumVal;
                        std::vector<double_t> sumVal2;
                        for (int l=0; l < m_AuxVarArLen; ++l)
                        {
                            if (t < numThreads)
                            {
                                val.push_back(auxVar[l]);
                            }
                            minVal.push_back(itk::NumericTraits<double_t>::max());
                            maxVal.push_back(itk::NumericTraits<double_t>::NonpositiveMin());
                            sumVal.push_back(itk::NumericTraits<double_t>::ZeroValue());
                            sumVal2.push_back(itk::NumericTraits<double_t>::ZeroValue());
                        }
                        if (t < numThreads)
                        {
                            m_vthAuxVarAr.push_back(val);
                        }
                        m_vthAuxVarValMin.push_back(minVal);
                        m_vthAuxVarValMax.push_back(maxVal);
                        m_vthAuxVarValSum.push_back(sumVal);
                        m_vthAuxVarValSum2.push_back(sumVal2);
                    }
                }
                // re-init min, max, and zero to 'neutral' starting values
                else if (m_PixCount < m_NumLPRPixels)
                {
                    for (int th=0; th < numThreads; ++th)
                    {
                        for (int len=0; len < m_AuxVarArLen; ++len)
                        {
                            m_vthAuxVarValMin[th][len] = itk::NumericTraits<double_t>::max();
                            m_vthAuxVarValMax[th][len] = itk::NumericTraits<double_t>::NonpositiveMin();
                            m_vthAuxVarValSum[th][len] = itk::NumericTraits<double_t>::ZeroValue();
                            m_vthAuxVarValSum2[th][len] = itk::NumericTraits<double_t>::ZeroValue();
                        }
                    }
                }
            //}
        //}
        //else
        //{
        //    m_AuxVarArLen = 0;
        //}
    //}
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                          itk::ThreadIdType threadId)
{
    this->RunKernelFunc(outputRegionForThread, threadId);
}


template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::AfterThreadedGenerateData(void)
{
    if (m_AuxVarArLen == 0)
    {
        ResetPipeline();
        return;
    }

    // sum thread / stream region values
    const int numThreads = this->GetNumberOfThreads();
    const int total = numThreads;

    // adding the overall thread totals!
    for (int t=0; t < numThreads; ++t)
    {
        for (int l=0; l < m_AuxVarArLen; ++l)
        {
            m_vthAuxVarValMin[total][l] = std::min(m_vthAuxVarValMin[total][l], m_vthAuxVarValMin[t][l]);
            m_vthAuxVarValMax[total][l] = std::max(m_vthAuxVarValMax[total][l], m_vthAuxVarValMax[t][l]);
            m_vthAuxVarValSum[total][l] += m_vthAuxVarValSum[t][l];
            m_vthAuxVarValSum2[total][l] += m_vthAuxVarValSum2[t][l];
        }
    }

    // write stats into the AuxData table
    if (m_PixCount < m_NumLPRPixels)
    {
        return;
    }

    std::vector<double_t> auxVarMean;
    std::vector<double_t> auxVarStDev;
    for (int n=0; n < m_AuxVarArLen; ++n)
    {
        const double_t sum_val2 = m_vthAuxVarValSum2[total][n];
        const double_t sum_val = m_vthAuxVarValSum[total][n];
        const double_t mean = sum_val / m_NumLPRPixels;
        const double_t sd = ::sqrt((sum_val2 / m_NumLPRPixels) - (mean * mean));

        auxVarMean.push_back(mean);
        auxVarStDev.push_back(sd);
    }

    std::vector< double_t > stats[] = {
        m_vthAuxVarValMin[total],
        m_vthAuxVarValMax[total],
        auxVarMean,
        auxVarStDev
    };

    // ----------------------------------------------------
    // create db fiels
    otb::SQLiteTable::Pointer auxTab = static_cast<otb::SQLiteTable*>(
                this->GetIndexedOutputs()[m_NumOutputs].GetPointer());

    if (    !auxTab.IsNotNull()
         || auxTab->GetDbConnection() == nullptr
       )
    {
        return;
    }
    auxTab->BeginTransaction();

    std::vector< std::string > colnames;
    colnames.push_back("statistic");
    auxTab->AddColumn(colnames[0], otb::AttributeTable::ATTYPE_STRING);
    std::vector< otb::AttributeTable::TableColumnType > coltypes;
    coltypes.push_back(otb::AttributeTable::ATTYPE_STRING);
    std::vector< otb::AttributeTable::ColumnValue > colvalues;
    otb::AttributeTable::ColumnValue sval;
    sval.type = otb::AttributeTable::ATTYPE_STRING;
    colvalues.push_back(sval);
    std::string statName[] = {"min", "max", "mean", "stdev"};

    for (int var=0; var < m_AuxVarArLen; ++var)
    {
        auxTab->AddColumn(m_AuxVarNames[var], otb::AttributeTable::ATTYPE_DOUBLE);
        colnames.push_back(m_AuxVarNames[var]);
        coltypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
        otb::AttributeTable::ColumnValue cval;
        cval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        colvalues.push_back(cval);
    }
    auxTab->EndTransaction();

    // -----------------------------------------------------
    // enter stats values
    auxTab->PrepareBulkSet(colnames, true);

    for (int s=0; s < 4; ++s)
    {
        char* sname = new char[statName[s].length()];
        ::sprintf(sname, "%s", statName[s].c_str());
        colvalues[0].tval = sname;
        for (int vl=1; vl < m_AuxVarArLen + 1; ++vl)
        {
            colvalues[vl].dval = stats[s][vl-1];
        }
        auxTab->DoBulkSet(colvalues);
    }

    // ------------------------
    // make sure everything is set back to square one!
    ResetPipeline();
}

} // end of namspace
#endif // end of include guard
