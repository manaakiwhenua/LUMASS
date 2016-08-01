/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLImageSliceMapper - OpenGL mapper for image slice display
// .SECTION Description
// NMVtkOpenGLImageSliceMapper is a concrete implementation of the abstract
// class vtkImageSliceMapper that interfaces to the OpenGL library.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.

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

#ifndef __NMVtkOpenGLImageSliceMapper_h
#define __NMVtkOpenGLImageSliceMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGLImageSliceMapper.h"

class vtkWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkOpenGLRenderWindow;
class vtkImageSlice;
class vtkImageProperty;
class vtkImageData;
class NMImageLayer;

class NMVtkOpenGLImageSliceMapper :
  public vtkOpenGLImageSliceMapper
{
public:
  static NMVtkOpenGLImageSliceMapper *New();
  vtkTypeMacro(NMVtkOpenGLImageSliceMapper, vtkOpenGLImageSliceMapper);
  //virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.  Perform the render.
  void Render(vtkRenderer *ren, vtkImageSlice *prop);

  virtual int ProcessRequest(vtkInformation *request,
                vtkInformationVector **inInfo,
                vtkInformationVector *outInfo);

  void SetDisplayExtent(int extent[6]);
  void SetDataWholeExtent(int extent[6]);

  void SetNMLayer(NMImageLayer* layer)
    {this->mLayer = layer;}


protected:
  NMVtkOpenGLImageSliceMapper();
  ~NMVtkOpenGLImageSliceMapper();

  NMImageLayer* mLayer;

private:
  NMVtkOpenGLImageSliceMapper(const NMVtkOpenGLImageSliceMapper&);  // Not implemented.
  void operator=(const NMVtkOpenGLImageSliceMapper&);  // Not implemented.
};

#endif
