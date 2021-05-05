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
* This class is derived on VTK's QVTKOpenGLWidget class and provides a slightly
* revised override of the original ::resizeEvent method making use of devicePixelRatioF
*
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


#include "NMQVTKOpenglWidget.h"

#include "QVTKInteractorAdapter.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderWindow.h"

#include <QApplication>
#include "private/qwindowcontainer_p.h"
#include <QOpenGLContext>
#include <QOpenGLWindow>
#include <QResizeEvent>
#include <QDesktopWidget>

//-----------------------------------------------------------------------------
NMQVTKOpenGLWidget::NMQVTKOpenGLWidget(QWidget* parent, Qt::WindowFlags f)
  : Superclass(parent, f)
{
    setEnableHiDPI(true);
#ifdef __linux__
    this->setProperty("_kde_no_window_grab", true);
#endif
}

//-----------------------------------------------------------------------------
//NMQVTKOpenGLWidget::NMQVTKOpenGLWidget(QOpenGLContext *shareContext,
//  QWidget* parent, Qt::WindowFlags f)
//  : NMQVTKOpenGLWidget(nullptr, shareContext, parent, f)
//{setEnableHiDPI(true);}

//-----------------------------------------------------------------------------
NMQVTKOpenGLWidget::NMQVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
  QWidget* parent, Qt::WindowFlags f)
  : Superclass(w, parent, f)
{
    setEnableHiDPI(true);
#ifdef __linux__
    this->setProperty("_kde_no_window_grab", true);
#endif
}


//-----------------------------------------------------------------------------
//NMQVTKOpenGLWidget::NMQVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
//  QOpenGLContext *shareContext, QWidget* parent, Qt::WindowFlags f)
//  : Superclass(w, shareContext, parent, f)
//{setEnableHiDPI(true);}

NMQVTKOpenGLWidget::~NMQVTKOpenGLWidget()
{}

/*
void NMQVTKOpenGLWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit(resized());

    int defaultsizeextension(0);

    QWindowContainer* wc = this->findChild<QWindowContainer*>(QString(), Qt::FindDirectChildrenOnly);
    QWindow* cw = const_cast<QWindow*>(wc->containedWindow());
    QOpenGLWindow* glwindow = qobject_cast<QOpenGLWindow*>(cw);

    const qreal devicePixelRatio_ = this->enableHiDPI() ? glwindow->devicePixelRatioF() : 1.;
    const QSize widgetSize = this->size();
    const QSize deviceSize = QSize(static_cast<int>((widgetSize.width() * devicePixelRatio_)),  //+0.5+defaultsizeextension)),
                                   static_cast<int>((widgetSize.height() * devicePixelRatio_)));//+0.5+defaultsizeextension));

    // pass the new size to the internal window
    if (this->GetInteractor())
    {
      this->GetInteractor()->SetSize(deviceSize.width(), deviceSize.height());
    }

    vtkGenericOpenGLRenderWindow* w = vtkGenericOpenGLRenderWindow::SafeDownCast(
      this->GetRenderWindow());

    if (w != nullptr)
    {
      w->SetScreenSize(deviceSize.width(), deviceSize.height());
      w->SetSize(deviceSize.width(), deviceSize.height());
      // Set screen size on render window.
      const QRect screenGeometry = QApplication::desktop()->screenGeometry(this);
      w->SetScreenSize(screenGeometry.width(), screenGeometry.height());
      w->SetPosition(this->x() * devicePixelRatio_, this->y() * devicePixelRatio_);
    }

    if (deviceSize.width() > 0 && deviceSize.height() > 0 && this->isValid())
    {
      this->GetRenderWindow()->GetInteractor()->Render();

      // Release the context for other windows to use it.
      glwindow->doneCurrent();
    }

    // Having this widget as a native widget can cause undesirable stacking
    // issues with its internal QOpenGLWindow.
    Q_ASSERT(this->testAttribute(Qt::WA_NativeWindow) == false);
}
*/
