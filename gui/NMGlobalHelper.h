#ifndef NMGLOBALHELPER_H
#define NMGLOBALHELPER_H

#include "vtkRenderWindow.h"
#include "otbmodellerwin.h"
#include "ui_otbmodellerwin.h"

#include <QString>
#include <QAbstractItemModel>
#include <QList>
#include <QItemSelection>

class NMGlobalHelper
{
public:
    OtbModellerWin* getMainWindow(void);
    vtkRenderWindow* getRenderWindow(void);
    QVTKWidget* getVTKWidget(void);

    void startBusy(void);
    void endBusy(void);

    // static functions
    static QString getMultiLineInput(const QString& title,
                              const QString& suggestion, QWidget* parent=0);

    static QItemSelection selectRows(const QAbstractItemModel *model,
                              QList<int>& ids);

};

#endif // NMGLOBALHELPER_H
