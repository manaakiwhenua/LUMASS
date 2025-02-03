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

#ifdef VTK_OPENGL2
    #include "vtk_glew.h"
#else
    #include "vtkOpenGLExtensionManager.h"
#endif


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
//#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPlane.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"
#include "NMImageLayer.h"

#include <math.h>

#include "vtk_glew.h"
#include "vtkOpenGLError.h"

#ifndef VTK_OPENGL2
    #include "vtkgl.h" // vtkgl namespace
#endif

vtkStandardNewMacro(NMVtkOpenGLImageSliceMapper);

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
NMVtkOpenGLImageSliceMapper::NMVtkOpenGLImageSliceMapper()
    : vtkOpenGLImageSliceMapper(), mLayer(0)
{
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
