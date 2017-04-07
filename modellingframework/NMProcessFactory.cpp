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

#include "NMProcessFactory.h"
#include "NMImageReader.h"
#include "NMRATBandMathImageFilterWrapper.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "NMNeighbourhoodCountingWrapper.h"
#include "NMRandomImageSourceWrapper.h"
#include "NMCostDistanceBufferImageWrapper.h"
#include "NMFocalNeighbourhoodDistanceWeightingWrapper.h"
#include "NMSumZonesFilterWrapper.h"
#include "NMItkCastImageFilterWrapper.h"
#include "NMResampleImageFilterWrapper.h"
#include "NMUniqueCombinationFilterWrapper.h"
#include "NMCombineTwoFilterWrapper.h"
#include "NMExternalExecWrapper.h"
#include "NMSQLiteProcessorWrapper.h"
#include "NMScriptableKernelFilterWrapper.h"
#include "NMScriptableKernelFilter2Wrapper.h"
#include "NMTableReader.h"
#include "NMVectorImageToImageFilterWrapper.h"
#include "NMExternalSortFilterWrapper.h"
#include "NMMosraFilterWrapper.h"
#include "NMVirtualStreamWriter.h"
/*$<IncludeWrapperHeader>$*/


NMProcessFactory::NMProcessFactory(QObject* parent)
{
	this->setParent(parent);

    //  dirty hack; needs to be replaced with proper
    //  process registration (i.e. classname plus
    //  individual process factory)
    mProcRegister << QString::fromLatin1("ImageReader")          ;
    mProcRegister << QString::fromLatin1("MapAlgebra")           ;
    mProcRegister << QString::fromLatin1("ImageWriter")          ;
    mProcRegister << QString::fromLatin1("NeighbourCounter")     ;
    mProcRegister << QString::fromLatin1("RandomImage")          ;
    mProcRegister << QString::fromLatin1("CostDistanceBuffer")   ;
    mProcRegister << QString::fromLatin1("FocalDistanceWeight")  ;
    mProcRegister << QString::fromLatin1("SummarizeZones")       ;
    mProcRegister << QString::fromLatin1("CastImage")            ;
    mProcRegister << QString::fromLatin1("ResampleImage")        ;
    mProcRegister << QString::fromLatin1("UniqueCombination")    ;
    mProcRegister << QString::fromLatin1("CombineTwo")    ;
    mProcRegister << QString::fromLatin1("ExternalExec");
    mProcRegister << QString::fromLatin1("SQLProcessor");
    mProcRegister << QString::fromLatin1("MapKernelScript");
    mProcRegister << QString::fromLatin1("MapKernelScript2");
    mProcRegister << QString::fromLatin1("TableReader");
    mProcRegister << QString::fromLatin1("ExtractBand");
    mProcRegister << QString::fromLatin1("ImageSorter");
    mProcRegister << QString::fromLatin1("SpatialOptimisation");
    mProcRegister << QString::fromLatin1("ImageBufferWriter");
/*$<RegisterComponentName>$*/

    mSinks << QString::fromLatin1("ImageWriter");
    mSinks << QString::fromLatin1("CostDistanceBuffer");
    mSinks << QString::fromLatin1("ExternalExec");
    mSinks << QString::fromLatin1("SQLProcessor");
    mSinks << QString::fromLatin1("UniqueCombination");
    mSinks << QString::fromLatin1("ImageSorter");
    mSinks << QString::fromLatin1("SpatialOptimisation");
    mSinks << QString::fromLatin1("ImageBufferWriter");
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

    if (alias.compare("ImageReader") == 0)
    {
        return "NMImageReader";
    }
    else if (alias.compare("MapAlgebra") == 0)
    {
        return "NMRATBandMathImageFilterWrapper";
    }
    else if (alias.compare("ImageWriter") == 0)
    {
        return "NMStreamingImageFileWriterWrapper";
    }
    else if (alias.compare("NeighbourCounter") == 0)
    {
        return "NMNeighbourhoodCountingWrapper";
    }
    else if (alias.compare("RandomImage") == 0)
    {
        return "NMRandomImageSourceWrapper";
    }
    else if (alias.compare("CostDistanceBuffer") == 0)
    {
        return "NMCostDistanceBufferImageWrapper";
    }
    else if (alias.compare("FocalDistanceWeight") == 0)
    {
        return "NMFocalNeighbourhoodDistanceWeightingWrapper";
    }
    else if (alias.compare("SummarizeZones") == 0)
    {
        return "NMSumZonesFilterWrapper";
    }
    else if (alias.compare("CastImage") == 0)
    {
        return "NMItkCastImageFilterWrapper";
    }
    else if (alias.compare("ResampleImage") == 0)
    {
        return "NMResampleImageFilterWrapper";
    }
    else if (alias.compare("UniqueCombination") == 0)
    {
        return "NMUniqueCombinationFilterWrapper";
    }
    else if (alias.compare("CombineTwo") == 0)
    {
        return "NMCombineTwoFilterWrapper";
    }
    else if (alias.compare("ExternalExec") == 0)
    {
        return "NMExternalExecWrapper";
    }
    else if (alias.compare("SQLProcessor") == 0)
    {
        return "NMSQLiteProcessorWrapper";
    }
    else if (alias.compare("MapKernelScript") == 0)
    {
        return "NMScriptableKernelFilterWrapper";
    }
    else if (alias.compare("MapKernelScript2") == 0)
    {
        return "NMScriptableKernelFilter2Wrapper";
    }
    else if (alias.compare("TableReader") == 0)
    {
        return "NMTableReader";
    }
    else if (alias.compare("ExtractBand") == 0)
    {
        return "NMVectorImageToImageFilterWrapper";
    }
    else if (alias.compare("ImageSorter") == 0)
    {
        return "NMExternalSortFilterWrapper";
    }
    else if (alias.compare("SpatialOptimisation") == 0)
    {
        return "NMMosraFilterWrapper";
    }
    else if (alias.compare("ImageBufferWriter") == 0)
    {
        return "NMVirtualStreamWriter";
    }
/*$<WrapperClassNameFromComponentName>$*/
    else return proc;
}

// TODO: this needs to be revised to support automatic
//       registration of supported classes;
NMProcess* NMProcessFactory::createProcess(const QString& procClass)
{
    NMProcess* proc = 0;

    if (procClass.compare("NMImageReader") == 0)
	{
        proc =  new NMImageReader(this);
	}
	else if (procClass.compare("NMRATBandMathImageFilterWrapper") == 0)
	{
        proc =  new NMRATBandMathImageFilterWrapper(this);
	}
	else if (procClass.compare("NMStreamingImageFileWriterWrapper") == 0)
	{
        proc =  new NMStreamingImageFileWriterWrapper(this);
	}
	else if (procClass.compare("NMNeighbourhoodCountingWrapper") == 0)
	{
        proc =  new NMNeighbourhoodCountingWrapper(this);
	}
	else if (procClass.compare("NMRandomImageSourceWrapper") == 0)
	{
        proc =  new NMRandomImageSourceWrapper(this);
	}
	else if (procClass.compare("NMCostDistanceBufferImageWrapper") == 0)
	{
        proc =  new NMCostDistanceBufferImageWrapper(this);
	}
	else if (procClass.compare("NMFocalNeighbourhoodDistanceWeightingWrapper") == 0)
	{
        proc =  new NMFocalNeighbourhoodDistanceWeightingWrapper(this);
	}
	else if (procClass.compare("NMSumZonesFilterWrapper") == 0)
	{
        proc =  new NMSumZonesFilterWrapper(this);
	}
    else if (procClass.compare("NMItkCastImageFilterWrapper") == 0)
    {
        proc =  new NMItkCastImageFilterWrapper(this);
    }
    else if (procClass.compare("NMResampleImageFilterWrapper") == 0)
    {
        proc =  new NMResampleImageFilterWrapper(this);
    }
    else if (procClass.compare("NMUniqueCombinationFilterWrapper") == 0)
    {
        proc =  new NMUniqueCombinationFilterWrapper(this);
    }
    else if (procClass.compare("NMCombineTwoFilterWrapper") == 0)
    {
        proc =  new NMCombineTwoFilterWrapper(this);
    }
    else if (procClass.compare("NMExternalExecWrapper") == 0)
    {
        proc =  new NMExternalExecWrapper(this);
    }
    else if (procClass.compare("NMSQLiteProcessorWrapper") == 0)
    {
        proc =  new NMSQLiteProcessorWrapper(this);
    }
    else if (procClass.compare("NMScriptableKernelFilterWrapper") == 0)
    {
        proc = new NMScriptableKernelFilterWrapper(this);
    }
    else if (procClass.compare("NMScriptableKernelFilter2Wrapper") == 0)
    {
        proc = new NMScriptableKernelFilter2Wrapper(this);
    }
    else if (procClass.compare("NMTableReader") == 0)
    {
        proc = new NMTableReader(this);
    }
    else if (procClass.compare("NMVectorImageToImageFilterWrapper") == 0)
    {
        proc = new NMVectorImageToImageFilterWrapper(this);
    }
    else if (procClass.compare("NMExternalSortFilterWrapper") == 0)
    {
        proc = new NMExternalSortFilterWrapper(this);
    }
    else if (procClass.compare("NMMosraFilterWrapper") == 0)
    {
        proc = new NMMosraFilterWrapper(this);
    }
    else if (procClass.compare("NMVirtualStreamWriter") == 0)
    {
        proc = new NMVirtualStreamWriter(this);
    }
/*$<CreateProcessObjFromWrapperClassName>$*/
    else
        proc =  0;

    if(proc && isSink(proc->objectName()))
    {
        proc->mIsSink = true;
    }

    return proc;
}

NMProcess*
NMProcessFactory::createProcessFromAlias(const QString& alias)
{
    QString procClass = this->procNameFromAlias(alias);

    return this->createProcess(procClass);
}
