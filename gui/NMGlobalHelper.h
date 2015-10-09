#ifndef NMGLOBALHELPER_H
#define NMGLOBALHELPER_H

#include "vtkRenderWindow.h"
#include "otbmodellerwin.h"
#include "ui_otbmodellerwin.h"

#include <QString>

class NMGlobalHelper
{
public:
    OtbModellerWin* getMainWindow(void);
    vtkRenderWindow* getRenderWindow(void);
    QVTKWidget* getVTKWidget(void);
    QString getMultiLineInput(const QString& title,
                              const QString& suggestion, QWidget* parent=0);

    void startBusy(void);
    void endBusy(void);
};

#endif // NMGLOBALHELPER_H
