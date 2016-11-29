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
#ifndef NMVTKINTERACTORSTYLEIMAGE_H
#define NMVTKINTERACTORSTYLEIMAGE_H

#include "vtkInteractorStyleImage.h"

class vtkUnsignedCharArray;

class NMVtkInteractorStyleImage : public vtkInteractorStyleImage
{
public:

    typedef enum {
        NM_INTERACT_ZOOM_IN = 0,
        NM_INTERACT_ZOOM_OUT,
        NM_INTERACT_PAN,
        NM_INTERACT_MULTI
    } NMVtkInteractorMode;

    static NMVtkInteractorStyleImage* New();
    vtkTypeMacro(NMVtkInteractorStyleImage, vtkInteractorStyleImage);
    //void PrintSelf(std::ostream &os, vtkIndent indent);

    virtual void OnMouseMove();
    virtual void OnLeftButtonDown();
    virtual void OnLeftButtonUp();
    virtual void OnMiddleButtonDown();
    virtual void OnMiddleButtonUp();
    virtual void OnRightButtonDown();
    virtual void OnRightButtonUp();

    virtual void OnMouseWheelForward();
    virtual void OnMouseWheelBackward();

    void setNMInteractorMode(NMVtkInteractorMode mode)
         {mNMInteractorMode = mode;}
    NMVtkInteractorMode getNMInteractorMode(void)
         {return mNMInteractorMode;}


protected:
    NMVtkInteractorStyleImage();
    ~NMVtkInteractorStyleImage();

    void DrawRubberBand();
    virtual void Zoom();

    int mStartPosition[2];
    int mEndPosition[2];
    int mMoving;

    vtkUnsignedCharArray* mPixelArray;

    NMVtkInteractorMode mNMInteractorMode;



private:
    NMVtkInteractorStyleImage(const NMVtkInteractorStyleImage&);
    void operator=(const NMVtkInteractorStyleImage&);
};

#endif // NMVTKINTERACTORSTYLEIMAGE_H
