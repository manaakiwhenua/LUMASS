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
#include "itkSmartPointerForwardReference.h"
#include "otbNMTableReader.h"

namespace otb
{

template< class TInputImage, class TOutputImage >
SQLiteProcessor< TInputImage, TOutputImage >
::SQLiteProcessor()
    : m_SQLStatement("")
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
void SQLiteProcessor< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer tab)
{
    // we only handle sqlite tables here, so we
    // refuse any other tables
    if (    tab.IsNotNull()
        &&  tab->GetTableType() == AttributeTable::ATTABLE_TYPE_SQLITE
       )
    {
        SQLiteTable* tabptr = static_cast<SQLiteTable*>(tab.GetPointer());
        if (tabptr != nullptr)
        {
            SQLiteTable::Pointer sqltab = tabptr;
            m_vRAT.resize(idx + 1);
            m_vRAT[idx] = sqltab;
            return;
        }
    }

    itkExceptionMacro(<< "Input table #" << idx << " is not a valid SQLite database!")
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

// since we're not really working on the image's bulk
// data, we just set a 1 pixel wide region
// to satisfy the pipeline but minimise data reading
template< class TInputImage, class TOutputImage >
void SQLiteProcessor< TInputImage, TOutputImage >
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
void SQLiteProcessor< TInputImage, TOutputImage >
::GenerateData()
{
    // since we don't do any real image processing, we just wave
    // the image data through and process the RATs only once
    // (m_ProcessedPixel)

    const TInputImage* img = this->GetInput(0);
    if (img)
    {
        this->GraftOutput(const_cast<TInputImage*>(img));
    }

    this->UpdateProgress(0.05);

    // =========================================================================
    // now we do all the table processing here

    if (m_vRAT.size() == 0)
    {
        itkExceptionMacro(<< "Please provide at least ONE valid SQLite database as input!")
        return;
    }
    else if (m_vRAT.at(0).GetPointer() == nullptr)
    {
        itkExceptionMacro(<< "Input table #0 is not valid SQLite database!")
        return;
    }

    if (m_SQLStatement.empty())
    {
        itkExceptionMacro(<< "SQL statement is empy!")
        return;
    }

    if (m_ImageNames.size() < m_vRAT.size())
    {
        itkExceptionMacro(<< "Please provide a 'UserID' for each input layer/table");
        return;
    }

    for (int i=1; i < m_vRAT.size(); ++i)
    {
        if (m_vRAT.at(i).GetPointer() == nullptr || !m_vRAT.at(0)->AttachDatabase(m_vRAT.at(i)->GetDbFileName(),
                                          m_ImageNames.at(i)))
        {
            if (m_vRAT.at(0)->getLastLogMsg().find("already attached") != std::string::npos)
            {
                NMProcInfo(<< "Database '" << m_vRAT.at(i)->GetDbFileName() << "' "
                    << "is already attached.");
            }
            else
            {
                NMProcWarn(<< "Failed attaching input databases - "
                    << m_vRAT.at(0)->getLastLogMsg());
                //return;
            }
        }
        //else
        //{
        //    NMProcWarn(<< "LUMASS did not attach input database #" << i << "!");
        //}
    }

    this->UpdateProgress(0.2);
    if (!m_vRAT.at(0)->SqlExec(m_SQLStatement))
    {
        // we only report an error for the following
        // conditions:
        // - database is locked
        // - database is readonly
        //
        // all other conditions are treated as OK; for instance
        // we just give a warning if try to a add a column that
        // is already present and in general doesn't affect the
        // overall modelling run


        std::string lastlog = m_vRAT.at(0)->getLastLogMsg();
        if (    lastlog.find("database is locked") != std::string::npos
            ||  lastlog.find("database is readonly") != std::string::npos
           )
        {
            NMProcErr(<< "SQL processing failed - "
                      << lastlog);
            itkExceptionMacro(<< "SQL processing failed - "
                              << lastlog);
        }
        else
        {
            NMProcWarn(<< "SQL processing failed - "
                       << lastlog);
        }
    }

    for (int i=1; i < m_vRAT.size(); ++i)
    {
        if (!m_vRAT.at(0)->DetachDatabase(m_ImageNames.at(i)))
        {
            //NMDebugCtx(ctx, << "done!");
            // we just warn here, because we had problems with detaching the
            // db for unknown reasons; no harm done, because we'll get
            // a proper exception when we try to use the this db
            // and it is still locked; attachement should be released
            // once the otbSQLiteTable pointer goes out of scope and
            // the host database is closed anyway ...
            NMProcWarn(<< "Failed detaching databases - "
                       << m_vRAT.at(0)->getLastLogMsg());
           // return;
        }
    }
    this->UpdateProgress(0.8);

    if (!m_vRAT.at(0)->PopulateTableAdmin())
    {
        NMProcWarn(<< "Failed repopulating the main table's admin structures! "
                          << m_vRAT.at(0)->getLastLogMsg());
    }

    this->UpdateProgress(1.0);


    for (int d=0; d < m_vRAT.size(); ++d)
    {
        if (m_vRAT.at(d).IsNull())
        {
            continue;
        }

        // provenance information
        std::stringstream sstr;

        // create table entity
        std::vector<std::string> args;
        std::vector<std::string> attrs;

        sstr << "db:" << m_vRAT.at(d)->GetTableName();
        args.push_back(sstr.str());

        sstr.str("");
        sstr << "nm:DbFileName=\"" << m_vRAT.at(d)->GetDbFileName() << "\"";
        attrs.push_back(sstr.str());

        sstr.str("");
        sstr << "nm:TableName=\"" << m_vRAT.at(d)->GetTableName() << "\"";
        attrs.push_back(sstr.str());

        NMProcProvN(itk::NMLogEvent::NM_PROV_ENTITY, args, attrs);

        // provn used by
        args.clear();
        attrs.clear();

        sstr.str("");
        sstr << "db:" << m_vRAT.at(d)->GetTableName();
        args.push_back(sstr.str());

        NMProcProvN(itk::NMLogEvent::NM_PROV_USAGE, args, attrs);

        if (d == 0)
        {
            args.clear();
            attrs.clear();

            sstr.str("");
            sstr << "db:" << m_vRAT.at(d)->GetTableName();
            args.push_back(sstr.str());

            sstr.str("");
            sstr << "db:" << m_vRAT.at(d)->GetTableName();
            args.push_back(sstr.str());

            sstr.str("");
            sstr << "prov:type='prov:Revision'";
            attrs.push_back(sstr.str());

            NMProcProvN(itk::NMLogEvent::NM_PROV_DERIVATION, args, attrs);
        }
    }
}


}

#endif // TXX DEFINITION
