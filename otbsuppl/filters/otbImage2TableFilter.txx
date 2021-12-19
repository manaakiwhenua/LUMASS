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

#include "otbImage2TableFilter.h"
#include "nmlog.h"
#include <typeinfo>
#include "otbImageIOBase.h"
#include "nmNetCDFIO.h"
#include "otbNMTableReader.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkProgressReporter.h"

namespace otb
{

template<class TInputImage>
Image2TableFilter<TInputImage>
::Image2TableFilter() :
    m_Tab(nullptr),
    m_PixelCounter(0),
    m_NumPixel(0),
    m_bInsertValues(true),
    m_UpdateMode(0)
{
    this->SetNumberOfThreads(1);
}

template<class TInputImage>
Image2TableFilter<TInputImage>
::~Image2TableFilter()
{
}

template<class TInputImage>
void Image2TableFilter<TInputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
    os << indent << "Image2TableFilter details: " << std::endl
       << indent << "Dimension: " << InputImageDimension << std::endl
       << indent << "PixelType: " << typeid(InputImagePixelType).name() << std::endl
       << indent << "UpdateMode: " << m_UpdateMode << std::endl
       << indent << "TableFileName: " << m_TableFileName << std::endl
       << indent << "TableName: " << m_TableName << std::endl
       << indent << "ImageVarName: " << m_ImageVarName << std::endl
       << indent << "NcImageContainer: " << m_NcImageContainer << std::endl
       << indent << "NcGroupName: " << m_NcGroupName << std::endl
       << indent << "DimVarNames: ";
    for (int v=0; v < m_DimVarNames.size(); ++v)
    {
        os << indent << m_DimVarNames.at(v) << " ";
    }
    os << std::endl
       << indent << "AuxVarNames: ";
    for (int a=0; a < m_AuxVarNames.size(); ++a)
    {
        os << indent << m_AuxVarNames.at(a) << " ";
    }
    os << std::endl
       << indent << "AusIsInteger: ";
    for (int i=0; i < m_AuxIsInteger.size(); ++i)
    {
        os << indent << m_AuxIsInteger.at(i) << " ";
    }
    os << std::endl
       << indent << "NumPixel: " << m_NumPixel << std::endl
       << indent << "PixelCounter: " << m_PixelCounter;
}

template<class TInputImage>
void Image2TableFilter<TInputImage>
::GenerateOutputInformation()
{
    Superclass::GenerateOutputInformation();

    InputImageType* output = const_cast<InputImageType*>(this->GetOutput());
    InputImageRegionType lpr = output->GetLargestPossibleRegion();

    if (m_StartIndex.size() == InputImageDimension)
    {
        for (int d=0; d < InputImageDimension; ++d)
        {
            lpr.SetIndex(d, m_StartIndex[d]);
        }
    }

    if (m_Size.size() == InputImageDimension)
    {
        for (int d=0; d < InputImageDimension; ++d)
        {
            lpr.SetSize(d, m_Size[d]);
        }
    }

    output->SetLargestPossibleRegion(lpr);

}

template<class TInputImage>
void Image2TableFilter<TInputImage>
::GenerateInputRequestedRegion()
{
    Superclass::GenerateInputRequestedRegion();

//    InputImageType* input = const_cast<InputImageType*>(this->GetInput());

//    if (input != nullptr)
//    {
//        InputImageRegionType reqReg = input->GetRequestedRegion();

//        if (m_StartIndex.size() == InputImageDimension)
//        {
//            for (int d=0; d < InputImageDimension; ++d)
//            {
//                reqReg.SetIndex(d, m_StartIndex[d]);
//            }
//        }

//        if (m_Size.size() == InputImageDimension)
//        {
//            for (int d=0; d < InputImageDimension; ++d)
//            {
//                reqReg.SetSize(d, m_Size[d]);
//            }
//        }

//        input->SetRequestedRegion(reqReg);
//    }

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


template< class TInputImage>
void Image2TableFilter< TInputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer tab)
{
    // we only accept one input table (i.e. the first we get)
    // and refuse all other tables
    if (    tab.IsNotNull()
        &&  tab->GetTableType() == AttributeTable::ATTABLE_TYPE_SQLITE
       )
    {
        SQLiteTable::Pointer sqltab = static_cast<SQLiteTable*>(tab.GetPointer());

        m_vRAT.resize(1);
        m_vRAT[0] = sqltab;
        this->m_Tab = m_vRAT[0];
    }
    else
    {
        NMProcWarn(<< "NULL-table or non-SQLite-table provided!")
    }
}


template< class TInputImage>
AttributeTable::Pointer Image2TableFilter< TInputImage>
::getRAT(unsigned int idx)
{
    if (idx >= m_vRAT.size())
    {
        return 0;
    }

    AttributeTable::Pointer tab = static_cast<AttributeTable*>(
                m_vRAT.at(idx).GetPointer());

    return tab;
}

template<class TInputImage>
bool Image2TableFilter<TInputImage>
::PrepTable()
{
    const InputImageType* input = this->GetInput();

    if (m_AuxVarNames.size() > 0)
    {
        if (    m_NcImageContainer.empty()
             || m_ImageVarName.empty()
           )
        {
            NMProcErr(<< "Please specify the 'NcImageContainer', 'NcGroupName', and 'ImageVarName', "
                         " if you specified 'AuxVarNames'!");
            return false;
        }
    }

    InputImageRegionType lpr = input->GetLargestPossibleRegion();
    //m_LprOffsets = lpr.GetOffsetTable();
    m_NumPixel = lpr.GetNumberOfPixels();
    m_ColNames.clear();
    m_ColValues.clear();
    m_KeyColNames.clear();
    m_KeyColValues.clear();
    m_KeyColDimId.clear();
    m_AuxIsInteger.clear();
    m_AuxVarDimMap.clear();
    m_DimColDimId.clear();

    bool bTableExists = false;
    std::vector<std::string> tableList = m_Tab->GetTableList();
    for (int t=0; t < tableList.size(); ++t)
    {
        if (tableList.at(t).compare(m_TableName) == 0)
        {
            bTableExists = true;
            m_bInsertValues = false;
            break;
        }
    }

    m_ColNames.push_back(m_ImageVarName);
    otb::AttributeTable::ColumnValue cval;

    std::string sqlImgDataType = "";
    if (   typeid(InputImageIOPixelType) == typeid(float)
        || typeid(InputImageIOPixelType) == typeid(double)
       )
    {
        sqlImgDataType = "REAL";
        cval.type = otb::AttributeTable::ATTYPE_DOUBLE;
        cval.dval = 0.0;
    }
    else
    {
        sqlImgDataType = "INTEGER";
        cval.type = otb::AttributeTable::ATTYPE_INT;
        cval.ival = 0;
    }

    m_ColValues.push_back(cval);

    // adding '_id' suffix to each dimvar because often
    // nc variables and dimensions share the same name, so
    // we need to avoid duplicate column names, or otherwise
    // we had to make the user ensure that those names
    // aren't duplicates which is confusing
    std::vector<std::string> allOtherCols;

    for (int dv=0; dv < m_DimVarNames.size(); ++dv)
    {
        if (!m_DimVarNames.at(dv).empty())
        {
            if (!m_UpdateMode)
            {
                allOtherCols.push_back(m_DimVarNames[dv] + "_id");
                m_DimColDimId.push_back(dv);
            }
            else
            {
                m_KeyColNames.push_back(m_DimVarNames[dv] + "_id");
                m_KeyColDimId.push_back(dv);
                otb::AttributeTable::ColumnValue keyColVal;
                keyColVal.type = otb::AttributeTable::ATTYPE_INT;
                keyColVal.ival = 0;
                m_KeyColValues.push_back(keyColVal);
            }
        }
    }

    NetCDFIO::Pointer nio;
    if (m_AuxVarNames.size() > 0)
    {
        allOtherCols.insert(allOtherCols.end(), m_AuxVarNames.begin(), m_AuxVarNames.end());
        std::string ncimgspec = m_NcImageContainer + ":" + m_NcGroupName + "/" + m_ImageVarName;
        nio = NetCDFIO::New();
        nio->SetFileName(ncimgspec.c_str());
        if (!nio->CanReadFile(ncimgspec.c_str()))
        {
            NMProcErr(<< "Failed to read image '" << ncimgspec << "'!");
            return false;
        }
        nio->ReadImageInformation();
    }

    std::stringstream strMakeTab;
    strMakeTab << "create table if not exists \"" << m_TableName << "\" ("
               << "\"" << m_ImageVarName << "\" " << sqlImgDataType;

    int didx = 0, aidx = 0;
    for (int all=0; all < allOtherCols.size(); ++all)
    {
        if (all == 0)
        {
            strMakeTab << ",";
        }

        std::string ctype = "";
        m_ColNames.push_back(allOtherCols[all]);
        if (didx < m_DimColDimId.size())
        {
            otb::AttributeTable::ColumnValue dimVal;
            dimVal.type = otb::AttributeTable::ATTYPE_INT;
            dimVal.ival = 0;
            m_ColValues.push_back(dimVal);
            ctype = "INTEGER";
            ++didx;
        }
        else if (m_AuxVarNames.size() > 0 && nio.IsNotNull())
        {
            otb::AttributeTable::ColumnValue auVal;
            auVal.type = otb::AttributeTable::ATTYPE_DOUBLE;
            auVal.dval = 0.0;
            ctype = "REAL";
            m_AuxIsInteger.push_back(0);
            m_AuxVarDimMap.push_back(nio->getDimMap(m_ImageVarName, m_AuxVarNames[aidx]));
            const netCDF::NcType nctype = nio->getVarType(m_AuxVarNames[aidx]);
            if (!nctype.isNull())
            {
                if (   nctype.getTypeClass() != netCDF::NcType::nc_FLOAT
                    && nctype.getTypeClass() != netCDF::NcType::nc_DOUBLE
                   )
                {
                    m_AuxIsInteger.back() = 1;
                    auVal.type = otb::AttributeTable::ATTYPE_INT;
                    auVal.ival = 0;
                    ctype = "INTEGER";
                }
            }
            m_ColValues.push_back(auVal);
            ++aidx;
        }

        strMakeTab << "\"" << allOtherCols[all] << "\" " << ctype;

        if (all < allOtherCols.size()-1)
        {
            strMakeTab << ",";
        }
    }
    strMakeTab << ");";

    if (!bTableExists)
    {
        if (!m_Tab->SqlExec(strMakeTab.str()))
        {
            NMProcErr(<< "Failed creating table '" << m_TableName << "' in '"
                      << m_TableFileName << "'! " << m_Tab->getLastLogMsg());
            return false;
        }
        bTableExists = true;
    }

    m_Tab->SetTableName(m_TableName);
    m_Tab->PopulateTableAdmin();

    // check whether we've got all columns we need!
    if (bTableExists &&  m_UpdateMode)
    {
        if (m_Tab->ColumnExists(m_ImageVarName) < 0)
        {
            if (!m_Tab->AddColumn(m_ImageVarName, m_ColValues.at(0).type))
            {
                NMProcErr(<< "Failed adding required column '" << m_ImageVarName << "' "
                          << "to the output table '" << m_TableName << "': " << m_Tab->getLastLogMsg());
                return false;
            }
        }

        for (int m=0, r=0; m < m_DimVarNames.size(); ++m)
        {
            if (!m_DimVarNames.at(m).empty())
            {
                if (m_Tab->ColumnExists(m_KeyColNames.at(r)) < 0)
                {
                    if (!m_Tab->AddColumn(m_KeyColNames.at(r), m_ColValues.at(r+1).type))
                    {
                        NMProcErr(<< "Failed adding required column '" << m_KeyColNames.at(r) << "' "
                                  << "to the output table '" << m_TableName << "': " << m_Tab->getLastLogMsg());
                        return false;
                    }
                }
                m_Tab->CreateIndex(m_KeyColNames, false);
                ++r;
            }
        }

        for (int a=0; a < m_AuxVarNames.size(); ++a)
        {
            if (m_Tab->ColumnExists(m_AuxVarNames.at(a)) < 0)
            {
                if (!m_Tab->AddColumn(m_AuxVarNames.at(a), m_ColValues.at(a+1+m_DimColDimId.size()).type))
                {
                    NMProcErr(<< "Failed adding required column '" << m_AuxVarNames.at(a) << "' "
                              << "to the output table '" << m_TableName << "': " << m_Tab->getLastLogMsg());
                    return false;
                }
            }
        }
    }


    // add aux vars (could be a second round and we need to add them!)
    // no harm done if that fails ...
    if (!bTableExists && m_AuxVarNames.size() > 0)
    {
        m_Tab->BeginTransaction();
        for (int ax=0; ax < m_AuxVarNames.size(); ++ax)
        {
            otb::AttributeTable::TableColumnType coltype = ax < m_AuxIsInteger.size()
                                                           ? m_AuxIsInteger[ax] == 1
                                                             ? otb::AttributeTable::ATTYPE_INT
                                                             : otb::AttributeTable::ATTYPE_DOUBLE
                                                           : otb::AttributeTable::ATTYPE_DOUBLE;
            m_Tab->AddColumn(m_AuxVarNames.at(ax), coltype);
        }
        m_Tab->EndTransaction();
    }

    return true;
}

template<class TInputImage>
void Image2TableFilter<TInputImage>
::GenerateData(void)
{
    // pass on the input as the output
    const InputImageType* input = this->GetInput();
    if (input != nullptr)
    {
        this->GraftOutput(const_cast<InputImageType*>(input));
    }

    if (m_TableName.empty())
    {
        NMProcErr(<< "Please specify a non-empty TableName!");
        this->AbortGenerateDataOn();
        return;
    }

    // ================================================================================
    //                              Prepare TABLE (if applicable)
    // ================================================================================
    if (m_Tab.IsNull())
    {
        m_Tab = otb::SQLiteTable::New();
        m_Tab->SetUseSharedCache(false);
        m_Tab->SetDbFileName(m_TableFileName);
        if (!m_Tab->openConnection())
        {
            NMProcErr(<< "Failed connecting to '" << m_TableFileName << "'! "
                      << m_Tab->getLastLogMsg());
            this->AbortGenerateDataOn();
            return;
        }
    }

    // make sure the table is in place
    if (m_PixelCounter == 0)
    {
        if (!PrepTable())
        {
            return;
        }
    }
    else
    {
        m_Tab->SetTableName(m_TableName);
        m_Tab->PopulateTableAdmin();
    }

    // READ AUX VARIABLES FROM NETCDF
    InputImageRegionType inregion = input->GetRequestedRegion();
    InputImageSizeType inputSize = inregion.GetSize();
    InputImageIndexType inputIndex = inregion.GetIndex();

    // ================================================================================
    //                              READ AUX BUFFERS FROM NETCDF
    // ================================================================================
    if (m_AuxVarNames.size() > 0)
    {
        std::string ncimgspec = m_NcImageContainer + ":" + m_NcGroupName + "/" + m_ImageVarName;
        NetCDFIO::Pointer nio = NetCDFIO::New();
        nio->SetFileName(ncimgspec.c_str());
        nio->SetFileName(ncimgspec.c_str());
        if (!nio->CanReadFile(ncimgspec.c_str()))
        {
            NMProcErr(<< "Failed to read image '" << ncimgspec << "'!");
            return;
        }
        nio->ReadImageInformation();

        if (this->GetAbortGenerateData() == true)
        {
            return;
        }

        // just to be on the safe side
        for (int e=0; e < m_AuxBuffer.size(); ++e)
        {
            if (m_AuxIsInteger[e])
            {
                delete[] static_cast<long long*>(m_AuxBuffer[e]);
            }
            else
            {
                delete[] static_cast<double*>(m_AuxBuffer[e]);
            }
        }
        m_AuxBuffer.clear();

        for (int au=0; au < m_AuxVarNames.size(); ++au)
        {
            size_t buflen = 1;
            std::vector<size_t> vidx;
            std::vector<size_t> vlen;
            for (int k=0; k < m_AuxVarDimMap[au].size(); ++k)
            {
                const size_t sz = static_cast<size_t>(inputSize[m_AuxVarDimMap[au][k]]);
                //const size_t sz = static_cast<size_t>(m_Size[m_AuxVarDimMap[au][k]]);
                vlen.push_back(sz);
                vidx.push_back(static_cast<size_t>(inputIndex[m_AuxVarDimMap[au][k]]));
                //vidx.push_back(static_cast<size_t>(m_StartIndex[m_AuxVarDimMap[au][k]]));
                buflen *= sz;
            }
            if (m_AuxIsInteger[au])
            {
                netCDF::NcType lttype(netCDF::NcType::nc_INT64);
                long long* lbuf = new long long[buflen];
                if (!nio->getVar(m_AuxVarNames[au], vidx, vlen, lttype, static_cast<void*>(lbuf)))
                {
                    NMProcWarn(<< "Issue reading '" << m_AuxVarNames[au] << "' ... !");
                }
                m_AuxBuffer.push_back(static_cast<void*>(lbuf));
            }
            else
            {
                netCDF::NcType dttype(netCDF::NcType::nc_DOUBLE);
                double* dbuf = new double[buflen];
                if (!nio->getVar(m_AuxVarNames[au], vidx, vlen, dttype, static_cast<void*>(dbuf)))
                {
                    NMProcWarn(<< "Issue reading '" << m_AuxVarNames[au] << "' ... !");
                }
                m_AuxBuffer.push_back(static_cast<void*>(dbuf));
            }
        }
    }

    // ================================================================================
    //                  WRTIE IMAGE & DIMS & AUX BUFFERS TO TABLE
    // ================================================================================

    using IterType      = itk::ImageRegionConstIteratorWithIndex<InputImageType>;
    using IterIndexType = typename IterType::IndexType;

    IterType imgIter(input, inregion);
    IterIndexType iterIndex;
    itk::ProgressReporter progress(this, 0, inregion.GetNumberOfPixels());


    if (!this->GetAbortGenerateData())
    {
        // ToDo -> always insert or not ?
        if (m_UpdateMode)
        {
            m_Tab->PrepareBulkSet(m_ColNames, m_KeyColNames);
        }
        else
        {
            m_Tab->PrepareBulkSet(m_ColNames, true);
        }
        m_Tab->BeginTransaction();

        imgIter.GoToBegin();
        while(!imgIter.IsAtEnd())
        {
            // main variable value
            if (m_ColValues[0].type == otb::AttributeTable::ATTYPE_DOUBLE)
            {
                m_ColValues[0].dval = static_cast<double>(imgIter.Get());
            }
            else
            {
                m_ColValues[0].ival = static_cast<long long>(imgIter.Get());
            }

            // dimension indices (always of integer type)
            iterIndex = imgIter.GetIndex();
            if (m_UpdateMode)
            {
                for (int k=0; k < m_KeyColDimId.size(); ++k)
                {
                    const long long llval = static_cast<long long>(iterIndex[m_KeyColDimId[k]]);
                    m_KeyColValues[k].ival = llval;
                }
            }
            else
            {
                for (int d=0; d < m_DimColDimId.size(); ++d)
                {
                    const long long llval = static_cast<long long>(iterIndex[m_DimColDimId[d]]);
                    m_ColValues[d+1].ival = llval;
                }
            }


            // auxillary variables (e.g. coordinate variables for nc files)
            const int idoff = m_UpdateMode ? 1 : m_DimColDimId.size() + 1;
            for (int v=0; v < m_AuxVarNames.size(); ++v)
            {
                //size_t boff = iterIndex[m_AuxVarDimMap[v][0]] - m_StartIndex[m_AuxVarDimMap[v][0]];
                size_t boff = iterIndex[m_AuxVarDimMap[v][0]] - inputIndex[m_AuxVarDimMap[v][0]];
                for (int bd=1; bd < m_AuxVarDimMap[v].size(); ++bd)
                {
                    //boff += (iterIndex[m_AuxVarDimMap[v][bd]] - m_StartIndex[m_AuxVarDimMap[v][bd]]) * m_Size[m_AuxVarDimMap[v][bd-1]];
                    boff += (iterIndex[m_AuxVarDimMap[v][bd]] - inputIndex[m_AuxVarDimMap[v][bd]]) * inputSize[m_AuxVarDimMap[v][bd-1]];
                }

                if (m_ColValues[v+idoff].type == otb::AttributeTable::ATTYPE_DOUBLE)
                {
                    m_ColValues[v+idoff].dval = static_cast<double*>(m_AuxBuffer[v])[boff];
                }
                else
                {
                    m_ColValues[v+idoff].ival = static_cast<long long*>(m_AuxBuffer[v])[boff];
                }
            }

            // write values to table
            if (m_UpdateMode)
            {
                if (!m_Tab->DoBulkSet(m_ColValues, m_KeyColValues))
                {
                    NMProcErr(<< m_Tab->getLastLogMsg());
                    m_Tab->EndTransaction();
                    return;
                }
            }
            else
            {
                if (!m_Tab->DoBulkSet(m_ColValues))
                {
                    NMProcErr(<< m_Tab->getLastLogMsg());
                    m_Tab->EndTransaction();
                    return;
                }
            }

            ++imgIter;
            progress.CompletedPixel();
        }
        m_Tab->EndTransaction();
    }

    // ================================================================================
    //                  CLEAN UP: RELEASE BUFFER MEMORY
    // ================================================================================
    for (int r=0; r < m_AuxBuffer.size(); ++r)
    {
        if (m_AuxIsInteger[r])
        {
            delete[] static_cast<long long*>(m_AuxBuffer[r]);
        }
        else
        {
            delete[] static_cast<double*>(m_AuxBuffer[r]);
        }
    }
    m_AuxBuffer.clear();

    m_PixelCounter += inregion.GetNumberOfPixels();

    // tidy up
    if (m_PixelCounter == m_NumPixel)
    {
        ResetPipeline();
    }
}

template<class TInputImage>
void Image2TableFilter<TInputImage>
::ResetPipeline()
{
    m_NumPixel = 0;
    m_PixelCounter = 0;
    m_Tab->CloseTable();
    m_ColNames.clear();
    m_ColValues.clear();
    m_AuxIsInteger.clear();
    m_vRAT.clear();
    m_KeyColNames.clear();
    m_KeyColValues.clear();
    m_KeyColDimId.clear();
    m_DimColDimId.clear();
}

} // end of namespace otb


