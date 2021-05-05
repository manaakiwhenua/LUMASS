/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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


#include "NMImage2DToCubeSliceFilterWrapperFactory.h"
#include "NMImage2DToCubeSliceFilterWrapper.h"

extern "C" NMIMAGE2DTOCUBESLICEFILTERWRAPPER_EXPORT
NMWrapperFactory* createWrapperFactory()
{
    return new NMImage2DToCubeSliceFilterWrapperFactory();
}

NMImage2DToCubeSliceFilterWrapperFactory::NMImage2DToCubeSliceFilterWrapperFactory(QObject *parent) : NMWrapperFactory(parent)
{

}

NMProcess*
NMImage2DToCubeSliceFilterWrapperFactory::createWrapper()
{
    return new NMImage2DToCubeSliceFilterWrapper();
}
