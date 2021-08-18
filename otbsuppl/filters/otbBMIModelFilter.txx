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
 *  notice reproduced below this section.
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
#include "itkImageRegionSplitterBase.h"
#include "itkImageRegionIterator.h"
#include "itkMultiThreader.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"
#include "itkExceptionObject.h"
#include "itkDataObject.h"
#include "itkImageSource.h"
#include "itkImportImageContainer.h"

#include <algorithm>
#include <typeindex>

namespace otb {

template <class TInputImage, class TOutputImage>
BMIModelFilter<TInputImage, TOutputImage>
::BMIModelFilter()
     : m_IsStreamable(true),
       m_IsThreadable(false),
       m_NumOutputs(1),
       m_PixCount(0)
{
    this->SetNumberOfRequiredOutputs(m_NumOutputs);
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

    this->Modified();
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SetBMIModule(const std::shared_ptr<bmi::Bmi>& bmiModule)
{
    if (bmiModule.get() == nullptr)
    {
        NMProcErr(<< "bmi::Bmi module is NULL!");
        return;
    }

    m_BMIModule = bmiModule;

    // set the number of outputs produced by
    // the configured BMI module
    const int outitems = this->m_BMIModule->GetOutputItemCount();
    if (outitems < 1 || outitems != this->m_BMIModule->GetOutputVarNames().size())
    {
        NMProcErr(<< "The number output items does not match the number of "
                  << "output variables names!");
    }

    this->SetNumberOfIndexedOutputs(outitems);
    for (int oid=1; oid < outitems; ++oid)
    {
        this->SetNthOutput(oid, this->MakeOutput(oid));
    }
    this->Modified();
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
    }
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::SetBMIValue(const std::string &bmiName, const std::type_index typeInfo, size_t numPixel, void* buf)
{
    const std::string bmiTypeName = bmiName + " type";
    const std::string bmiItemSizeName = bmiName + " itemsize";
    const std::string bmiGridSizeName = bmiName + " gridsize";
    const std::string bmiGridRankName = bmiName + " gridrank";
    const std::string bmiGridShapeName = bmiName + " gridshape";

    size_t gridRank = 1;
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

    this->m_BMIModule->SetValue(bmiGridSizeName, static_cast<void*>(&numPixel));
    this->m_BMIModule->SetValue(bmiGridShapeName, static_cast<void*>(&numPixel));
    this->m_BMIModule->SetValue(bmiGridRankName, static_cast<void*>(&gridRank));
    this->m_BMIModule->SetValue(bmiName, buf);
}


template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::ConnectData(const OutputImageRegionType & outputWorkRegion)
{
    std::vector<std::string> bmiInputNames = this->m_BMIModule->GetInputVarNames();
    for (int in=0; in < m_InputNames.size(); ++in)
    {
        if (std::find(bmiInputNames.begin(), bmiInputNames.end(), m_InputNames.at(in)) != bmiInputNames.end())
        {
            InputImageType* inImg = const_cast<InputImageType*>(this->GetInput(in));
            InputImageRegionType bufReg = inImg->GetBufferedRegion();
            const size_t numPixel = bufReg.GetNumberOfPixels();
            InputImagePixelType* inbuf = inImg->GetBufferPointer();
            const std::type_index vtypeInfo = typeid(InputImagePixelType);

            this->SetBMIValue(m_InputNames.at(in), vtypeInfo, numPixel, inbuf);
        }
        else
        {
            NMProcErr(<< "Sorry, but the BMI module is actually not looking for "
                      << "an input such as '" << m_InputNames.at(in) << "'!");
            return;
        }
    }

    std::vector<std::string> outNames = this->m_BMIModule->GetOutputVarNames();
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
                const size_t outNumPixel = outReg.GetNumberOfPixels();
                OutputImagePixelType* outbuf = outImg->GetBufferPointer();
                const std::type_index outTypeInfo = typeid(OutputImagePixelType);

                this->SetBMIValue(outNames.at(out), outTypeInfo, outNumPixel, static_cast<void*>(outbuf));
            }
        }
    }
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
    this->AllocateOutputs();
    
    ///TOOD: multi-threaded implementation
    /*
    if (this->m_IsThreadable)
    {
        this->BeforeThreadedGenerateData();

        ThreadStruct str;
        str.Filter = this;

        // Get the output pointer
        const OutputImageType *outputPtr = this->GetOutput();
        const itk::ImageRegionSplitterBase * splitter = this->GetImageRegionSplitter();
        const unsigned int validThreads = splitter->GetNumberOfSplits( outputPtr->GetRequestedRegion(), this->GetNumberOfThreads() );

        this->GetMultiThreader()->SetNumberOfThreads( validThreads );
        this->GetMultiThreader()->SetSingleMethod(this->ThreaderCallback, &str);

        // multithread the execution
        this->GetMultiThreader()->SingleMethodExecute();

        // Call a method that can be overridden by a subclass to perform
        // some calculations after all the threads have completed
        this->AfterThreadedGenerateData();
    }
    else
    */
    {
        this->SingleThreadedGenerateData();
    }
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::SingleThreadedGenerateData(void)
{
    // here we rely on parallel processing implemented in the
    // model itself

    OutputImageType* out = this->GetOutput(0);
    OutputImageRegionType outputRegion = out->GetRequestedRegion();

    // feed input data into the m_BMIModule
    this->ConnectData(outputRegion);

    // run the model
    this->m_BMIModule->Update();

    // fetch  the output data from the m_BMIModule
    using ImportContainerType = itk::ImportImageContainer<OutputImageSizeValueType, OutputImagePixelType>;
    using ImportContainerPointer = typename ImportContainerType::Pointer;

    std::vector<std::string> outnames = this->m_BMIModule->GetOutputVarNames();
    for(int i=0; i < outnames.size(); ++i)
    {
        out = this->GetOutput(i);
        out->SetBufferedRegion(out->GetRequestedRegion());

        const int gid = this->m_BMIModule->GetVarGrid(outnames[i]);
        const int gsize = this->m_BMIModule->GetGridSize(gid);
        OutputImagePixelType* bmibuf = static_cast<OutputImagePixelType*>(this->m_BMIModule->GetValuePtr(outnames[i]));

        // graft the output data from the m_BMIModule onto the output image
        ImportContainerPointer pixCont = ImportContainerType::New();
        pixCont->SetImportPointer(bmibuf, static_cast<OutputImageSizeValueType>(gsize), false);
        out->SetPixelContainer(pixCont);
    }
}

template <class TInputImage, class TOutputImage>
void
BMIModelFilter<TInputImage, TOutputImage>
::ResetPipeline(void)
{
    m_PixCount = 0;
    m_BMIModule = nullptr;
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
}

template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                          itk::ThreadIdType threadId)
{
    // awesome processing stuff goes in here ...
}


template <class TInputImage, class TOutputImage>
void BMIModelFilter<TInputImage, TOutputImage>
::AfterThreadedGenerateData(void)
{
    // mob up the bits and pieces ...

}

} // end namespace

#endif
