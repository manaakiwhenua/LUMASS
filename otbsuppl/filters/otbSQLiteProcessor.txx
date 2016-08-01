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
 * otbSQLiteProcessor.txx
 *
 *  Created on: 08/11/2015
 *      Author: alex
 *
 */


#ifndef __otbSQLiteProcessor_txx
#define __otbSQLiteProcessor_txx

#include "nmlog.h"
#include "otbSQLiteProcessor.h"

namespace otb
{

template< class TInputImage, class TOutputImage >
SQLiteProcessor< TInputImage, TOutputImage >
::SQLiteProcessor()
    : m_SQLStatement(""),
      m_ProcessedPixel(0)
{
    this->SetNumberOfRequiredInputs(1);
        this->SetNumberOfRequiredOutputs(1);

}

template< class TInputImage, class TOutputImage >
void SQLiteProcessor< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer tab)
{
    // we only handle sqlite tables here, so we
    // refuse any other tables
    if (    tab.IsNotNull()
        &&  tab->GetTableType() == AttributeTable::ATTABLE_TYPE_SQLITE
       )
    {
        SQLiteTable::Pointer sqltab = static_cast<SQLiteTable*>(tab.GetPointer());

        m_vRAT.resize(idx+1);
        m_vRAT[idx] = sqltab;
    }
    else
    {
        itkWarningMacro(<< "NULL-table or non-SQLite-table provided!")
    }
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer SQLiteProcessor< TInputImage, TOutputImage >
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

//template< class TInputImage, class TOutputImage >
//void SQLiteProcessor< TInputImage, TOutputImage >
//::SetImageNames(const std::vector<std::string>& imgNames)
//{
//    m_ImageNames.clear();
//    for (int i=0; i < imgNames.size(); ++i)
//    {
//        m_ImageNames.push_back(imgNames.at(i));
//    }
//}

template< class TInputImage, class TOutputImage >
void SQLiteProcessor< TInputImage, TOutputImage >
::GenerateData()
{
    NMDebugCtx(ctx, << "...");

    // since we don't do any real image processing, we just wave
    // the image data through (in case this filter is part of a
    // streamed processing pipeline); we process the RATs only when
    // the final piece of data processed (and this is because, if
    // we've got a StreamingRATImageFileWriter running in update mode
    // downstream, we don't want to write the RAT each time again, but only once

    const TInputImage* img = this->GetInput(0);
    this->GraftOutput(const_cast<TInputImage*>(img));

    long long totalPixel = img->GetLargestPossibleRegion().GetNumberOfPixels();
    long long procPixel  = img->GetRequestedRegion().GetNumberOfPixels();

    m_ProcessedPixel += procPixel;
    if (m_ProcessedPixel < totalPixel)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // =========================================================================
    // now we do all the table processing here

    if (m_SQLStatement.empty())
    {
        NMDebugCtx(ctx, << "done!");
        itkExceptionMacro(<< "SQL statement is empy!")
        return;
    }

    if (m_ImageNames.size() < m_vRAT.size())
    {
        NMDebugCtx(ctx, << "done!");
        itkExceptionMacro(<< "Please provide a 'UserID' for each input layer/table");
        return;
    }

    for (int i=1; i < m_vRAT.size(); ++i)
    {
        if (!m_vRAT.at(0)->AttachDatabase(m_vRAT.at(i)->GetDbFileName(),
                                          m_ImageNames.at(i)))
        {
            NMDebugCtx(ctx, << "done!");
            itkExceptionMacro(<< "Failed attaching input databases!");
            return;
        }
    }

    if (!m_vRAT.at(0)->SqlExec(m_SQLStatement))
    {
        NMDebugCtx(ctx, << "done!");

        // we just give a warning here, since we might do things
        // like adding a column; and if the column is already
        // present, the overall modelling run is not interrupted
        itkWarningMacro(<< "SQL processing failed!");
        return;
    }


    for (int i=1; i < m_vRAT.size(); ++i)
    {
        if (!m_vRAT.at(i)->DetachDatabase(m_ImageNames.at(i)))
        {
            NMDebugCtx(ctx, << "done!");
            // we just warn here, because we had problems with detaching the
            // db for unknown reasons; no harm done, because we'll get
            // a proper exception when we try to use the this db
            // and it is still locked; attachement should be released
            // once the otbSQLiteTable pointer goes out of scope and
            // the host database is closed anyway ...
            itkWarningMacro(<< "Failed detaching databases!");
            return;
        }
    }

    if (!m_vRAT.at(0)->PopulateTableAdmin())
    {
        NMDebugCtx(ctx, << "done!");
        itkExceptionMacro(<< "Failed repopulating the main table's admin structures!");
    }

    m_ProcessedPixel = 0;

}

template< class TInputImage, class TOutputImage >
void SQLiteProcessor< TInputImage, TOutputImage >
::ResetPipeline()
{
    m_ProcessedPixel = 0;
}


}

#endif // TXX DEFINITION
