
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

#include "NMLumassEngine.h"

//////////////////////////////////////////////////////
/// lumassengine implementation
//////////////////////////////////////////////////////

static const std::string ctx = "LUMASS_engine";

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

    enum WhatToDo {
            NM_ENGINE_MOSO,
            NM_ENGINE_MODEL,
            NM_ENGINE_NOPLAN
    };

    WhatToDo todo = NM_ENGINE_NOPLAN;
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
            todo = NM_ENGINE_MOSO;
        }
        else if (theArg == "--model")
        {
            modelFileName = argv[arg+1];
            if (!isFileAccessible(modelFileName))
            {
                NMDebugCtxNoMPI(ctx, << "done!");
                return EXIT_SUCCESS;
            }
            todo = NM_ENGINE_MODEL;
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

        ++arg;
    }

    QScopedPointer<NMLumassEngine> engine(new NMLumassEngine(argc, argv));
    if (!losFileName.isEmpty() && !modelFileName.isEmpty())
    {
        NMWarn(ctx, << "Please select either --moso or --model!"
               << std::endl);
        showHelp();
        NMDebugCtx(ctx, << "done!");
        return EXIT_SUCCESS;
    }

    switch(todo)
    {
    case NM_ENGINE_MOSO:
        engine->doMOSO(losFileName);
        break;
    case NM_ENGINE_MODEL:
        engine->doModel(modelFileName, workspace, enginePath, bLogProv, runComponent);
        break;
    default:
        NMWarn(ctx, << "Please specify either an optimisation "
                    << " settings file or a model file!"
                    << std::endl);
        break;
    }

    // shutdown the engine (python and mpi libs)
    engine->shutdown();

    NMDebugCtxNoMPI(ctx, << "done!");
    return EXIT_SUCCESS;
}
