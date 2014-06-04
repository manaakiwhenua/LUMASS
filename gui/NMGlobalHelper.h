#ifndef NMGLOBALHELPER_H
#define NMGLOBALHELPER_H

#include "vtkRenderWindow.h"
#include "otbmodellerwin.h"

class NMGlobalHelper
{
public:
    OtbModellerWin* getMainWindow(void);
    vtkRenderWindow* getRenderWindow(void);
};

#endif // NMGLOBALHELPER_H
