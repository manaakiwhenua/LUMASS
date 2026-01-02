/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd
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
 * NMProcessFactory.cpp
 *
 *  Created on: 5/06/2012
 *      Author: alex
 */

#include <QApplication>
#include <QLibrary>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>

#include "NMProcessFactory.h"
#include "NMProcess.h"
#include "NMWrapperFactory.h"
#include "NMImageReader.h"
#include "NMTableReader.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "NMStreamingROIImageFilterWrapper.h"


NMProcessFactory::NMProcessFactory(QObject* parent)
    : bLibInitialised(false)
{
    this->setParent(parent);

    // init core components
    mProcRegister << QString::fromLatin1("ImageReader")          ;
    mProcRegister << QString::fromLatin1("TableReader");
    mProcRegister << QString::fromLatin1("ImageWriter")          ;
    mProcRegister << QString::fromLatin1("ExtractImageRegion");

    mAliasClassMap[QStringLiteral("ImageReader")] = QStringLiteral("NMImageReader");
    mAliasClassMap[QStringLiteral("TableReader")] = QStringLiteral("NMTableReader");
    mAliasClassMap[QStringLiteral("ImageWriter")] = QStringLiteral("NMStreamingImageFileWriterWrapper");
    mAliasClassMap[QStringLiteral("ExtractImageRegion")] = QStringLiteral("NMStreamingROIImageFilterWrapper");

    mSinks << QString::fromLatin1("ImageWriter");
}

NMProcessFactory::~NMProcessFactory()
{
}

NMProcessFactory& NMProcessFactory::instance(void)
{
    static NMProcessFactory fab;
    return fab;
}

bool
NMProcessFactory::isSink(const QString& process)
{
    bool sink = false;
    foreach(const QString& p, mSinks)
    {
        if (process.startsWith(p))
        {
            sink = true;
            break;
        }
    }

    return sink;
}

QString
NMProcessFactory::procNameFromAlias(const QString &alias)
{
    QString proc = "";

    QMap<QString, QString>::const_iterator it = mAliasClassMap.constFind(alias);
    if (it != mAliasClassMap.cend())
    {
        proc = it.value();
    }

    return proc;

// DEPRECATED CODE - just keeping it for reference of alias <-> classname mapping
//    if (alias.compare("ImageReader") == 0)
//    {
//        return "NMImageReader";
//    }
//    else if (alias.compare("MapAlgebra") == 0)
//    {
//        return "NMRATBandMathImageFilterWrapper";
//    }
//    else if (alias.compare("ImageWriter") == 0)
//    {
//        return "NMStreamingImageFileWriterWrapper";
//    }
//    else if (alias.compare("NeighbourCounter") == 0)
//    {
//        return "NMNeighbourhoodCountingWrapper";
//    }
//    else if (alias.compare("RandomImage") == 0)
//    {
//        return "NMRandomImageSourceWrapper";
//    }
//    else if (alias.compare("CostDistanceBuffer") == 0)
//    {
//        return "NMCostDistanceBufferImageWrapper";
//    }
//    else if (alias.compare("FocalDistanceWeight") == 0)
//    {
//        return "NMFocalNeighbourhoodDistanceWeightingWrapper";
//    }
//    else if (alias.compare("SummarizeZones") == 0)
//    {
//        return "NMSumZonesFilterWrapper";
//    }
//    else if (alias.compare("CastImage") == 0)
//    {
//        return "NMItkCastImageFilterWrapper";
//    }
//    else if (alias.compare("ResampleImage") == 0)
//    {
//        return "NMResampleImageFilterWrapper";
//    }
//    else if (alias.compare("UniqueCombination") == 0)
//    {
//        return "NMUniqueCombinationFilterWrapper";
//    }
//    else if (alias.compare("CombineTwo") == 0)
//    {
//        return "NMCombineTwoFilterWrapper";
//    }
//    else if (alias.compare("ExternalExec") == 0)
//    {
//        return "NMExternalExecWrapper";
//    }
//    else if (alias.compare("SQLProcessor") == 0)
//    {
//        return "NMSQLiteProcessorWrapper";
//    }
//    else if (alias.compare("MapKernelScript") == 0)
//    {
//        return "NMScriptableKernelFilterWrapper";
//    }
//    else if (alias.compare("MapKernelScript2") == 0)
//    {
//        return "NMScriptableKernelFilter2Wrapper";
//    }
//    else if (alias.compare("TableReader") == 0)
//    {
//        return "NMTableReader";
//    }
//    else if (alias.compare("ExtractBand") == 0)
//    {
//        return "NMVectorImageToImageFilterWrapper";
//    }
//    else if (alias.compare("ImageSorter") == 0)
//    {
//        return "NMExternalSortFilterWrapper";
//    }
//    else if (alias.compare("SpatialOptimisation") == 0)
//    {
//        return "NMMosraFilterWrapper";
//    }
//    else if (alias.compare("ImageBufferWriter") == 0)
//    {
//        return "NMVirtualStreamWriter";
//    }
//    else if (alias.compare("RAMFlowAcc") == 0)
//    {
//        return "NMFlowAccumulationFilterWrapper";
//    }
//    else if (alias.compare("TerrainAttributes") == 0)
//    {
//        return "NMDEMSlopeAspectFilterWrapper";
//    }
//    else if (alias.compare("ExtractImageRegion") == 0)
//    {
//        return "NMStreamingROIImageFilterWrapper";
//    }
//    else if (alias.compare("JSMapKernelScript") == 0)
//    {
//        return QStringLiteral("NMJSKernelFilterWrapper");
//    }
//    else if (alias.compare("BMIModel") == 0)
//    {
//        return QStringLiteral("NMBMIWrapper");
//    }
//    else if (alias.compare("SQLRouter") == 0)
//    {
//        return QStringLiteral("NMSQLiteRouterWrapper");
//    }
//        else if (alias.compare("Image2DtoCubeSlice") == 0)
//    {
//        return "NMImage2DToCubeSliceFilterWrapper";
//    }
//    else if (alias.compare("CubeSliceToImage2D") == 0)
//    {
//        return "NMCubeSliceToImage2DFilterWrapper";
//    }
//    else if (alias.compare("Image2Table") == 0)
//    {
//        return "NMImage2TableFilterWrapper";
//    }
//    else if (alias.compare("Table2NetCDF") == 0)
//    {
//        return "NMTable2NetCDFFilterWrapper";
//    }
/*$<WrapperClassNameFromComponentName>$*/

//    else return proc;
}

QStringList
NMProcessFactory::getRegisteredComponents(void)
{
    if (!bLibInitialised)
    {
        this->initializeProcessLibrary();
    }
    return mProcRegister;
}

void
NMProcessFactory::initializeProcessLibrary()
{
    QString path = mLumassPath;

#ifdef __linux__
    path += "/../lib";

    QString modframecorelib = QStringLiteral("libNMModFrameCore.so");
    QString wrapperLibEnding = QStringLiteral("Wrapper.so");

#elif defined(_WIN32)
    QDir _dir(path);
    QString chgDirStr;
    // ... were we launched from a msvc build dir ? 
    if (   _dir.dirName().compare(QStringLiteral("Debug")) == 0
        || _dir.dirName().compare(QStringLiteral("Release")) == 0
       )
    {
        chgDirStr = QString("../../lib/%1").arg(_dir.dirName());
    }
    else
    {
        chgDirStr = QStringLiteral("../utils/bin");
    }
    _dir.cd(chgDirStr);
    path = _dir.absolutePath();

    QString modframecorelib = QStringLiteral("NMModFrameCore.dll");
    QString wrapperLibEnding = QStringLiteral("Wrapper.dll");
#endif

    QDir libDir(path);

    // double check, whether we're in the right directory ... 
    QStringList _libList = libDir.entryList();
    if (!_libList.contains(modframecorelib))
    {
        QString _pcDir = QFileDialog::getExistingDirectory(nullptr, QStringLiteral("Process component directory"), path);
        if (!_pcDir.isEmpty())
        {
            QDir _dir(_pcDir);
            path = _dir.absolutePath();
            libDir.setPath(path);
        }
    }

    NM_CREATE_FACTORY_FUNC factoryFunc = 0;

    QFileInfoList libInfoList = libDir.entryInfoList();
    foreach(const QFileInfo& libInfo, libInfoList)
    {
        QString libname = QString("%1/%2").arg(path).arg(libInfo.fileName());
        if (     libname.endsWith(wrapperLibEnding) 
             &&  QLibrary::isLibrary(libname)
        )
        {
            QLibrary wrapperLib(libname);
            if (!wrapperLib.load())
            {
                // DEBUG: Log the absolute path being loaded
#ifdef _WIN32
                DWORD lastWinError = GetLastError();
#else
                std::string lastWinError = "We're on Linux!";
#endif
                NMLogError(<< "Failed loading '" << libname.toStdString() << "'!" 
                           << std::endl << wrapperLib.errorString().toStdString() << std::endl
                           << "WIN ERROR: " << lastWinError);
            }

            factoryFunc = (NM_CREATE_FACTORY_FUNC)wrapperLib.resolve("createWrapperFactory");

            if (factoryFunc != nullptr)
            {
                NMWrapperFactory* factory = factoryFunc();
                factory->setParent(this);
                QString className = factory->getWrapperClassName();
                QString alias = factory->getComponentAlias();
                QMap<QString, NMWrapperFactory*>::const_iterator frit =
                        mFactoryRegister.constFind(className);
                if (frit != mFactoryRegister.cend())
                {
                    NMErr("NMProcessFactory::initializePrcessLibrary()",
                          << "Process component '" << className.toStdString()
                               << "' has already been registered! "
                               << "We'd better skip this one!");
                    continue;
                }

                mFactoryRegister[className] = factory;
                mAliasClassMap[alias] = className;

                if (mProcRegister.contains(alias))
                {
                    NMWarn("NMProcessFactory::initializePrcessLibrary()",
                         << "Process component alias '" << alias.toStdString()
                               << "' has already been registered! "
                               << "We'll use its full class name "
                               << "instead: '" << className.toStdString() << "'!");
                    alias = className;
                }

                mProcRegister << alias;

                if (factory->isSinkProcess())
                {
                    mSinks << alias;
                }
            }
            else
            {
#ifdef _WIN32
                DWORD lastWinError = GetLastError();
#else
                std::string lastWinError = "We're on Linux!";
#endif

                NMLogError(<< "Failed accessing `::createWrapperFactory()` method in library '" << libname.toStdString() << "'!\n"
                           << wrapperLib.errorString().toStdString() << std::endl
                           << "WIN ERROR: " << lastWinError);

            }
        }
    }

    bLibInitialised = true;
}

NMProcess* NMProcessFactory::createProcess(const QString& procClass)
{
    if (!bLibInitialised)
    {
        initializeProcessLibrary();
    }

    NMProcess* proc = 0;

    if (procClass.compare("NMImageReader") == 0)
    {
        proc =  new NMImageReader(this);
    }
    else if (procClass.compare("NMTableReader") == 0)
    {
        proc = new NMTableReader(this);
    }
    else if (procClass.compare("NMStreamingImageFileWriterWrapper") == 0)
    {
        proc = new NMStreamingImageFileWriterWrapper(this);
        proc->mIsSink = true;
    }
    else if (procClass.compare("NMStreamingROIImageFilterWrapper") == 0)
    {
        proc = new NMStreamingROIImageFilterWrapper(this);
    }
    else
    {
        QMap<QString, NMWrapperFactory*>::const_iterator facIt =
                mFactoryRegister.find(procClass);

        if (facIt != mFactoryRegister.constEnd())
        {
            proc = (*facIt)->createWrapper();
        }

        if(proc && this->isSink(procClass))
        {
            proc->mIsSink = true;
        }
    }

    return proc;
}

NMProcess*
NMProcessFactory::createProcessFromAlias(const QString& alias)
{
    QString procClass = this->procNameFromAlias(alias);

    return this->createProcess(procClass);
}
