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
#include <netcdf>

#include "lumassmainwin.h"
#include "NMLogWidget.h"

//class QVTKOpenGLWidget;
#include "QVTKOpenGLNativeWidget.h"
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
    static QVTKOpenGLNativeWidget* getVTKWidget(void);
    static QString getUserSetting(const QString& key);
    static QStringList getUserSettingsList(void);
    static QStringList getModelSettingsList(void);
    static QString getMultiLineInput(const QString& title,
                              const QString& suggestion, QWidget* parent=0);
    static QString getNetCDFVarPathInput(const QString& ncFilename, const QString& exclude);
    static void parseNcGroup(const netCDF::NcGroup& grp, QStringList& varPaths, const QString& exclude);
    static QStringList getMultiItemSelection(const QString& title,
                                         const QString& label,
                                         const QStringList& items,
                                         QStringList selectedItems=QStringList(),
                                         QWidget* parent=0);
    static QString getItemSelection(const QString& title,
                                          const QString& label,
                                          const QStringList& items,
                                          QWidget* parent=0);

    static QStringList searchPropertyValues(const QObject *obj, const QString& searchTerm);
    static QItemSelection selectRows(const QAbstractItemModel *model,
                              QList<int>& ids);
    static qreal getLUMASSVersion(void);
    static QString getRandomString(int len);
    static void logQsqlConnections(void);
    static void appendLogMsg(const QString& msg);
    static bool attachDatabase(QSqlDatabase dbTarget, const QString dbFileName, const QString schemaName=QStringLiteral("attached"));
    static bool detachDatabase(QSqlDatabase db, QString schemaName);
    static bool detachMultipleDbs(QSqlDatabase dbTarget, const QStringList& dbFileNames);
    static bool detachAllDbs(QSqlDatabase dbTarget);

    static bool attachMultipleDbs(QSqlDatabase dbTarget, const QStringList &dbFileNames);
    static QStringList identifyExternalDbs(QSqlDatabase targetDb, const QString& origexpr);

private:
    static const std::string ctx;

};

#endif // NMGLOBALHELPER_H
