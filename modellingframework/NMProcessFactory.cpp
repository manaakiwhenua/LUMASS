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

NMProcessFactory::NMProcessFactory(QObject* parent)
{
	this->setParent(parent);
}

NMProcessFactory::~NMProcessFactory()
{
}

NMProcessFactory& NMProcessFactory::instance(void)
{
	static NMProcessFactory fab;
	return fab;
}

// TODO: this needs to be revised to support automatic
//       registration of supported classes;
NMProcess* NMProcessFactory::createProcess(QString procClass)
{
	if (procClass.compare("NMImageReader") == 0)
	{
		return new NMImageReader(this);
	}
	else if (procClass.compare("NMRATBandMathImageFilterWrapper") == 0)
	{
		return new NMRATBandMathImageFilterWrapper(this);
	}
	else if (procClass.compare("NMStreamingImageFileWriterWrapper") == 0)
	{
		return new NMStreamingImageFileWriterWrapper(this);
	}
	else if (procClass.compare("NMNeighbourhoodCountingWrapper") == 0)
	{
		return new NMNeighbourhoodCountingWrapper(this);
	}
	else if (procClass.compare("NMRandomImageSourceWrapper") == 0)
	{
		return new NMRandomImageSourceWrapper(this);
	}
	else if (procClass.compare("NMCostDistanceBufferImageWrapper") == 0)
	{
		return new NMCostDistanceBufferImageWrapper(this);
	}
	else
		return 0;
}
