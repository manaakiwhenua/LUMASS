#ifndef NMGLOBALHELPER_H
#define NMGLOBALHELPER_H

#include "vtkRenderWindow.h"
#include "otbmodellerwin.h"
#include "ui_otbmodellerwin.h"

class NMGlobalHelper
{
public:
    OtbModellerWin* getMainWindow(void);
    vtkRenderWindow* getRenderWindow(void);
    QVTKWidget* getVTKWidget(void);
    void startBusy(void);
    void endBusy(void);
};

#endif // NMGLOBALHELPER_H
