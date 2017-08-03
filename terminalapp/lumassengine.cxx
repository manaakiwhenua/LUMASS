
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

//#include "GUI_template_inst.h"

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#ifdef DEBUG
    // required for LUMASS debug output
    #ifndef _WIN32
        int nmlog::nmindent = 1;
    #endif

    #ifdef RMANDEBUG
        int indentLevel;
        bool debugOutput;
    #endif
#else
    #ifdef RMANDEBUG
        #ifndef _WIN32
            int nmlog::nmindent = 1;
        #endif
        int indentLevel;
        bool debugOutput;
    #endif
#endif


#include "lumassengine.h"
#include "LUMASSConfig.h"

#include <QtCore>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QFuture>
#include <QScopedPointer>
#include <QDateTime>

#include "gdal.h"
#include "gdal_priv.h"
#include <sqlite3.h>

#include "vtkSmartPointer.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkTable.h"
#include "vtkDelimitedTextWriter.h"

#include "NMMosra.h"
#include "MOSORunnable.h"
#include "NMModelController.h"
#include "NMModelSerialiser.h"
#include "NMSequentialIterComponent.h"


//////////////////////////////////////////////////////
/// NMLoggingProvider implementation
//////////////////////////////////////////////////////

NMLoggingProvider::NMLoggingProvider()
{
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(false);
}

NMLoggingProvider::~NMLoggingProvider()
{
    if (mLogFile.isOpen())
    {
        mLogFile.flush();
        mLogFile.close();
    }

    if (mLogger)
    {
        delete mLogger;
    }
}

NMLoggingProvider *NMLoggingProvider::This()
{
    static NMLoggingProvider provider;
    return &provider;
}

NMLogger *NMLoggingProvider::getLogger() const
{
    return mLogger;
}

void
NMLoggingProvider::setLogFileName(const QString &fn)
{
    mLogFile.setFileName(fn);
    if (!mLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        NMErr("NMLoggingProvider", << "Failed creating log file!");
        return;
    }

    connect(mLogger, SIGNAL(sendLogMsg(QString)), This(), SLOT(writeLogMsg(QString)));
}

void
NMLoggingProvider::writeLogMsg(const QString &msg)
{
    if (!mLogFile.isOpen())
    {
        NMErr("NMLoggingProvider", << "Failed writing log message - log file is closed!");
        return;
    }

    QTextStream out(&mLogFile);
    out << msg;
}

//////////////////////////////////////////////////////
/// lumassengine implementation
//////////////////////////////////////////////////////


static const std::string ctx = "LUMASS_engine";

/*
 * \brief terminal version of LUMASS to run models without graphical user interface
 *
 */

void doMOSO(const QString& losFileName)
{
	NMDebugCtx(ctx, << "...");

	QScopedPointer<NMMosra> mosra(new NMMosra());
    mosra->setLogger(NMLoggingProvider::This()->getLogger());
	mosra->loadSettings(losFileName);
	if (mosra->doBatch())
	{
		doMOSObatch(losFileName);
	}
    else
    {
        doMOSOsingle(losFileName);
    }

	NMDebugCtx(ctx, << "done!");
}

void doMOSObatch(const QString& losFileName)
{
	QScopedPointer<NMMosra> mosra(new NMMosra());
    mosra->setLogger(NMLoggingProvider::This()->getLogger());
	mosra->loadSettings(losFileName);
	if (!mosra->doBatch())
	{
		NMErr(ctx, << "We don't quite have what we need for batch processing!");
		return;
	}

	QString dsFileName = QString("%1/%2.vtk").arg(mosra->getDataPath())
			.arg(mosra->getLayerName());

	QFileInfo dsinfo(dsFileName);

	if (!dsinfo.isReadable())
	{
		NMErr(ctx, << "Could not read file '" << dsFileName.toStdString() << "'!");
		return;
	}


    int nReps = mosra->getNumberOfPerturbations();
    if (nReps > 1)
    {
        int nthreads = QThread::idealThreadCount();
        nthreads = nthreads > nReps ? nReps : nthreads;
        int chunksize = nReps / nthreads;
        int rest = nReps - (nthreads * chunksize);
        rest = rest < 0 ? 0 : rest;

        // here we account for the lazyness of the user:
        // it could well be that the user wants to test either
        // different sets of criteria or constraints while using the
        // same set of uncertainties or he wants to use a different
        // set of uncertainties with the same set of criteria or constraints ...
        //
        // ... this means the length of the lists (i.e. PERTURB or UNCERTAINTIES)
        // may be different; so what we do is, when we reach the end of either of
        // of the lists, we just keep using the last parameter set (i.e. comma separated)
        // again ... and again ... and again ... ... as often the user wants us to
        // (i.e. as long as the other list has still a new parameter set!)

        QStringList pertubItems = mosra->getPerturbationItems();
        int pertubCnt = pertubItems.size();
        const QList<QList<float> >& uncertaintyLevels = mosra->getAllUncertaintyLevels();
        int uncertCnt = uncertaintyLevels.size();
        int maxCnt = std::max(pertubCnt, uncertCnt);

        for (int cnt=0; cnt < maxCnt; ++cnt)
        {
            int pertubIdx = cnt < pertubCnt ? cnt : pertubCnt-1;
            int uncertIdx = cnt < uncertCnt ? cnt : uncertCnt-1;

            QString item = pertubItems.at(pertubIdx);
            QList<float> levels = mosra->getUncertaintyLevels(uncertIdx);
            int runstart = 1;
            for (int t=0; t < nthreads; ++t)
            {
                if (t == nthreads-1)
                    chunksize += rest;

                MOSORunnable* m = new MOSORunnable();
                m->setData(dsFileName, mosra->getLosFileName(),
                        item, levels, runstart, chunksize);
                QThreadPool::globalInstance()->start(m);
                runstart += chunksize;
            }
        }
    }
    // we paralise over the uncertainty levels
    else
    {
        foreach(const QString& item, mosra->getPerturbationItems())
        {
            QList< QList<float> > levels = mosra->getAllUncertaintyLevels();
            for (int l=0; l < levels.size(); ++l)
            {
                QList<float> itemUncertainties = levels.at(l);
                MOSORunnable* m = new MOSORunnable();
                m->setData(dsFileName, mosra->getLosFileName(),
                        item, itemUncertainties, 1, 1);
                QThreadPool::globalInstance()->start(m);
            }
        }
    }
    QThreadPool::globalInstance()->waitForDone();
}

void doMOSOsingle(const QString& losFileName)
{
    QScopedPointer<NMMosra> mosra(new NMMosra());
    mosra->setLogger(NMLoggingProvider::This()->getLogger());
    mosra->loadSettings(losFileName);
    QString dsFileName = QString("%1/%2.vtk").arg(mosra->getDataPath())
            .arg(mosra->getLayerName());

    QFileInfo dsinfo(dsFileName);

    if (!dsinfo.isReadable())
    {
        NMErr(ctx, << "Could not read file '" << dsFileName.toStdString() << "'!");
        return;
    }

    QList<float> levels;
    levels << 0;
    MOSORunnable* m = new MOSORunnable();
    m->setData(dsFileName, mosra->getLosFileName(),
               "", levels, 1, 1);
    QThreadPool::globalInstance()->start(m);
}

void doModel(const QString& modelFile, QString &workspace)
{
    NMDebugCtx(ctx, << "...");

    // ==============================================
    //  import the model
    // ==============================================
    QFileInfo fi(modelFile);
    if (fi.suffix().compare(QString("lmx"), Qt::CaseInsensitive) != 0)
    {
        NMErr(ctx, << "Invalid model specified!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    //NMModelController* ctrl = NMModelController::getInstance();
    QScopedPointer<NMModelController> ctrl(new NMModelController());
    ctrl->setLogger(NMLoggingProvider::This()->getLogger());
    ctrl->updateSettings("LUMASSPath",
                         qApp->applicationDirPath());

    // ====================================================
    //   set ModelController settings
    // ====================================================
    QSettings settings("LUMASS", "GUI");

#ifdef __linux__
    settings.setIniCodec("UTF-8");
#endif

    settings.beginGroup("Directories");

    QVariant val = settings.value("Workspace");
    if (val.isValid() && workspace.isEmpty())
    {
        workspace = val.toString();
    }

    ctrl->updateSettings("Workspace", QVariant::fromValue(workspace));
    sqlite3_temp_directory = const_cast<char*>(
                workspace.toStdString().c_str());

    val = settings.value("UserModels");
    if (val.isValid())
    {
        ctrl->updateSettings("UserModels", val);
    }

    settings.endGroup();

    // ====================================================

    ctrl->getLogger()->setHtmlMode(false);

    QMap<QString, QString> nameRegister;
    NMModelSerialiser xmlS;
    xmlS.setModelController(ctrl.data());
    xmlS.setLogger(ctrl->getLogger());

    // connect the logger to the logging provider
    ctrl->connect(ctrl->getLogger(), SIGNAL(sendLogMsg(QString)),
                  NMLoggingProvider::This(), SLOT(writeLogMsg(QString)));

    NMSequentialIterComponent* root = new NMSequentialIterComponent();
    root->setObjectName("root");
    root->setDescription("Top level model component managed by the ModelController");
    ctrl->addComponent(root);

    nameRegister = xmlS.parseComponent(modelFile, 0, ctrl.data());

    // ==============================================
    //  EXECUTE MODEL
    // ==============================================
    GDALAllRegister();
    GetGDALDriverManager()->AutoLoadDrivers();
    sqlite3_temp_directory = const_cast<char*>(workspace.toStdString().c_str());//getenv("HOME");

    ctrl->executeModel("root");

    GDALDestroyDriverManager();
   // delete mLogger;
    NMDebugCtx(ctx, << "done!");
}

void showHelp()
{
    std::cout << std::endl << "LUMASS (lumassengine) "
                           << _lumass_version_major << "."
                           << _lumass_version_minor << "."
                           << _lumass_version_revision
                           << std::endl << std::endl;
    std::cout << "Usage: lumassengine --moso <settings file (*.los)> | "
                                  << "--model <LUMASS model file (*.lmx)> "
                                  << "[--workspace <absolute directory path for '$[LUMASS:Workspace]$'>] "
                                  << "[--logfile <file name>]"
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
        NMErr(ctx, << "Specified file could not be read!");
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
	NMDebugCtx(ctx, << "...");

	if (argc < 2)
	{
		showHelp();
		NMDebugCtx(ctx, << "done!");
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
    QString logFileName;
    QString workspace;

	int arg = 1;
	while (arg < argc-1)
	{
		QString theArg = argv[arg];
		theArg = theArg.toLower();

		if (theArg == "--moso")
		{
            losFileName = argv[arg+1];
            if (!isFileAccessible(losFileName))
			{
				NMDebugCtx(ctx, << "done!");
                return EXIT_SUCCESS;
			}
            todo = NM_ENGINE_MOSO;
		}
        else if (theArg == "--model")
        {
            modelFileName = argv[arg+1];
            if (!isFileAccessible(modelFileName))
            {
                NMDebugCtx(ctx, << "done!");
                return EXIT_SUCCESS;
            }
            todo = NM_ENGINE_MODEL;
        }
        else if (theArg == "--logfile")
        {
            logFileName = argv[arg+1];
            QFileInfo fifo(logFileName);
            QFileInfo difo(fifo.absoluteDir().absolutePath());
            if (!difo.isWritable())
            {
                NMWarn(ctx, << "Log file directory is not writeable!");
                logFileName.clear();
            }
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

		++arg;
	}

    if (!losFileName.isEmpty() && !modelFileName.isEmpty())
    {
        NMWarn(ctx, << "Please select either --moso or --model!"
               << std::endl);
        showHelp();
        NMDebugCtx(ctx, << "done!");
        return EXIT_SUCCESS;
    }



#ifdef DEBUG
    NMLoggingProvider::This()->getLogger()->setLogLevel(NMLogger::NM_LOG_DEBUG);
#else
    NMLoggingProvider::This()->getLogger()->setLogLevel(NMLogger::NM_LOG_INFO);
#endif

    if (!logFileName.isEmpty())
    {
        QString logstart = QString("lumassengine - %1, %2")
                .arg(QDate::currentDate().toString())
                .arg(QTime::currentTime().toString());

        NMLoggingProvider::This()->setLogFileName(logFileName);
        NMLoggingProvider::This()->writeLogMsg(logstart);
    }


	switch(todo)
	{
	case NM_ENGINE_MOSO:
		doMOSO(losFileName);
		break;
    case NM_ENGINE_MODEL:
        doModel(modelFileName, workspace);
        break;
	default:
        NMWarn(ctx, << "Please specify either an optimisation "
                    << " settings file or a model file!"
                    << std::endl);
		break;
	}

	NMDebugCtx(ctx, << "done!");
	return EXIT_SUCCESS;
}

