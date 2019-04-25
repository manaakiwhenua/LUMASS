/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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


#include "NMCostDistanceBufferImageWrapperFactory.h"
#include "NMCostDistanceBufferImageWrapper.h"

extern "C" NMCOSTDISTANCEBUFFERIMAGEWRAPPER_EXPORT
NMWrapperFactory* createWrapperFactory()
{
    return new NMCostDistanceBufferImageWrapperFactory();
}

NMCostDistanceBufferImageWrapperFactory::NMCostDistanceBufferImageWrapperFactory(QObject *parent) : NMWrapperFactory(parent)
{

}

NMProcess*
NMCostDistanceBufferImageWrapperFactory::createWrapper()
{
    return new NMCostDistanceBufferImageWrapper();
}
