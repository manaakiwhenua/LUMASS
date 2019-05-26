/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandZoom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/******************************************************************************
* Created by Alexander Herzig
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
/*
 *   The implementation of this class contains signification portions of
 *   vtkInteractorStyleImage and vtkInteractorStyleRubberBandZoom tailored
 *   to the needs of LUMASS
 */

#include "nmlog.h"

#include "NMVtkInteractorStyleImage.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCommand.h"

//vtkStandardNewMacro(NMVtkInteractorStyleImage);

NMVtkInteractorStyleImage::NMVtkInteractorStyleImage()
{
    this->mStartPosition[0] = this->mStartPosition[1] = 0;
    this->mEndPosition[0] = this->mEndPosition[1] = 0;

    this->State = VTKIS_IMAGE2D;
    mNMInteractorMode = NM_INTERACT_PAN;
    mMoving = 0;
    mDPR = 1;

    mPixelArray = vtkUnsignedCharArray::New();

}

NMVtkInteractorStyleImage::~NMVtkInteractorStyleImage()
{
    mPixelArray->Delete();
}

NMVtkInteractorStyleImage*
NMVtkInteractorStyleImage::New()
{
    NMVtkInteractorStyleImage* iasm = new NMVtkInteractorStyleImage();
    iasm->InitializeObjectBase();
    return iasm;
}

void
NMVtkInteractorStyleImage::OnMouseMove()
{
    mEndPosition[0] = (double)this->Interactor->GetEventPosition()[0]*mDPR;
    mEndPosition[1] = (double)this->Interactor->GetEventPosition()[1]*mDPR;

    if (this->mMoving && this->Interactor)
    {
        if (this->mNMInteractorMode == NM_INTERACT_PAN)
        {
            this->FindPokedRenderer(mEndPosition[0], mEndPosition[1]);
            this->Pan();
            this->InvokeEvent(vtkCommand::InteractionEvent, 0);
        }
        else if (   this->mNMInteractorMode == NM_INTERACT_ZOOM_IN
                 || this->mNMInteractorMode == NM_INTERACT_ZOOM_OUT
                )
        {
            this->DrawRubberBand();
        }
    }
}

void
NMVtkInteractorStyleImage::DrawRubberBand()
{
    int *size = this->Interactor->GetRenderWindow()->GetSize();
    if (this->mEndPosition[0] > (size[0]-1))
      {
      this->mEndPosition[0] = size[0]-1;
      }
    if (this->mEndPosition[0] < 0)
      {
      this->mEndPosition[0] = 0;
      }
    if (this->mEndPosition[1] > (size[1]-1))
      {
      this->mEndPosition[1] = size[1]-1;
      }
    if (this->mEndPosition[1] < 0)
      {
      this->mEndPosition[1] = 0;
      }

    vtkUnsignedCharArray *tmpPixelArray = vtkUnsignedCharArray::New();
    tmpPixelArray->DeepCopy(this->mPixelArray);

    unsigned char *pixels = tmpPixelArray->GetPointer(0);

    int min[2], max[2];
    min[0] = this->mStartPosition[0] <= this->mEndPosition[0] ?
      this->mStartPosition[0] : this->mEndPosition[0];
    min[1] = this->mStartPosition[1] <= this->mEndPosition[1] ?
      this->mStartPosition[1] : this->mEndPosition[1];
    max[0] = this->mEndPosition[0] > this->mStartPosition[0] ?
      this->mEndPosition[0] : this->mStartPosition[0];
    max[1] = this->mEndPosition[1] > this->mStartPosition[1] ?
      this->mEndPosition[1] : this->mStartPosition[1];

    int i;
    for (i = min[0]; i <= max[0]; i++)
      {
      pixels[3*(min[1]*size[0]+i)] = 255 ^ pixels[3*(min[1]*size[0]+i)];
      pixels[3*(min[1]*size[0]+i)+1] = 255 ^ pixels[3*(min[1]*size[0]+i)+1];
      pixels[3*(min[1]*size[0]+i)+2] = 255 ^ pixels[3*(min[1]*size[0]+i)+2];
      pixels[3*(max[1]*size[0]+i)] = 255 ^ pixels[3*(max[1]*size[0]+i)];
      pixels[3*(max[1]*size[0]+i)+1] = 255 ^ pixels[3*(max[1]*size[0]+i)+1];
      pixels[3*(max[1]*size[0]+i)+2] = 255 ^ pixels[3*(max[1]*size[0]+i)+2];
      }
    for (i = min[1]+1; i < max[1]; i++)
      {
      pixels[3*(i*size[0]+min[0])] = 255 ^ pixels[3*(i*size[0]+min[0])];
      pixels[3*(i*size[0]+min[0])+1] = 255 ^ pixels[3*(i*size[0]+min[0])+1];
      pixels[3*(i*size[0]+min[0])+2] = 255 ^ pixels[3*(i*size[0]+min[0])+2];
      pixels[3*(i*size[0]+max[0])] = 255 ^ pixels[3*(i*size[0]+max[0])];
      pixels[3*(i*size[0]+max[0])+1] = 255 ^ pixels[3*(i*size[0]+max[0])+1];
      pixels[3*(i*size[0]+max[0])+2] = 255 ^ pixels[3*(i*size[0]+max[0])+2];
      }

    this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0]-1, size[1]-1, pixels, 1);

    tmpPixelArray->Delete();
}

void
NMVtkInteractorStyleImage::setDevicePixelRatio(double dpr)
{
#ifdef QT_HIGHDPI_SUPPORT
#   ifdef VTK_OPENGL2
        this->mDPR = 1;
#   else
        this->mDPR = dpr;
#   endif
#else
    this->mDPR = 1;
#endif
}
void
NMVtkInteractorStyleImage::OnLeftButtonDown()
{
    mStartPosition[0] = (double)this->Interactor->GetEventPosition()[0]*mDPR;
    mStartPosition[1] = (double)this->Interactor->GetEventPosition()[1]*mDPR;
    mEndPosition[0] = mStartPosition[0];
    mEndPosition[1] = mStartPosition[1];

    this->FindPokedRenderer(mStartPosition[0], mStartPosition[1]);
    if (this->CurrentRenderer == 0)
    {
        return;
    }

    switch(mNMInteractorMode)
    {
    case NM_INTERACT_PAN:
        this->StartPan();
        break;


    case NM_INTERACT_ZOOM_IN:
    case NM_INTERACT_ZOOM_OUT:
        if (this->Interactor)
        {
            vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();

            this->mPixelArray->Initialize();
            this->mPixelArray->SetNumberOfComponents(3);
            int *size = renWin->GetSize();
            this->mPixelArray->SetNumberOfTuples(size[0]*size[1]);

            renWin->GetPixelData(0, 0, size[0]-1, size[1]-1, 1, this->mPixelArray);
        }
        break;

    case NM_INTERACT_MULTI:
        Superclass::OnLeftButtonDown();
        break;
    }

    this->mMoving = 1;

}

void
NMVtkInteractorStyleImage::OnLeftButtonUp()
{
    mEndPosition[0] = (double)this->Interactor->GetEventPosition()[0]*mDPR;
    mEndPosition[1] = (double)this->Interactor->GetEventPosition()[1]*mDPR;

    switch (mNMInteractorMode)
    {
    case NM_INTERACT_PAN:
        this->EndPan();
        break;

    case NM_INTERACT_ZOOM_IN:
    case NM_INTERACT_ZOOM_OUT:
        if (this->Interactor && this->mMoving)
        {
            if (    mStartPosition[0] != mEndPosition[0]
                ||  mStartPosition[1] != mEndPosition[1]
               )
            {
                this->Zoom();
            }
        }
        break;

    case NM_INTERACT_MULTI:
        Superclass::OnLeftButtonUp();
        break;
    }

    this->mMoving = 0;
}

void
NMVtkInteractorStyleImage::Zoom()
{
    int width, height;
    width = abs(this->mEndPosition[0] - this->mStartPosition[0]);
    height = abs(this->mEndPosition[1] - this->mStartPosition[1]);
    int *size = this->CurrentRenderer->GetSize();
    int *origin = this->CurrentRenderer->GetOrigin();
    vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

    int min[2];
    double rbcenter[3];
    min[0] = this->mStartPosition[0] < this->mEndPosition[0] ?
                this->mStartPosition[0] : this->mEndPosition[0];
    min[1] = this->mStartPosition[1] < this->mEndPosition[1] ?
                this->mStartPosition[1] : this->mEndPosition[1];

    rbcenter[0] = min[0] + 0.5*width;
    rbcenter[1] = min[1] + 0.5*height;
    rbcenter[2] = 0;

    this->CurrentRenderer->SetDisplayPoint(rbcenter);
    this->CurrentRenderer->DisplayToView();
    this->CurrentRenderer->ViewToWorld();

    double invw;
    double worldRBCenter[4];
    this->CurrentRenderer->GetWorldPoint(worldRBCenter);
    invw = 1.0/worldRBCenter[3];
    worldRBCenter[0] *= invw;
    worldRBCenter[1] *= invw;
    worldRBCenter[2] *= invw;

    double winCenter[3];
    winCenter[0] = origin[0] + 0.5*size[0];
    winCenter[1] = origin[1] + 0.5*size[1];
    winCenter[2] = 0;

    this->CurrentRenderer->SetDisplayPoint(winCenter);
    this->CurrentRenderer->DisplayToView();
    this->CurrentRenderer->ViewToWorld();

    double worldWinCenter[4];
    this->CurrentRenderer->GetWorldPoint(worldWinCenter);
    invw = 1.0/worldWinCenter[3];
    worldWinCenter[0] *= invw;
    worldWinCenter[1] *= invw;
    worldWinCenter[2] *= invw;

    double translation[3];
    translation[0] = worldRBCenter[0] - worldWinCenter[0];
    translation[1] = worldRBCenter[1] - worldWinCenter[1];
    translation[2] = worldRBCenter[2] - worldWinCenter[2];

    double pos[3], fp[3];
    cam->GetPosition(pos);
    cam->GetFocalPoint(fp);

    pos[0] += translation[0]; pos[1] += translation[1]; pos[2] += translation[2];
    fp[0] += translation[0];  fp[1] += translation[1];  fp[2] += translation[2];

    cam->SetPosition(pos);
    cam->SetFocalPoint(fp);

    double zoomFactor;
    if (width > height)
    {
        if (mNMInteractorMode == NM_INTERACT_ZOOM_IN)
        {
            zoomFactor = size[0] / static_cast<double>(width);
        }
        else
        {
            zoomFactor = width / static_cast<double>(size[0]);
        }
    }
    else
    {
        if (mNMInteractorMode == NM_INTERACT_ZOOM_IN)
        {
            zoomFactor = size[1] / static_cast<double>(height);
        }
        else
        {
            zoomFactor = height / static_cast<double>(size[1]);
        }
    }

    // we just call Dolly function of the Superclass' Superclass,
    // i.e. vtkInteractorStyleTrackballCamera, since it handles
    // the clipping much nicer than the vtkInteractorStyleRubberbandZoom
    // implementation
    this->Dolly(zoomFactor);
    this->Interactor->Render();
}

void
NMVtkInteractorStyleImage::OnMiddleButtonDown()
{
    Superclass::OnMiddleButtonDown();
}

void
NMVtkInteractorStyleImage::OnMiddleButtonUp()
{
    Superclass::OnMiddleButtonUp();
}

void
NMVtkInteractorStyleImage::OnRightButtonDown()
{
    Superclass::OnRightButtonDown();
}

void
NMVtkInteractorStyleImage::OnRightButtonUp()
{
    Superclass::OnRightButtonUp();
}

void
NMVtkInteractorStyleImage::OnMouseWheelForward()
{
    if (    this->Interactor->GetControlKey()
        &&  this->Interactor->GetShiftKey()
       )
    {
        this->SetMotionFactor(0.5);
    }
    else if (this->Interactor->GetControlKey())
    {
        this->SetMotionFactor(5.0);
    }
    else
    {
        this->SetMotionFactor(20);
    }

    Superclass::OnMouseWheelForward();
}

void
NMVtkInteractorStyleImage::OnMouseWheelBackward()
{
    if (    this->Interactor->GetControlKey()
        &&  this->Interactor->GetShiftKey()
       )
    {
        this->SetMotionFactor(0.2);
    }
    else if (this->Interactor->GetControlKey())
    {
        this->SetMotionFactor(1.0);
    }
    else
    {
        this->SetMotionFactor(20);
    }

    Superclass::OnMouseWheelBackward();
}
