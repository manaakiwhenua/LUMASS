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

#include "NMProcessFactory.h"
#include "NMProcess.h"
#include "NMWrapperFactory.h"
#include "NMImageReader.h"
#include "NMTableReader.h"


NMProcessFactory::NMProcessFactory(QObject* parent)
    : bLibInitialised(false)
{
    this->setParent(parent);

    // init core components
    mProcRegister << QString::fromLatin1("ImageReader")          ;
    mProcRegister << QString::fromLatin1("TableReader");
    mAliasClassMap[QStringLiteral("ImageReader")] = QStringLiteral("NMImageReader");
    mAliasClassMap[QStringLiteral("TableReader")] = QStringLiteral("NMTableReader");

    //  dirty hack; needs to be replaced with proper
    //  process registration (i.e. classname plus
    //  individual process factory)
    // mProcRegister << QString::fromLatin1("BMIModel")             ;

    // mProcRegister << QString::fromLatin1("MapAlgebra")           ;
    // mProcRegister << QString::fromLatin1("ImageWriter")          ;
    // mProcRegister << QString::fromLatin1("NeighbourCounter")     ;
    // mProcRegister << QString::fromLatin1("RandomImage")          ;
    // mProcRegister << QString::fromLatin1("CostDistanceBuffer")   ;
    // mProcRegister << QString::fromLatin1("FocalDistanceWeight")  ;
    // mProcRegister << QString::fromLatin1("SummarizeZones")       ;
    // mProcRegister << QString::fromLatin1("CastImage")            ;
    // mProcRegister << QString::fromLatin1("ResampleImage")        ;
    // mProcRegister << QString::fromLatin1("UniqueCombination")    ;
    // mProcRegister << QString::fromLatin1("CombineTwo")    ;
    // mProcRegister << QString::fromLatin1("ExternalExec");
    // mProcRegister << QString::fromLatin1("SQLProcessor");
    // mProcRegister << QString::fromLatin1("SQLRouter");
    // mProcRegister << QString::fromLatin1("MapKernelScript");
    // mProcRegister << QString::fromLatin1("MapKernelScript2");

    // mProcRegister << QString::fromLatin1("ExtractBand");
    // mProcRegister << QString::fromLatin1("ImageSorter");
    // mProcRegister << QString::fromLatin1("SpatialOptimisation");
    // mProcRegister << QString::fromLatin1("ImageBufferWriter");
    // mProcRegister << QString::fromLatin1("RAMFlowAcc");
    // mProcRegister << QString::fromLatin1("TerrainAttributes");
    // mProcRegister << QString::fromLatin1("ExtractImageRegion");
    // mProcRegister << QString::fromLatin1("Image2DtoCubeSlice");
    // mProcRegister << QString::fromLatin1("CubeSliceToImage2D");
    // mProcRegister << QString::fromLatin1("Image2Table");
    // mProcRegister << QString::fromLatin1("Table2NetCDF");
/*$<RegisterComponentName>$*/

    // mSinks << QString::fromLatin1("ImageWriter");
    // mSinks << QString::fromLatin1("CostDistanceBuffer");
    // mSinks << QString::fromLatin1("ExternalExec");
    // mSinks << QString::fromLatin1("SQLProcessor");
    // mSinks << QString::fromLatin1("UniqueCombination");
    // mSinks << QString::fromLatin1("ImageSorter");
    // mSinks << QString::fromLatin1("SpatialOptimisation");
    // mSinks << QString::fromLatin1("ImageBufferWriter");
    // mSinks << QString::fromLatin1("SQLRouter");
/*$<RegisterComponentAsSink>$*/

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
#endif

    QDir libDir(path);

#ifdef _WIN32
    // could well be that we're run from a windows build directory
    // for debugging purposes (and LUMASS_DEBUG is not defined)
    // and we can't find the libraries in the folder where
    // lumass.exe is, so we try a directory like
    // c:/pathtobuilddir/lib/<BuildType>
    // - maybe we're lucky there ...
    QStringList allentries = libDir.entryList();
    if (!allentries.contains("NMModFrameCore.dll"))
    {
        QString dirName = libDir.dirName();
        QString chgDirStr = QString("../../lib/%1").arg(dirName);
        libDir.cd(chgDirStr);
        path = libDir.absolutePath();
    }
#endif

    NM_CREATE_FACTORY_FUNC factoryFunc = 0;

    QFileInfoList libInfoList = libDir.entryInfoList();
    foreach(const QFileInfo& libInfo, libInfoList)
    {
        QString libname = QString("%1/%2").arg(path).arg(libInfo.fileName());
        if (QLibrary::isLibrary(libname))
        {
            QLibrary wrapperLib(libname);
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
