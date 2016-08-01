/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkLookupTable.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/******************************************************************************
* Adapted by Alexander Herzig
* Copyright 2014 Landcare Research New Zealand Ltd
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

/*!
    adaptation of vtkLookupTable

    The main difference to the original class is that it allows for specifying
    a 'lower' and 'upper' value range for scalars smaller/greater the specified
    value range and to assign specific colours for those ranges.

*/

#ifndef NMVTKLOOKUPTABLE_H
#define NMVTKLOOKUPTABLE_H

#include "vtkLookupTable.h"

class NMVtkLookupTable : public vtkLookupTable
{
public:
    static NMVtkLookupTable* New();
    vtkTypeMacro(NMVtkLookupTable,vtkLookupTable);

    void PrintSelf(ostream& os, vtkIndent indent)
    {Superclass::PrintSelf(os, indent);}


    void setLowerUpperClrOn(void);
    void setLowerUpperClrOff(void);

    void setLowerClr(double val, unsigned char clr[4]);
    void setUpperClr(double val, unsigned char clr[4]);

    void GetColor(double v, double rgb[3]);

    unsigned char* MapValue(double v);

    void MapScalarsThroughTable2(void *input,
                         unsigned char *output,
                         int inputDataType,
                         int numberOfValues,
                         int inputIncrement,
                         int outputFormat);

    inline unsigned char* linearLookupMain(double v,
                    unsigned char* table,
                    double maxIndex,
                    double shift, double scale);


protected:
    NMVtkLookupTable(int sze=256, int ext=256);
    ~NMVtkLookupTable();

    bool mbUseLowerUpperClr;

    double mUpper;
    double mLower;

    unsigned char mUpperClr[4];
    unsigned char mLowerClr[4];

private:
    NMVtkLookupTable(const NMVtkLookupTable&);
    void operator=(const NMVtkLookupTable&);

};

#endif // NMVTKLOOKUPTABLE_H
