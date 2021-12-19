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
 * otbNMMosraFilter.txx
 *
 *  Created on: 03/03/2017
 *      Author: Alex Herzig
 *
 */

#include "NMMosra.h"

#include <ctime>

#include "otbNMMosraFilter.h"
#include "otbNMTableReader.h"
#include "vtkSmartPointer.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "NMvtkDelimitedTextWriter.h"

namespace otb
{

template< class TInputImage, class TOutputImage >
NMMosraFilter< TInputImage, TOutputImage >::NMMosraFilter()
    : m_TimeOut(-1), m_Workspace(""), m_CompName(""),
      m_GenerateReports(false)
{
    m_DataSet = new NMMosraDataSet();

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
NMMosraFilter< TInputImage, TOutputImage >::~NMMosraFilter()
{
    if (m_DataSet)
    {
        delete m_DataSet;
    }
}


template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::setRAT(unsigned int idx, AttributeTable::Pointer tab)
{
    // we only handle one input table in total, so
    // replace any previously set once
    if (m_DataSet && tab.IsNotNull())
    {
        m_DataSet->setDataSet(tab);
    }
    else
    {
        NMProcWarn(<< "NULL-table provided!")
    }
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer NMMosraFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    AttributeTable::Pointer tab;
    if (m_DataSet && m_DataSet->getDataSetType() == NMMosraDataSet::NM_MOSRA_DS_OTBTAB)
    {
        tab = m_DataSet->getOtbAttributeTable();
    }

    return tab;
}

template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::GenerateInputRequestedRegion()
{
    InputImagePointerType inputPtr =
            const_cast< InputImageType *>(this->GetInput());

    if (inputPtr.IsNotNull())
    {
        typename TInputImage::RegionType inputReqRegion;
        inputReqRegion = inputPtr->GetRequestedRegion();

        for (unsigned int d=0; d < InputImageDimension; ++d)
        {
            inputReqRegion.SetSize(d, 1);
        }
        inputPtr->SetRequestedRegion(inputReqRegion);
    }

    // in case we've got just a table reader (as instead of an image
    // reader - this is an image to image filter after all) as 'proper' input,
    // we have to make sure that the table(s) is (are) actually read, and that's
    // what we're doing now ...

    otb::SQLiteTable::Pointer sqltab =
            static_cast<otb::SQLiteTable*>(this->m_DataSet->getOtbAttributeTable().GetPointer());

    if (sqltab.IsNotNull())
    {
        if (sqltab->GetDbConnection() == 0)
        {
            if (sqltab->GetSource())
            {
                typename itk::SmartPointerForwardReference<itk::ProcessObject> src =
                        sqltab->GetSource();

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
void NMMosraFilter< TInputImage, TOutputImage >
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

    NMMosra* mosra = new NMMosra();
    mosra->setScenarioName(m_ScenarioName.c_str());
    mosra->setItkProcessObject(this);
    mosra->parseStringSettings(m_LosSettings.c_str());
    mosra->setDataSet(m_DataSet->getOtbAttributeTable());
    if (m_TimeOut >= 0)
    {
        mosra->setBreakAtFirst(false);
        mosra->setTimeOut(m_TimeOut);
    }
    else
    {
        mosra->setTimeOut(0);
        mosra->setBreakAtFirst(true);
    }

    //this->SetProgress(0.8);
    //mosra->mapLp();

    // get the current time
    time_t timestamp;
    struct tm* timeinfo;
    time(&timestamp);
    timeinfo = localtime(&timestamp);
    static char curTime[128];
    sprintf(curTime, "%.2d-%.2d-%.2dT%.2d-%.2d-%.2d",
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);

    // write report and lp file (i.e. equations) into
    // lumass workspace or into data folder, if m_GenerateReports has been set
    if (m_GenerateReports)
    {
        m_Workspace = mosra->getDataPath().toStdString();
    }
    std::string descriptor = m_CompName;
    if (!m_ScenarioName.empty())
    {
        descriptor = m_ScenarioName;
    }

    std::stringstream fns;
    fns << m_Workspace << "/report_" << descriptor << "_" << curTime << ".txt";
    std::string reportFN = fns.str();

    fns.str("");
    fns << m_Workspace << "/lp_" << descriptor << "_" << curTime << ".lp";
    std::string lpFN = fns.str();

    fns.str("");
    fns << m_Workspace << "/los_" << descriptor << "_" << curTime << ".txt";
    std::string losFN = fns.str();

    QFile losFile(losFN.c_str());
    if (losFile.open(QIODevice::WriteOnly))
    {
        QByteArray bar(m_LosSettings.c_str());
        losFile.write(bar);
        losFile.close();
    }



    // this will write the problem before it's attampted to being solved
    mosra->writeProblem(lpFN.c_str(), NMMosra::NM_MOSO_LP);

    // solve the problem
    this->UpdateProgress(0.2);
    static char solveTime[128];
    if (mosra->solveLp())
    {
        time(&timestamp);
        timeinfo = localtime(&timestamp);
        sprintf(solveTime, "%.2d-%.2d-%.2dT%.2d-%.2d-%.2d",
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);

        this->SetProgress(0.8);
        if (mosra->mapLp() && this->m_GenerateReports)
        {
            this->GenerateReportData(mosra, solveTime);
        }
    }
    else
    {
        this->SetProgress(0.8);
        NMProcErr( << "optimisation failed!");
    }

    mosra->writeReport(reportFN.c_str());

    this->SetProgress(1.0);
    delete mosra;
}

template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::GenerateReportData(NMMosra* mosra, char* theTime)
{
    if (mosra == nullptr)
    {
        return;
    }

    QString path = mosra->getDataPath();
    if (path.isEmpty())
    {
        path = this->m_Workspace.c_str();
    }
    QString losName, tabName, resName, chgName, relName, totName;

    QString scen = m_ScenarioName.empty() ? "" : QString("_%1").arg(m_ScenarioName.c_str());

    //losName = QString("%1/los%2_%3.txt").arg(path).arg(scen).arg(theTime);
    tabName = QString("%1/tab%2_%3.csv").arg(path).arg(scen).arg(theTime);
    resName = QString("%1/res%2_%3.csv").arg(path).arg(scen).arg(theTime);
    chgName = QString("%1/chg%2_%3.csv").arg(path).arg(scen).arg(theTime);
    relName = QString("%1/rel%2_%3.csv").arg(path).arg(scen).arg(theTime);
    totName = QString("%1/rel%2_%3.csv").arg(path).arg(scen).arg(theTime);

    vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();

    vtkSmartPointer<vtkTable> chngmatrix;
    vtkSmartPointer<vtkTable> sumres = mosra->sumResults(chngmatrix);

    NMvtkDelimitedTextWriter* writer = NMvtkDelimitedTextWriter::New();
    writer->SetFieldDelimiter(",");

    writer->SetInputData(tab);
    writer->SetFileName(tabName.toStdString().c_str());
    writer->Update();

    writer->SetInputData(sumres);
    writer->SetFileName(resName.toStdString().c_str());
    writer->Update();

    writer->SetInputData(chngmatrix);
    writer->SetFileName(chgName.toStdString().c_str());
    writer->Update();

}


} // END OF OTB NAMESPACE
