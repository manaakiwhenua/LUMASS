 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
#include <QApplication>
#include <QSurfaceFormat>
#include <QResource>
#include <QMessageBox>
#include "lumassmainwin.h"

#include "QVTKOpenGLNativeWidget.h"

#ifdef BUILD_RASSUPPORT
/// RASDAMAN includes
#ifdef EARLY_TEMPLATE
#define __EXECUTABLE__
#ifdef __GNUG__
#include "raslib/template_inst.hh"
#include "template_rimageio_inst.hh"
#endif
#endif
#endif

// explicit instantiation of class templates used
// in the GUI code
//#include "GUI_template_inst.h"

//#ifdef EARLY_TEMPLATE
//#ifdef __GNUG__
//#include "LUMASS_TemplateInst.h"
//#endif

// this prevents the console window to show up
// under windows
//#ifdef _WIN32
//	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
//#endif

#ifdef LUMASS_DEBUG
    // required for LUMASS debug output
    #ifndef _WIN32
        #include "nmlog.h"
//        int nmlog::nmindent = 1;
    #endif
    #ifdef RMANDEBUG
        int indentLevel;
        bool debugOutput;
    #endif
#else
    #ifdef RMANDEBUG
        #ifndef _WIN32
            #include "nmlog.h"
//            int nmlog::nmindent = 1;
        #endif
        int indentLevel;
        bool debugOutput;
    #endif
#endif
#include <csignal>
#include "Python_wrapper.h"

namespace
{
  volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
  gSignalStatus = signal;
  std::cout << "LUMASS received SIGNAL=" << gSignalStatus
            << " and gracefully bows out ... good bye!" << std::endl;
  abort();
}

//class LumassApp : public QApplication
//{
//   public:
//    LumassApp(int argc, char* argv[])
//        : QApplication(argc, argv) {}

//    virtual ~LumassApp(){}

//    bool notify(QObject *receiver, QEvent *event)
//    {
//        try
//        {
//            return QApplication::notify(receiver, event);
//        }
//        catch(std::exception& e)
//        {
//            QString msg = QString("Ooopsie - something went wrong ...\n%1").arg(e.what());
//            QMessageBox::critical(nullptr, "LUMASS caught an excpetion!", msg);
//        }
//        return false;
//    }
//};

int main(int argc, char *argv[])
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGFPE, signal_handler);

    auto format = QVTKOpenGLNativeWidget::defaultFormat();
#ifdef _WIN32
    // with VTK 8.2 on Windows, use compatibility profile;
    // adopted from https://discourse.vtk.org/t/problem-in-vtk-8-2-with-defaultformat-and-qvtkopenglwidget-on-windows-10-intel/998/10
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif
    QSurfaceFormat::setDefaultFormat(format);

#ifdef QT_HIGHDPI_SUPPORT
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication lumass(argc, argv);
    //LumassApp lumass(argc, argv);
    LUMASSMainWin w;
    w.show();

    int ret;
//    try
//    {
        ret = lumass.exec();
//    }
//    catch(...)
//    {
//        ret = EXIT_FAILURE;
//        std::cout << "LUMASS received SIGNAL=" << gSignalStatus
//                  << " and gracefully bows out ... good bye!" << std::endl;
//        abort();
//    }

    return ret;
}
