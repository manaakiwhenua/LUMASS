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
	name: otbRasdamanImageIOFactory.cxx
	
*/

#include "otbRasdamanImageIOFactory.h"

#include "itkCreateObjectFunction.h"
#include "otbRasdamanImageIO.h"
#include "itkVersion.h"


namespace otb
{

RasdamanImageIOFactory::RasdamanImageIOFactory()
{
  this->RegisterOverride("itkImageIOBase",
                         "otbRasdamanImageIO",
                         "Rasdaman Image IO",
                         1,
                         itk::CreateObjectFunction<RasdamanImageIO>::New());
}

RasdamanImageIOFactory::~RasdamanImageIOFactory()
{
}

const char*
RasdamanImageIOFactory::GetITKSourceVersion(void) const
{
  return ITK_SOURCE_VERSION;
}

const char*
RasdamanImageIOFactory::GetDescription() const
{
  return "Rasdaman ImageIO Factory, allows for loading rasdaman images from OTB";
}

} // end namespace otb


