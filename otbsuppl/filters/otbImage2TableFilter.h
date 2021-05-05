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


#ifndef OTBIMAGE2TABLEFILTER_H
#define OTBIMAGE2TABLEFILTER_H

#include "nmlog.h"

#include "itkMacro.h"
#include "itkImageToImageFilter.h"
#include "otbImage.h"
#include "otbSQLiteTable.h"
#include "otbsupplfilters_export.h"

namespace otb
{
template<class TInputImage>
class OTBSUPPLFILTERS_EXPORT Image2TableFilter : public itk::ImageToImageFilter<TInputImage, TInputImage>
{
public:
    using Self          = Image2TableFilter;
    using Superclass    = itk::ImageToImageFilter<TInputImage, TInputImage>;
    using Pointer       = itk::SmartPointer<Self>;
    using ConstPointer  = itk::SmartPointer<const Self>;

    itkNewMacro(Self);

    itkTypeMacro(Image2TableFilter, itk::ImageToImageFilter);

    using InputImageType                = TInputImage;
    using InputImagePointer             = typename InputImageType::Pointer;
    using InputImageRegionType          = typename InputImageType::RegionType;
    using InputImagePixelType           = typename InputImageType::PixelType;
    using InputImageIOPixelType         = typename InputImageType::IOPixelType;
    using SizeValueType                 = typename InputImageRegionType::SizeValueType;
    using OffsetTableType               = typename InputImageRegionType::OffsetTableType;

    itkStaticConstMacro(InputImageDimension, unsigned int, InputImageType::ImageDimension);

    /** The name of the sql database (*.ldb) the table is going to be stored in */
    itkGetMacro(TableFileName, std::string)
    /** The table inside the SQLite database the data is going to be stored in*/
    itkGetMacro(TableName    , std::string)
    /** The name of the netCDF image (variable) to be stored */
    itkGetMacro(ImageVarName , std::string)
    /** The name of the netCDF image container (*.nc) containing the data
     *  to be added to the table. This is the same *.nc file that needs
     *  to be specified as part of the input filename of the ImageReader
     *  component providing reading the core image. It only needs to be
     *  specified, if additional associated variables shall be stored in the
     *  table, e.g. coordinate variables for individual image dimensions.
     */
    itkGetMacro(NcImageContainer, std::string)
    /** The netCDF group from which to read the auxillary data */
    itkGetMacro(NcGroupName   , std::string)
    /** The output table column names to used for the individual image dimension
     *  indices of the input image (core variable).
     */
    itkGetMacro(DimVarNames  , std::vector<std::string>)
    /** Addtional (auxillary) variables to be read from the netCDF file that
     *  are associated with the (core) image (variable), e.g. variables for
     *  individual image dimensions storing coordinate values.
     */
    itkGetMacro(AuxVarNames  , std::vector<std::string>)


    itkSetMacro(TableFileName, std::string)
    itkSetMacro(TableName    , std::string)
    itkSetMacro(ImageVarName , std::string)
    itkSetMacro(NcImageContainer, std::string)
    itkSetMacro(NcGroupName   , std::string)
    itkSetMacro(UpdateMode, int)

    /** The start index of the image region to be extracted from the image */
    void SetStartIndex(std::vector<int> sindex){m_StartIndex = sindex;}
    /** The size of the image region to be extracted from the image*/
    void SetSize(std::vector<int> size){m_Size = size;}

    void SetDimVarNames(std::vector<std::string> dnames)
    {m_DimVarNames = dnames;}
    void SetAuxVarNames(std::vector<std::string> anames)
    {m_AuxVarNames = anames;}

    AttributeTable::Pointer getRAT(unsigned int idx);
    void setRAT(unsigned int idx, AttributeTable::Pointer);

    void PrintSelf(std::ostream &os, itk::Indent indent) const;
    void GenerateOutputInformation(void);
    void GenerateInputRequestedRegion(void);


protected:
    Image2TableFilter();
    Image2TableFilter(const Self&);
    ~Image2TableFilter();
    void operator=(const Self&);

    bool PrepTable(void);
    void GenerateData(void) override;
    void ResetPipeline();

    int m_UpdateMode;
    std::string m_TableFileName;
    std::string m_TableName;
    std::string m_ImageVarName;

    std::vector<int> m_StartIndex;
    std::vector<int> m_Size;

    std::string m_NcImageContainer;
    std::string m_NcGroupName;
    std::vector<std::string> m_DimVarNames; // x, y, z  INTEGER (0...n-1)
    std::vector<int> m_DimColDimId;
    std::vector<std::string> m_AuxVarNames; // long, lat, time (coordinates associated
                                            // with the dimensions)
    std::vector<int> m_AuxIsInteger; // [0,1]
    std::vector<void*> m_AuxBuffer;
    std::vector<std::vector<size_t> > m_AuxVarDimMap;

    std::vector<std::string> m_ColNames;
    std::vector<otb::AttributeTable::ColumnValue> m_ColValues;

    std::vector<std::string> m_KeyColNames;
    std::vector<otb::AttributeTable::ColumnValue> m_KeyColValues;
    std::vector<int> m_KeyColDimId;

    bool m_bInsertValues;
    otb::SQLiteTable::Pointer m_Tab;

    std::vector<SQLiteTable::Pointer> m_vRAT;

    OffsetTableType m_LprOffsets;


    SizeValueType m_NumPixel;
    SizeValueType m_PixelCounter;
};

} // end of namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbImage2TableFilter.txx"
#endif

#endif // OTBIMAGE2TABLEFILTER_H
