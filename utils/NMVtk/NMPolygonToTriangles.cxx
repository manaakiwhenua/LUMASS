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

#include "NMPolygonToTriangles.h"

#include "vtkObject.h"
#include "vtkLongArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkLookupTable.h"
#include "vtkDoubleArray.h"

#include "avtPolygonToTrianglesTesselator.h"

vtkStandardNewMacro(NMPolygonToTriangles);

int
NMPolygonToTriangles::RequestData(vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    vtkPolyData* input = vtkPolyData::SafeDownCast(
                inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData* outTris = vtkPolyData::SafeDownCast(
                outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (input == nullptr || outTris == nullptr)
    {
        vtkErrorMacro(<< "No input or output defined!");
        return 0;
    }

    vtkCellArray* inputCells = input->GetPolys();
    if (inputCells == nullptr)
    {
        vtkErrorMacro(<< "No input polygons defined!");
        return 0;
    }

    vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
                input->GetCellData()->GetArray("nm_hole"));
    if (hole == nullptr)
    {
        vtkErrorMacro(<< "No hole array defined!");
        return 0;
    }


    outTris->Allocate(input->GetNumberOfCells()*2);
    vtkSmartPointer<vtkPoints> outPts = vtkSmartPointer<vtkPoints>::New();
    outPts->Allocate(input->GetNumberOfCells()*2*3);
    outTris->SetPoints(outPts);

    vtkPoints* inputPts = input->GetPoints();

    vtkIdType npolys = inputCells->GetNumberOfCells();
    std::cout << "polys2tris: npolys: " << npolys << "\n";

    vtkIdType numPts;
    vtkIdType* pts;

    vtkSmartPointer<vtkLongArray> nm_id = vtkSmartPointer<vtkLongArray>::New();
    nm_id->SetName("nm_id");
    nm_id->Allocate(input->GetNumberOfCells()*2*3);


    // ==========================================================================================
    // create lookup table
    if (InputColors.GetPointer() != nullptr)
    {

        std::cout << "polys2tris: input colors: " << InputColors->GetNumberOfColors() << "\n";

        // prepare lookup table
        if (OutputColors.GetPointer() != nullptr)
        {
            OutputColors->Delete();
        }
        OutputColors = vtkSmartPointer<vtkLookupTable>::New();
    }
    std::vector<unsigned char> rawclr;

    avtPolygonToTrianglesTesselator tessellator(outPts);
    tessellator.SetNormal(0.0,0.0,1.0);


    // ============================================================================================
    // tessellate input polygons
    std::cout << "polys2tris: processing input polygons ... \n";

    // clear the poly id mapping
    PolyIds.clear();

    inputCells->InitTraversal();
    vtkIdType triscount = 0;
    for (vtkIdType polycount=0; polycount < npolys && !this->GetAbortExecute(); ++polycount)
    {

        //        std::cout << "\tcell id: " << polycount << " ring: 0 \n";

        // get the input color
        unsigned char tupleClr[] = {1,1,1,1};
        if (InputColors.GetPointer() != nullptr)
        {
            vtkLookupTable::GetColorAsUnsignedChars(InputColors->GetTableValue(polycount), tupleClr);
        }

        // add the first contour we always have
        inputCells->GetNextCell(numPts, pts);
        tessellator.BeginContour();
        for (int p=0; p < numPts; ++p)
        {
            double coords[3];
            inputPts->GetPoint(pts[p], coords);
            tessellator.AddContourVertex(coords);
        }
        tessellator.EndContour();

        // add more rings to the polygon if we've got
        // a hole filed in the poly data set
        // (wouldn't know how to detect more rings otherwise ... )
        if (hole != nullptr)
        {
            while (polycount < npolys-1 && hole->GetValue(polycount+1) == 1)
            {
                inputCells->GetNextCell(numPts, pts);
                tessellator.BeginContour();
                for (int r=0; r < numPts; ++r)
                {
                    double ring[3];
                    inputPts->GetPoint(pts[r], ring);
                    tessellator.AddContourVertex(ring);
                }
                tessellator.EndContour();
                ++polycount;
            }
        }

        int ntris = tessellator.Tessellate(outTris);
        for (int t=0; t < ntris; ++t)
        {
            nm_id->InsertValue(triscount, triscount);
            PolyIds.push_back(polycount);
            ++triscount;
        }

        this->UpdateProgress((float)polycount / npolys);
    }
    nm_id->Squeeze();
    outPts->Squeeze();
    outTris->Squeeze();
    outTris->GetCellData()->SetScalars(nm_id);
    outTris->BuildCells();


    vtkIdType nTriCells = outTris->GetPolys()->GetNumberOfCells();
    vtkDebugMacro(<< "polys2tris: triscount: " << triscount-1 << "\n");
    vtkDebugMacro(<< "polys2tris: nTriCells: " << nTriCells-1 << "\n");

    // construct output lookup table from rawclr vector
    if (InputColors.GetPointer() != nullptr)
    {
        int arsize = rawclr.size() / 4.0;
        vtkSmartPointer<vtkUnsignedCharArray> lutArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
        lutArray->SetNumberOfComponents(4);
        lutArray->SetArray(&rawclr[0], arsize, 1);
        OutputColors->SetTable(lutArray);
        OutputColors->SetNumberOfTableValues(nTriCells);
        OutputColors->SetTableRange(0, nTriCells-1);

        vtkDebugMacro(<< "polys2tris: num table values:  " << OutputColors->GetNumberOfTableValues() << "\n");
        vtkDebugMacro(<< "polys2tris: num output colors: " << OutputColors->GetNumberOfColors() << "\n");
        vtkDebugMacro(<< "polys2tris: rawclr.size()/4.0: " << arsize << "\n");
    }

    return 1;
}


void NMPolygonToTriangles::reportTris(vtkPolyData *pd, int minCell, int maxCell)
{

    if (pd == nullptr)
    {
        return;
    }

    vtkPoints* pts = pd->GetPoints();
    vtkCellArray* cells = pd->GetPolys();
    int numcells = cells->GetNumberOfCells();
    minCell = minCell < 0 ? 0 : minCell;
    maxCell = maxCell < 0 ? numcells-1 : maxCell;

    std::cout << "CELLS: \n";

    cells->InitTraversal();
    vtkIdType numpts = 0;
    vtkIdType* cellpts;
    vtkIdType cellcount = 0;
    while (cells->GetNextCell(numpts, cellpts))
    {
        if (     (minCell >= 0 && maxCell >= 0)
             &&  cellcount >= minCell && cellcount < maxCell
            )
        {
            std::cout << "\t" << cellcount << ": ";
            for (int i=0; i < numpts; ++i)
            {
                double cc[3];
                pts->GetPoint(cellpts[i], cc);
                std::cout << "(" << cc[0] << "," << cc[1] << "," << cc[2] << ") ";
            }
            std::cout << "\n";
        }
        ++cellcount;
    }

    std::cout << "\n\n";
}


