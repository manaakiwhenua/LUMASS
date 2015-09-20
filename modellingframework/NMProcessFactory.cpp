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
    //mProcRegister << QString::fromLatin1("UniqueCombination")    ;
    mProcRegister << QString::fromLatin1("CombineTwo")    ;


    mSinks << QString::fromLatin1("ImageWriter");
    mSinks << QString::fromLatin1("CostDistanceBuffer");

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
//    else if (alias.compare("UniqueCombination") == 0)
//    {
//        return "NMUniqueCombinationFilterWrapper";
//    }
    else if (alias.compare("CombineTwo") == 0)
    {
        return "NMCombineTwoFilterWrapper";
    }
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
//    else if (procClass.compare("NMUniqueCombinationFilterWrapper") == 0)
//    {
//        proc =  new NMUniqueCombinationFilterWrapper(this);
//    }
    else if (procClass.compare("NMCombineTwoFilterWrapper") == 0)
    {
        proc =  new NMCombineTwoFilterWrapper(this);
    }
    else
        proc =  0;

    if(isSink(proc->objectName()))
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
