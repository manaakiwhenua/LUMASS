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

#ifndef NMCHARTVIEW_H
#define NMCHARTVIEW_H

//#include "QVTKWidget.h"
#include "QVTKOpenGLNativeWidget.h"
//#include <QVTKOpenGLNativeWidget.h>
#include <QObject>
#include <QWidget>

#include "vtkContextView.h"
#include "vtkSmartPointer.h"

class vtkObject;
class vtkEventQtSlotConnect;
class vtkRenderWindow;
class NMLogger;

class NMChartView : public QWidget
{
    Q_OBJECT
public:
    explicit NMChartView(QWidget *parent = 0);
    ~NMChartView();

    void setLogger(NMLogger* logger) {mLogger = logger;}

    vtkRenderWindow* getRenderWindow(void);
    vtkContextView* getContextView(void)
        {return mContextView;}

signals:

public slots:

    void mouseLButtonPressed(vtkObject* vtkObj);
    void mouseRButtonPressed(vtkObject* vtkObj);

protected:

    NMLogger* mLogger;
    //QVTKWidget* mVTKView;
    QVTKOpenGLNativeWidget* mVTKView;
    vtkSmartPointer<vtkContextView> mContextView;
    vtkSmartPointer<vtkEventQtSlotConnect> mSlotConnect;
};

#endif // NMCHARTVIEW_H
