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
#include "pythonbmi.h"

#include "itkMultiThreader.h"
#include "itkImageToImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkNMConstShapedNeighborhoodIterator.h"
#include "itkNeighborhood.h"

#include "otbSQLiteTable.h"

#include "nmotbsupplfilters_export.h"

/**  Enables the integration of a pixel/point-based BMI-compliant model
 *   into an ITK/OTB processing pipeline, leveraging the sequential and
 *   parallel processing capabilities
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
    typedef typename InputImageType::PointType  OriginType;
    typedef typename InputImageType::PointValueType OriginValueType;
    typedef typename InputImageType::SpacingType SpacingType;
    typedef typename InputImageType::SpacingValueType SpacingValueType;
    typedef typename InputImageType::SizeType   SizeType;

    typedef TOutputImage						OutputImageType;
    typedef typename OutputImageType::Pointer	OutputImagePointer;
    typedef typename OutputImageType::RegionType OutputImageRegionType;
    typedef typename OutputImageType::PixelType  OutputImagePixelType;
    typedef typename OutputImageType::SizeValueType OutputImageSizeValueType;

    typedef typename itk::ConstNeighborhoodIterator<InputImageType> InputNeighborhoodIterator;
    typedef typename itk::ImageRegionIterator<OutputImageType> OutputRegionIterator;

    typedef itk::NeighborhoodAllocator<InputImagePixelType> InputNeighborhoodAllocType;
    typedef itk::Neighborhood<InputImagePixelType, InputImageType::ImageDimension, InputNeighborhoodAllocType> InputNeighborhoodType;

    /* Signature of kernel callback function to be implemented in python
    /  void(numDim, numInputs, numOutputs, numNHPix,
    /       auxIntLen, auxDoubleLen, auxVarArLen,
    /       shape[numDim], spacing[numDim], outPixIndex[numDim],
    /       inputs[(numInputs, numNHPix)], outputs[(numOutputs, 1)],
    /       aux_int_ar[(auxIntLen, )], aux_double_ar[(auxDoubleLen, )], auxVarAr[(auxVarArLen, )])
    */
    typedef void (*kfunc_type)(int32_t, int32_t, int32_t, int32_t,
                               int32_t, int32_t, int32_t,
                               uint64_t*, double_t*, int64_t*,
                               InputImagePixelType**, OutputImagePixelType*,
                               int64_t*, double_t*, double_t*);

    itkSetMacro(YamlConfigFileName, std::string)
    itkGetMacro(YamlConfigFileName, std::string)

    //itkSetMacro(WrapperName, std::string)
    //itkGetMacro(WrapperName, std::string)

    itkGetMacro(AuxVarDataIndex, int)

    itkSetMacro(WorkspacePath, std::string)

    itkSetMacro(IsStreamable, bool)
    itkSetMacro(IsThreadable, bool)

    itkSetStringMacro(KernelShape)
    void SetKernelRadius(std::vector<int> radius);

    void SetBMIModule(const std::shared_ptr<bmi::PythonBMI>& bmiModule);
    void SetInputNames(const std::vector<std::string>& inputNames);
    void SetOutputNames(const std::vector<std::string>& outputNames)
        {m_OutputNames = outputNames;}
    void SetAuxIntData(const std::vector<int64_t>& auxIntData);
    void SetAuxDoubleData(const std::vector<double_t>& auxDoubleData);


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

    void SetBMIImageValue(const std::string& bmiName, const std::type_index typeInfo,
                     size_t* numPixel, size_t* rank, size_t* shape,
                     SpacingValueType* spacing, OriginValueType* origin, void* buf);

    void SetBMIRegionValue(const std::string& bmiName, size_t* numPixel,
                           size_t* rank, uint64_t* shape, int64_t* index);


    void ConnectData(const OutputImageRegionType & outputWorkRegion);

    void AllocateOutputs();
    void ResetPipeline(void);
    void GenerateData(void);
    void BeforeThreadedGenerateData(void);
    void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                              itk::ThreadIdType threadId);
    void AfterThreadedGenerateData(void);
    void SingleThreadedGenerateData(void);
    void RunKernelFunc(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId);

    void PrepareNeighbourhoodProcessing(void);
    void CheckInputDataCongruence(void);


    static ITK_THREAD_RETURN_TYPE ThreaderCallback(void *arg);


    bool m_IsStreamable;
    bool m_IsThreadable;

    std::string m_YamlConfigFileName;

    std::vector<std::string> m_InputNames;
    std::vector<size_t> m_InputNumPix;
    std::vector<std::string> m_OutputNames;
    std::vector<size_t> m_OutputNumPix;

    /*! for Python models:
     *  required to fetch the associated python module
     *  from the global map LumassPythonModuleMap */
    //std::string m_WrapperName;

    std::shared_ptr<bmi::PythonBMI> m_BMIModule;
    std::string m_KernelFuncName;
    kfunc_type m_KernelFunc;
    int m_AuxIntDataSize;
    int m_AuxDoubleDataSize;
    std::vector<int64_t> m_AuxIntData;
    std::vector<double_t> m_AuxDoubleData;

    int m_AuxVarDataIndex;
    int m_AuxVarArLen;
    std::string m_AuxVarNames_Name;
    std::string m_AuxVarAr_Name;

    std::vector<std::string> m_AuxVarNames;
    std::vector<std::vector<double_t> > m_vthAuxVarAr;
    std::vector<std::vector<double_t> > m_vthAuxVarValMin;
    std::vector<std::vector<double_t> > m_vthAuxVarValMax;
    std::vector<std::vector<double_t> > m_vthAuxVarValSum;
    std::vector<std::vector<double_t> > m_vthAuxVarValSum2;


    uint8_t m_NumOutputs;
    uint8_t m_NumOuputImages;
    uint64_t m_PixCount;

    /*! Neighbourhood attributes
     */
    std::string m_KernelShape;
    SizeType m_KernelRadius;
    SpacingType m_Spacing;
    OriginType m_Origin;

    std::string m_LPRName;
    std::string m_SRName;

    OutputImageRegionType m_StreamRegion;
    OutputImageRegionType m_LargestPossibleRegion;

    int64_t m_LPRIndex[TOutputImage::ImageDimension];
    uint64_t m_LPRSize[TOutputImage::ImageDimension];
    SpacingValueType m_LPRSpacing[TOutputImage::ImageDimension];
    OriginValueType m_LPROrigin[TOutputImage::ImageDimension];

    int64_t m_StreamRegIndex[TOutputImage::ImageDimension];
    uint64_t m_StreamRegSize[TOutputImage::ImageDimension];

    int m_RegionValueType;

    size_t m_NumNeighbourPixel;
    uint64_t m_NumStreamRegPixels;
    uint64_t m_NumLPRPixels;
    size_t m_ImageRegionDimension;
    size_t m_ImageBufferDimension;

    //std::vector<NeighborIndexType> m_ActiveKernelIndices;
    //std::vector<OutputImagePixelType> m_NeighbourDistance;
    //std::map<std::string, InputShapedIterator> m_mapNameImgNeighbourValues;

    //NeighborIndexType m_CentrePixelIndex;
    //OutputImageSizeValueType m_ActiveNeighborhoodSize;

    std::string m_WorkspacePath;
    otb::SQLiteTable::Pointer m_AuxTable;

};

} // end namespace

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbBMIModelFilter.txx"
#endif

#endif /* BMIModelFilter_H_ */


