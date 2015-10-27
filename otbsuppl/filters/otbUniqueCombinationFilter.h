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

#ifndef OTBUNIQUECOMBINATIONFILTER_H_
#define OTBUNIQUECOMBINATIONFILTER_H_

#include <map>

//#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"
#include "itkImageToImageFilter.h"
#include "otbImage.h"

//#include "tchdb.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*! \class UniqueCombinationFilter
 *  \brief Identifies unique combinations of input image
 *         pixel and writes them into the output image
 */

template< class TInputImage, class TOutputImage = TInputImage >
class OTBSUPPLFILTERS_EXPORT UniqueCombinationFilter
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

    //typedef typename std::map< UniqueValueType, std::vector<long long> >  CombinationPedigreeMap;
    //typedef typename CombinationPedigreeMap::iterator 		          CombinationPedigreeIterator;

    void SetNthInput(unsigned int idx, const InputImageType * image);

    void setRAT(unsigned int idx, AttributeTable::Pointer table);
    AttributeTable::Pointer getRAT(unsigned int idx);

    void SetUVTableName(const std::string& name);



    void SetInputNodata(const std::vector<long long>& inNodata);


    void SetImageNames(std::vector<std::string> imgNames)
    {m_ImageNames = imgNames;}

    virtual void ResetPipeline();

protected:
    UniqueCombinationFilter();
    virtual ~UniqueCombinationFilter();
    //virtual void PrintSelf(std::ostream &os, itk::Indent indent) const;

    //void BeforeThreadedGenerateData();
    //void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId );
    //void AfterThreadedGenerateData();
    void GenerateData();

    void InternalAllocateOutput();
    std::string getRandomString(int length=15);
    unsigned int nextUpperIterationIdx(unsigned int idx, IndexType& accIdx);

    std::vector<AttributeTable::Pointer> m_vInRAT;
    std::vector<AttributeTable::Pointer> m_vOutRAT;

//    std::vector<TCHDB*> m_threadHDB;
//    TCHDB* m_tcHDB;

    std::string m_UVTableName;
    std::string m_WorkingDirectory;
    SQLiteTable::Pointer m_UVTable;

    bool m_StreamingProc;
    bool m_DropTmpTables;

    long long m_TotalStreamedPix;
    OutputPixelType m_OutIdx;
    IndexType m_UVTableIndex;

    std::vector<InputPixelType> m_InputNodata;
    std::vector<std::string> m_ImageNames;


private:
    static const std::string ctx;

};

}

template< class TInputImage, class TOutputImage>
const std::string otb::UniqueCombinationFilter<TInputImage, TOutputImage>::ctx = "UniqueCombinationFilter";

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbUniqueCombinationFilter.txx"
#endif

#endif /* OTBUNIQUECOMBINATIONFILTER */
