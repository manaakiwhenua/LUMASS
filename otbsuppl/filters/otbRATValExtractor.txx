/*=========================================================================

  Program:   ORFEO Toolbox
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
  See OTBCopyright.txt for details.

  Some parts of this code are derived from ITK. See ITKCopyright.txt
  for details.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

/*=========================================================================
 *
 * Parts of the code in this file are taken from itkImageToImageFilter.hxx
 * which is licenced as follows:
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
 */

/******************************************************************************
 * Adapted by Alexander Herzig
 * Copyright 2021 Landcare Research New Zealand Ltd
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


#ifndef __otbRATValExtractor_txx
#define __otbRATValExtractor_txx
#include "otbRATValExtractor.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkNumericTraits.h"
#include "itkProgressReporter.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"

#include "nmlog.h"

#include <iostream>
#include <string>

namespace otb
{

/** Constructor */
template <class TImage>
RATValExtractor<TImage>
::RATValExtractor()
{
    //This number will be incremented each time an image
    //is added over the one minimumrequired
    this->SetNumberOfRequiredInputs( 1 );
    this->InPlaceOff();

    m_NbExpr = 1;
    m_UnderflowCount = 0;
    m_OverflowCount = 0;
    m_ThreadUnderflow.SetSize(1);
    m_ThreadOverflow.SetSize(1);
    m_ConcatChar = "__";
    m_UseTableColumnCache = false;

    for (int t=0; t < this->GetNumberOfThreads(); ++t)
    {
        std::vector<TablePointer> vT;
        m_VRAT.push_back(vT);
    }
}

/** Destructor */
template <class TImage>
RATValExtractor<TImage>
::~RATValExtractor()
{
}

template <class TImage>
void RATValExtractor<TImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);

    os << indent << "Expression: "      << m_Expression                  << std::endl;
    os << indent << "Computed values follow:"                            << std::endl;
    os << indent << "UnderflowCount: "  << m_UnderflowCount              << std::endl;
    os << indent << "OverflowCount: "   << m_OverflowCount               << std::endl;
    os << indent << "itk::NumericTraits<PixelType>::NonpositiveMin()  :  "
       << itk::NumericTraits<PixelType>::NonpositiveMin()      << std::endl;
    os << indent << "itk::NumericTraits<PixelType>::max()  :             "
       << itk::NumericTraits<PixelType>::max()                 << std::endl;
}

template<class TImage>
void RATValExtractor<TImage>
::SetNthAttributeTable(unsigned int idx, AttributeTable::Pointer tab,
                       std::vector<std::string> vAttrNames	)
{
    // exclude invalid settings
    if (idx < 0)// || tab.IsNull() || tab.GetPointer() == 0)
    {
        itkExceptionMacro(<< "Invalid index for ::SetNthAttributeTable()!");
        return;
    }

    otb::SQLiteTable::Pointer sqlTab = 0;
    if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE)
    {
        sqlTab = static_cast<otb::SQLiteTable*>(tab.GetPointer());
        if (sqlTab->GetDbConnection() == 0)
        {
            if (!sqlTab->openConnection() || !sqlTab->PopulateTableAdmin())
            {
                itkExceptionMacro(<< "Failed connecting to "
                                  << sqlTab->GetDbFileName() << ": "
                                  << sqlTab->getLastLogMsg());
                return;
            }
        }
    }


    std::vector<int> columns;
    std::vector<ColumnType> types;
    if (tab.IsNotNull())
    {
        std::string extrcol = m_Expression;
        if (extrcol.empty() && vAttrNames.size() > 0)
        {
            extrcol = vAttrNames[0];
        }

        int c = -1;
        if ((c = tab->ColumnExists(extrcol) >= 0))
        {
                columns.push_back(c);

                if (tab->GetColumnType(c) == otb::AttributeTable::ATTYPE_STRING)
                {
                    itkExceptionMacro(<< "Don't support extraction of STRING values!");
                }
                types.push_back(tab->GetColumnType(c));
        }
        else
        {
            itkExceptionMacro(<< "Please provide the column to be extracted as 'MapExpression' or 'Attribute Names'!");
        }
    }

    int nt = this->GetNumberOfThreads();

    for (int i=this->m_VRAT[0].size(); i < idx; ++i)
    {
        for (int t=0; t < nt; ++t)
        {
            this->m_VRAT[t].push_back(0);
        }

        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    if (this->m_VAttrTypes.size() == idx)
    {
        this->m_VRAT[0].push_back(tab);
        for (int t=1; t < nt; ++t)
        {
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
            {
                this->m_VRAT[t].push_back(tab);
            }
            else
            {
                //otb::SQLiteTable::Pointer stab = static_cast<otb::SQLiteTable*>(tab.GetPointer());
                otb::SQLiteTable::Pointer thtab = otb::SQLiteTable::New();
                thtab->SetUseSharedCache(false);
                thtab->SetOpenReadOnly(true);
                if (thtab->CreateTable(sqlTab->GetDbFileName(), "1") != otb::SQLiteTable::ATCREATE_READ)
                {
                    itkExceptionMacro(<< "Failed creating table connection for thread: "
                                      << thtab->getLastLogMsg());
                    return;
                }
                otb::AttributeTable::Pointer ctp = static_cast<otb::AttributeTable*>(thtab.GetPointer());
                this->m_VRAT[t].push_back(ctp);
            }
        }

        this->m_VAttrTypes.push_back(types);
        this->m_VTabAttr.push_back(columns);
    }
    else
    {
        this->m_VRAT[0][idx] = tab;
        for (int t=1; t < nt; ++t)
        {
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
            {
                this->m_VRAT[t][idx] = tab;
            }
            else
            {
                otb::SQLiteTable::Pointer thtab = otb::SQLiteTable::New();
                thtab->SetUseSharedCache(false);
                thtab->SetOpenReadOnly(true);
                if (thtab->CreateTable(sqlTab->GetDbFileName(), "1") != otb::SQLiteTable::ATCREATE_READ)
                {
                    itkExceptionMacro(<< "Failed creating table connection for thread: "
                                      << thtab->getLastLogMsg());
                    return;
                }
                this->m_VRAT[t][idx] = thtab;
            }
        }

        this->m_VAttrTypes[idx] = types;
        this->m_VTabAttr[idx] = columns;
    }


    if (m_UseTableColumnCache)
    {
        CacheTableColumns(idx);
    }

    this->Modified();
}

template<class TImage>
void RATValExtractor<TImage>
::ResetPipeline()
{
    m_TableColumnDblCache.clear();
    m_TableColumnLLongCache.clear();
    m_VAttrTypes.clear();
    m_VTabAttr.clear();
    m_VRAT.clear();
}


template<class TImage>
void RATValExtractor<TImage>
::CacheTableColumns(int idx)
{
    if (idx < m_VRAT[0].size())
    {
        int t = idx;
        otb::AttributeTable::Pointer tab = m_VRAT[0].at(t);

        // there's no point in using a cache for an already cached
        // RAM-based table, really
        if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_RAM)
        {
            m_UseTableColumnCache = false;
            m_TableColumnDblCache.clear();
            m_TableColumnLLongCache.clear();
            return;
        }


        otb::SQLiteTable::Pointer dbtab = static_cast<otb::SQLiteTable*>(tab.GetPointer());

        std::map<int, std::map<long long, double> > tab_Dblmap;
        std::map<int, std::map<long long, long long> > tab_LLongmap;

        std::vector<std::string> colnames;
        colnames.push_back(dbtab->GetPrimaryKey());

        for (int c=0; c < m_VTabAttr.at(t).size(); ++c)
        {
            std::string col_name = tab->GetColumnName(m_VTabAttr.at(t).at(c));
            if (col_name.compare(colnames[0]) != 0)
            {
                colnames.push_back(col_name);
            }

            if (m_VAttrTypes.at(t).at(c) == otb::AttributeTable::ATTYPE_INT)
            {
                std::map<long long, long long> llong_map;
                tab_LLongmap.insert(std::pair<int, std::map<long long, long long> >(
                                   m_VTabAttr.at(t).at(c), llong_map));
            }
            else
            {
                std::map<long long, double> dbl_map;
                tab_Dblmap.insert(std::pair<int, std::map<long long, double> >(
                                   m_VTabAttr.at(t).at(c), dbl_map));
            }
        }

        if (m_VAttrTypes.at(0).at(0) == otb::AttributeTable::ATTYPE_INT)
        {
            if (!dbtab->GreedyNumericFetch(colnames, tab_LLongmap))
            {
                m_UseTableColumnCache = false;
                m_TableColumnLLongCache.clear();
                return;
            }
            m_TableColumnLLongCache.push_back(tab_LLongmap);
        }
        else
        {
            if (!dbtab->GreedyNumericFetch(colnames, tab_Dblmap))
            {
                m_UseTableColumnCache = false;
                m_TableColumnDblCache.clear();
                return;
            }
            m_TableColumnDblCache.push_back(tab_Dblmap);
        }
    }
}

template<class TImage>
AttributeTable::Pointer RATValExtractor<TImage>
::GetNthAttributeTable(unsigned int idx)
{
    if (idx < 0 || idx >= m_VRAT[0].size())
        return 0;

    return m_VRAT[0][idx];
}

template<class TImage>
std::vector<std::string> RATValExtractor<TImage>
::GetNthTableAttributes(unsigned int idx)
{
    std::vector<std::string> ret;
    if (idx < 0 || idx >= m_VTabAttr.size())
    {
        return ret;
    }

    for (int i=0; i < m_VTabAttr[idx].size(); ++i)
        ret.push_back(m_VRAT[0][idx]->GetColumnName(m_VTabAttr[idx][i]));

    return ret;
}


template <class TImage>
void RATValExtractor<TImage>
::SetNthInput(unsigned int idx, const ImageType * image)
{
    this->SetInput(idx, const_cast<TImage *>( image ));
    unsigned int nbInput = idx+1;
    while (nbInput < this->GetNumberOfInputs())
    {
        this->RemoveInput(this->GetNumberOfInputs()-1);
    }

    m_VVarName.resize(nbInput+4);
    std::ostringstream varName;
    varName << "b" << nbInput;
    m_VVarName[idx] = varName.str();
    m_VVarName[idx+1] = "idxX";
    m_VVarName[idx+2] = "idxY";
    m_VVarName[idx+3] = "idxPhyX";
    m_VVarName[idx+4] = "idxPhyY";


    // increase the RAT related vectors according to the
    // given idx, if necessary
    for (int i=this->m_VRAT[0].size(); i <= idx; ++i)
    {
        for (int t=0; t < this->GetNumberOfThreads(); ++t)
        {
            this->m_VRAT[t].push_back(0);
        }

        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    this->Modified();

}

template <class TImage>
void RATValExtractor<TImage>
::SetNthInput(unsigned int idx, const ImageType * image, const std::string& varName)
{
    this->SetInput(idx, const_cast<TImage *>( image ));
    unsigned int nbInput = idx+1;
    while (nbInput < this->GetNumberOfInputs())
    {
        this->RemoveInput(this->GetNumberOfInputs()-1);
    }

    m_VVarName.resize(nbInput);
    m_VVarName[idx] = varName;
    m_VVarName[idx+1] = "idxX";
    m_VVarName[idx+2] = "idxY";
    m_VVarName[idx+3] = "idxPhyX";
    m_VVarName[idx+4] = "idxPhyY";

    // increase the RAT related vectors according to the
    // given idx, if necessary
    for (int i=this->m_VRAT[0].size(); i <= idx; ++i)
    {
        for (int t=0; t < this->GetNumberOfThreads(); ++t)
        {
            this->m_VRAT[t].push_back(0);
        }
        std::vector<int> columns;
        this->m_VTabAttr.push_back(columns);
        std::vector<ColumnType> types;
        this->m_VAttrTypes.push_back(types);
    }

    this->Modified();
}

template <class TImage>
void RATValExtractor<TImage>
::SetNthInputName(unsigned int idx, const std::string& varName)
{
    m_UserNames.resize(idx+1);
    m_UserNames[idx] = varName;
    this->Modified();
}

template <typename TImage>
TImage * RATValExtractor<TImage>
::GetNthInput(unsigned int idx)
{
    return const_cast<TImage *>(this->GetInput(idx));
}

template< typename TImage >
void RATValExtractor<TImage>
::SetExpression(const std::string& expression)
{
    if (m_Expression != expression)
        m_Expression = expression;
    this->Modified();
}

template< typename TImage >
std::string RATValExtractor<TImage>
::GetExpression() const
{
    return m_Expression;
}

template< typename TImage >
std::string RATValExtractor<TImage>
::GetNthInputName(unsigned int idx) const
{
    return idx < m_VVarName.size() ? m_VVarName.at(idx) : "";
}

template< typename TImage >
void RATValExtractor<TImage>
::ThreadedGenerateData(const ImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
    unsigned int j, r;
    unsigned int nbInputImages = this->GetNumberOfInputs();
    unsigned int nbOutputImages = this->GetNumberOfOutputs();

    typedef itk::ImageRegionConstIterator<TImage> ImageRegionConstIteratorType;
    typedef itk::ImageRegionIterator<TImage> ImageRegionIteratorType;

    std::vector<ImageRegionConstIteratorType> Vit;
    Vit.resize(nbInputImages);
    for (j = 0; j < nbInputImages; j++)
    {
        Vit[j] = ImageRegionConstIteratorType(this->GetNthInput(j),
                                              outputRegionForThread);
    }

    std::vector<ImageRegionIteratorType> Vot;
    Vot.resize(nbOutputImages);
    for (r = 0; r < nbOutputImages; ++r)
    {
        Vot[r] = ImageRegionIteratorType(this->GetOutput(r),
                                         outputRegionForThread);
    }

    // support progress methods/callbacks
    itk::ProgressReporter progress(this, threadId,
                                   outputRegionForThread.GetNumberOfPixels());

    PixelType theVal;

    while (!Vit.at(0).IsAtEnd())
    {
        const ColumnType t = m_VAttrTypes[0][0];
        const int colidx = m_VTabAttr[0][0];
        const long long rowidx = static_cast<long long>(Vit[0].Get());
        switch(t)
        {
        case otb::AttributeTable::ATTYPE_INT:
            theVal = static_cast<PixelType>(m_TableColumnLLongCache[0][colidx][rowidx]);
            break;
        case otb::AttributeTable::ATTYPE_DOUBLE:
            theVal = static_cast<PixelType>(m_TableColumnDblCache[0][colidx][rowidx]);
            break;
        default:
            ;
        }

        Vot.at(0).Set(theVal);

        ++(Vot.at(0));
        ++(Vit.at(0));

        progress.CompletedPixel();
   }
}

}// end namespace otb

#endif
