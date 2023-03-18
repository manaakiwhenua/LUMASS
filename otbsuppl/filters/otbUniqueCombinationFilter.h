/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2015 Landcare Research New Zealand Ltd
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

/*  ACKNOWLEDGEMENT
 *
 *  This implementation is based on a conceptual algorithm for
 *  computing unique combinations developed by my brillant colleague
 *
 *  Robbie Price (Manaaki Whenua - Landcare Research New Zealand Ltd)
 *
 *
 */

#ifndef OTBUNIQUECOMBINATIONFILTER_H_
#define OTBUNIQUECOMBINATIONFILTER_H_

#include <map>

//#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"
#include "itkImageToImageFilter.h"
#include "otbImage.h"

//#include "tchdb.h"

#include "nmotbsupplfilters_export.h"

namespace otb
{
/*! \class UniqueCombinationFilter
 *  \brief Identifies unique combinations of input image
 *         pixel and writes them into the output image
 */

template< class TInputImage, class TOutputImage = TInputImage >
class NMOTBSUPPLFILTERS_EXPORT UniqueCombinationFilter
        : public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:

    /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
                        TInputImage::ImageDimension);
    itkStaticConstMacro(OutputImageDimension, unsigned int,
                        TOutputImage::ImageDimension);

    typedef TInputImage  InputImageType;
    typedef TOutputImage OutputImageType;

    typedef typename InputImageType::Pointer	InputImagePointerType;
    typedef typename OutputImageType::Pointer OutputImagePointerType;

    /** Standard class typedefs. */
    typedef UniqueCombinationFilter                                            Self;
    typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
    typedef itk::SmartPointer<Self>                                   Pointer;
    typedef itk::SmartPointer<const Self>                             ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(UniqueCombinationFilter, itk::ImageToImageFilter);

    /** Image typedef support. */
    typedef typename InputImageType::PixelType   InputPixelType;
    typedef typename OutputImageType::PixelType  OutputPixelType;

    typedef typename InputImageType::RegionType  InputImageRegionType;
    typedef typename OutputImageType::RegionType OutputImageRegionType;

    typedef typename InputImageType::SizeType    InputImageSizeType;
    typedef typename OutputImageType::SizeType   OutputImageSizeType;

    typedef typename InputImageType::SpacingType  InputImageSpacingType;
    typedef typename OutputImageType::SpacingType OutputImageSpacingType;

    typedef long long IndexType;

    itkGetMacro(Workspace, std::string);
    itkSetMacro(Workspace, std::string);

    void SetInput(unsigned int idx, const InputImageType * image);

    void setRAT(unsigned int idx, AttributeTable::Pointer table);
    AttributeTable::Pointer getRAT(unsigned int idx);

    void SetUVTableName(const std::string& name);
    void SetOutputImageFileName(const std::string& name) {m_OutputImageFileName = name;}

    void SetInputNodata(const std::vector<long long>& inNodata);


    void SetImageNames(std::vector<std::string> imgNames)
    {m_ImageNames = imgNames;}

    virtual void ResetPipeline();

    // un-orthodox short-cut to enable un-orthodox composite filter ...
    void Update();

protected:
    UniqueCombinationFilter();
    virtual ~UniqueCombinationFilter();

    void GenerateData();
    // outputs are produced by the internally used filters ...
    void AllocateOutputs(){}

    std::string getRandomString(int length=15);
    unsigned int nextUpperIterationIdx(unsigned int idx, unsigned long long& accIdx);
    void determineProcOrder(void);

    std::string m_Workspace;
    std::string m_OutputImageFileName;
    std::vector<AttributeTable::Pointer> m_vInRAT;
    std::vector<AttributeTable::Pointer> m_vOutRAT;

    std::string m_UVTableName;
    std::string m_WorkingDirectory;
    SQLiteTable::Pointer m_UVTable;

    bool m_StreamingProc;
    bool m_DropTmpTables;

    long long m_TotalStreamedPix;
    OutputPixelType m_OutIdx;
    IndexType m_UVTableIndex;

    std::vector<typename InputImageType::Pointer> m_InputImages;
    std::vector<InputPixelType> m_InputNodata;
    std::vector<std::string> m_ImageNames;
    std::vector<int> m_ProcOrder;


private:
    static const std::string ctx;

};

}

template< class TInputImage, class TOutputImage>
const std::string otb::UniqueCombinationFilter<TInputImage, TOutputImage>::ctx = "UniqueCombinationFilter";

//#include "otbUniqueCombinationFilter_ExplicitInst.h"

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbUniqueCombinationFilter.txx"
#endif

#endif /* OTBUNIQUECOMBINATIONFILTER */
