/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

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

#include "NMLumassEngine.h"
#include "lumassmainwin.h"
#include "nmlog.h"

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

#ifdef _WIN32
    #include <windows.h>
#endif

#include <csignal>
#include "Python_wrapper.h"

namespace
{
  volatile std::sig_atomic_t gSignalStatus;
}

extern "C" void signal_handler(int signal)
{
  gSignalStatus = signal;
  std::cout << "LUMASS received SIGNAL=" << gSignalStatus
            << " and gracefully bows out ... good bye!" << std::endl;
  exit(gSignalStatus);
}

int main(int argc, char *argv[])
{
    std::stringstream allargs;
    bool bseq = false;
    int add_args = 4;
    for (int a=0; a < argc; ++a)
    {
        const std::string anarg = argv[a];
        // if '--mpi' was supplied by the user, we assume lumass
        // was called with 'mpiexec -np 1 /a/path/lumass --mpi ...'
        if (anarg.compare("--mpi") == 0)
        {
            add_args -= 1;
        }

        if (anarg.compare("--seq") == 0)
        {
            bseq = true;
        }

        allargs << argv[a] << " ";
    }
    NMDebugAINoMPI(<< "main was called by: " << allargs.str() << std::endl);

    // if --mpi wasn't provided as a cmd line argument,
    // we add it and relaunch lumass with mpiexec
    // unless --seq was specified (e.g. for sequential debugging)
    if (add_args == 4 && bseq == false)
    {
        // create new mpiexec process command
        const int argc2 = argc + add_args +1;
        char** argv2 = new char*[argc2];
        char arg1[] = "mpiexec";
        char arg2[] = "-np";
        char arg3[] = "1";
        argv2[0]    = arg1;
        argv2[1]    = arg2;
        argv2[2]    = arg3;

        for (int n=3; n < argc2-1; ++n)
        {
            std::string thearg;
            if (n < argc2-2)
            {
                thearg = argv[n-3];
            }
            else
            {
                thearg = "--mpi";
            }

            argv2[n] = new char[thearg.size()+1];
            strcpy(argv2[n], thearg.c_str());
        }
        argv2[argc2-1] = new char[1];
        argv2[argc2-1] = NULL;

        // report new process command
        std::stringstream _r;
#ifdef _WIN32
        for (int b=0; b < argc2-1; ++b)
#else 
        for (int b = 0; b < argc2; ++b)
#endif
        {
            _r << argv2[b] << " ";
        }

        NMDebugAINoMPI(<< "Launch command: " << _r.str() << std::endl);

        // (copy and) add to current environment (variables) ... 
        // then launch new process
#ifdef _WIN32 
        // WIN32 approach adapted from Gemini

        SetEnvironmentVariable("RDMAV_FORK_SAFE", "1");

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        // Ensure the new process shares the current console window
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        ZeroMemory(&pi, sizeof(pi));

        std::string _cmdstr = _r.str();
        std::vector<char> cmdLine(_cmdstr.begin(), _cmdstr.end());
        
        // Create the mpiexec process
        if (!CreateProcessA(
            NULL,           // Application name (use command line instead for args)
            cmdLine.data(), // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            TRUE,           // Set handle inheritance to TRUE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi             // Pointer to PROCESS_INFORMATION structure
        )) {
            NMDebugAINoMPI(<< "CreateProcess failed (" << GetLastError() << ")");
            std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
            return 1;
        }

        // Close process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Get rid of console window
        FreeConsole();

#else   // LINUX approach 

        // get current environment
        // and add 'RDMAV_FORK_SAFE=1'
        std::vector<std::string> venv;
        int ecount = 0;
        while (environ[ecount])
        {
            venv.push_back(environ[ecount]);
            ecount++;
        }

        char** newenv = new char* [ecount + 2];
        for (int e = 0; e < ecount; ++e)
        {
            newenv[e] = new char[venv[e].size() + 1];
            strcpy(newenv[e], venv[e].c_str());
        }

        std::string forstr = "RDMAV_FORK_SAFE=1";
        newenv[ecount] = new char[forstr.size() + 1];
        strcpy(newenv[ecount], forstr.c_str());

        newenv[ecount + 1] = new char[1];
        newenv[ecount + 1] = NULL;

        for (int s = 0; s < ecount + 1; ++s)
        {
            _r << newenv[s] << " ";
        }
        NMDebugAINoMPI(<< "Launch ENV: " << _r.str() << std::endl);

        // launch new mpi-enabled lumass process
        execvpe("mpiexec", argv2, newenv);

        // in case something went wrong, we get the errno
        NMDebugAINoMPI(<< "execvpe errno=" << errno << std::endl);
#endif
        return EXIT_SUCCESS;
    }


    std::signal(SIGINT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGFPE, signal_handler);

    auto format = QVTKOpenGLNativeWidget::defaultFormat();
    QSurfaceFormat::setDefaultFormat(format);

#ifdef QT_HIGHDPI_SUPPORT
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication lumass(argc, argv);
    NMLumassEngine engine(argc, argv, NMLumassEngine::NM_APP_GUI);
    LUMASSMainWin w(nullptr, &engine);
    w.show();

    int ret;
    ret = lumass.exec();
    //engine.shutdown();
    return ret;
}
