 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013,2014 Landcare Research New Zealand Ltd
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
 * vtkEditableDiscreteClrTransferFunc.cxx
 *
 *  Created on: 17/01/2014
 *      Author: alex
 */

#include "vtkEditableDiscreteClrTransferFunc.h"
#include "vtkObjectFactory.h"

// to utilise the VTK object factory
vtkStandardNewMacro(vtkEditableDiscreteClrTransferFunc);

vtkEditableDiscreteClrTransferFunc::vtkEditableDiscreteClrTransferFunc()
{
}

vtkEditableDiscreteClrTransferFunc::~vtkEditableDiscreteClrTransferFunc()
{
}

void
vtkEditableDiscreteClrTransferFunc::replaceColourTableValue(
		double value, double rgba[4])
{
		vtkIdType idx = this->LookupTable->GetIndex(value);
		this->LookupTable->SetTableValue(idx, rgba);
}

void
vtkEditableDiscreteClrTransferFunc::setColourTableValue(vtkIdType idx, double rgba[4])
{
		this->LookupTable->SetTableValue(idx, rgba);
}

int
vtkEditableDiscreteClrTransferFunc::getColourTableIndex(
		double value)
{
	return this->LookupTable->GetIndex(value);
}

unsigned char*
vtkEditableDiscreteClrTransferFunc::MapValue(double value)
{
	  this->Build();

	  std::map<double, std::vector<double> >::const_iterator it =
			  this->SpecialNodes.find(value);
	  if (it != this->SpecialNodes.end())
	  {
		  this->UnsignedCharRGBAValue[0] = (it->second)[0]*255.0 + 0.5;
		  this->UnsignedCharRGBAValue[1] = (it->second)[1]*255.0 + 0.5;
		  this->UnsignedCharRGBAValue[2] = (it->second)[2]*255.0 + 0.5;
		  this->UnsignedCharRGBAValue[3] = (it->second)[3]*255.0 + 0.5;
		  return this->UnsignedCharRGBAValue;
	  }
	  else
	  {
		  return this->Superclass::MapValue(value);
	  }
}

void
vtkEditableDiscreteClrTransferFunc::setSpecialNode(
		double value, std::vector<double> rgba)
{
	this->SpecialNodes.insert(std::pair<double, std::vector<double> >(value, rgba));
}

//void
//vtkEditableDiscreteClrTransferFunc::Build(void)
//{
//	// first we call the superclass function to build the
//	// colour table
//	vtkDiscretizableColorTransferFunction::Build();
//
//	// now we have a look at either end of the table and see
//	// whether we've got space for our special values, which we
//	// want to be transparent, if desired
//
//
//}






