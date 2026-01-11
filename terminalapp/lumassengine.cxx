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


/*
 * lumassengine.cxx
 *
 *  Created on: 21/06/2013
 *      Author: alex
 */

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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#ifdef LUMASS_DEBUG
    // required for LUMASS debug output
//    #ifndef _WIN32
//        int nmlog::nmindent = 1;
//    #endif

    #ifdef RMANDEBUG
        int indentLevel;
        bool debugOutput;
    #endif
#else
    #ifdef RMANDEBUG
//        #ifndef _WIN32
//            int nmlog::nmindent = 1;
//        #endif
        int indentLevel;
        bool debugOutput;
    #endif
#endif

#include "lumassengine.h"
#include "LUMASSConfig.h"

#include <QApplication>
#include <QtCore>
#include <QDir>
#include <QFileInfo>
#include <QScopedPointer>
#include <QDateTime>

#include <csignal>

#include "NMLumassEngine.h"

//////////////////////////////////////////////////////
/// lumassengine implementation
//////////////////////////////////////////////////////

static const std::string ctx = "LUMASS_engine";

//signal handler
namespace
{
  volatile std::sig_atomic_t gSignalStatus;
}

extern "C" void signal_handler(int signal)
{
  gSignalStatus = signal;
  std::cout << "LUMASS (engine) received SIGNAL=" << gSignalStatus
            << " and gracefully bows out ... good bye!" << std::endl;
  exit(gSignalStatus);
}

/*
 * \brief terminal version of LUMASS to run models without graphical user interface
 *
 */
void showHelp()
{
    std::cout << std::endl << "LUMASS (lumassengine) "
                           << _lumass_version_major << "."
                           << _lumass_version_minor << "."
                           << _lumass_version_revision
                           << std::endl << std::endl;
    std::cout << "Usage: lumassengine --moso <settings file (*.los)> | "
                                  << "--model <LUMASS model file (*.lmx | *.yaml)> "
                                  << "[--workspace <absolute directory path for '$[LUMASS:Workspace]$'>] "
                                  << "[--logfile <file name>] [--logprov]"
                                  << std::endl << std::endl;
}

bool isFileAccessible(const QString& fileName)
{
    if (fileName.isNull() || fileName.isEmpty())
    {
        NMErr(ctx, << "No settings file has been specified!");
        showHelp();
        return false;
    }

    QFileInfo losInfo(fileName);
    if (!losInfo.isReadable())
    {
        NMErr(ctx, << "Specified file '" << fileName.toStdString()
                   << "' could not be read!");
        return false;
    }

    return true;
}


int main(int argc, char** argv)
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGFPE, signal_handler);

    // capture path to lumassengine
    QCoreApplication engineApp(argc, argv);
    QString enginePath = engineApp.applicationDirPath();

    NMDebugCtxNoMPI(ctx, << "...");
    // process args
    if (argc < 2)
    {
        showHelp();
        NMDebugCtxNoMPI(ctx, << "done!");
        return EXIT_SUCCESS;
    }

    NMLumassEngine::EngineMode todo = NMLumassEngine::NM_ENGINE_MODE_UNKNOWN;
    QString losFileName;
    QString modelFileName;
    QString runComponent = QStringLiteral("root");
    //QString logFileName;
    QString workspace;
    bool bLogProv = false;

    int arg = 1;
    while (arg < argc)
    {
        QString theArg = argv[arg];
        theArg = theArg.toLower();

        if (theArg == "--moso")
        {
            losFileName = argv[arg+1];
            if (!isFileAccessible(losFileName))
            {
                NMDebugCtxNoMPI(ctx, << "done!");
                return EXIT_SUCCESS;
            }
            todo = NMLumassEngine::NM_ENGINE_MODE_MOSO;
        }
        else if (theArg == "--model")
        {
            modelFileName = argv[arg+1];
            if (!isFileAccessible(modelFileName))
            {
                NMDebugCtxNoMPI(ctx, << "done!");
                return EXIT_SUCCESS;
            }
            todo = NMLumassEngine::NM_ENGINE_MODE_MODEL;
        }
        else if (theArg == "--workspace")
        {
            workspace = argv[arg+1];
            QFileInfo difo(workspace);
            if (!difo.isDir() || !difo.isWritable())
            {
                NMErr(ctx, << "Cannot write into workspace '"
                           << workspace.toStdString() << "'!");
            }
        }
        else if (theArg == "--logprov")
        {
            bLogProv = true;
        }
        else if (theArg == "--comp")
        {
            runComponent = argv[arg+1];
        }
#ifdef LUMASS_DEBUG
        else if (theArg == "--test")
        {
            todo = NMLumassEngine::NM_ENGINE_MODE_TEST;
        }
#endif
        ++arg;
    }

    QScopedPointer<NMLumassEngine> engine(new NMLumassEngine(argc, argv, NMLumassEngine::NM_APP_ENGINE));
    if (engine.isNull())
    {
        NMWarn(ctx, << "Failed to launch lumassengine app!"
               << std::endl);
        showHelp();
        NMDebugCtx(ctx, << "done!");
        return EXIT_SUCCESS;
    }
    if (!losFileName.isEmpty() && !modelFileName.isEmpty()
#ifdef LUMASS_DEBUG
         && todo != NMLumassEngine::NM_ENGINE_MODE_TEST
#endif
       )
    {
        NMWarn(ctx, << "Please select either --moso or --model!"
               << std::endl);
        showHelp();
        engine->notifyParentProcess(0, 73);
        engine->shutdown();
        NMDebugCtx(ctx, << "done!");
        return EXIT_SUCCESS;
    }

    switch(todo)
    {
    case NMLumassEngine::NM_ENGINE_MODE_MOSO:
        engine->doMOSO(losFileName);
        break;
    case NMLumassEngine::NM_ENGINE_MODE_MODEL:
         engine->doModel(modelFileName, workspace, enginePath, bLogProv, runComponent);
        break;
#ifdef LUMASS_DEBUG
    case NMLumassEngine::NM_ENGINE_MODE_TEST:
        engine->test();
        break;
#endif
    default:
        NMWarn(ctx, << "Please specify either an optimisation "
                    << " settings file or a model file!"
                    << std::endl);
        engine->notifyParentProcess(0, 73);
        break;
    }

    NMDebugCtxNoMPI(ctx, << "done!");
    return EXIT_SUCCESS;
}
