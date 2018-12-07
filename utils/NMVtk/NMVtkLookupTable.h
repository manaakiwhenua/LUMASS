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
* Adapted/Extended by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
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
    adaptation of vtkLookupTable (VTK 6.3)

    supports map-based look up of colour
    values for non-contigous map values;
    rather than linearly interpolating the
    colour value
*/

#ifndef NMVTKLOOKUPTABLE_H
#define NMVTKLOOKUPTABLE_H

#include <map>
#include "vtkLookupTable.h"

class NMVtkLookupTable : public vtkLookupTable
{
public:
    static NMVtkLookupTable* New();
    vtkTypeMacro(NMVtkLookupTable,vtkLookupTable);

    void PrintSelf(ostream& os, vtkIndent indent)
    {Superclass::PrintSelf(os, indent);}

    virtual void MapScalarsThroughTable2(void *input,
                         unsigned char *output,
                         int inputDataType,
                         int numberOfValues,
                         int inputIncrement,
                         int outputFormat);

    /*!
     * \brief sets the colour and lookup table index for
     *        the given map value
     * \param indx  the lookup table index
     * \param mapVal the map value
     * \param r red
     * \param g green
     * \param b blue
     * \param a alpha
     */
    void SetMappedTableValue(vtkIdType indx, double mapVal,
                                double r, double g, double b, double a=1.0);

    void SetMappedTableValue(vtkIdType indx, double mapVal, double rgba[4])
        {SetMappedTableValue(indx, mapVal, rgba[0], rgba[1], rgba[2], rgba[3]);}

    /*!
     * \brief turn map-based index lookup on/off
     * \param bmap == true ? on : off
     */
    void SetUseIndexMapping(bool bmap)
        {mLutIdxMapping = bmap;}



protected:
    NMVtkLookupTable(int sze=256, int ext=256);
    ~NMVtkLookupTable();

    bool mLutIdxMapping;
    std::multimap<double, vtkIdType> mLutIdxMap;

private:
    NMVtkLookupTable(const NMVtkLookupTable&);
    void operator=(const NMVtkLookupTable&);

};

#endif // NMVTKLOOKUPTABLE_H
