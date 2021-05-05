
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
#include <QThread>
#include <QFuture>
#include <QScopedPointer>
#include <QDateTime>

#include "gdal.h"
#include "gdal_priv.h"
#include <sqlite3.h>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/exceptions.h>

#ifdef LUMASS_PYTHON
#include "Python_wrapper.h"
#endif

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
/// YAML PARSING FUNCTIONS
//////////////////////////////////////////////////////

QString
getYamlNodeTypeAsString(const YAML::Node &node)
{
    // Undefined, Null, Scalar, Sequence, Map
    QString type = "";

    switch(node.Type())
    {
    case YAML::NodeType::Null: type = "Null"; break;
    case YAML::NodeType::Scalar: type = "Scalar"; break;
    case YAML::NodeType::Sequence: type = "Sequence"; break;
    case YAML::NodeType::Map: type = "Map"; break;
    default: type = "Undefined";
    }

    return type;
}

bool
isYamlSequence(const YAML::Node& node)
{
    bool ret = false;

    if (node.Type() == YAML::NodeType::Sequence)
    {
        ret = true;
    }

    return ret;
}

QVariant
parseYamlSetting(const YAML::const_iterator& nit, const QObject* obj)
{

    QVariant ret;

    // 'global' LUMASS setting
    if (obj == nullptr)
    {
        QString value;
        if (isYamlSequence(nit->second))
        {
            QStringList sl;
            for (int l=0; l < nit->second.size(); ++l)
            {
                sl << nit->second[l].as<std::string>().c_str();
            }
            value = sl.join(' ');
        }
        else
        {
            value = nit->second.as<std::string>().c_str();
        }
        ret = QVariant::fromValue(value);
    }
    else
    {
        QString name = nit->first.as<std::string>().c_str();
        QVariant prop = obj->property(name.toStdString().c_str());

        if (QString("QString").compare(prop.typeName()) == 0)
        {
            QString sv = nit->second.as<std::string>().c_str();
            ret = QVariant::fromValue(sv);
        }
        else if (QString("QStringList").compare(prop.typeName()) == 0)
        {
            QStringList lst;
            for (int e=0; isYamlSequence(nit->second) && e < nit->second.size(); ++e)
            {
                lst << nit->second[e].as<std::string>().c_str();
            }

            ret = QVariant::fromValue(lst);
        }
        else if (QString("QList<QStringList>").compare(prop.typeName()) == 0)
        {
            QList<QStringList> lsl;
            for (int l=0; isYamlSequence(nit->second) && l < nit->second.size(); ++l)
            {
                QStringList lst;
                for (int g=0; isYamlSequence(nit->second[l]) && g < nit->second[l].size(); ++g)
                {
                    lst << nit->second[l][g].as<std::string>().c_str();
                }
                lsl << lst;
            }
            ret = QVariant::fromValue(lsl);
        }
        else if (QString("QList<QList<QStringList> >").compare(prop.typeName()) == 0)
        {
            QList<QList<QStringList> > llsl;
            for (int ll=0; isYamlSequence(nit->second) && ll < nit->second.size(); ++ll)
            {
                QList<QStringList> lsl;
                for (int l=0; isYamlSequence(nit->second[ll]) && l < nit->second[ll].size(); ++l)
                {
                    QStringList lst;
                    for (int g=0; isYamlSequence(nit->second[ll][l]) && g < nit->second[ll][l].size(); ++g)
                    {
                        lst << nit->second[ll][l][g].as<std::string>().c_str();
                    }
                    lsl << lst;
                }
                llsl << lsl;
            }
            ret = QVariant::fromValue(llsl);
        }
    }

    return ret;
}

int
configureModel(const YAML::Node& modelConfig, NMModelController* mController)
{
    NMLoggingProvider* log = NMLoggingProvider::This();

    mController->clearModelSettings();

    // =====================================================================
    // iterate over model components

    //std::cout << "configure model from YAML ... " << std::endl;

    YAML::const_iterator mit = modelConfig.begin();
    while (mit != modelConfig.end())
    {
         YAML::Node compNode = mit->second;
         if (!compNode.IsNull())
         {
             QString compName = mit->first.as<std::string>().c_str();
             //std::cout << "  " << compName.toStdString().c_str() << std::endl;

             bool bCompProps = true;
             bool bProcProps = false;
             if (compName.compare("Settings") == 0)
             {
                 bCompProps = false;
                 bProcProps = false;
             }

             NMModelComponent* comp = mController->getComponent(compName);
             if (bCompProps && comp == nullptr)
             {
                 std::stringstream msg;
                 msg << "ERROR - Model Configuration: Couldn't find component '"
                     << compName.toStdString() << "'!";
                 log->writeLogMsg(msg.str().c_str());
                 return 1;
             }

             // ----------------------------------------------------------
             // iterate over component properties

             YAML::const_iterator pit = compNode.begin();
             while (pit != compNode.end())
             {
                 QString propName = pit->first.as<std::string>().c_str();
                 //std::cout << "    - " << propName.toStdString().c_str() << "=";

                 // check whether the specified property is a property of the
                 // given model component
                 NMProcess* proc = nullptr;
                 if (bCompProps)
                 {
                     QVariant prop;
                     QStringList props = mController->getPropertyList(comp);
                     if (!props.contains(propName))
                     {
                         NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
                         if (ic != nullptr && ic->getProcess() != nullptr)
                         {
                             proc = ic->getProcess();
                             QStringList procProps = mController->getPropertyList(proc);
                             if (procProps.contains(propName))
                             {
                                 bProcProps = true;
                                 prop = proc->property(propName.toStdString().c_str());
                             }
                         }

                         if (!bCompProps && !bProcProps)
                         {
                             std::stringstream wmsg;
                             wmsg << "WARN - Model Configuration: Component '"
                                       << compName.toStdString() << "' doesn't "
                                       << " have a '" << propName.toStdString()
                                       << " ' property'! We better skip this one.";
                             log->writeLogMsg(wmsg.str().c_str());
                             ++pit;
                             continue;
                         }
                     }
                     else
                     {
                         prop = comp->property(propName.toStdString().c_str());
                     }
                 }
                 else
                 {
                     bProcProps = false;
                 }

                 QVariant val;
                 if (bProcProps)
                 {
                     val = parseYamlSetting(pit, proc);
                     if (val.isNull() || !val.isValid())
                     {
                         std::stringstream emsg;
                         emsg << "ERROR - Found invalid value for '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log->writeLogMsg(emsg.str().c_str());
                     }
                     if (!proc->setProperty(propName.toStdString().c_str(), val))
                     {
                         std::stringstream emsg;
                         emsg << "ERROR - Failed setting '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log->writeLogMsg(emsg.str().c_str());
                     }
                 }
                 else if (bCompProps)
                 {
                     val = parseYamlSetting(pit, comp);
                     if (val.isNull() || !val.isValid())
                     {
                         std::stringstream emsg;
                         emsg << "ERROR - Found invalid value for '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log->writeLogMsg(emsg.str().c_str());
                     }
                     // set property value
                     if (!comp->setProperty(propName.toStdString().c_str(), val))
                     {
                         std::stringstream emsg;
                         emsg << "ERROR - Failed setting '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log->writeLogMsg(emsg.str().c_str());
                     }
                 }
                 else
                 {
                     val = parseYamlSetting(pit, nullptr);
                     // update global setting
                     mController->updateSettings(propName, val);
                 }

                 //std::cout << val.toString().toStdString().c_str() << std::endl;

                 ++pit;
             }
         }
         ++mit;
    }

    return 0;
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
        NMWarn(ctx, << "Could not read file '" << dsFileName.toStdString() << "'!");
        dsFileName = QString("%1/%2.ldb").arg(mosra->getDataPath())
                .arg(mosra->getLayerName());
        NMMsg(<< "Trying this file '" << dsFileName.toStdString() << "' instead ...");

        dsinfo.setFile(dsFileName);
        if (!dsinfo.isReadable())
        {
            NMErr(ctx, << "Could not read file '" << dsFileName.toStdString() << "'!");
            return;
        }
        else
        {
            NMMsg(<< "... and it worked!");
        }
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
				m->setLogger(const_cast<NMLogger*>(NMLoggingProvider::This()->getLogger()));
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
                m->setLogger(const_cast<NMLogger*>(NMLoggingProvider::This()->getLogger()));
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

    QStringList ltypes;
    ltypes << "vtk" << "ldb" << "db" << "sqlite" << "gpkg";

    QString dsFileName;
    QFileInfo dsinfo;
    for (int t=0; t < ltypes.size(); ++t)
    {
        dsFileName = QString("%1/%2.%3")
                .arg(mosra->getDataPath())
                .arg(mosra->getLayerName())
                .arg(ltypes.at(t));

        dsinfo.setFile(dsFileName);
        if (dsinfo.isReadable())
        {
            break;
        }
        dsFileName.clear();
    }

    if (!dsinfo.isReadable())
    {
        NMErr(ctx, << "Could not read file '" << mosra->getDataPath().toStdString() << "/"
                                              << mosra->getLayerName().toStdString() << ".*' !"
                                              << "'!");
        return;
    }

    QList<float> levels;
    levels << 0;
    MOSORunnable* m = new MOSORunnable();
    m->setLogger(const_cast<NMLogger*>(NMLoggingProvider::This()->getLogger()));
    m->setData(dsFileName, mosra->getLosFileName(),
               "", levels, 1, 1);
    QThreadPool::globalInstance()->start(m);
}

void doModel(const QString& userFile, QString &workspace, QString& enginePath, bool bLogProv)
{
    NMDebugCtx(ctx, << "...");

    // ==============================================
    //  import the model
    // ==============================================
    QString modelFile;
    QFileInfo ufinfo(userFile);
    YAML::Node configFile;
    if (ufinfo.suffix().compare(QString("yaml"), Qt::CaseInsensitive) == 0)
    {
        // read yaml configuration file
        try
        {
            configFile = YAML::LoadFile(userFile.toStdString());
            YAML::Node engineConfig;
            if (configFile["EngineConfig"])
            {
                engineConfig = configFile["EngineConfig"];
            }
            else
            {
                NMErr(ctx, << "No 'EngineConfig' found in YAML file!");
                return;
            }

            if (engineConfig["modelfile"])
            {
                modelFile = engineConfig["modelfile"].as<std::string>().c_str();
            }
        }
        catch (YAML::BadConversion& bc)
        {
            std::stringstream msg;
            msg << "ERROR: " << bc.what();
            NMErr(ctx, << msg.str().c_str());
            //std::cout << "cout: " << msg.str() << std::endl;
            return;
        }
        catch (YAML::ParserException& pe)
        {
            std::stringstream msg;
            msg << "ERROR: " << pe.what();
            NMErr(ctx, << msg.str().c_str());
            //std::cout << "cout: " << msg.str() << std::endl;
            return;
        }
    }
    else
    {
        modelFile = userFile;
    }


    QFileInfo fi(modelFile);
    if (fi.suffix().compare(QString("lmx"), Qt::CaseInsensitive) != 0)
    {
        NMErr(ctx, << "Invalid model specified!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QScopedPointer<NMModelController> ctrl(new NMModelController());
    ctrl->setLogger(NMLoggingProvider::This()->getLogger());
    ctrl->updateSettings("LUMASSPath", enginePath);
    ctrl->updateSettings("TimeFormat", "yyyy-MM-ddThh:mm:ss.zzz");

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

    // if we've got a YAML, use that for overriding the
    // models hard wired configuration
    if (!configFile.IsNull())
    {
        try
        {
            if (configFile["ModelConfig"])
            {
                YAML::Node modelConfig = configFile["ModelConfig"];
                configureModel(modelConfig, ctrl.data());
            }
        }
        catch(YAML::ParserException& pe)
        {
            NMErr(ctx, << pe.what());
        }
        catch(YAML::RepresentationException& re)
        {
            NMErr(ctx, << re.what());
        }
    }

    // ==============================================
    //  EXECUTE MODEL
    // ==============================================
    GDALAllRegister();
    GetGDALDriverManager()->AutoLoadDrivers();
    sqlite3_temp_directory = const_cast<char*>(workspace.toStdString().c_str());//getenv("HOME");

    if (bLogProv)
    {
        ctrl->setLogProvOn();
    }

    ctrl->executeModel("root");

    GDALDestroyDriverManager();

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

    NMDebugCtx(ctx, << "...");

    // process args
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
        else if (theArg == "--logprov")
        {
            bLogProv = true;
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


    if (!logFileName.isEmpty())
    {
        QString logstart = QString("lumassengine - %1, %2")
                .arg(QDate::currentDate().toString())
                .arg(QTime::currentTime().toString());

        NMLoggingProvider::This()->setLogFileName(logFileName);
        NMLoggingProvider::This()->writeLogMsg(logstart);
    }
    else
    {
        // turn off logging altoghether
        NMLoggingProvider::This()->getLogger()->setLogLevel(NMLogger::NM_LOG_NOLOG);
    }


	switch(todo)
	{
	case NM_ENGINE_MOSO:
		doMOSO(losFileName);
		break;
    case NM_ENGINE_MODEL:
        doModel(modelFileName, workspace, enginePath, bLogProv);
        break;
	default:
        NMWarn(ctx, << "Please specify either an optimisation "
                    << " settings file or a model file!"
                    << std::endl);
		break;
	}

#ifdef LUMASS_PYTHON
    if (Py_IsInitialized())
    {
        pybind11::finalize_interpreter();
    }
#endif

	NMDebugCtx(ctx, << "done!");
	return EXIT_SUCCESS;
}
