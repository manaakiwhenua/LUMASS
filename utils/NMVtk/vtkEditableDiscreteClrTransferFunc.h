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
 * vtkEditableDiscreteClrTransferFunc.h
 *
 *  Created on: 17/01/2014
 *      Author: alex
 */

#ifndef VTKEDITABLEDISCRETECLRTRANSFERFUNC_H_
#define VTKEDITABLEDISCRETECLRTRANSFERFUNC_H_

#include <vtkDiscretizableColorTransferFunction.h>
#include "vtkLookupTable.h"
#include <map>

class VTK_FILTERING_EXPORT vtkEditableDiscreteClrTransferFunc: public vtkDiscretizableColorTransferFunction
{
public:
	static vtkEditableDiscreteClrTransferFunc* New();
	vtkTypeMacro(vtkEditableDiscreteClrTransferFunc, vtkDiscretizableColorTransferFunction);

	void replaceColourTableValue(double value, double rgba[4]);
	void setColourTableValue(vtkIdType idx, double rgba[4]);
	int getColourTableIndex(double value);
	unsigned char* MapValue(double value);

	void setSpecialNode(double value, std::vector<double> rgba);

	//virtual void Build(void);


protected:
	vtkEditableDiscreteClrTransferFunc();
	~vtkEditableDiscreteClrTransferFunc();

	std::map<double, std::vector<double> > SpecialNodes;

private:
    vtkEditableDiscreteClrTransferFunc(const vtkEditableDiscreteClrTransferFunc&); // Not implemented.
	void operator=(const vtkEditableDiscreteClrTransferFunc&); // Not implemented.

};

#endif /* VTKEDITABLEDISCRETECLRTRANSFERFUNC_H_ */
