/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
/* NMLUAllocation.h
 *
 *  Created on: 16/01/2013
 *      Author: alex
 */

#ifndef NMLUALLOCATIONWRAPPER_H_
#define NMLUALLOCATIONWRAPPER_H_

#include "NMProcess.h"
#include "nmmodframe_export.h"

class NMMODFRAME_EXPORT NMLUAllocationWrapper: public NMProcess
{
public:
	NMLUAllocationWrapper();
	virtual ~NMLUAllocationWrapper();
};

#endif /* NMLUALLOCATIONWRAPPER_H_ */
