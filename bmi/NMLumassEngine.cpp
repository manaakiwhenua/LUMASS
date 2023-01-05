/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include <QTextStream>
#include <QVariant>
#include <QThreadPool>
#include <QScopedPointer>

#include <sqlite3.h>
#include "gdal.h"
#include "gdal_priv.h"

#include "NMLumassEngine.h"
#include "NMModelController.h"
#include "NMModelSerialiser.h"
#include "NMModelComponent.h"
#include "NMSequentialIterComponent.h"

#ifdef LUMASS_PYTHON
#include "Python_wrapper.h"
#endif

#ifndef _WIN32
#   include <mpi.h>
#endif

#include "NMMosra.h"
#include "MOSORunnable.h"

////////////////////////////////////////
/// some MPI HELPERS
////////////////////////////////////////
#define NM_MPIGUARD( rank )  \
if (m_Rank != rank )         \
{                            \
    return;                  \
}

// store who we are and how many of us there are
int m_Rank;
int m_Nproc;
int m_ThreadSupport;
QString m_ThreadSupportStr;

const std::string NMLumassEngine::ctx = "NMLumassEngine";

NMLumassEngine::NMLumassEngine(QObject* parent)
    : QObject(parent),
      mController(nullptr),
      mMosra(nullptr),
      mBMILogger(nullptr),
      mMode(NM_ENGINE_MODE_UNKNOWN),
      mbMPICleanUp(false)
{
    NMDebugCtx(ctx, << "...");
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(false);

    // init mpi
    int mpierr = 0;
    int mpiinit = 0;
    MPI_Initialized(&mpiinit);

    QString mpiEnv;
    if (!mpiinit)
    {
        MPI_Init_thread(nullptr, nullptr, MPI_THREAD_FUNNELED, &m_ThreadSupport);

        switch(m_ThreadSupport)
        {
        case 0:  m_ThreadSupportStr = QStringLiteral("MPI_THREAD_SINGLE"); break;
        case 1:  m_ThreadSupportStr = QStringLiteral("MPI_THREAD_FUNNELED"); break;
        case 2:  m_ThreadSupportStr = QStringLiteral("MPI_THREAD_SERIALIZED"); break;
        case 3:  m_ThreadSupportStr = QStringLiteral("MPI_THREAD_MULTIPLE"); break;
        default: m_ThreadSupportStr = QStringLiteral("NO_CLUE_WHATSOEVER"); break;
        }

        // Letzter macht das Licht aus!
        mbMPICleanUp = true;
    }
    else
    {
        NMDebugAI(<< "MPI parallel processing failed to initialize correctly!" << endl);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &m_Nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_Rank);

    if (m_Rank == 0)
    {
        mpiEnv = QString("MPI Environment: \nMPI_COMM_WORLD=%1\nMPI_COMM_NULL=%2\n"
                         "MPI_GROUP_NULL=%6\nMPI_GROUP_EMPTY=%7\nMPI_UNDEFINED=%3\n"
                         "#proc=%4\nthread support=%5\n")
                .arg(MPI_COMM_WORLD).arg(MPI_COMM_NULL).arg(MPI_UNDEFINED)
                .arg(m_Nproc).arg(m_ThreadSupportStr).arg(MPI_GROUP_NULL).arg(MPI_GROUP_EMPTY);

        NMDebugAI(<< mpiEnv.toStdString() << endl);
        mLogger->sendLogMsg(mpiEnv);
    }

    mController = new NMModelController(this);
    mController->setLogger(mLogger);
    mController->setRank(m_Rank);
    mController->setNumProcs(m_Nproc);

    NMSequentialIterComponent* root = new NMSequentialIterComponent();
    root->setObjectName("root");
    root->setDescription("Top level model component managed by the ModelController");
    mController->addComponent(root);

    NMDebugCtx(ctx, << "done!");
}

NMLumassEngine::~NMLumassEngine()
{
}

void
NMLumassEngine::shutdown(void)
{
    if (mLogFile.isOpen())
    {
        mLogFile.flush();
        mLogFile.close();
    }

#ifdef LUMASS_PYTHON
    if (Py_IsInitialized())
    {
        pybind11::finalize_interpreter();
    }
#endif

    // clean up mpi
    int mpiInit;
    int mpierr = MPI_Initialized(&mpiInit);
    if (mpierr == 0 && mpiInit && mbMPICleanUp)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
    }
}

int NMLumassEngine::runModel(double fromTimeStep, double toTimeStep)
{
    // for now we're ignoring any fancy from and to time steps!

    NMLogDebug(<< "Running a model ... ");

    mController->executeModel("root");

    return 0;
}

void
NMLumassEngine::setSetting(const QString &key, const QString &value)
{

    NMLogDebug(<< "::setSetting(" << key.toStdString() << ", " << value.toStdString() << ")");
    mController->updateSettings(key, value);
}

void
NMLumassEngine::setLogProvenance(bool logProv)
{
    if (logProv)
    {
        mController->setLogProvOn();
        NMLogInfo(<< "Model provenance tracking enabled!");
    }
    else
    {
        mController->setLogProvOff();
        NMLogInfo(<< "Model provenance tracking disabled!");
    }
}

std::string NMLumassEngine::processStringParameter(const QString& param)
{
    std::string ret = param.toStdString();
    ret = this->mController->processStringParameter(nullptr, ret.c_str()).toStdString();
    return ret;
}

void
NMLumassEngine::doModel(const QString& userFile, QString &workspace, QString& enginePath, bool bLogProv)
{
    NMDebugCtx(ctx, << "...");

    // ==============================================
    //  import the model
    // ==============================================
    bool bYaml = false;
    QString modelFile;
    QString yamlWorkspace;
    QString yamlLogfileName;
    bool byamlLogProv;

    QFileInfo ufinfo(userFile);
    YAML::Node configFile;
    if (    ufinfo.suffix().compare(QString("yaml"), Qt::CaseInsensitive) == 0
         || ufinfo.suffix().compare(QString("yml"), Qt::CaseInsensitive) == 0
       )
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

            if (engineConfig["workspace"])
            {
                yamlWorkspace = engineConfig["workspace"].as<std::string>().c_str();
            }

            if (engineConfig["logfile"])
            {
                yamlLogfileName = engineConfig["logfile"].as<std::string>().c_str();
            }

            if (engineConfig["logprovenance"])
            {
                byamlLogProv = engineConfig["logprovenance"].IsDefined() ?
                    engineConfig["logprovenance"].as<bool>() : false;
            }

            bYaml = true;
        }
        catch (YAML::BadConversion& bc)
        {
            std::stringstream msg;
            msg << "ERROR: " << bc.what();
            NMErr("NMLumassEngine", << msg.str().c_str());
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
        catch (std::exception& se)
        {
            std::stringstream msg;
            msg << "ERROR: " << se.what();
            NMErr(ctx, << msg.str().c_str());
            //std::cout << "cout: " << msg.str() << std::endl;
            return;
        }
    }
    else
    {
        modelFile = userFile;
    }

    //QScopedPointer<NMModelController> ctrl(new NMModelController());
    //ctrl->setLogger(NMLoggingProvider::This()->getLogger());
    NMModelController* ctrl = mController;
    ctrl->updateSettings("LUMASSPath", enginePath);
    ctrl->updateSettings("TimeFormat", "yyyy-MM-ddThh:mm:ss.zzz");

    // if no appropriate command line settings were passed,
    // use as much as possible the yaml configuration settings
    if (bYaml)
    {
        ctrl->updateSettings("ConfigPath", ufinfo.canonicalPath());
        modelFile = ctrl->processStringParameter(nullptr, modelFile);

        if (workspace.isEmpty())
        {
            workspace = ctrl->processStringParameter(nullptr, yamlWorkspace);
        }

        if (mLogFileName.isEmpty() && !yamlLogfileName.isEmpty())
        {
            yamlLogfileName = ctrl->processStringParameter(nullptr, yamlLogfileName);

            mLogger->setLogLevel(NMLogger::NM_LOG_INFO);
            this->setLogFileName(yamlLogfileName);

            QString mpiEnv = QString("MPI Environment: #proc=%1; thread support=%2\n")
                             .arg(m_Nproc).arg(m_ThreadSupportStr);
            this->writeLogMsg(mpiEnv);
        }
    }


    QFileInfo fi(modelFile);
    if (fi.suffix().compare(QString("lmx"), Qt::CaseInsensitive) != 0)
    {
        NMErr(ctx, << "Invalid model specified!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

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
    xmlS.setModelController(ctrl);
    xmlS.setLogger(ctrl->getLogger());

    //// connect the logger to the logging provider
    //ctrl->connect(ctrl->getLogger(), SIGNAL(sendLogMsg(QString)),
    //              NMLoggingProvider::This(), SLOT(writeLogMsg(QString)));

//    NMSequentialIterComponent* root = new NMSequentialIterComponent();
//    root->setObjectName("root");
//    root->setDescription("Top level model component managed by the ModelController");
//    ctrl->addComponent(root);

    nameRegister = xmlS.parseComponent(modelFile, 0, ctrl);
    if (nameRegister.size() == 0)
    {
        NMErr(ctx, << "Invalid model file specified!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // if we've got a YAML, use that for overriding the
    // models hard wired configuration
    if (!configFile.IsNull())
    {
        try
        {
            if (configFile["ModelConfig"])
            {
                YAML::Node modelConfig = configFile["ModelConfig"];
                this->configureModel(modelConfig);
            }
        }
        catch(YAML::ParserException& pe)
        {
            NMErr("NMLumassEngine", << pe.what());
        }
        catch(YAML::RepresentationException& re)
        {
            NMErr("NMLumassEngine", << re.what());
        }
        catch (std::exception& se)
        {
            NMErr("NMLumassEngine", << se.what());
        }
    }

    // ==============================================
    //  EXECUTE MODEL
    // ==============================================
    GDALAllRegister();
    GetGDALDriverManager()->AutoLoadDrivers();
    sqlite3_temp_directory = const_cast<char*>(workspace.toStdString().c_str());//getenv("HOME");

    if (bLogProv || byamlLogProv)
    {
        ctrl->setLogProvOn();
    }

    ctrl->executeModel("root");

    GDALDestroyDriverManager();

    NMDebugCtx("NMLumassEngine", << "done!");
}

int
NMLumassEngine::loadModel(const QString &modelfile)
{
    // let's chuck out the old controller and install
    // a brand new one!

    NMIterableComponent* root = qobject_cast<NMIterableComponent*>(mController->getComponent("root"));
    NMModelComponentIterator cit = root->getComponentIterator();
    while (*cit != nullptr)
    {
        root->removeModelComponent(cit->objectName());
        ++cit;
    }

    QMap<QString, QString> nameRegister;
    NMModelSerialiser xmlS;
    xmlS.setModelController(mController);
    xmlS.setLogger(mController->getLogger());

    std::stringstream msg;
    msg << "Loading model '" << modelfile.toStdString() << "' ...";
    log("INFO", msg.str().c_str());
    nameRegister = xmlS.parseComponent(modelfile, 0, mController);

    this->mMode = NM_ENGINE_MODE_MODEL;
    return 0;
}

QString
NMLumassEngine::getYamlNodeTypeAsString(const YAML::Node &node)
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
NMLumassEngine::isYamlSequence(const YAML::Node& node)
{
    bool ret = false;

    if (node.Type() == YAML::NodeType::Sequence)
    {
        ret = true;
    }

    return ret;
}



QVariant
NMLumassEngine::parseYamlSetting(const YAML::const_iterator& nit, const QObject* obj)
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
NMLumassEngine::configureModel(const YAML::Node& modelConfig)
{
    //mController->clearModelSettings();

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
                 msg << "Model Configuration: Couldn't find component '"
                     << compName.toStdString() << "'!";
                 log("ERROR", msg.str().c_str());
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
                             wmsg << "Model Configuration: Component '"
                                       << compName.toStdString() << "' doesn't "
                                       << " have a '" << propName.toStdString()
                                       << " ' property'! We better skip this one.";
                             log("WARN", wmsg.str().c_str());
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
                         emsg << "Found invalid value for '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log("ERROR", emsg.str().c_str());
                     }
                     if (!proc->setProperty(propName.toStdString().c_str(), val))
                     {
                         std::stringstream emsg;
                         emsg << "Failed setting '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log("ERROR", emsg.str().c_str());
                     }
                 }
                 else if (bCompProps)
                 {
                     val = parseYamlSetting(pit, comp);
                     if (val.isNull() || !val.isValid())
                     {
                         std::stringstream emsg;
                         emsg << "Found invalid value for '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log("ERROR", emsg.str().c_str());
                     }
                     // set property value
                     if (!comp->setProperty(propName.toStdString().c_str(), val))
                     {
                         std::stringstream emsg;
                         emsg << "Failed setting '" << comp->objectName().toStdString() << ":"
                                    << propName.toStdString() << "'!";
                         log("ERROR", emsg.str().c_str());
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

int NMLumassEngine::loadOptimisationSettings(const QString & losFileName)
{

    QFile los(losFileName);
    if (!los.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::stringstream emsg;
        emsg << "LUMASS Engine: Failed reading optimisation settings file!";
        log("ERROR", emsg.str().c_str());
        return 1;
    }
    QTextStream str(&los);
    if (mMosra != nullptr)
    {
        delete mMosra;
    }

    mMosra = new NMMosra(this);
    mMosra->setLosSettings(str.readAll());
    los.close();
    mMosra->setLosFileName(losFileName);

    mMode = NM_ENGINE_MODE_MOSO;
    return 0;
}

int NMLumassEngine::configureOptimisation(const YAML::Node & modelConfig)
{
    std::stringstream emsg;
    emsg << "Looking for YAML-based optimisation scenario configuration ...";
    log("DEBUG", emsg.str().c_str());
    emsg.str("");

    // no need to parse settings if we haven't got a settings file
    // loaded yet
    if (mMosra == nullptr || mMosra->getLosSettings().isEmpty())
    {
        emsg << "Please load an optimisation settings file before parsing "
            << "YAML-based optimisation settings!";
        log("ERROR", emsg.str().c_str());
        return 1;
    }
    QString settings = mMosra->getLosSettings();

    // parse optimisation settings in YAML file and evaluate
    // *.los file LUMASS expressions
    if (!modelConfig.IsNull() && modelConfig.IsDefined())
    {
        YAML::const_iterator mit = modelConfig.begin();
        while (mit != modelConfig.end())
        {
            YAML::Node modelConfigNode = mit->second;
            if (!modelConfigNode.IsNull())
            {
                QString modelNodeName = mit->first.as<std::string>().c_str();
                std::stringstream msg;
                msg << "Parsing YAML ModelConifg::" << modelNodeName.toStdString().c_str() << " ...";
                log("INFO", msg.str().c_str());

                // node for moso configurations, we only look for global
                // settings that are then accessible via $[LUMASS:<propName>]$
                // in the *.los file
                if (modelNodeName.compare("Settings") == 0)
                {
                    YAML::const_iterator cnit = modelConfigNode.begin();
                    while(cnit != modelConfigNode.end())
                    {
                        QString propName = cnit->first.as<std::string>().c_str();
                        QVariant val = parseYamlSetting(cnit, nullptr);
                        mController->updateSettings(propName, val);
                        msg.str("");
                        msg << "  ModelConfig::Settings::"
                            << propName.toStdString().c_str() << "="
                            << val.toString().toStdString().c_str();
                        log("INFO", msg.str().c_str());
                        ++cnit;
                    }
                }
            }
            ++mit;
        }

        // evaluate and replace LUMASS expressions in *.los file
        NMModelComponent* root = mController->getComponent("root");
        if (root != nullptr)
        {
            settings = mController->processStringParameter(root, settings);
            if (settings.startsWith(QStringLiteral("ERROR")))
            {
                emsg << "The settings file contains undefined LUMASS settings, e.g. $[LUMASS:DataPath]$!"
                    << " Please double check your *.yaml configuration file, e.g. for 'DataPath: \"/mypath\"' !";
                log("ERROR", emsg.str().c_str());
                return 1;
            }
        }
    }
    else
    {
        emsg << "No YAML-based optimisation scenario configuration found!";
        log("DEBUG", emsg.str().c_str());
    }

    // need to store losFileName here since,
    // the settings parsing resets that member variable!
    QString losFileName = mMosra->getLosFileName();
    mMosra->setLosSettings(settings);
    mMosra->parseStringSettings(settings);
    mMosra->setLosFileName(losFileName);

    return 0;
}

void
NMLumassEngine::doMOSO(const QString& losFileName)
{
//    QScopedPointer<NMMosra> mosra(new NMMosra());
//    mosra->setLogger(NMLoggingProvider::This()->getLogger());
//    mosra->loadSettings(losFileName);

    loadOptimisationSettings(losFileName);
    mMosra->parseStringSettings(mMosra->getLosSettings());
    if (mMosra->doBatch())
    {
        doMOSObatch();
    }
    else
    {
        doMOSOsingle();
    }
}

int NMLumassEngine::runOptimisation(void)
{
    if (mMosra == nullptr)
    {
        std::stringstream emsg;
        emsg << "Please specify the optimisation problem (i.e. settings) before "
            << "trying to solve it!";
        log("ERROR", emsg.str().c_str());
        return 1;
    }

    int ret = 0;
    if (mMosra->doBatch())
    {
        ret = doMOSObatch();
    }
    else
    {
        ret = doMOSOsingle();
    }


    return ret;
}

int
NMLumassEngine::doMOSObatch()
{
    QScopedPointer<NMMosra> mosra(new NMMosra());
    mosra->setLogger(mLogger);
    mosra->setLosSettings(mMosra->getLosSettings());
    mosra->parseStringSettings(mosra->getLosSettings());
    mosra->setLosFileName(mMosra->getLosFileName());

    if (!mosra->doBatch())
    {
        std::stringstream emsg;
        emsg << "NMLumassEngine::doMOSObatch(): We don't quite have what we need for batch processing!";
        log("ERROR", emsg.str().c_str());
        return 1;
    }

    QString dsFileName = QString("%1/%2.vtk").arg(mosra->getDataPath())
            .arg(mosra->getLayerName());

    QFileInfo dsinfo(dsFileName);

    if (!dsinfo.isReadable())
    {
        std::stringstream emsg;
        emsg << "NMLumassEngine::doMOSObatch(): Could not read file '" << dsFileName.toStdString() << "'!";
        log("WARN", emsg.str().c_str());

        dsFileName = QString("%1/%2.ldb").arg(mosra->getDataPath())
                .arg(mosra->getLayerName());

        emsg.str("");
        emsg << "Trying this file '" << dsFileName.toStdString() << "' instead ...";
        log("INFO", emsg.str().c_str());

        dsinfo.setFile(dsFileName);
        if (!dsinfo.isReadable())
        {
            emsg.str("");
            emsg << "NMLumassEngine::doMOSObatch(): Could not read file '" << dsFileName.toStdString() << "'!";
            log("ERROR", emsg.str().c_str());
            return 1;
        }
        else
        {
            emsg.str("");
            emsg << "... and it worked!";
            log("INFO", emsg.str().c_str());
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
                m->setLogger(mLogger);
                m->setData(dsFileName, mosra->getLosFileName(),
                        item, levels, runstart, chunksize, mosra->getLosSettings());
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
                m->setLogger(mLogger);
                m->setData(dsFileName, mosra->getLosFileName(),
                        item, itemUncertainties, 1, 1, mosra->getLosSettings());
                QThreadPool::globalInstance()->start(m);
            }
        }
    }
    QThreadPool::globalInstance()->waitForDone();

    return 0;
}

int
NMLumassEngine::doMOSOsingle()
{
    QScopedPointer<NMMosra> mosra(new NMMosra());
    mosra->setLogger(mLogger);
    mosra->setLosSettings(mMosra->getLosSettings());
    mosra->parseStringSettings(mosra->getLosSettings());
    mosra->setLosFileName(mMosra->getLosFileName());

//    QString dsFileName = QString("%1/%2.vtk").arg(mosra->getDataPath())
//            .arg(mosra->getLayerName());

    QStringList ltypes;
    ltypes << "vtk" << "ldb" << "db" << "sqlite";

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
        std::stringstream emsg;
        emsg << "NMLumassEngine::doMOSOsingle(): Could not read file '" << dsFileName.toStdString() << "'!";
        log("ERROR", emsg.str().c_str());
        return 1;
    }

    QList<float> levels;
    levels << 0;
    MOSORunnable* m = new MOSORunnable();
    m->setLogger(mLogger);
    m->setData(dsFileName, mosra->getLosFileName(),
               "", levels, 1, 1, mosra->getLosSettings());
    QThreadPool::globalInstance()->start(m);
    QThreadPool::globalInstance()->waitForDone();

    return 0;
}

void
NMLumassEngine::about(void)
{
    log("DEBUG", "This is all about NMLumassEngine!");
}

void
NMLumassEngine::setLogFileName(const QString &fn)
{
    if (mLogFile.isOpen())
    {
        mLogFile.close();
    }

    mLogFile.setFileName(fn);
    if (!mLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::stringstream emsg;
        emsg << "Failed creating log file!";
        log("ERROR", emsg.str().c_str());
        return;
    }
    mLogFileName = fn;

    connect(mLogger, SIGNAL(sendLogMsg(QString)), this, SLOT(writeLogMsg(QString)));

    // write the first message
    QString logstart = QString("LUMASS Engine - %1, %2\n")
            .arg(QDate::currentDate().toString())
            .arg(QTime::currentTime().toString());

    this->writeLogMsg(logstart);
    //mBMILogger(2, logstart.toStdString().c_str());
    log("INFO", logstart);
}

void
NMLumassEngine::log(const QString& type, const QString &qmsg)
{
    std::string msg = qmsg.toStdString();

    if (type.compare("WARN") == 0)
    {
        NMLogWarn(<< msg);
        if (mBMILogger != nullptr)
        {
            mBMILogger(3, msg.c_str());
        }
    }
    else if (type.compare("ERROR") == 0)
    {
        NMLogError(<< msg);
        if (mBMILogger != nullptr)
        {
            mBMILogger(4, msg.c_str());
        }
    }
    else if (type.compare("INFO") == 0)
    {
        NMLogInfo(<< msg);
        if (mBMILogger != nullptr)
        {
            mBMILogger(2, msg.c_str());
        }
    }
    else if (type.compare("DEBUG") == 0)
    {
        NMLogDebug(<< msg);
        if (mBMILogger != nullptr)
        {
            mBMILogger(1, msg.c_str());
        }
    }
}

void
NMLumassEngine::writeLogMsg(const QString& msg)
{
    QString themsg = msg;
    QObject* sender = this->sender();
    if (sender != nullptr)
    {
        themsg = QString("r%1:%2::%3").arg(m_Rank).arg(sender->objectName()).arg(msg);
    }

    if (!mLogFile.isOpen())
    {
        NMErr("NMLumassEngine", << "Failed writing log message - log file is closed!");
        //mBMILogger(4, "NMLumassEngine: Failed writing log message - log file is closed!");
        log("ERROR", "NMLumassEngine: Failed writing log message - log file is closed!");
        return;
    }

    QTextStream out(&mLogFile);
    out << themsg;

}
