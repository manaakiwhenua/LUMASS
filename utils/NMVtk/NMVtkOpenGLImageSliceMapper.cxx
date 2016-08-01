/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VtkOpenGLImageSliceMapper.cxx

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

#include "NMVtkOpenGLImageSliceMapper.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkMapper.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPlane.h"
#include "NMImageLayer.h"


#include <math.h>

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include "vtkgl.h" // vtkgl namespace

vtkStandardNewMacro(NMVtkOpenGLImageSliceMapper);

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
NMVtkOpenGLImageSliceMapper::NMVtkOpenGLImageSliceMapper()
    : vtkOpenGLImageSliceMapper(), mLayer(0)
{
}

int
NMVtkOpenGLImageSliceMapper::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // compute display extent
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    int wholeExtent[6];
    int *extent = this->DataWholeExtent;
    //int extent[6];
    double *spacing = this->DataSpacing;
    double *origin = this->DataOrigin;

    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    inInfo->Get(vtkDataObject::SPACING(), spacing);
    inInfo->Get(vtkDataObject::ORIGIN(), origin);

    vtkMatrix4x4 *matrix = this->GetDataToWorldMatrix();

    for (int k = 0; k < 6; k++)
      {
      extent[k] = wholeExtent[k];
      }

    if (this->Cropping)
      {
      for (int i = 0; i < 3; i++)
        {
        if (extent[2*i] < this->CroppingRegion[2*i])
          {
          extent[2*i] = this->CroppingRegion[2*i];
          }
        if (extent[2*i+1] > this->CroppingRegion[2*i+1])
          {
          extent[2*i+1] = this->CroppingRegion[2*i+1];
          }
        }
      }

    if (this->SliceFacesCamera || this->SliceAtFocalPoint)
      {
      vtkRenderer *ren = this->GetCurrentRenderer();

      if (matrix && ren)
        {
        vtkCamera *camera = ren->GetActiveCamera();

        if (this->SliceFacesCamera)
          {
          this->Orientation = this->GetOrientationFromCamera(matrix, camera);
          this->Orientation = this->Orientation % 3;
          }

        if (this->SliceAtFocalPoint)
          {
          this->SliceNumber = this->GetSliceFromCamera(matrix, camera);
          }
        }
      }

    int orientation = this->Orientation % 3;

    this->SliceNumberMinValue = wholeExtent[2*orientation];
    this->SliceNumberMaxValue = wholeExtent[2*orientation + 1];

    if (this->SliceNumber < extent[2*orientation])
      {
      this->SliceNumber = extent[2*orientation];
      }
    if (this->SliceNumber > extent[2*orientation + 1])
      {
      this->SliceNumber = extent[2*orientation + 1];
      }

    extent[2*orientation] = this->SliceNumber;
    extent[2*orientation + 1] = this->SliceNumber;

    this->DisplayExtent[0] = extent[0];
    this->DisplayExtent[1] = extent[1];
    this->DisplayExtent[2] = extent[2];
    this->DisplayExtent[3] = extent[3];
    this->DisplayExtent[4] = extent[4];
    this->DisplayExtent[5] = extent[5];

    // Create point and normal of plane
    double point[4];
    point[0] = 0.5*(extent[0] + extent[1])*spacing[0] + origin[0];
    point[1] = 0.5*(extent[2] + extent[3])*spacing[1] + origin[1];
    point[2] = 0.5*(extent[4] + extent[5])*spacing[2] + origin[2];
    point[3] = 1.0;

    double normal[4];
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 0.0;
    normal[3] = -point[orientation];
    normal[orientation] = 1.0;

    if (matrix)
      {
      // Convert point and normal to world coords
      double mat[16];
      vtkMatrix4x4::DeepCopy(mat, matrix);
      vtkMatrix4x4::MultiplyPoint(mat, point, point);
      point[0] /= point[3];
      point[1] /= point[3];
      point[2] /= point[3];
      vtkMatrix4x4::Invert(mat, mat);
      vtkMatrix4x4::Transpose(mat, mat);
      vtkMatrix4x4::MultiplyPoint(mat, normal, normal);
      vtkMath::Normalize(normal);
      }

    this->SlicePlane->SetOrigin(point);
    this->SlicePlane->SetNormal(normal);

    return 1;
    }
  else
  {
      return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }
}

void
NMVtkOpenGLImageSliceMapper::SetDisplayExtent(int extent[6])
{
    this->DisplayExtent[0] = extent[0];
    this->DisplayExtent[1] = extent[1];
    this->DisplayExtent[2] = extent[2];
    this->DisplayExtent[3] = extent[3];
    this->DisplayExtent[4] = extent[4];
    this->DisplayExtent[5] = extent[5];

}
void
NMVtkOpenGLImageSliceMapper::SetDataWholeExtent(int extent[6])
{
    this->DataWholeExtent[0] = extent[0];
    this->DataWholeExtent[1] = extent[1];
    this->DataWholeExtent[2] = extent[2];
    this->DataWholeExtent[3] = extent[3];
    this->DataWholeExtent[4] = extent[4];
    this->DataWholeExtent[5] = extent[5];
}


//----------------------------------------------------------------------------
NMVtkOpenGLImageSliceMapper::~NMVtkOpenGLImageSliceMapper()
{
  this->RenderWindow = NULL;
}

//----------------------------------------------------------------------------
// Set the modelview transform and load the texture
void
NMVtkOpenGLImageSliceMapper::Render(vtkRenderer *ren, vtkImageSlice *prop)
{
    // ask the layer to update scalars
    if (this->mLayer != 0)
    {
        this->mLayer->mapExtentChanged();
        vtkImageData* img = this->GetInput();
        if (this->mLayer->getLegendType() == NMLayer::NM_LEGEND_RGB)
        {
            this->ColorEnable = true;
            this->mLayer->mapRGBImageScalars(img);
        }
        else
        {
            this->mLayer->setScalars(img);
        }
    }

    this->Superclass::Render(ren, prop);
}
