
#include "NMGlobalHelper.h"
#include <QApplication>

OtbModellerWin*
NMGlobalHelper::getMainWindow()
{
    OtbModellerWin* mainWin = 0;
    QWidgetList tlw = qApp->topLevelWidgets();
    QWidgetList::ConstIterator it = tlw.constBegin();
    for (; it != tlw.constEnd(); ++it)
    {
        QWidget* w = const_cast<QWidget*>(*it);
        if (w->objectName().compare("OtbModellerWin") == 0)
        {
            mainWin = qobject_cast<OtbModellerWin*>(w);
        }
    }

    return mainWin;
}

vtkRenderWindow*
NMGlobalHelper::getRenderWindow()
{
   return getMainWindow()->getRenderWindow();
}