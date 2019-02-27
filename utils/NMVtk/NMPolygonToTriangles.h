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

/*
 * NMPolygonToTriangles.h
 *
 * VTK-based filter to turn vtkPolyData polygons into triangles using
 * the avtPolygonToTriangleTesselator from the VisIt project that is
 * based on the SGI libtess2 library.
 *
 * Created on: 05/01/2019
 *     Author: Alex Herzig
 *
*/

#ifndef NMPOLYGONTOTRIANGLES_H
#define NMPOLYGONTOTRIANGLES_H

#include "vtkPolyDataAlgorithm.h"

#include <string>

class vtkLookupTable;


class NMPolygonToTriangles : public vtkPolyDataAlgorithm
{
public:
    static NMPolygonToTriangles* New();

    vtkTypeMacro(NMPolygonToTriangles,vtkPolyDataAlgorithm);

    void SetInputColors(vtkLookupTable* inColors)
        {InputColors = inColors;}
    vtkLookupTable* GetOutputColors()
        {return OutputColors.GetPointer();}

    std::vector<vtkIdType> GetPolyIdMap() {return PolyIds;}

protected:
    NMPolygonToTriangles() {}
    ~NMPolygonToTriangles() override {}

    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

    void reportTris(vtkPolyData* pd, int minCell=-1, int maxCell=-1);

    vtkPolyData* m_Source;
    vtkPolyData* m_Output;

    vtkSmartPointer<vtkLookupTable> InputColors;
    vtkSmartPointer<vtkLookupTable> OutputColors;

    std::vector<vtkIdType> PolyIds;

private:
    NMPolygonToTriangles(const NMPolygonToTriangles&) = delete;
    void operator=(const NMPolygonToTriangles&) = delete;
};


#endif // header protection
