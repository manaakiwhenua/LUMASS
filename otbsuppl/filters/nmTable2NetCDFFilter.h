/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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
 * nmTable2NetCDFFilter.h
 *
 *  Created on: 2023-06-22
 *      Author: Alex Herzig
 *
 */

#ifndef NMTABLE2NETCDFFILTER_H_
#define NMTABLE2NETCDFFILTER_H_

#include <string>

#include "itkImageToImageFilter.h"
#include "otbImage.h"
#include "otbSQLiteTable.h"

#include "nmotbsupplfilters_export.h"
#include "itkImageScanlineIterator.h"

/*!
 *  \brief Table2NetCDFFilter copies a table into a netcdf file
 *
 */

namespace nm
{

template< class TInputImage, class TOutputImage = TInputImage >
class NMOTBSUPPLFILTERS_EXPORT Table2NetCDFFilter
        : public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:

    /** Convenient typedefs for simplifying declarations. */


    using InputImageType = TInputImage  ;
    using OutputImageType = TOutputImage ;

    using Self         = Table2NetCDFFilter;
    using Superclass   = itk::ImageToImageFilter< InputImageType, OutputImageType>;
    using Pointer      = itk::SmartPointer<Self>       ;
    using ConstPointer = itk::SmartPointer<const Self> ;

    itkNewMacro(Self)
    itkTypeMacro(Table2NetCDFFilter, itk::ImageToImageFilter)


    /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
                        TInputImage::ImageDimension);
    itkStaticConstMacro(OutputImageDimension, unsigned int,
                        TOutputImage::ImageDimension);

    using InputImagePointerType  = typename InputImageType::Pointer  ;
    using OutputImagePointerType = typename OutputImageType::Pointer ;

    /** Image typedef support. */
    using InputPixelType        = typename InputImageType::PixelType   ;
    using OutputPixelType       = typename OutputImageType::PixelType  ;

    using InputImageRegionType  = typename InputImageType::RegionType  ;
    using OutputRegionType      = typename OutputImageType::RegionType ;
    using OutputSpacingType     = typename OutputImageType::SpacingType;
    using OutputPointType       = typename OutputImageType::PointType  ;
    using OutputDirectionType   = typename OutputImageType::DirectionType;

    using OutputIteratorType    = typename itk::ImageScanlineIterator<OutputImageType>;


    using SizeValueType = size_t;


    /** The table inside the SQLite database the data is going to be stored in*/
    itkGetMacro(InputTableName    , std::string)
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

    itkGetMacro(VarAndDimDescriptors, std::vector<std::string>)


    /** Addtional (auxillary) variables to be read from the netCDF file that
     *  are associated with the (core) image (variable), e.g. variables for
     *  individual image dimensions storing coordinate values.
     */
    //itkGetMacro(AuxVarNames  , std::vector<std::string>)


    itkGetMacro(SQLWhereClause, std::string)
    itkSetMacro(SQLWhereClause, std::string)


    itkSetMacro(InputTableName    , std::string)
    itkSetMacro(ImageVarName , std::string)
    itkSetMacro(NcImageContainer, std::string)
    itkSetMacro(NcGroupName   , std::string)

    //void SetImageNames(std::vector<std::string> names) {m_ImageNames = names;}

    otb::AttributeTable::Pointer getRAT(unsigned int idx);
    void setRAT(unsigned int idx, otb::AttributeTable::Pointer);


    void SetDimMapping(std::vector<int>& vec) {m_DimMapping = vec;}
    std::vector<int> GetDimMapping(std::vector<int>& vec) {return m_DimMapping;}

    void SetOutputOrigin(std::vector<double>& vec) {m_OutputOrigin = vec;}
    std::vector<double> GetOutputOrigin(std::vector<double>& vec) {return m_OutputOrigin;}

    void SetOutputSpacing(std::vector<double>& vec) {m_OutputSpacing = vec;}
    std::vector<double> GetOutputSpacing(std::vector<double>& vec) {return m_OutputSpacing;}

    void SetOutputSize(std::vector<long long>& vec) {m_OutputSize = vec;}
    std::vector<long long> GetOutputSize(std::vector<long long>& vec) {return m_OutputSize;}

    void SetOutputIndex(std::vector<long long>& vec) {m_OutputIndex = vec;}
    std::vector<long long> GetOutputIndex(std::vector<long long>& vec) {return m_OutputIndex;}


    void SetDimVarNames(std::vector<std::string> dnames)
    {m_DimVarNames = dnames;}

    void SetVarAndDimDescriptors(std::vector<std::string> descriptors)
    {m_VarAndDimDescriptors = descriptors;}

    //void SetAuxVarNames(std::vector<std::string> anames)
    //{m_AuxVarNames = anames;}



    void GenerateOutputInformation(void);
    void GenerateInputRequestedRegion();


protected:
    Table2NetCDFFilter();
    Table2NetCDFFilter(const Self&);
    void operator=(const Self&);

    virtual void VerifyInputInformation() ITK_OVERRIDE {}

    bool PrepOutput(void);
    void GenerateData();
    void ResetPipeline();


    //std::vector<std::string>  m_ImageNames;
    std::string m_SQLWhereClause;

    std::vector<int> m_DimMapping;

    std::vector<long long> m_OutputSize;
    std::vector<long long> m_OutputIndex;
    std::vector<double> m_OutputSpacing;
    std::vector<double> m_OutputOrigin;


    std::string m_InputTableName;
    std::string m_ImageVarName;

    std::vector<int> m_StartIndex;
    std::vector<int> m_Size;

    std::string m_NcImageContainer;
    std::string m_NcGroupName;
    std::vector<std::string> m_DimVarNames; // x, y, z  INTEGER (0...n-1)
    std::vector<std::string> m_VarAndDimDescriptors; // var, dim1, dim2, ...
    std::vector<int> m_DimColDimId;



    std::vector<int> m_AuxIsInteger; // [0,1]
    std::vector<void*> m_AuxBuffer;
    std::vector<std::vector<size_t> > m_AuxVarDimMap;

    std::vector<std::string> m_ColNames;
    std::vector<otb::AttributeTable::ColumnValue> m_ColValues;

    std::vector<std::string> m_KeyColNames;
    std::vector<otb::AttributeTable::ColumnValue> m_KeyColValues;
    std::vector<int> m_KeyColDimId;

    std::vector<std::string> m_WhereClauseHelper;

    bool m_bNCPreped;
    otb::SQLiteTable::Pointer m_Tab;

    std::vector<otb::SQLiteTable::Pointer> m_vRAT;

    SizeValueType m_NumPixel;
    SizeValueType m_PixelCounter;



private:
    static const std::string ctxTable2NetCDFFilter;

};

}   // END NAMESPACE OTB

template< class TInputImage, class TOutputImage>
const std::string nm::Table2NetCDFFilter<TInputImage, TOutputImage>::ctxTable2NetCDFFilter = "otb::Table2NetCDFFilter";

//#include "otbTable2NetCDFFilter_ExplicitInst.h"

#ifndef OTB_MANUAL_INSTANTIATION
#include "nmTable2NetCDFFilter.txx"
#endif

#endif // HEADER DEFINITION

