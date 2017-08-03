/******************************************************************************
* Created by Alexander Herzig
* Copyright 2010-2016 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This programs distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef NMGLOBALHELPER_H
#define NMGLOBALHELPER_H

//#include "vtkRenderWindow.h"
//#include "lumassmainwin.h"

#include <QString>
#include <QAbstractItemModel>
#include <QList>
#include <QItemSelection>
#include "lumassmainwin.h"
#include "NMLogWidget.h"

class QVTKWidget;
class vtkRenderWindow;
class NMModelController;

class NMGlobalHelper
{
public:

    // static functions
    static void startBusy(void);
    static void endBusy(void);

    static LUMASSMainWin* getMainWindow(void);
    static NMModelController* getModelController(void);
    static NMLogWidget* getLogWidget(void);
    static vtkRenderWindow* getRenderWindow(void);
    static QVTKWidget* getVTKWidget(void);
    static QString getUserSetting(const QString& key);
    static QStringList getUserSettingsList(void);
    static QString getMultiLineInput(const QString& title,
                              const QString& suggestion, QWidget* parent=0);
    static QStringList getMultiItemSelection(const QString& title,
                                         const QString& label,
                                         const QStringList& items,
                                         QWidget* parent=0);
    static QItemSelection selectRows(const QAbstractItemModel *model,
                              QList<int>& ids);
    static qreal getLUMASSVersion(void);

    static QString getRandomString(int len);

    static void appendLogMsg(const QString& msg);

private:
    static const std::string ctx;

};

#endif // NMGLOBALHELPER_H
