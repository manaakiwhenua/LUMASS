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
 * nmTable2NetCDFFilter.txx
 *
 *  Created on: 2023-06-22
 *      Author: Alex Herzig
 *
 */


#ifndef __nmTable2NetCDFFilter_txx
#define __nmTable2NetCDFFilter_txx

#include "nmlog.h"
#include "nmTable2NetCDFFilter.h"
#include "itkSmartPointerForwardReference.h"
#include "otbNMTableReader.h"
#include "nmNetCDFIO.h"
#include "otbSQLiteTable.h"
#include "otbImageMetadata.h"

namespace nm
{

template< class TInputImage, class TOutputImage >
Table2NetCDFFilter< TInputImage, TOutputImage >
::Table2NetCDFFilter()
    : m_bNCPreped(false),
      m_NumPixel(0),
      m_PixelCounter(0)
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    // set-up a dummy input, just in case this
    // filter is only fed by a 'table reader'
    // rather than an image/RAT reader
    InputImagePointerType img = InputImageType::New();

    typename InputImageType::RegionType region;
    region = img->GetRequestedRegion();

    for (unsigned int d=0; d < InputImageDimension; ++d)
    {
        region.SetIndex(d, 0);
        region.SetSize(d, 1);
    }
    img->SetRegions(region);

    this->SetInput(0, img);

}

template< class TInputImage, class TOutputImage >
void Table2NetCDFFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, otb::AttributeTable::Pointer tab)
{
    // we only handle sqlite tables here, so we
    // refuse any other tables
    if (    tab.IsNotNull()
        &&  tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE
       )
    {
        otb::SQLiteTable* tabptr = static_cast<otb::SQLiteTable*>(tab.GetPointer());
        if (tabptr != nullptr)
        {
            otb::SQLiteTable::Pointer sqltab = tabptr;
            m_vRAT.resize(idx + 1);
            m_vRAT[idx] = sqltab;
            return;
        }
    }

    itkExceptionMacro(<< "Input table #" << idx << " is not a valid SQLite database!")
}

template< class TInputImage, class TOutputImage >
otb::AttributeTable::Pointer Table2NetCDFFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    if (idx >= m_vRAT.size())
    {
        return 0;
    }

    otb::AttributeTable::Pointer tab = static_cast<otb::AttributeTable*>(
                m_vRAT.at(idx).GetPointer());

    return tab;
}

template <class TInputImage, class TOutputImage>
void Table2NetCDFFilter<TInputImage, TOutputImage>
::GenerateOutputInformation(void)
{
    NMDebugCtx(ctxTable2NetCDFFilter, << "...");

    /*
     * sizes, band, resolutions, etc.(?)
     */

    if (    m_OutputSize.size() != OutputImageType::ImageDimension
         || m_OutputIndex.size() != OutputImageType::ImageDimension
         || m_OutputSpacing.size() != OutputImageType::ImageDimension
         //|| m_DimVarNames.size() != OutputImageType::ImageDimension
       )
    {
        itkExceptionMacro(<< "At least one output parameter's number of dimensions does not match "
                             "the actual number of dimensions of the output image!");
        return;
    }

    TOutputImage* OutImg = const_cast<TOutputImage*>(this->GetOutput());

    OutputRegionType outRegion;
    OutputSpacingType     outSpacing;
    OutputPointType       outOrigin;
    OutputDirectionType   outDirection;
    outDirection.Fill(0);
    for (int d=0; d < OutputImageType::ImageDimension; ++d)
    {
        outRegion.SetSize(d, m_OutputSize[d]);
        outRegion.SetIndex(d, m_OutputIndex[d]);
        outSpacing[d] = m_OutputSpacing[d];
        outOrigin[d]  = m_OutputOrigin[d];
        outDirection[d][d] = d == 1 ? -1 : 1;
    }

    OutImg->SetLargestPossibleRegion(outRegion);
    OutImg->SetSpacing(outSpacing);
    OutImg->SetDirection(outDirection);
    OutImg->SetOrigin(outOrigin);


    // set image metadata for netcdf file
    otb::ImageMetadata imd = OutImg->GetImageMetadata();

    std::stringstream strDimVarNames;
    // note DimVarNames need to be in the following
    // axis order: x, y, z, ...
    // i.e. starting with the fastest moving index!
    for (int dn=0; dn < m_DimVarNames.size(); ++dn)
    {
        strDimVarNames << m_DimVarNames[dn];
        if (dn < m_DimVarNames.size()-1)
        {
            strDimVarNames << ",";
        }
    }

    imd.ExtraKeys.insert(
                std::pair<std::string, std::string>(
                    "DimVarNames", strDimVarNames.str()));

    std::stringstream vddStr;
    // variable and dimension descriptors are in the order
    // image variable name, then the dimension in the same
    // order as above
    for (int vd=0; vd < m_VarAndDimDescriptors.size(); ++vd)
    {
        const std::string descr = m_VarAndDimDescriptors[vd];
        vddStr << descr;
        if (vd < m_VarAndDimDescriptors.size()-1)
        {
            vddStr << ",";
        }
    }

    imd.ExtraKeys.insert(
                std::pair<std::string, std::string>(
                    "VarAndDimDescriptors", vddStr.str()));
    OutImg->SetImageMetadata(imd);

    NMDebugCtx(ctxTable2NetCDFFilter, << "done!");
}

// since we're not really working on the image's bulk
// data, we just set a 1 pixel wide region
// to satisfy the pipeline but minimise data reading
template< class TInputImage, class TOutputImage >
void Table2NetCDFFilter< TInputImage, TOutputImage >
::GenerateInputRequestedRegion()
{
    int numInputs = this->GetInputs().size();

    InputImagePointerType inputPtr;
    typename TInputImage::RegionType inputReqRegion;

    for (int in = 0; in < numInputs; ++in)
    {
        inputPtr = dynamic_cast<InputImageType*>(this->GetInputs()[in].GetPointer());

        if (inputPtr.IsNotNull())
        {
            inputReqRegion = inputPtr->GetRequestedRegion();

            for (unsigned int d = 0; d < InputImageDimension; ++d)
            {
                inputReqRegion.SetSize(d, 1);
            }
            inputPtr->SetRequestedRegion(inputReqRegion);
        }
    }

    // in case we've got just a table reader (as instead of an image
    // reader - this is an image to image filter after all) as 'proper' input,
    // we have to make sure that the table(s) is (are) actually read, and that's
    // what we're doing now ...
    for (int i=0; i < m_vRAT.size(); ++i)
    {
        if (m_vRAT.at(i).IsNotNull() && m_vRAT.at(i)->GetDbConnection() == 0)
        {
            otb::SQLiteTable::Pointer tab = m_vRAT.at(i);
            if (tab->GetSource())
            {
                typename itk::SmartPointerForwardReference<itk::ProcessObject> src =
                        tab->GetSource();

                otb::NMTableReader::Pointer treader = static_cast<otb::NMTableReader*>(src.GetPointer());
                if (treader.IsNotNull())
                {
                    treader->GenerateData();
                }
            }
        }
    }
}

template< class TInputImage, class TOutputImage >
void Table2NetCDFFilter< TInputImage, TOutputImage >
::ResetPipeline(void)
{
    Superclass::ResetPipeline();

    m_bNCPreped = false;
    m_NumPixel = 0;
    m_PixelCounter = 0;
    if (m_Tab.IsNotNull())
    {
        m_Tab->CloseTable(false);
    }

    m_WhereClauseHelper.clear();
    m_ColNames.clear();
    m_ColValues.clear();
}

template< class TInputImage, class TOutputImage >
bool Table2NetCDFFilter< TInputImage, TOutputImage >
::PrepOutput(void)
{
    // --------------------------------------------------
    //     DO WE HAVE A VALID INPUT TABLE?

    if (m_vRAT.size() != 1)
    {
        itkExceptionMacro(<< "Please provide ONE valid SQLite database as input!")
        return false;
    }

    if (m_vRAT.at(0).GetPointer() == nullptr)
    {
        itkExceptionMacro(<< "Input table #0 is not a valid SQLite database!")
        return false;
    }


    // ---------------------------------------------------
    //      DOES THE TABLE HAVE THE REQUIRED FIELDS ?
    // PREP VECTORS FOR FETCHING TABLE DATA AT THE SAME TIME

    otb::SQLiteTable::Pointer sqltab = m_vRAT.at(0);

    // double check whether we've got the required input columns
    for (int cn=0; cn < m_DimVarNames.size(); ++cn)
    {
        // only check, if outputsize is > 1 otherwise this is a
        // dummy dimension and we just set index 0 all the time
        if (m_OutputSize.at(cn) > 1)
        {
            if (sqltab->ColumnExists(m_DimVarNames.at(cn)) < 0)
            {
                itkExceptionMacro(<< "Couldn't find the specified dimension variable '"
                                  << m_DimVarNames.at(cn) << "' in the input table!"
                                  << endl);
                return false;
            }
            m_WhereClauseHelper.push_back(m_DimVarNames.at(cn));
        }
        else
        {
            m_WhereClauseHelper.push_back("__dummy__");
        }
    }

    if (sqltab->ColumnExists(m_ImageVarName) < 0)
    {
        itkExceptionMacro(<< "Couldn't find the specified image variable '"
                          << m_ImageVarName << "' in the input table!"
                          << endl);
        return false;
    }

    m_ColNames.push_back(m_ImageVarName);
    otb::AttributeTable::ColumnValue cv;
    m_ColValues.push_back(cv);


    // -----------------------------------------------------
    //          ALLOCATE OUTPUT

    this->AllocateOutputs();

    return true;
}

template< class TInputImage, class TOutputImage >
void Table2NetCDFFilter< TInputImage, TOutputImage >
::GenerateData()
{
    if (m_PixelCounter == 0)
    {
        if (!PrepOutput())
        {
            return;
        }
    }

    OutputImagePointerType outImg = this->GetOutput(0);
    OutputRegionType outRegion = outImg->GetRequestedRegion();
    OutputRegionType outLPR = outImg->GetLargestPossibleRegion();
    m_NumPixel = outLPR.GetNumberOfPixels();

    std::stringstream orderBy;
    orderBy << " order by ";
    std::stringstream strWC;
    strWC << " where ";
    for (int col=0, cnt=0; col < m_WhereClauseHelper.size(); ++col, ++cnt)
    {
        if (m_WhereClauseHelper.at(col).compare("__dummy__") > 0)
        {
            strWC << m_WhereClauseHelper.at(col) << " between "
                  << outRegion.GetIndex(col) << " and "
                  << (outRegion.GetIndex(col) + outRegion.GetSize(col) - 1)
                  << " ";


            orderBy << m_WhereClauseHelper.at(col);
        }
        else
        {
            continue;
        }

        if (col < cnt-1)
        {
            strWC << " and ";
            orderBy << ", ";
        }
    }
    std::string whereClause = strWC.str() + orderBy.str();

    otb::SQLiteTable::Pointer sqltab = m_vRAT.at(0);
    if (!sqltab->PrepareBulkGet(m_ColNames, whereClause, false))
    {
        itkExceptionMacro(<< "Fetching table values for output region failed: "
                          << sqltab->getLastLogMsg() << endl);
        return;
    }

    sqltab->BeginTransaction();
    bool brow = sqltab->DoBulkGet(m_ColValues);
    if (brow == false)
    {
        itkExceptionMacro(<< "Fetching table values for output region failed: "
                          << sqltab->getLastLogMsg() << endl);
        sqltab->EndTransaction();
        return;
    }

    OutputIteratorType outIter(outImg, outRegion);

    while (!outIter.IsAtEnd() && brow)
    {
        while (!outIter.IsAtEndOfLine() && brow)
        {
            const otb::AttributeTable::TableColumnType colType = m_ColValues[0].type;
            switch(colType)
            {
                case otb::AttributeTable::ATTYPE_INT:
                    outIter.Set(static_cast<OutputPixelType>(m_ColValues[0].ival));
                    break;
                case otb::AttributeTable::ATTYPE_DOUBLE:
                    outIter.Set(static_cast<OutputPixelType>(m_ColValues[0].dval));
                    break;
                default: break;
            }

            ++outIter;
            ++m_PixelCounter;
            this->UpdateProgress(m_PixelCounter / m_NumPixel);

            brow = sqltab->DoBulkGet(m_ColValues);
        }
        outIter.NextLine();
    }
    sqltab->EndTransaction();

    this->UpdateProgress(1.0);
}


}

#endif // TXX DEFINITION
