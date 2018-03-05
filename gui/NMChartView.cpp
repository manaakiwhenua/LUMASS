/******************************************************************************
* Created by Alexander Herzig
* Copyright 2017 Landcare Research New Zealand Ltd
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
* NMSqlTableView.h
*
*  Created on: 09/11/2017
*      Author: alex
*/

#include "NMChartView.h"

#include <QVBoxLayout>

#include "vtkEventQtSlotConnect.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "NMMacros.h"
#include "NMLogger.h"

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

NMChartView::NMChartView(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::Window);

    //========================================================
    //              QVTKWidget setup
    //========================================================
    qreal pr = this->devicePixelRatioF();

    QRect rect = this->geometry();
    rect.setWidth((int)(600.0*pr));
    rect.setHeight((int)(500.0*pr));
    this->setGeometry(rect);

    mVTKView = new QVTKWidget(parent);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(mVTKView);

    mContextView = vtkSmartPointer<vtkContextView>::New();
    mContextView->SetInteractor(mVTKView->GetInteractor());
    mVTKView->SetRenderWindow(mContextView->GetRenderWindow());

    //mVTKView->GetRenderWindow()->SetSize(800,600);
    mVTKView->GetRenderWindow()->SetMultiSamples(0);


    //========================================================
    //              SIGNALS & SLOTS
    //========================================================
//    vtkRenderWindowInteractor* vact = mVTKView->GetRenderWindow()->GetInteractor();
//    mSlotConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
//    mSlotConnect->Connect(vact, vtkCommand::LeftButtonPressEvent,
//                          this, SLOT(mouseLButtonPressed(vtkObject*)));
//    mSlotConnect->Connect(vact, vtkCommand::RightButtonPressEvent,
//                          this, SLOT(mouseRButtonPressed(vtkObject*)));

}

NMChartView::~NMChartView(void)
{
    delete mVTKView;
}


vtkRenderWindow*
NMChartView::getRenderWindow(void)
{
    return mVTKView->GetRenderWindow();
}

void
NMChartView::mouseLButtonPressed(vtkObject *vtkObj)
{
    vtkRenderWindowInteractor* iact = vtkRenderWindowInteractor::SafeDownCast(vtkObj);
    int eventPos[2];
    iact->GetEventPosition(eventPos);

    std::cout << "pos: " << eventPos[0] << ", " << eventPos[1];
}

void
NMChartView::mouseRButtonPressed(vtkObject *vtkObj)
{
    vtkRenderWindowInteractor* iact = vtkRenderWindowInteractor::SafeDownCast(vtkObj);
    int eventPos[2];
    iact->GetEventPosition(eventPos);

    std::cout << "pos: " << eventPos[0] << ", " << eventPos[1];
}



