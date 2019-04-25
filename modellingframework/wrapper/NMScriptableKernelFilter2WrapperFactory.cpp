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


#include "NMScriptableKernelFilter2WrapperFactory.h"
#include "NMScriptableKernelFilter2Wrapper.h"

extern "C" NMSCRIPTABLEKERNELFILTER2WRAPPER_EXPORT
NMWrapperFactory* createWrapperFactory()
{
    return new NMScriptableKernelFilter2WrapperFactory();
}

NMScriptableKernelFilter2WrapperFactory::NMScriptableKernelFilter2WrapperFactory(QObject *parent) : NMWrapperFactory(parent)
{

}

NMProcess*
NMScriptableKernelFilter2WrapperFactory::createWrapper()
{
    return new NMScriptableKernelFilter2Wrapper();
}
