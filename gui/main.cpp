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
#include <QResource>
#include "lumassmainwin.h"


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

#ifdef DEBUG
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

int main(int argc, char *argv[])
{
#ifdef QT_HIGHDPI_SUPPORT
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication lumass(argc, argv);
    LUMASSMainWin w;
    w.show();
    return lumass.exec();
}
