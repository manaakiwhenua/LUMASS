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

#include <ctime>
#include <cstdio>

#include <QFileInfo>

#include "otbNMMosraFilter.h"
#include "otbNMTableReader.h"
#include "vtkSmartPointer.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "NMvtkDelimitedTextWriter.h"

#include "NMMosra.h"

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
    // need to set them after parsing string settings as
    // the latter is resetting the LosFileName to "" !!!
    mosra->setLosFileName(m_LosFileName.c_str());
    mosra->setDataSet(m_DataSet->getOtbAttributeTable());

    const NMMosra::NMSolverType solver = mosra->getSolverType();

    switch(solver)
    {
    case NMMosra::NM_SOLVER_GA:
        openGA(mosra);
        break;
    case NMMosra::NM_SOLVER_IPOPT:
        ipOpt(mosra);
        break;
    case NMMosra::NM_SOLVER_LPSOLVE:
    default:
        lpSolve(mosra);
        break;
    }

    this->SetProgress(1.0);
    delete mosra;
}

template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::openGA(NMMosra *mosra)
{
    mosra->solveOpenGA();
}

template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::lpSolve(NMMosra *mosra)
{
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
    this->UpdateProgress(0.25);

    // solve the problem
    static char solveTime[128];
    if (mosra->solveLp())
    {
        this->UpdateProgress(0.75);
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
            this->UpdateProgress(0.9);
            this->GenerateReportData(mosra, solveTime);
        }
        this->UpdateProgress(0.9);
    }
    else
    {
        this->SetProgress(0.9);
        NMProcErr( << "optimisation failed!");
    }

    mosra->writeReport(reportFN.c_str());
}

template< class TInputImage, class TOutputImage >
bool NMMosraFilter< TInputImage, TOutputImage >
::needToUpdateNlFile(const std::string& nlFile, const std::string& losFile)
{
#ifdef _WIN32
#define stat64 _stat64
#endif

    bool ret = false;

    struct stat64 resLos;
    if (stat64(losFile.c_str(), &resLos) == 0)
    {
        struct stat64 resNl;
        if (stat64(nlFile.c_str(), &resNl) == 0)
        {
            std::stringstream infomsg;
            // los is more recent
            if (resLos.st_mtime > resNl.st_mtime)
            {
                infomsg << "NMMosraFilter::needToUpdateNlFile() - Yes, the "
                        << " provided LOS file '" << losFile
                        << "' is newer than the NL file '" << nlFile
                        << "'!";
                ret = true;
            }
            else
            {
                infomsg << "NMMosraFilter::needToUpdateNlFile() - No, the "
                        << " provided LOS file '" << losFile
                        << "' is older than the NL file '" << nlFile
                        << "'!";
            }
            NMProcInfo(<< infomsg.str());
        }
        else
        {
            ret = true;
        }
    }
    else
    {
        std::stringstream errmsg;
        errmsg << "NMMosraFilter::needToUpdateNlFile() - Couldn't access "
               << "the provided LOS file '" << losFile << "'!";
        NMProcErr(<< errmsg.str());
    }

    return ret;
}

template< class TInputImage, class TOutputImage >
void NMMosraFilter< TInputImage, TOutputImage >
::ipOpt(NMMosra *mosra)
{

    QFileInfo losInfo(mosra->getLosFileName());
    QString nlFileName = QString("%1/%2.nl")
            .arg(losInfo.canonicalPath())
            .arg(losInfo.baseName());

    bool bDoLUCControl = false;
    if (mosra->getEnableLUCControl())
    {
        bDoLUCControl = true;
        NMProcInfo(<< "backing up LUCControl ...");
        mosra->backupLUCControl();
    }

    // =======================================================
    // ==           LUCControl Loop                       ====
    // =======================================================
    this->UpdateProgress(0.1);
    int iRound = 1;
    do
    {
        // whether we need to (re-create) the NL file, or whether
        // we're re-using an already available one
        //if (    this->needToUpdateNlFile(nlFileName.toStdString(), mosra->getLosFileName().toStdString())
        //     || bDoLUCControl
        //   )
        {
            QFile nlfile(nlFileName);
            nlfile.remove();

            // creates *.nl file
            if (!mosra->solveProblem())
            {
                NMProcErr(<< "Failed creating the *.nl file!");
                return;
            }
        }
        //else
        //{
        //    QString solFN = QString("%1/%2.dvars").arg(losInfo.canonicalPath()).arg(losInfo.baseName());
        //    mosra->setSolFileName(solFN);
        //}

        if (bDoLUCControl)
        {
            this->UpdateProgress(0.15);
        }
        else
        {
            this->UpdateProgress(0.65);
        }


        // test whether we've got the ipopt.sh script in place
        QString ipoptScriptFN = QString("%1/../utils/ipopt.sh").arg(this->m_ApplicationDirPath.c_str());
        QFileInfo ipScr(ipoptScriptFN);
        if (!ipScr.isExecutable())
        {
            NMProcWarn(<< "Attention there's no 'ipopt.sh' installed in '"
                      << ipScr.canonicalPath().toStdString() << "' - double check your setup for running IpOpt with LUMASS!");
        }

        // create the command that calls ipopt.sh that calls ipopt inside the ipopt container ...
        //      ipopt.sh : "Synopsis: $ ipopt <IpOptContainerPath> <IpOptExecPath> <ScenarioDir> <NlFileName> <DVarFileName>"
        //const std::string sp = "\" \"";
        //std::stringstream cmd;
        //cmd << ipoptScriptFN.toStdString() << "\""
        //    << this->m_IpOptContainerPath << sp
        //    << "\"/opt/ipopt/install/bin/ipopt\"" << sp
        //    << mosra->getDataPath().toStdString() << sp
        //    << mosra->getNlFileName().toStdString() << sp
        //    << mosra->getSolFileName().toStdString() << "\"";

        NMProcInfo(<< this->m_IpOptCommand << " ...");

        enum NmIpoptResult{
            NM_IPOPT_RES_UNKNOWN = 0,
            NM_IPOPT_RES_OPTIMAL,
            NM_IPOPT_RES_ITEREX,
            NM_IPOPT_RES_INFEASIBLE
        };

        NmIpoptResult optRes = NM_IPOPT_RES_UNKNOWN;

        //    if (::system(nullptr))
        //    {
        //        if (!::system(cmd.str().c_str()))
        //        {

        char outBuf[128];
        std::string outString;

        FILE* ipOptOutput = popen(m_IpOptCommand.c_str(), "r");
        if (!ipOptOutput)
        {
            NMProcErr(<< "Running IpOpt failed - double check your settings!");
            bDoLUCControl = false;
        }

        while (fgets(outBuf, sizeof outBuf, ipOptOutput) != nullptr)
        {
            outString += outBuf;
        }

        if (pclose(ipOptOutput) == -1)
        {
            NMProcErr(<< "Unfortunately, the issued command returned with an ERROR, "
                      << "and this is the last we heard: "
                      << outString);
            return;
        }

        if (outString.find("Ipopt::TNLP::INVALID_TNLP") != std::string::npos)
        {
            NMProcErr(<< "Ipopt::TNLP::INVALID_TNLP - Invalid NL file provided to ipopt!"
                      << " This shouldn't happen! Double check your equations and constraints!");
            return;
        }

        if (bDoLUCControl)
        {
            this->UpdateProgress(0.4);
        }
        else
        {
            this->UpdateProgress(0.8);
        }

        NMProcInfo(<< outString);

        // check whether we've got a optimal solution
        ifstream outFile;
        std::string line;
        outFile.open(mosra->getSolFileName().toStdString().c_str());
        if (outFile.is_open())
        {
            while (getline(outFile, line))
            {
                if (line.find("Optimal Solution Found") != std::string::npos)
                {
                    optRes = NM_IPOPT_RES_OPTIMAL;
                    //NMProcInfo(<< "Optimal Solution Found");
                }
                else if (line.find("Problem may be infeasible") != std::string::npos)
                {
                    optRes = NM_IPOPT_RES_INFEASIBLE;
                    //NMProcInfo(<< "Problem may be infeasible");
                }
                else if (line.find("Number of Iterations Exceeded") != std::string::npos)
                {
                    optRes = NM_IPOPT_RES_ITEREX;
                    //NMProcInfo(<< "Number of Iterations Exceeded");
                }
                else if (line.find("_svar") != std::string::npos)
                {
                    break;
                }
            }
        }

        // that's what we got ...
        //NMProcInfo(<< "NlFileName: '" << mosra->getNlFileName().toStdString() << "'");
        //NMProcInfo(<< "DvarsFileName: '" << mosra->getSolFileName().toStdString() << "'");
        //NMProcInfo(<< "IpOptCommand: '" << this->m_IpOptCommand << "'");
        //NMProcInfo(<< "LUMASS Path: '" << this->m_ApplicationDirPath << "'");
        //NMProcInfo(<< "EnableLUCC: " << mosra->getEnableLUCControl());
        //NMProcInfo(<< "MaxOptLucAlloc: " << mosra->getMaxOptAlloc());
        //NMProcInfo(<< "LUCCField: " << mosra->getLUCControlField().toStdString());

        // say what we're doing next
        switch(optRes)
        {
        case NM_IPOPT_RES_OPTIMAL:
            NMProcInfo(<< "You did it! Optimal solution found!");
            break;
        case NM_IPOPT_RES_ITEREX:
            NMProcInfo(<< "Mmh, we've exhausted all available iterations. Let's see what we've got ...")
            break;
        case NM_IPOPT_RES_UNKNOWN:
            NMProcInfo(<< "Not quite sure what's going on here, you'd better investigate!");
            bDoLUCControl = false;
            return;
        case NM_IPOPT_RES_INFEASIBLE:
        default:
            NMProcInfo(<< "Sorry, mate, that didn't work! Check your problem for conflicting constraints or try scaling (other / additional) input parameters! Good luck!");
            bDoLUCControl = false;
            return;
        }

        NMProcInfo(<< "Mapping current solution ...");
        if (!mosra->mapNL())
        {
            NMProcErr(<< "Failed mapping current solution '" << mosra->getSolFileName().toStdString() << "'!");
            bDoLUCControl = false;
        }
        if (bDoLUCControl)
        {
            if (iRound == 1)
            {
                const int numRecChanges = mosra->getNumRecLUCChange();
                NMProcInfo(<< "Changed LUC settings  for " << numRecChanges << " parcels ...");

                mosra->setEnableLUCControl(false);
                iRound++;
            }
            else if (iRound == 2)
            {
                bDoLUCControl = false;
                mosra->restoreLUCControl();
            }
            this->UpdateProgress(0.5);
        }
        else
        {
            this->UpdateProgress(0.95);
        }

    } while (   bDoLUCControl
             && !this->GetAbortGenerateData()
            );

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
