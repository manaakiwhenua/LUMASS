/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/******************************************************************************
* Adapted for the use in LUMASS by Alexander Herzig
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

#ifndef NMQVTKOPENGLWIDGET_H
#define NMQVTKOPENGLWIDGET_H

#include <QVTKOpenGLWidget.h>

class NMQVTKOpenGLWidget : public QVTKOpenGLWidget
{
    Q_OBJECT
    typedef QVTKOpenGLWidget Superclass;
public:
    NMQVTKOpenGLWidget(QWidget* parent = Q_NULLPTR,
                       Qt::WindowFlags f = Qt::WindowFlags());
    NMQVTKOpenGLWidget(QOpenGLContext *shareContext, QWidget* parent = Q_NULLPTR,
      Qt::WindowFlags f = Qt::WindowFlags());
    NMQVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
      QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    NMQVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w, QOpenGLContext *shareContext,
      QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~NMQVTKOpenGLWidget() override;

protected:
    /*! \brief replaces the integer with the qreal devicePixelRatio */
    virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;
};

#endif // NMQVTKOPENGLWIDGET_H
