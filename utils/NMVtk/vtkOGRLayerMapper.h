/******************************************************************************
 * This file has been adopted by Alexander Herzig from the below mentioned
 * original source file. It uses GLU tesselation functions to
 * render non-convex polygons.
 *
 * Please see the copyright notice below.
  ******************************************************************************/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOGRLayerMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOGRLayerMapper - a OGRLayerMapper for the OpenGL library
// .SECTION Description
// vtkOGRLayerMapper is a subclass of vtkPolyDataMapper.
// vtkOGRLayerMapper is a geometric PolyDataMapper for the OpenGL
// rendering library.

#ifndef __vtkOGRLayerMapper_h
#define __vtkOGRLayerMapper_h

#include "nmlog.h"
#define ctxOGRLayerMapper "vtkOGRLayerMapper"

#include <GL/glu.h>

#include "vtkPolyDataMapper.h"

#include "vtkOpenGL.h" // Needed for GLenum

class vtkCellArray;
class vtkPoints;
class vtkProperty;
class vtkRenderWindow;
class vtkOpenGLRenderer;
class vtkOpenGLTexture;
// VTK_RENDERING_EXPORT
class vtkOGRLayerMapper : public vtkPolyDataMapper
{
public:
  static vtkOGRLayerMapper *New();
  vtkTypeMacro(vtkOGRLayerMapper,vtkPolyDataMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for OpenGL.
  virtual int Draw(vtkRenderer *ren, vtkActor *a);

protected:
  // callback functions for the gluTesselation routines
  static void beginGLU(GLenum type);
  static void vertexGLU(GLvoid* data);
  static void endGLU(void);
  static void errorGLU(GLenum errno);
  static void combineGLU(GLdouble coords[3],
		  GLdouble* d[4], GLfloat weights[4],
		  GLdouble** dataOut);



  vtkOGRLayerMapper();
  ~vtkOGRLayerMapper();

  void DrawPoints(int idx,
                  vtkPoints *p, 
                  vtkDataArray *n,
                  vtkUnsignedCharArray *c,
                  vtkDataArray *t,
                  vtkIdType &cellNum,
                  int &noAbort,
                  vtkCellArray *ca,
                  vtkRenderer *ren);
  
  void DrawLines(int idx,
                 vtkPoints *p, 
                 vtkDataArray *n,
                 vtkUnsignedCharArray *c,
                 vtkDataArray *t,
                 vtkIdType &cellNum,
                 int &noAbort,
                 vtkCellArray *ca,
                 vtkRenderer *ren);

  void DrawPolygons(int idx,
                    vtkPoints *p, 
                    vtkDataArray *n,
                    vtkUnsignedCharArray *c,
                    vtkDataArray *t,
                    vtkIdType &cellNum,
                    int &noAbort,
                    GLenum rep,
                    vtkCellArray *ca,
                    vtkRenderer *ren);

  void DrawTStrips(int idx,
                   vtkPoints *p, 
                   vtkDataArray *n,
                   vtkUnsignedCharArray *c,
                   vtkDataArray *t,
                   vtkIdType &cellNum,
                   int &noAbort,
                   GLenum rep,
                   vtkCellArray *ca,
                   vtkRenderer *ren);

  void DrawGLUTessPolys(vtkPolyData* data);
    
  vtkIdType TotalCells;
  int ListId;
  vtkOpenGLTexture* InternalColorTexture;

private:
  vtkOGRLayerMapper(const vtkOGRLayerMapper&);  // Not implemented.
  void operator=(const vtkOGRLayerMapper&);  // Not implemented.
};

#endif
