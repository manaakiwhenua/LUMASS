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
 * BMIModelFilter.h
 *
 *  Created on: 30/04/2020
 *      Author: alex
 */

#ifndef BMIModelFilter_H_
#define BMIModelFilter_H_

#define ctxBMIModelFilter "BMIModelFilter"

#include "nmlog.h"
#include <string>
#include <memory>
#include "bmi.hxx"

#include "itkImageToImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkMultiThreader.h"

#include "nmotbsupplfilters_export.h"

/**  Enables the integration of a pixel/point-based BMI-compliant model
 *   into an ITK/OTB processing pipeline, leveraging the sequential and
 *   parallel processing capacities
 */

namespace otb {

template <class TInputImage, class TOutputImage = TInputImage>
class NMOTBSUPPLFILTERS_EXPORT BMIModelFilter
        : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

    typedef BMIModelFilter							Self;
    typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
    typedef itk::SmartPointer<Self>								Pointer;
    typedef itk::SmartPointer<const Self>							ConstPointer;

    itkNewMacro(Self)
    itkTypeMacro(BMIModelFilter, itk::ImageToImageFilter)

    typedef TInputImage						InputImageType;
    typedef typename InputImageType::Pointer	InputImagePointer;
    typedef typename InputImageType::RegionType InputImageRegionType;
    typedef typename InputImageType::PixelType  InputImagePixelType;

    typedef TOutputImage						OutputImageType;
    typedef typename OutputImageType::Pointer	OutputImagePointer;
    typedef typename OutputImageType::RegionType OutputImageRegionType;
    typedef typename OutputImageType::PixelType  OutputImagePixelType;
    typedef typename OutputImageType::SizeValueType OutputImageSizeValueType;

    itkSetMacro(YamlConfigFileName, std::string)
    itkGetMacro(YamlConfigFileName, std::string)

    itkSetMacro(WrapperName, std::string)
    itkGetMacro(WrapperName, std::string)

    itkSetMacro(NumOutputs, unsigned int)
    itkGetMacro(NumOutputs, unsigned int)

    itkSetMacro(IsStreamable, bool)
    itkSetMacro(IsThreadable, bool)


    void SetBMIModule(const std::shared_ptr<bmi::Bmi>& bmiModule);
    void SetInputNames(const std::vector<std::string>& inputNames);
    void SetOutputNames(const std::vector<std::string>& outputNames)
        {m_OutputNames = outputNames;}

    OutputImageType* GetOutputByName(const std::string& name);

    /*! In case the filter needs a larger input requested region than
     * the output requested region, or we'll need to process the whole
     * lot at once!
     * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
    void GenerateInputRequestedRegion();

    void SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input);


protected:
    BMIModelFilter();
    virtual ~BMIModelFilter();
    void PrintSelf(std::ostream& os, itk::Indent indent) const;

    struct ThreadStruct
    {
        Pointer Filter;
    };

    void SetBMIValue(const std::string& bmiName, const std::type_index typeInfo, size_t numPixel, void* buf);

    void ConnectData(const OutputImageRegionType & outputWorkRegion);

    void AllocateOutputs();
    void ResetPipeline(void);
    void GenerateData(void);
    void BeforeThreadedGenerateData(void);
    void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                              itk::ThreadIdType threadId);
    void AfterThreadedGenerateData(void);
    void SingleThreadedGenerateData(void);


    static ITK_THREAD_RETURN_TYPE ThreaderCallback(void *arg);


    bool m_IsStreamable;
    bool m_IsThreadable;

    std::string m_YamlConfigFileName;

    std::vector<std::string> m_InputNames;
    std::vector<std::string> m_OutputNames;

    /*! for Python models:
     *  required to fetch the associated python module
     *  from the global map LumassPythonModuleMap */
    std::string m_WrapperName;

    std::shared_ptr<bmi::Bmi> m_BMIModule;

    unsigned int m_NumOutputs;
    unsigned long m_PixCount;
};

} // end namespace

//#include "otbBMIModelFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbBMIModelFilter.txx"
#endif

#endif /* BMIModelFilter_H_ */


