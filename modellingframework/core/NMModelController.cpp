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
/*
 * NMModelController.cpp
 *
 *  Created on: 11/06/2012
 *      Author: alex
 */

#include <cstddef>
#include <QFuture>
//#include <QtConcurrentRun>
#include <QFileInfo>
#include <QString>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#ifdef LUMASS_PYTHON
#include "Python_wrapper.h"
namespace py = pybind11;
//namespace lupy = lumass_python;
#endif

#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QDomDocument>

#include "NMModelController.h"
#include "NMIterableComponent.h"
#include "NMParallelIterComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMParameterTable.h"
#include "NMDataComponent.h"
#include "NMMfwException.h"
#include "NMImageReader.h"
#include "NMTableReader.h"
#include "NMModelSerialiser.h"
#include "otbMultiParser.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "NMMPIRunnable.h"
#include "NMLumassEngine.h"

const std::string NMModelController::ctx = "NMModelController";

NMModelController::NMModelController(NMLumassEngine *engine, QObject* parent)
    : mEngine(engine),
      mbModelIsRunning(false),
      mRootComponent(0), mbAbortionRequested(false),
      mbLogProv(false),
      mRank(0),
      mNumProcs(1),
      mParentMPIComm(MPI_COMM_NULL),
      mMergedComm(MPI_COMM_NULL),
      mInterComm(MPI_COMM_NULL),
      mAppMode(4),
      mbIsMPIEventLoopRunning(false),
      mbHasMPIRuntime(false),
      mMPICompProgWin(MPI_WIN_NULL),
      mMPICompState(nullptr),
      mMPIAbort(nullptr)
{
    this->setParent(parent);
    this->mModelStarted = QDateTime::currentDateTime();
    this->mModelStopped = this->mModelStarted;
    mLogger = new NMLogger(this);

    qRegisterMetaType<NMMPIRunnable*>("NMMPIRunnable*");

    // mParentMPIComm is ever only not MPI_COMM_NULL
    // for child processes spawned by the GUI
    // that are running the 'GUI model' in
    // multiple instances of the lumassengine,
    // calling NMModelController::executeMPIChildModel()
    // which lumassengine processes are exeting
    // afterwards
    NMDebugAI(<< "+++++ MPI_Comm_get_parent() ...")
    MPI_Comm_get_parent(&mParentMPIComm);

    // create the one and only root model component
    NMSequentialIterComponent* root = new NMSequentialIterComponent();
    root->setObjectName("root");
    root->setDescription("Top level model component managed by the ModelController");
    this->addComponent(root);
}

bool NMModelController::getUsesMPIRuntime(void)
{
    return mbHasMPIRuntime;
}

void NMModelController::setUsesMPIRuntime(bool hasRuntime)
{
    mbHasMPIRuntime = hasRuntime;
}

NMModelController::~NMModelController()
{
}

void
NMModelController::finalizePythonInterpreter()
{
#ifdef LUMASS_PYTHON
        if (!Py_IsInitialized())
        {
            py::finalize_interpreter();
        }
#endif
}

void
NMModelController::setLogger(NMLogger* logger)
{
    if (mLogger != nullptr)
    {
        delete mLogger;
    }

    mLogger = logger;
}

QSharedPointer<NMItkDataObjectWrapper>
NMModelController::getOutputFromSource(const QString& inputSrc)
{
    QSharedPointer<NMItkDataObjectWrapper> w;
    w.clear();

    // parse the input source string
    QStringList inputSrcParams = inputSrc.split(":", Qt::SkipEmptyParts);
    QString inputCompName = inputSrcParams.at(0);

    NMModelComponent* mc = this->getComponent(inputCompName);
    if (mc == 0)
        return w;

    bool bOK;
    int outIdx = 0;
    if (inputSrcParams.size() == 2)
    {
        outIdx = inputSrcParams.at(1).toInt(&bOK);
        if (!bOK)
        {
            NMLogError(<< ctx << ": failed to interpret input source parameter"
                    << "'" << inputSrc.toStdString() << "'");
            return w;
        }
    }

    w = mc->getOutput(outIdx);
    return w;
}


void
NMModelController::mpiSignalProgress(MPICompProg &progStruct)
{
    // only signalling to our parent/server model
    if (mParentMPIComm == MPI_COMM_NULL)
    {
        return;
    }

    QStringList sortedModelComps = mComponentMap.keys();
    sortedModelComps.sort(Qt::CaseInsensitive);
    const int compId = sortedModelComps.indexOf(progStruct.compName);
    if (compId < 0)
    {
        NMLogError(<< "Signalling progress failed! Couldn't find sender '"
                   << progStruct.compName.toStdString() << "'!");
        return;
    }

    // get rank and size info
    int mrank, msize, localRank, localSize;
    MPI_Comm_rank(this->mMergedComm, &mrank);
    MPI_Comm_size(this->mMergedComm, &msize);

    MPI_Comm_rank(MPI_COMM_WORLD, &localRank);
    MPI_Comm_size(MPI_COMM_WORLD, &localSize);

    std::vector<int> eventProg = {
        static_cast<int>(progStruct.event),
        static_cast<int>(progStruct.progress+0.5)
        };

    const int ncomps = sortedModelComps.size();
    const int nvals = 2;

    const int target_pos = localRank * ncomps * nvals + compId * nvals;

    MPI_Win_lock(MPI_LOCK_SHARED, mrank, MPI_MODE_NOCHECK, mMPICompProgWin);
    MPI_Put(static_cast<void*>(&eventProg[0]), 2, MPI_INT, mrank, target_pos, 2, MPI_INT, mMPICompProgWin);
    MPI_Win_unlock(mrank, mMPICompProgWin);


#ifdef LUMASS_DEBUG

    std::string eventName;
    const int eventId = eventProg[0];
    switch (eventId)
    {
        case 2: eventName = "PROGRESS"; break;
        case 3: eventName = "EXEC_START"; break;
        case 4: eventName = "EXEC_STOP"; break;
        case 5: eventName = "ABORT_EXEC"; break;
        case 6: eventName = "START_EXEC"; break;
        case 7: eventName = "EXEC_ABORTED"; break;
        case 8: eventName = "NUMITER_CHGD"; break;
        case 9: eventName = "MODEL_COMPLETED"; break;
        case 1:
        default:
             eventName = "UNKNOWN"; break;
    }

    NMDebugAI(<< "#" << localRank << " target_pos=" << target_pos
              << " posted: " << progStruct.compName.toStdString()
              << "::" << eventName << "=" << eventProg[1] << std::endl);
#endif

    // check whether we're supposed to abort
    MPI_Win_lock(MPI_LOCK_SHARED, 0, MPI_MODE_NOCHECK, mMPIParentAbort);
    MPI_Get(static_cast<void*>(mMPIAbort), 1, MPI_INT, 0, 0, 1, MPI_INT, mMPIParentAbort);
    MPI_Win_unlock(0, mMPIParentAbort);

    if (*mMPIAbort == 1)
    {
        NMDebugAI(<< ctx << ": MPI child 'ModelController::abortModel()'!\n");
        this->abortModel();
    }
}

bool
NMModelController::isModelRunning(void)
{
    return this->mbModelIsRunning;
}

void
NMModelController::reportExecutionStopped(const QString compName)
{
    for (int i=0; i < this->mExecutionStack.size(); ++i)
    {
        if (compName.compare(this->mExecutionStack.at(i)) == 0)
        {
            this->mExecutionStack.remove(i);
            break;
        }
    }
}

void
NMModelController::reportExecutionStarted(const QString compName)
{
    this->mExecutionStack.push(compName);
}


void
NMModelController::abortModel(void)
{
    NMDebugCtx(ctx, << "...");
    if (this->mbModelIsRunning)
    {
        QString name;
        if (this->mExecutionStack.size() > 0)
            name = this->mExecutionStack.pop();

        NMIterableComponent* comp =
                qobject_cast<NMIterableComponent*>(this->getComponent(name));
        if (comp != 0)
        {
            NMProcess* proc = comp->getProcess();
            if (proc != 0)
            {
                proc->abortExecution();
            }
        }
        this->mbAbortionRequested = true;

        NMLogInfo(<< "ModelController: Model '" << name.toStdString()
                  << "' has been requested to abort execution at the next opportunity!");
    }
    NMDebugCtx(ctx, << "done!");
}

void
NMModelController::resetExecutionStack(void)
{
    // we take all remaining component names from the stack and
    // signal that they're actually not running any more
    for (int n=0; n < this->mExecutionStack.size(); ++n)
    {
        QString name = this->mExecutionStack.pop();
        emit signalExecutionStopped(name);
    }
}

NMIterableComponent*
NMModelController::identifyRootComponent(void)
{
    NMDebugCtx(ctx, << "...");

    NMIterableComponent* root = 0;

    QMapIterator<QString, NMModelComponent*> cit(this->mComponentMap);
    while(cit.hasNext())
    {
        cit.next();
        NMDebugAI(<< "checking '" << cit.value()->objectName().toStdString()
                << "' ...");
        if (cit.value()->getHostComponent() == 0)
        {
            root = qobject_cast<NMIterableComponent*>(cit.value());
            NMDebug(<< " got it !" << std::endl);
            break;
        }
        NMDebug(<< " nope!" << std::endl);
    }

    this->mRootComponent = root;

    NMDebugCtx(ctx, << "done!");
    return root;
}

//void
//NMModelController::executeAndDestroy(NMModelComponent *comp)
//{
//    QString cname = this->addComponent(comp);
//    this->executeModel(cname);
//    this->removeComponent(cname);
//}

void
NMModelController::deleteComponentsLater(QStringList compNames)
{
    if (this->isModelRunning())
    {
        this->mToBeDeleted.append(compNames);
        return;
    }

    foreach(const QString& name, compNames)
    {
        this->removeComponent(name);
    }
}

void
NMModelController::initPythonInterpreter(void)
{
#ifdef LUMASS_PYTHON
        if (!Py_IsInitialized())
        {
            py::initialize_interpreter();
        }
#endif
}

QString
NMModelController::getYamlConfigValue(const QString &configFN, const QString &configNodeStr,
                                      const QString& itemStr)
{
    QString retValue;

    QFileInfo yamlInfo(configFN);
    if (yamlInfo.isReadable())
    {
        YAML::Node fileNode = YAML::LoadFile(configFN.toStdString());
        if (fileNode[configNodeStr.toStdString()])
        {
            YAML::Node node1 = fileNode[configNodeStr.toStdString()];
            retValue = node1[itemStr.toStdString()].as<std::string>().c_str();
        }
    }
    else
    {
        NMLogError(<< "The configuration file '"
                   << configFN.toStdString() << "' "
                   << "could not be edited!");
    }

    return retValue;
}

void
NMModelController::setYamlConfigValue(const QString &configFN, YAML::Node &fileNode,
                                      const QString &configNodeStr, const QString& settingNodeStr,
                                      const QString& itemStr, const QString &value)
{
    QFileInfo yamlInfo(configFN);
    if (yamlInfo.isWritable())
    {
        fileNode = YAML::LoadFile(configFN.toStdString());
        if (fileNode[configNodeStr.toStdString()])
        {
            YAML::Node node1 = fileNode[configNodeStr.toStdString()];

            if (settingNodeStr.isEmpty())
            {
                node1[itemStr.toStdString()] = value.toStdString();
            }
            else
            {
                YAML::Node settingNode = node1[settingNodeStr.toStdString()];
                settingNode[itemStr.toStdString()] = value.toStdString();
            }
        }

        //singleQuoteYamlValues(fileNode);
    }
    else
    {
        NMLogError(<< "The configuration file '"
                   << configFN.toStdString() << "' "
                   << "could not be edited!");
        return;
    }
}

void
NMModelController::emitYaml(YAML::Emitter& emitter, const YAML::Node& node)
{
    if (node.IsNull() || !node.IsDefined())
    {
        return;
    }

    if (node.IsMap())
    {
        emitter << YAML::BeginMap;
        YAML::const_iterator mit=node.begin();
        for (; mit != node.end(); ++mit)
        {
            emitter << YAML::Key << mit->first;
            if (mit->second.IsScalar())
            {
                emitter << YAML::SingleQuoted << YAML::Value << mit->second.as<std::string>();
            }
            else
            {
                YAML::Node m2node = mit->second;
                emitYaml(emitter, m2node);
            }
        }
        emitter << YAML::EndMap;
    }
    else if (node.IsSequence())
    {
        emitter << YAML::BeginSeq;
        for (int s=0; s < node.size(); ++s)
        {
            if (node[s].IsScalar())
            {
                emitter << YAML::SingleQuoted << YAML::Value << node[s].as<std::string>();
            }
            else
            {
                YAML::Node pnode = node[s];
                emitYaml(emitter, pnode);
            }
        }
        emitter << YAML::EndSeq;
    }
}

QStringList
NMModelController::getModelSettingsList(void)
{
    QStringList modelSettings = mSettings.keys();
    modelSettings.removeAll("UserModels");
    modelSettings.removeAll("Workspace");
    modelSettings.removeAll("LUMASSPath");
    modelSettings.removeAll("TimeFormat");

    return modelSettings;
}

void
NMModelController::clearModelSettings(void)
{
    QStringList modelSettings = mSettings.keys();
    QStringList sys;
    sys << "UserModels" << "Workspace" << "LUMASSPath" << "TimeFormat" << "MaxProcCount" << "MaxThreadCount";

    foreach(const QString& key, modelSettings)
    {
        if (!sys.contains(key))
        {
            mSettings.remove(key);
            emit settingsUpdated(key, QVariant());
        }
    }
}

void
NMModelController::updateSettings(const QString key, QVariant value)
{
    if (value.isValid())
    {
        mSettings[key] = value;
    }
    else
    {
        QStringList sys;
        sys << "UserModels" << "Workspace" << "LUMASSPath";

        if (!sys.contains(key))
        {
            mSettings.remove(key);
        }
    }

    emit settingsUpdated(key, value);
}

void
NMModelController::notifyParentProcess(int msg, int tag)
{
    if (this->mAppMode == 1 && this->mParentMPIComm != MPI_COMM_NULL)
    {
        NMDebugAI(<< "notifyParentProcess: msg=" << msg << " | tag=" << tag << std::endl);
        MPI_Ssend(&msg, 1, MPI_INT, 0, tag, this->mParentMPIComm);
    }
}

void
NMModelController::executeModel(const QString compName,
                                const QString yamlFN)
{
    NMDebugCtx(ctx, << "...");

    NMLogDebug(<< "ModelController ThreadId: " << QThread::currentThreadId());

    // do we have parallel components in the model at all?
    bool bParallelModel = false;
    QMap<NMModelController::ModelParallelism, QStringList> paraComps;

    mParallelHosts.clear();
    this->identifyParallelComponents(compName, paraComps, mParallelHosts);

    if (paraComps.size() > 0)
    {
        bParallelModel = true;
        std::stringstream paralog;
        auto pcit = paraComps.constBegin();
        for (; pcit != paraComps.constEnd(); ++pcit)
        {
            switch(pcit.key())
            {
            case NM_PARALLEL_ITERATOR:  paralog << "Parallel Iterators: "; break;
            case NM_PARALLEL_TIMELEVEL: paralog << "Parallel Time Levels: "; break;
            case NM_PARALLEL_PIPELINE:  paralog << "Parallel Pipelines: "; break;
            default: break;
            }

            foreach(const QString& cn, pcit.value())
            {
                paralog << cn.toStdString() << " ";
            }
            paralog << std::endl;
        }
        NMLogInfo(<< "Parallel Model Components: \n" << paralog.str());
        NMDebugAI(<< "Parallel Model Components: \n" << paralog.str() << std::endl);
        paralog.str("");
        auto dphc = mParallelHosts.cbegin();
        for (; dphc != mParallelHosts.cend(); ++dphc)
        {
            paralog << (*dphc).toStdString() << " ";
        }
        NMLogInfo(<< "Parallel Host Components: " << paralog.str());
        NMDebugAI(<< "Parallel Host Components: " << paralog.str() << std::endl);

    }
    else
    {
        NMLogInfo(<< "No parallel model components detected.");
        NMDebugAI(<< "No parallel model components detected." << std::endl);
    }

    std::string pcom = mParentMPIComm == MPI_COMM_NULL ? "MPI_COMM_NULL" : "INSTANTIATED";
    NMDebugAI(<< ">>>>>> mAppMode: " << mAppMode << std::endl);
    NMDebugAI(<< ">>>>>> mbHasMPIRuntime: " << mbHasMPIRuntime << std::endl);
    NMDebugAI(<< ">>>>>> mParentMPIComm: " << pcom << std::endl);
    NMLogDebug(<< ">>>>>> mAppMode: " << mAppMode << std::endl);
    NMLogDebug(<< ">>>>>> mbHasMPIRuntime: " << mbHasMPIRuntime << std::endl);
    NMLogDebug(<< ">>>>>> mParentMPIComm: " << pcom << std::endl);


    // starting provenance logs, if enabled
    if (mbLogProv)
    {
        NMModelComponent* comp = this->getComponent(compName);
        if (comp == nullptr)
        {
            NMLogError(<< ctx << ": couldn't find '"
                    << compName.toStdString() << "'!");
            NMDebugCtx(ctx, << "done!");
            notifyParentProcess(0, 73);
            return;
        }
        QString userID = comp->getUserID();
        if (userID.isEmpty())
        {
            userID = comp->objectName();
        }

        // - if we're in MPI mode, we need to determine our appropriate
        //   rank number to be appended to the provenance file name:
        //   -> if we're an engine in mpi child model mode, we increase
        //      our rank by +1 as rank #0 is the gui-based mpi parent model
        // - if we're not in MPI mode, logRank remains an empty string
        QString logRank = "";
        if (mbHasMPIRuntime)
        {
            int _log_rank_num = mRank;
            if (    mAppMode == 1
                 && mParentMPIComm != MPI_COMM_NULL
               )
            {
                _log_rank_num++;
            }
            logRank = QString("_r%1").arg(_log_rank_num);
        }

        QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        stamp = stamp.replace(":", "");
        stamp = stamp.replace("-", "");

        QString provFN = QString("%1/%2_%3%4.provn")
                         .arg(this->getSetting("Workspace").toString())
                         .arg(userID)
                         .arg(stamp)
                         .arg(logRank);
        startProv(provFN, comp->objectName());
    }

    int yamlProcs = this->getNumProcs();
    QVariant  vYamlProcs = this->getSetting(QStringLiteral("MaxProcCount"));
    NMDebugAI(<< ">>>>>> ParentProcCount: " << this->getNumProcs() << std::endl);
    if (vYamlProcs.isValid())
    {
        bool pconv = false;
        int _yamlProcs = vYamlProcs.toInt(&pconv);
        if (pconv)
        {
            yamlProcs = _yamlProcs;
        }
    }
    NMDebugAI(<< ">>>>>> yamlProcs: " << yamlProcs << std::endl);

    // gui launches parallel parent model
    if (    mAppMode == 3   // GUI
         && mbHasMPIRuntime
         && mParentMPIComm == MPI_COMM_NULL
         && bParallelModel
         && yamlProcs > 1
       )
    {
        if (mbIsMPIEventLoopRunning)
        {
            NMLogError(<< "Sorry, I can only run one MPI child model at a time! "
                       << "Please wait for it to finish and then try again!\n");
            NMDebugCtx(ctx, << "done!");
            return;
        }
        executeMPIParentModel(compName, yamlFN);
    }
    // engine launches paralell child model
    else if (   mAppMode == 1 // ENGINE
             && mbHasMPIRuntime
            )
    {
        if (mParentMPIComm != MPI_COMM_NULL)
        {
            executeMPIChildModel(compName);
        }
        else
        {
            executeSeqModel(compName);
        }
    }
    // all other cases ...
    // ... probably need to add mpi support for running lumass through the BMI interface ...
    else if (mAppMode == 4)
    {
        executeSeqModel(compName);
    }
    else
    {
        executeSeqModel(compName);
        //mConcurrentVoidReturn = QtConcurrent::run(this, &NMModelController::executeSeqModel,
        //                                           compName, QString());
        //mConcurrentVoidWatcher.setFuture(mConcurrentVoidReturn);

    }

    this->mAlphaComps.clear();

    emit signalModelStopped();

    NMDebugCtx(ctx, << "done!");
}

void
NMModelController::identifyParallelComponents(const QString& compName,
                         QMap<ModelParallelism, QStringList> &parallelComps,
                         QSet<QString> &parallelHosts)
{
    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(getComponent(compName));
    if (ic == nullptr)
    {
        return;
    }

    NMParallelIterComponent* pic = qobject_cast<NMParallelIterComponent*>(ic);
    if (pic != nullptr)
    {
        auto it = parallelComps.find(NM_PARALLEL_ITERATOR);
        if (it != parallelComps.end())
        {
            it.value().append(compName);
        }
        else
        {
            QStringList pit;
            pit << compName;
            parallelComps.insert(NM_PARALLEL_ITERATOR, pit);
        }
        parallelHosts << compName;
    }


    // identify pipelines writing NetCDF (*.nc)
    // files in parallel
    if (ic->getProcess() != nullptr)
    {
        if (ic->objectName().startsWith(QStringLiteral("ImageWriter")))
        {
            NMStreamingImageFileWriterWrapper* writer =
                qobject_cast<NMStreamingImageFileWriterWrapper*>(ic->getProcess());
            QStringList filenames = writer->getFileNames();

            bool bParallel = false;
            foreach(const QString& fn, filenames)
            {
                if (    fn.contains(QStringLiteral(".nc"))
                     && writer->getWriteProcs() > 1
                   )
                {
                    bParallel = true;
                    break;
                }
            }

            // get all pipeline components
            if (bParallel)
            {
                parallelHosts << ic->getHostComponent()->objectName();

                QStringList pipeComps;
                ic->getUpstreamPipelineComponents(pipeComps);
                pipeComps.push_back(ic->objectName());

                auto pcmit = parallelComps.find(NM_PARALLEL_PIPELINE);
                if (pcmit != parallelComps.end())
                {
                    pcmit.value().append(pipeComps);
                }
                else
                {
                    parallelComps.insert(NM_PARALLEL_PIPELINE, pipeComps);
                }
            }
        }
    }
    // look for parallel sub-components
    else
    {
        // any time level parallelism?
        QMap<unsigned int, QMap<QString, NMModelComponent*> > levelMap;
        ic->mapTimeLevels(ic->getTimeLevel(), levelMap);

        auto levelIt = levelMap.constBegin();
        while (levelIt != levelMap.constEnd())
        {
            const QMap<QString, NMModelComponent*>& levelComp = levelIt.value();
            QStringList levelParallelList;
            if (levelIt.value().keys().size() > 1)
            {
                // look at individual pieplines at this level
                QList<QStringList> execList;
                QStringList execComps = ic->findExecutableComponents(levelComp, levelIt.key(), 0);

                foreach(const QString& ec, execComps)
                {
                    QStringList pipelineComps;
                    NMIterableComponent* iexc = qobject_cast<NMIterableComponent*>(this->getComponent(ec));
                    if (iexc != nullptr)
                    {
                        iexc->getUpstreamPipelineComponents(pipelineComps);
                        pipelineComps.push_back(ec);
                    }
                    execList.push_back(pipelineComps);
                }

                //levelParallelList.append(execComps);

                // look at aggregate components at this level
                QStringList parallelICs;
                foreach(const QString& pag, levelComp.keys())
                {
                    NMIterableComponent* plit = qobject_cast<NMIterableComponent*>(this->getComponent(pag));
                    if (    plit != nullptr
                         && plit != ic
                         && plit->getHostComponent() != ic
                         && plit->getProcess() == nullptr
                       )
                    {
                        parallelICs.append(pag);
                    }
                }

                // if we've got 2 or more pipelines or aggregate components
                // or at least one of each sort, we could execute them
                // in parallel
                if (    (parallelICs.size() > 0 && execList.size() > 0)
                     || (parallelICs.size() > 1 || execList.size() > 1)
                   )
                {
                    foreach(const QStringList& el, execList)
                    {
                        levelParallelList.append(el);
                    }

                    levelParallelList.append(parallelICs);

                    // add all parallel components we found to our map
                    auto p2it = parallelComps.find(NM_PARALLEL_TIMELEVEL);
                    if (p2it != parallelComps.end())
                    {
                        p2it.value().append(levelParallelList);
                    }
                    else
                    {
                        parallelComps.insert(NM_PARALLEL_TIMELEVEL, levelParallelList);
                    }
                }
            }
            ++levelIt;

            foreach(const QString& lpc, levelParallelList)
            {
                parallelHosts << this->getComponent(lpc)->getHostComponent()->objectName();
            }
        }

        // one level deeper into the parallel rabbit hole ...
        NMModelComponentIterator cit = ic->getComponentIterator();
        while(*cit != nullptr)
        {
            this->identifyParallelComponents((*cit)->objectName(), parallelComps, parallelHosts);
            ++cit;
        }
    }
}

void
NMModelController::executeMPIParentModel(const QString &compName,
                                           const QString &yamlFN)
{
    NMDebugCtx(ctx, << "...");

    // serialize the model
    NMModelComponent* comp = this->getComponent(compName);
    if (comp == nullptr)
    {
        NMLogError(<< "Cannot execute a NULL model!");
        return;
    }

    // --------------------------------------------------------------------------
    // create model and config file names for parallel execution of this component

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss");
    QString lumassPath = this->getSetting("LUMASSPath").toString();
    QString workspace = this->getSetting("Workspace").toString();
    QString modelFN = QString("%1/%2_%3.lmx").arg(workspace).arg(compName).arg(timestamp);
    QString newYamlFN = QString("%1/%2_%3.yaml").arg(workspace).arg(compName).arg(timestamp);
    QString logFN = QString("%1/%2_%3.log").arg(workspace).arg(compName).arg(timestamp);

    QFileInfo modelInfo(modelFN);
    QString modelBaseName = QString("$[LUMASS:ConfigPath]$/%1.lmx").arg(modelInfo.completeBaseName());

    // --------------------------------------------------------------
    // create new yaml config file adapting the model file name
    YAML::Node node;
    this->setYamlConfigValue(yamlFN, node, "EngineConfig", QString(), "modelfile", modelBaseName);


    // ..................................................
    // replace 'ConfigPath' with original path

    // work out original value of `ConfigPath`
    QFileInfo yfinfo(yamlFN);
    QString origConfigPath;
    if (yfinfo.isFile() && yfinfo.isReadable())
    {
        origConfigPath = yfinfo.absolutePath();
    }
    const QString pathExpr = QStringLiteral("$[LUMASS:ConfigPath]$");

    YAML::Emitter emitter;
    emitYaml(emitter, node);
    QString yamlStr = emitter.c_str();
    int settingPos = yamlStr.indexOf(QStringLiteral("Settings:"));

    QString engineConf = yamlStr.left(settingPos);
    QString settingStr = yamlStr.right(yamlStr.size() - settingPos);
    settingStr = settingStr.replace(pathExpr, origConfigPath);

    // ........................................................
    // write new YAML file

    QFile yamlFile(newYamlFN);
    if (!yamlFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        yamlFile.close();
        NMLogError(<< "Couldn't create config file '" << newYamlFN.toStdString() << "'!");
        return;
    }

    // write yaml
    QTextStream yamlOut(&yamlFile);
    yamlOut << engineConf.toStdString().c_str() << settingStr.toStdString().c_str();
    yamlFile.close();

    // --------------------------------------------------------------------
    // create model file (*.lmx)

    // identify top component
    QStringList saveComponents = this->getRepository().keys();

    // never save the root component as it is handled automatically by
    // the ModelController instance
    saveComponents.removeOne(QStringLiteral("root"));
    NMDebugAI(<< "Parent's repo's comps exported for child model: " << saveComponents.join(" ").toStdString() << std::endl);

    QDomDocument doc;
    QDomElement modElem = doc.createElement("Model");
    modElem.setAttribute("description", "the one and only model element");
    doc.appendChild(modElem);

    NMModelSerialiser xmlS;
    xmlS.setModelController(this);
    xmlS.setLogger(mLogger);

    foreach (const QString& cn, saveComponents)
    {
        NMModelComponent* ec = this->getComponent(cn);
        xmlS.serialiseComponent(ec, doc);
    }

    // replace ConfigPath var with original ConfigPath value: 'origConfigPath'
    QString lmxStr = doc.toString(4);
    lmxStr = lmxStr.replace(pathExpr, origConfigPath);


    QFile modelFile(modelFN);
    if (!modelFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        modelFile.close();
        NMLogError(<< "Couldn't create model file '" << modelFN.toStdString() << "'!");
        return;
    }

    QTextStream xmlOut(&modelFile);
    xmlOut << lmxStr;//doc.toString(4);
    modelFile.close();

    // --------------------------------------------------------------------------------
    // SPAWN CHILD PROCESSES FOR PARALLEL PROCESSING
    // --------------------------------------------------------------------------------

    QString cmd_suffix = "";
#ifdef _WIN32
    cmd_suffix = ".exe";
#endif

    //QString cmd = QString("xterm");
    QString cmd = QString("%1/lumassengine%2")
                    .arg(lumassPath).arg(cmd_suffix);
    QString args = QString("--model %1 --logfile %2 --comp %3 --logprov").arg(newYamlFN).arg(logFN).arg(compName);
    //QString args = QString("-e gdb --args %1/lumassengine --model %2 --logfile %3")
    //                .arg(lumassPath).arg(modelFN).arg(logFN);
    QStringList argsList = args.split(" ", Qt::SkipEmptyParts);


    std::stringstream argstr;
    char** argv = new char*[argsList.size()+1];

    for (int i=0; i < argsList.size(); ++i)
    {
        argv[i] = new char[argsList.at(i).size()+1];
        argstr << strcpy(argv[i], argsList.at(i).toStdString().c_str()) << " ";
    }
    argv[argsList.size()] = NULL;

    // get the number of processes to be used for running
    // the model
    bool bok;
    int nprocs = this->getSetting("MaxProcCount").toUInt(&bok);
    if (!bok)
    {
        nprocs = 1;
    }
    NMLogInfo(<< "MaxProcCount = " << nprocs);

    // provide some feedback of what is happening ...
    std::stringstream infoMsg;
    infoMsg << "Spawning " << nprocs
            << " child processes: " << cmd.toStdString() << " "
            << argstr.str() << std::endl;
    NMLogInfo(<< infoMsg.str());
    NMDebugAI(<< infoMsg.str());

    // create nprocs child processes running the model in parallel
    int errCodes[nprocs];
    int err = MPI_Comm_spawn(cmd.toStdString().c_str(), argv, nprocs,
                             MPI_INFO_NULL, 0, MPI_COMM_SELF, &mInterComm, errCodes);
    if (err != MPI_SUCCESS)
    {
        NMLogError(<< "Spawning processes for parallel processing failed (" << err << ")!");
        for (int n=0; n < nprocs; ++n)
        {
            NMLogError(<< "rank #" << n << ": " << errCodes[n]);
        }
        // free memory
        for (int k=0; k < argsList.size(); ++k)
        {
            delete[] argv[k];
        }
        delete[] argv;

        NMLogError(<< "Model execution aborted!");
        return;
    }

    // free memory
    for (int k=0; k < argsList.size(); ++k)
    {
        delete[] argv[k];
    }
    delete[] argv;

    // ----------------------------------------------------------------------
    // check-in with child processes if we can continue ...
    // ----------------------------------------------------------------------

    NMLogInfo(<< "Checking in with the child processes ...");

    int goAhead[nprocs];
    for (int p=0; p < nprocs; ++p)
    {
        goAhead[p] = 0;
    }

    int msgCount = 0;
    MPI_Status status;
    int flag = 0;
    while (msgCount < nprocs)
    {
        MPI_Iprobe(MPI_ANY_SOURCE, 73, mInterComm, &flag, &status);
        if (flag)
        {
            MPI_Recv(&goAhead[status.MPI_SOURCE], 1, MPI_INT, status.MPI_SOURCE, 73, mInterComm, MPI_STATUS_IGNORE);
            msgCount++;

            NMDebugAI(<< "ChildMsg: C" << status.MPI_SOURCE << " tag=73 | " << " msg=" << goAhead[status.MPI_SOURCE] << std::endl);

            // no point in waiting around ...
            if (goAhead[status.MPI_SOURCE] == 0)
            {
                break;
            }
        }
    }
    NMDebug(<< std::endl);

    // check whether we've got the goAhead
    int sum = 0;
    for (int s=0; s < nprocs; ++s)
    {
        sum += goAhead[s];
    }

    if (sum < nprocs)
    {
        NMLogError(<< "Parallel processing has failed! Please see logs of child processes!");
        return;
    }

    NMLogInfo(<< "Child processes are ready to go!");


    // --------------------------------------------------------------------------------
    // PREPARE MPI-COMMUNICATORS AND SHARED MEMORY WINDOWS
    // --------------------------------------------------------------------------------

    int interSize, interRank;
    MPI_Comm_size(mInterComm, &interSize);
    MPI_Comm_rank(mInterComm, &interRank);
    NMDebugAI(<< "InterComm: I am THE PARENT, and my rank is #"
              << interRank << " of " << interSize << std::endl);

    // merge inter comm
    NMDebugAI(<< "ParentPROC: merging interComm\n");
    MPI_Intercomm_merge(mInterComm, 0, &mMergedComm);

    // create the 'empty' shared mem window in parent proc
    MPI_Win_create(nullptr, 0, sizeof(int), MPI_INFO_NULL, mMergedComm, &mMPICompProgWin);

    // allocate int-memory for shared 'abort' status window on parent side
    NMDebugAI( << "allocating MPIAbort memory" << std::endl);
    MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, static_cast<void*>(&mMPIAbort));
    *mMPIAbort = 0;
    MPI_Win_create(static_cast<void*>(mMPIAbort), sizeof(int),
                   sizeof(int), MPI_INFO_NULL, mMergedComm, &mMPIParentAbort);

    NMDebugAI(<< "ParentPROC has reached 1st InterComm Barrier!\n");
    MPI_Barrier(mMergedComm);

    int irank, isize;
    MPI_Comm_rank(mInterComm, &irank);
    MPI_Comm_size(mInterComm, &isize);
    NMDebugAI(<< "I am THE parent, and my rank is #"
              << irank << " of " << isize << std::endl);

    // --------------------------------------------------------------------
    // RESOURCE ALLOCATION COMPONENT STATE 'BOARD'

    QStringList sortedModelComps = saveComponents;
    if (!sortedModelComps.contains("root"))
    {
        sortedModelComps << QString("root");
    }
    sortedModelComps.sort(Qt::CaseInsensitive);

    NMDebugAI(<< "parent's model - monitored comps: ")
    foreach(const QString& co, sortedModelComps)
    {
        NMDebug(<< co.toStdString() << " ");
    }
    NMDebug(<< std::endl);
    /// END DEBUG


    // this is a conceptual 3D array [ncomps][nprocs][nvals], that is
    // flattend to 1D; its index is calucated as:
    const int ncomps = sortedModelComps.size();
    const int nvals  = 2; // {event==value, progress==value+1}

    NMDebugAI( << "allocating MPICompState memory" << std::endl);
    mMPICompState = new int[ncomps * nprocs * nvals];
    for (int acomp=0; acomp < sortedModelComps.size(); ++acomp)
    {
        for (int aproc=0; aproc < nprocs; ++aproc)
        {
            mMPICompState[aproc * ncomps * nvals + acomp * nvals + 0] = 1;
            mMPICompState[aproc * ncomps * nvals + acomp * nvals + 1] = 0;
        }
    }


    NMDebugAI(<< ">>>>>> Instantiating MPIRunnable ... \n");
    int maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    int curThreadCount = QThreadPool::globalInstance()->activeThreadCount();
    NMDebugAI(<< "ParentProc: max threads: " << maxThreadCount
              << " active Threads: " << curThreadCount << std::endl);

    mbIsMPIEventLoopRunning = true;
    mbAbortionRequested = false;

    // --------------------------------------------------------------------
    // CREATE MPIRUNNABLE (thread) for monitoring CHILD processes' messages

    // note: NMMPIRunnable is auto deleting by default, so don't worry about allocated resources
    NMMPIRunnable* mpi = new NMMPIRunnable();
    emit signalMPIRunnable(mpi);
    connect(mpi, &NMMPIRunnable::signalMPILoopFinished, this, &NMModelController::slotMPIEventLoopFinished);

    mpi->setLogger(mLogger);
    mpi->setData(nprocs, sortedModelComps, lumassPath, newYamlFN, logFN, this->mLogger,
                 mMergedComm, mParentMPIComm, mMPICompProgWin, mMPIParentAbort,
                 mMPICompState, mMPIAbort);
    QThreadPool::globalInstance()->start(mpi);

    NMDebugCtx(ctx, << "done!");
}


void NMModelController::slotMPIEventLoopFinished(NMMPIRunnable* obj)
{
    mbIsMPIEventLoopRunning = false;
    MPI_Win_free(&mMPICompProgWin);
    mMPICompProgWin = MPI_WIN_NULL;
    MPI_Win_free(&mMPIParentAbort);
    mMPIParentAbort = MPI_WIN_NULL;

    mMPIAbort = 0;
    mbAbortionRequested = false;
    NMDebugAI(<< "ParentProcess freed RMA window" << std::endl);
    MPI_Comm_free(&mMergedComm);
    mMergedComm = MPI_COMM_NULL;
    NMDebugAI(<< "ParentProcess freed merged MPI_Comm" << std::endl);
    //MPI_Comm_free(&mInterComm);
    //NMDebugAI(<< "ParentProcess freed inter comm" << std::endl);

    // free RMA resources
    MPI_Free_mem(static_cast<void*>(mMPIAbort));
    mMPIAbort = nullptr;

    delete[] mMPICompState;
    mMPICompState = nullptr;
    NMDebugAI(<< "ParentProcess freed mMPICompState array" << std::endl);

    NMLogDebug(<< "ParentProc is disconnecting intercomm to children ...!\n");
    //MPI_Barrier(mInterComm);
    MPI_Comm_disconnect(&mInterComm);
    mInterComm = MPI_COMM_NULL;

    NMLogInfo(<< "MPI child processes have completed!");
    NMDebugAI(<< "MPI parent model's event cleaned up!");
}

void
NMModelController::executeMPIChildModel(const QString &compName)
{
    NMDebugCtx(ctx, << "...");

    ///DEBUG
    /// order of model components in ComponentMap.keys().sort(Qt::CaseInsensitive)
    QStringList sortedModelComps = mComponentMap.keys();
    sortedModelComps.sort(Qt::CaseInsensitive);
    if (this->mRank == 0)
    {
        NMDebugAI(<< "child's model comps: ")
        foreach(const QString& co, sortedModelComps)
        {
            NMDebug(<< co.toStdString() << " ");
        }
        NMDebug(<< std::endl);
    }

    if (sortedModelComps.size() > 0 && this->getComponent(compName) != nullptr)
    {
        // let the parent know we're ready!
        notifyParentProcess(1, 73);
    }

    MPI_Intercomm_merge(mParentMPIComm, 1, &mMergedComm);

    int mrank, msize;
    MPI_Comm_rank(this->mMergedComm, &mrank);
    MPI_Comm_size(this->mMergedComm, &msize);
    NMDebugAI(<< "MergedComm: I am a child, and my rank is #"
              << mrank << " of " << msize << std::endl);

    int childSize, childRank;
    MPI_Comm_size(MPI_COMM_WORLD, &childSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &childRank);
    NMDebugAI(<< "WorldComm: I am a child, and my rank is #"
              << childRank << " of " << childSize << std::endl);


    int prank, psize;
    MPI_Comm_rank(this->mParentMPIComm, &prank);
    MPI_Comm_size(this->mParentMPIComm, &psize);
    NMDebugAI(<< "ParentComm: I am a child, and my rank is #"
              << prank << " of " << psize << std::endl);


    // ModelComponents = {compId=0, compId=1, ..., compId=ncomps-1}
    // States per Component (=Values) = {val=0, val=1=nvals-1}
    // Number of Processes = {proc=0, proc=1, ..., proc=nprocs-1}
    // this is a conceptual 3D array [ncomps][nvals][nprocs] that is
    // flattend to 1D; its index is calucated as:
    //          mMPICompState[proc * ncomps * nvals + compId * nvals + val];
    const int ncomps = sortedModelComps.size();
    const int nprocs = childSize;
    const int nvals  = 2; // {event==value, progress==value+1}
    const int stateSize = nprocs * ncomps * nvals * sizeof(int);

    // allocate memory for component state matrix and initialize
    //mMPICompState = new int[nprocs*ncomps*nvals];
    NMDebugAI( << "allocating MPICompState memory" << std::endl);
    MPI_Alloc_mem(stateSize, MPI_INFO_NULL, static_cast<void*>(&mMPICompState));   
    // initialize component states
    for (int acomp=0; acomp < ncomps; ++acomp)
    {
        for (int aproc=0; aproc < nprocs; ++aproc)
        {
            for (int val=0; val <= 1; ++val)
            {
                mMPICompState[aproc * ncomps * nvals + acomp * nvals + val] = val;
            }
        }
    }

    MPI_Win_create(static_cast<void*>(mMPICompState),
                   ncomps*nprocs*nvals*sizeof(int),
                   sizeof(int),
                   MPI_INFO_NULL,
                   mMergedComm,
                   &mMPICompProgWin);


    // setup mMPIAbort state var on child side
    mMPIAbort = new int;
    *mMPIAbort = 0;
    MPI_Win_create(nullptr, 0, sizeof(int), MPI_INFO_NULL, mMergedComm, &mMPIParentAbort);

    NMDebugAI(<< "ChildPROC #" << prank << " has reached 1st InterComm Barrier!\n");
    MPI_Barrier(this->mMergedComm);

    executeSeqModel(compName);

    // wait for siblings to finish their models
    MPI_Barrier(MPI_COMM_WORLD);

    // it can happen that we're reaching this point
    // without any model components having actually
    // started and therefore no progress messages
    // have reached the parent controller, so
    // we just be explicit that the model is completed!
    NMModelController::MPICompProg compProg;
    compProg.compName = compName;
    compProg.event = NMModelController::NM_EVENT_MODEL_COMPLETED;
    compProg.progress = 100;
    mpiSignalProgress(compProg);

    NMDebugAI(<< "ChildPROC #" << prank << " has completed seqModel\n");
    NMLogDebug(<< "ChildPROC #" << prank << " has completed seqModel\n");

    MPI_Win_free(&mMPICompProgWin);
    MPI_Win_free(&mMPIParentAbort);
    mMPICompProgWin = MPI_WIN_NULL;
    mMPIParentAbort = MPI_WIN_NULL;

    MPI_Free_mem(static_cast<void*>(mMPICompState));
    mMPICompState = nullptr;
    delete mMPIAbort;
    mMPIAbort = nullptr;
    NMLogDebug(<< "ChildPROC #" << prank << " freed mMPICompState array\n");

    MPI_Comm_free(&mMergedComm);
    mMergedComm = MPI_COMM_NULL;
    NMLogDebug(<< "ChildPROC #" << prank << " freed merged MPI_Comm\n");

    NMLogDebug(<< "ChildPROC #" << prank << " is disconnecting itercomm to parent\n");
    //MPI_Barrier(mParentMPIComm);
    MPI_Comm_disconnect(&mParentMPIComm);

    //MPI_Comm_free(&mParentMPIComm);
    NMLogDebug(<< "ChildPROC #" << prank << " freed MPI Intercomm to parent!\n");
    std::stringstream ctd;
    ctd << "Cr" << prank << ": NMModelController::executeMPIChildModel(): done!";
    NMDebug(<< ctd.str() << std::endl);
}


void
NMModelController::executeSeqModel(const QString &compName, const QString& yamlFN)
{
    NMDebugCtx(ctx, << "...");

    NMModelComponent* comp = this->getComponent(compName);
    if (comp == 0)
    {
        NMLogError(<< ctx << ": couldn't find '"
                << compName.toStdString() << "'!");
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QString msg;
    QString userID = comp->getUserID();
    if (userID.isEmpty())
    {
        userID = comp->objectName();
    }

    if (this->mbModelIsRunning)
    {
        NMDebugAI(<< "There is already a model running! "
                << "Be patient and try later!" << endl);
        if (mLogger)
        {
            msg = QString("Model Controller: Cannot execute %1! "
                          "There is already a model running!"
                           ).arg(userID);


            mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(),
                                   NMLogger::NM_LOG_INFO,
                                   msg);
        }
        NMDebugCtx(ctx, << "done!");
        return;
    }

        //this->mRootComponent = this->identifyRootComponent();
        //QString name = this->mRootComponent->objectName();


        // we only execute 'iterable / executable' components
    //	NMIterableComponent* icomp =
    //			qobject_cast<NMIterableComponent*>(comp);
    //	if (icomp == 0)
    //	{
    //		NMLogError(<< ctx << ": component '" << compName.toStdString()
    //				<< "' is of type '" << comp->metaObject()->className()
    //				<< "' which is non-executable!");
    //		return;
    //	}


    // ================================================
    // starting the provenance

    //if (mbLogProv)
    //{
    //    QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    //    stamp = stamp.replace(":", "");
    //    stamp = stamp.replace("-", "");

    //    QString provFN = QString("%1/%2_%3.provn")
    //                     .arg(this->getSetting("Workspace").toString())
    //                     .arg(userID)
    //                     .arg(stamp);
    //    startProv(provFN, comp->objectName());
    //}

    // ================================================
    // we reset all the components
    // (and thereby delete all data buffers)
    this->resetComponent(compName);

    NMDebugAI(<< "running model on thread: "
            << this->thread()->currentThreadId() << endl);

    msg = QString("Model Controller: Executing model %1 ...").arg(userID);
    NMLogInfo(<< msg.toStdString());
    //mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(),
    //                       NMLogger::NM_LOG_INFO,
    //                       msg);

    // model management
    this->mbModelIsRunning = true;
    this->mbAbortionRequested = false;

    this->mModelStarted = QDateTime::currentDateTime();

    emit signalModelStarted();

    // we catch all exceptions thrown by ITK/OTB, rasdaman
    // and the LUMASS MFW components
    // and just report them for now; note this includes
    // the 'abortion-exception' thrown by ITK/OTB as response to
    // user-requested model abortion
    bool bUnexpectedEnd = false;
    try
    {
        comp->update(this->mComponentMap);
    }
    catch (NMMfwException& nmerr)
    {
        bUnexpectedEnd = true;
        NMLogError(<< "Model Controller: " << nmerr.what());
        //NMDebugCtx(ctx, << "done!");
    }
    catch (itk::ExceptionObject& ieo)
    {
        bUnexpectedEnd = true;
        NMLogError(<< "Model Controller: " << ieo.what());
        //NMDebugCtx(ctx, << "done!");
    }
    catch (std::exception& e)
    {
        bUnexpectedEnd = true;
        NMLogError(<< "Model Controller: " << e.what());
        //NMDebugCtx(ctx, << "done!");
    }

    // FOR MPI RUNS ONLY
    // wait for all children and then exit in an orderly fashion
    //if ( (bUnexpectedEnd || this->mbAbortionRequested)
    //     &&
    //     mParentMPIComm != MPI_COMM_NULL
    //     && mMergedComm != MPI_COMM_NULL
    //   )
    //{
    //    NMDebugAI(<< "Waiting in executeSeqModel for the others ... \n")
    //    MPI_Barrier(MPI_COMM_WORLD);
    //}

    this->mModelStopped = QDateTime::currentDateTime();
    int msec = this->mModelStarted.msecsTo(this->mModelStopped);
    int min = msec / 60000;
    double sec = (msec % 60000) / 1000.0;

    QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
    //NMDebugAI(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);
    NMMsg(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);
    NMLogInfo(<< "Model Controller: Model completed in (min:sec): " << elapsedTime.toStdString());

    this->mbModelIsRunning = false;
    this->mbAbortionRequested = false;
    emit signalIsControllerBusy(false);

    // ================================================
    // end provenance
    if (mbLogProv)
    {
        endProv();
    }

    // to be on the safe side, we reset the execution stack and
    // notify all listeners, that those components are no longer
    // running
    this->resetExecutionStack();
    //this->resetComponent(compName);

    // remove objects scheduled for deletion
    foreach(const QString& name, this->mToBeDeleted)
    {
        this->removeComponent(name);
    }
    this->mToBeDeleted.clear();

    //emit signalModelStopped();

    NMDebugCtx(ctx, << "done!");
}

void
NMModelController::getSubComponents(NMIterableComponent *ic, QStringList &subComps)
{
    if (ic == nullptr)
    {
        return;
    }

    NMModelComponentIterator it = ic->getComponentIterator();
    while (!it.isAtEnd())
    {
        subComps << (*it)->objectName();
        NMIterableComponent* sic = qobject_cast<NMIterableComponent*>(*it);
        if (sic != nullptr)
        {
            getSubComponents(sic, subComps);
        }

        ++it;
    }
}

void
NMModelController::resetComponent(const QString compName)
{
//	NMDebugCtx(ctx, << "...");

    if (this->isModelRunning())
    {
        NMLogError(<< "ModelController: Cannot reset '"
                   << compName.toStdString() << "' while a model is running!");
        return;
    }

    NMModelComponent* comp = this->getComponent(compName);
    if (comp == 0)
    {
        NMLogError(<< ctx << ": couldn't find '"
                << compName.toStdString() << "'!");
        return;
    }

    NMLogInfo(<< "ModelController: Resetting component '" << compName.toStdString() << "'");

//	NMDebugAI(<< "resetting component '" << compName.toStdString()
//			  << "'" << endl);

    comp->reset();
//	NMDebugCtx(ctx, << "done!");
}

QList<NMModelComponent*>
NMModelController::getComponents(const QString &userId)
{
    QList<NMModelComponent*> ret;

    QMultiMap<QString, QString>::const_iterator it =
            mUserIdMap.cbegin();
    while (it != mUserIdMap.end())
    {
        if (it.key().compare(userId) == 0)
        {
            NMModelComponent* comp = this->getComponent(it.value());
            if (comp)
            {
                ret << comp;
            }
        }
        ++it;
    }

    return ret;
}

QString
NMModelController::addComponent(NMModelComponent* comp,
        NMModelComponent* host)
{
    //	NMDebugCtx(ctx, << "...");

    if (comp == nullptr)
    {
        NMLogError(<< ctx << ": cannot add NULL component to model!");
        return "failed";
    }

    NMIterableComponent* ihost = 0;
    if (host != nullptr)
    {
        ihost = qobject_cast<NMIterableComponent*>(host);
    }

    if (this->mComponentMap.values().contains(comp))
    {
        NMLogError(<< ctx << ": model component already present in repository!");
        return "failed";
    }

    QRegExp re("[0-9]{0,4}$");
    QString cname = comp->objectName();
    QString tname = cname;
    QString numstr;
    unsigned long cnt = 1;
    bool bok;

    while (this->mComponentMap.keys().contains(tname))
    {
        if (re.indexIn(tname) > 0)
        {
            numstr = re.capturedTexts().at(0);
            cnt = numstr.toLong(&bok);
            if (bok)
            {
                if (cname.endsWith(numstr))
                    cname = cname.left(cname.size() - numstr.size());
                ++cnt;
            }
            else
                cnt = 1;
        }

        tname = QString(tr("%1%2")).arg(cname).arg(cnt);
    }

    comp->setParent(nullptr);
    comp->moveToThread(this->thread());
    comp->setObjectName(tname);
    comp->setParent(this);
    comp->setLogger(this->mLogger);
    comp->setModelController(this);

    this->mComponentMap.insert(tname, comp);
    this->mUserIdMap.insert(comp->getUserID(), tname);
    connect(comp, SIGNAL(ComponentUserIDChanged(QString, QString)),
            this, SLOT(setUserId(QString, QString)));

    // check, whether we've go a valid host
    if (ihost != 0)
    {
        if (this->mComponentMap.keys().contains(host->objectName()))
        {
            ihost->addModelComponent(comp);
        }
    }

    return tname;
}

void NMModelController::setUserId(const QString oldId, const QString newId)
{
    NMModelComponent* comp = qobject_cast<NMModelComponent*>(this->sender());
    if (comp == 0)
    {
        return;
    }

    int nrem = 0;
    if (!oldId.isNull() && mUserIdMap.size() > 0 && mUserIdMap.contains(oldId, comp->objectName()))
    {
        nrem = mUserIdMap.remove(oldId, comp->objectName());
    }

    //    if (nrem == 0)
    //    {
    //        NMLogWarn(<< "Failed to unregister old UserID' "
    //                  << oldId.toStdString() << "' for '"
    //                  << comp->objectName().toStdString() << "'!");
    //    }
    mUserIdMap.insert(newId, comp->objectName());
}

QStringList
NMModelController::getUserIDs()
{
    return mUserIdMap.keys();
}

bool
NMModelController::contains(const QString& compName)
{
    bool ret;
    if (    this->mComponentMap.size() > 0
         && this->mComponentMap.keys().contains(compName)
       )
    {
        ret = true;
    }
    else
    {
        ret = false;
    }

    return ret;
}

QString NMModelController::getComponentNameFromInputSpec(const QString& inputSpec)
{
    if (inputSpec.isEmpty())
        return QString();

    QStringList specList = inputSpec.split(":");
    return specList.at(0);
}

bool NMModelController::removeComponent(const QString& name)
{
    NMModelComponent* comp = this->getComponent(name);
    if (comp == 0)
    {
        NMDebugAI(<< "component '" << name.toStdString() << "' is not controlled by this "
                << "controller!");
        return false;
    }

    NMIterableComponent* host = comp->getHostComponent();
    if (host != 0)
    {
        host->removeModelComponent(name);
    }
    else // name must be 'root' in this case
    {
        NMLogError(<< "You cannot remove the root model componet!");
        return false;
    }


    QString oldId = comp->getUserID();
    int nrem = 0;
    if (!oldId.isNull() && mUserIdMap.size() > 0 && mUserIdMap.contains(oldId, comp->objectName()))
    {
        nrem = mUserIdMap.remove(oldId, comp->objectName());
    }

//    if (nrem == 0)
//    {
//        NMLogWarn(<< "Failed removing '" << comp->objectName().toStdString()
//                  << "' from the UserID map!");
//    }

    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
    if (ic != 0)
        ic->destroySubComponents(this->mComponentMap);
    this->mComponentMap.remove(name);

    //std::map<std::string, py::object>::iterator pyit = lupy::ctrlPyObjects.find(name.toStdString());
    //if (pyit != lupy::ctrlPyObjects.end())
    //{
    //    pyit->second = py::none();
    //    lupy::ctrlPyObjects.erase(name.toStdString());
    //}

    //std::map<std::string, py::module_>::iterator pymodIt = lupy::ctrlPyModules.find(name.toStdString());
    //if (pymodIt != lupy::ctrlPyModules.end())
    //{
    //    pymodIt->second = py::none();
    //    lupy::ctrlPyModules.erase(name.toStdString());
    //}

    this->mPythonComponents.removeOne(name);

    delete comp;

    emit componentRemoved(name);
    return true;
}


NMModelComponent*
NMModelController::getComponent(const QString& name)
{
    NMModelComponent* comp = 0;
    QMap<QString, NMModelComponent*>::iterator cit = this->mComponentMap.find(name);
    if (cit != this->mComponentMap.end())
        comp = cit.value();

    return comp;
}

int
NMModelController::getRank(const QString& comp)
{
    MPI_Comm comm = this->getNextUpstrMPIComm(comp);

    int rank = 0;
    if (comm != MPI_COMM_NULL)
    {
        MPI_Comm_rank(comm, &rank);
    }
    return rank;
}

int
NMModelController::getNumProcs(const QString& comp)
{
    MPI_Comm comm = this->getNextUpstrMPIComm(comp);

    int nprocs = 1;
    if (comm != MPI_COMM_NULL)
    {
        MPI_Comm_size(comm, &nprocs);
    }
    return nprocs;
}

MPI_Comm NMModelController::getNextUpstrMPIComm(const QString &compName)
{
    NMDebugCtx(ctx, << "...");
    MPI_Comm nextComm = MPI_COMM_NULL;

    //// if we're (g)root ( ;-) ) create the top most comm...
    //MPI_Comm rootComm = MPI_COMM_NULL;
    //if (this->objectName().compare(QStringLiteral("root")) == 0)
    //{
    //    MPI_Comm_dup(MPI_COMM_WORLD, &rootComm);
    //    controller->registerParallelGroup(QStringLiteral("root"), rootComm);
    //}


    //auto aiter = mAlphaComps.find("root");
    //if (aiter == mAlphaComps.end())
    //{
    //    nextComm = MPI_COMM_NULL;
    //    NMLogDebug(<< "root component has no registered MPI_COMM! "
    //               << "Something went horribly wrong!");
    //    return nextComm;
    //}
    //else
    //{
    //    nextComm = aiter.value();
    //}

    if (this->getNumProcs() == 1)
    {
        NMDebugAI(<< "Controller says, we've got just 1 proc! :-( "
                  << " No private conversation possible ;-) ..." << endl);
        NMDebugCtx(ctx, << "done!");
        return nextComm;
    }

    NMIterableComponent* aggrComp = qobject_cast<NMIterableComponent*>(this->getComponent(compName));

    //if (aggrComp->getProcess() != nullptr)
    //{
    //    aggrComp = qobject_cast<NMIterableComponent*>(aggrComp->getHostComponent());
    //}

    if (aggrComp == nullptr)
    {
        NMDebugAI(<< "'" << compName.toStdString() << "' does not reference a registered model component!"<< endl);
        NMDebugCtx(ctx, << "done!");
        return nextComm;
    }

    // =========================================
    // DEBUG DEBUG DEBUG
    // =========================================
    MPI_Comm _tmpComm = MPI_COMM_NULL;
    char comm_name[MPI_MAX_OBJECT_NAME];
    int  cn_len;
    NMDebugAI(<< "MPI-Debug: Registered comms ... " << endl);
    auto iter = mAlphaComps.cbegin();
    while (iter != mAlphaComps.cend())
    {
        _tmpComm = iter.value();
        if (_tmpComm != MPI_COMM_NULL)
        {
            MPI_Comm_get_name(_tmpComm, comm_name, &cn_len);
        }
        else
        {
            ::sprintf(comm_name, "MPI_COMM_NULL");
        }
        NMDebugAI(<< "  ... '" << iter.key().toStdString() << "' : #" << comm_name << endl);
        ++iter;
    }
    // =========================================
    // DEBUG DEBUG DEBUG
    // =========================================
    QMap<QString, MPI_Comm>::iterator citer = mAlphaComps.find(aggrComp->objectName());
    if (citer != mAlphaComps.end())
    {
        _tmpComm = citer.value();
        if (_tmpComm != MPI_COMM_NULL)
        {
            MPI_Comm_get_name(_tmpComm, comm_name, &cn_len);
        }
        else
        {
            ::sprintf(comm_name, "MPI_COMM_NULL");
        }
        NMDebugAI(<< "'" << compName.toStdString() << "' is managed by comm #" << comm_name
                  << " registered with '" << citer.key().toStdString() << "'" << endl);
        NMDebugCtx(ctx, << "done!");
        return citer.value();
    }

    while ((aggrComp = aggrComp->getHostComponent()) != nullptr)
    {
        citer = mAlphaComps.find(aggrComp->objectName());
        if (citer != mAlphaComps.end())
        {
            _tmpComm = citer.value();
            if (_tmpComm != MPI_COMM_NULL)
            {
                MPI_Comm_get_name(_tmpComm, comm_name, &cn_len);
            }
            else
            {
                ::sprintf(comm_name, "MPI_COMM_NULL");
            }
            NMDebugAI(<< "'" << compName.toStdString() << "' is managed by comm #" << comm_name
                      << " registered with '" << citer.key().toStdString() << "'" << endl);
            NMDebugCtx(ctx, << "done!");
            return citer.value();
        }
    }


    // if haven't found a registered component yet, but do have more than
    // one process available to execute the model, the user is just
    // executing a subcomponent of the whole model and 'compName' is it and we
    // therefore need to register it NOW to make use of the processes
    // available!
    this->registerParallelGroup(compName, MPI_COMM_WORLD);
    nextComm = MPI_COMM_WORLD;

    NMDebugCtx(ctx, << "done!");
    return nextComm;
}


void
NMModelController::registerParallelGroup(const QString &compName,
        MPI_Comm comm)
{
    if (comm != MPI_COMM_NULL)
    {
        mAlphaComps[compName] = comm;
    }
}

void
NMModelController::deregisterParallelGroup(const QString &compName)
{
    auto it = mAlphaComps.find(compName);
    if (it != mAlphaComps.end())
    {
        mAlphaComps.erase(it);
    }
}

QStringList
NMModelController::getPropertyList(const QObject* obj)
{
    QStringList propList;
    const QMetaObject* meta = obj->metaObject();
    for (int i=0; i < meta->propertyCount(); ++i)
    {
        QMetaProperty prop = meta->property(i);
        propList << prop.name();
    }

    return propList;
}

otb::AttributeTable::Pointer
NMModelController::getComponentTable(const NMModelComponent* comp)
{
    NMModelComponent* mc = const_cast<NMModelComponent*>(comp);
    NMDataComponent* dc = qobject_cast<NMDataComponent*>(mc);
    const QString yamlConfig = "";
    otb::AttributeTable::Pointer tab;
    // data component
    if (dc)
    {
        // update the data component
        NMItkDataObjectWrapper* dw = dc->getOutput(0).data();
        if (dw != nullptr)
        {
            tab = dw->getOTBTab();
        }
        else
        {
            this->executeModel(dc->objectName(), yamlConfig);
            NMItkDataObjectWrapper* dwupd = dc->getOutput(0).data();
            if (dwupd != nullptr)
            {
                tab = dwupd->getOTBTab();
            }
        }
    }
    // looking for a reader (table or image)
    else
    {
        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(mc);
        if (ic && ic->getProcess())
        {
            NMImageReader* ir = qobject_cast<NMImageReader*>(ic->getProcess());
            NMTableReader* tr = qobject_cast<NMTableReader*>(ic->getProcess());
            if (ir)
            {
                try
                {
                    ir->instantiateObject();
                    tab = ir->getRasterAttributeTable(1);
                }
                catch (NMMfwException& mex)
                {
                    NMLogError(<< "Model Controller: " << mex.what());
                    return tab;
                }
            }
            else if (tr)
            {
                this->executeModel(ic->objectName(), yamlConfig);
                NMItkDataObjectWrapper* dw = ic->getOutput(0).data();
                if (dw != nullptr)
                {
                    tab = dw->getOTBTab();
                }
            }
        }
    }
    return tab;
}


QStringList
NMModelController::getDataComponentProperties(const NMModelComponent* comp,
                                       DataComponentPropertyType type)
{
    QStringList props;
    if (comp == 0)
    {
        NMLogDebug(<< "Failed fetching data component properties for NULL object!");
        return props;
    }

    if (type != NM_DATAPROP_COLUMNS)
    {
        NMLogDebug(<< "NMModelController::getDataComponentProperties() - "
                   << "unsupported property type!");
        return props;
    }

    otb::AttributeTable::Pointer tab = this->getComponentTable(comp);

    if (tab.IsNotNull())
    {
        for (int c=0; c < tab->GetNumCols(); ++c)
        {
            props << QString(tab->GetColumnName(c).c_str());
        }
    }

    return props;
}

QStringList
NMModelController::getDataComponentProperties(const QString& compName,
                                       DataComponentPropertyType type)
{
    return getDataComponentProperties(this->getComponent(compName), type);
}


QStringList
NMModelController::getNextParamExpr(const QString &expr)
{
    QStringList innerExpr;

    QList<int> startPos;

    for (int i=0; i < expr.size(); ++i)
    {
        if (    expr.at(i) == '$'
            &&  (i+1 < expr.size() && expr.at(i+1) == '[')
           )
        {
            startPos << i;
        }
        else if (    expr.at(i) == ']'
                 &&  (i+1 < expr.size() && expr.at(i+1) == '$')
                )
        {
            if (startPos.size() > 0)
            {
                int start = startPos.last();
                int len = i - start + 2;
                QStringRef sub = expr.midRef(start, len);
                innerExpr << sub.toString();
                startPos.clear();
            }
        }
    }

    return innerExpr;
}

bool
NMModelController::registerPythonRequest(const QString &compName)
{
    bool ret = true; // whether or not the module / object needs to be reloaded
    if (!mPythonComponents.contains(compName))
    {
        mPythonComponents << compName;
        ret = false;
    }

    return ret;
}

QString
NMModelController::evalFunc(const QString& funcName, const QStringList& args,
                            const NMIterableComponent* host)
{
    QString ret;
    if (funcName.compare("cond") == 0)
    {
        if (args.size() < 3)
        {
            ret = QString("ERROR: Not enough arguments!");
        }
        else
        {
            ret = args.at(2);

            QString exp = args.at(0);

            bool isnum;
            int ival = exp.toInt(&isnum, 10);

            if (isnum || exp.compare("0.0") == 0 || exp.compare("0") == 0)
            {
                if (ival != 0)
                {
                    ret = args.at(1);
                }
            }
            else
            {
                QString exprtrim = exp.trimmed();
                if (    exprtrim.compare("false", Qt::CaseInsensitive) == 0
                     || exprtrim.compare("no", Qt::CaseInsensitive) == 0
                   )
                {
                    ret = args.at(2);
                }
                else if (   exprtrim.compare("true", Qt::CaseInsensitive) == 0
                         || exprtrim.compare("yes", Qt::CaseInsensitive) == 0
                        )
                {
                    ret = args.at(1);
                }
                // interpret the string as mu-parser expression
                // if the expression value is not zero (i.e. exprVal != 0)
                // we return the 'false' argument (i.e. the third function argument)
                else
                {
                    double resVal;
                    QString resStr = this->evalMuParserExpression(nullptr, exp, &resVal);
                    if (!resStr.startsWith("ERROR", Qt::CaseSensitive) && resVal != 0)
                    {
                        if (resVal != 0)
                        {
                            ret = args.at(1);
                        }
                    }
                    else
                    {
                        ret = resStr;
                    }
                }
            }
        }
    }
    else if (funcName.compare("hasAttribute") == 0)
    {
        ret = "0";

        if (args.size() >= 2 && host != nullptr)
        {
            NMModelComponent* comp = this->getComponent(args.at(0));
            if (comp == nullptr)
            {
                NMIterableComponent* ic = const_cast<NMIterableComponent*>(host);
                if (ic != nullptr)
                {
                    comp = ic->findUpstreamComponentByUserId(args.at(0));
                }
            }

            if (comp != nullptr)
            {
                QStringList cols = this->getDataComponentProperties(comp);
                if (cols.contains(args.at(1), Qt::CaseInsensitive))
                {
                    ret = "1";
                }
            }
        }
    }
    else if (funcName.compare("isFile") == 0)
    {
        if (args.size() < 1)
        {
            ret = "0";
        }
        else
        {
            QFileInfo fifo(args.at(0));
            if (fifo.isFile())
            {
                ret = "1";
            }
            else
            {
                ret = "0";
            }
        }
    }
    else if (funcName.compare("isDir") == 0)
    {
        if (args.size() < 1)
        {
            ret = "0";
        }
        else
        {
            QFileInfo fifo(args.at(0));
            if (fifo.isDir())
            {
                ret = "1";
            }
            else
            {
                ret = "0";
            }
        }
    }
    else if (funcName.compare("fileBaseName") == 0)
    {
        if (args.size() < 1)
        {
            ret = QString("");
        }
        else
        {
            QFileInfo fifo(args.at(0));
            ret = fifo.baseName();
        }
    }
    else if (funcName.compare("fileCompleteBaseName") == 0)
    {
        if (args.size() < 1)
        {
            ret = QString("");
        }
        else
        {
            QFileInfo fifo(args.at(0));
            ret = fifo.completeBaseName();
        }
    }
    else if (funcName.compare("filePath") == 0)
    {
        if (args.size() < 1)
        {
            ret = QString("");
        }
        else
        {
            QFileInfo fifo(args.at(0));
            ret = fifo.absolutePath();
        }
    }
    else if (funcName.compare("fileSuffix") == 0)
    {
        if (args.size() < 1)
        {
            ret = QString("");
        }
        else
        {
            QFileInfo fifo(args.at(0));
            ret = fifo.suffix();
        }
    }
    else if (funcName.compare("fileCompleteSuffix") == 0)
    {
        if (args.size() < 1)
        {
            ret = QString("");
        }
        else
        {
            QFileInfo fifo(args.at(0));
            ret = fifo.completeSuffix();
        }
    }
    else if (funcName.compare("fileOneIsNewer") == 0)
    {
        if (args.size() < 2)
        {
            ret = "0";
        }
        else
        {
            QFileInfo fiOne(args.at(0));
            QFileInfo fiTwo(args.at(1));

            if (fiOne.isFile() && fiTwo.isFile())
            {
                if (fiOne.lastModified() > fiTwo.lastModified())
                {
                    ret = "1";
                }
                else
                {
                    ret = "0";
                }
            }
            else if (fiOne.isFile() && !fiTwo.isFile())
            {
                ret = "1";
            }
            else
            {
                ret = "0";
            }
        }
    }
    else if (funcName.compare("strReplace") == 0)
    {
        if (args.size() < 3)
        {
            if (args.size() > 0)
            {
                ret = args.at(0);
            }
            else
            {
                ret = QString("ERROR: Not enough arguments!");
            }
        }
        else
        {
            QString in = args.at(0);
            ret = in.replace(args.at(1), args.at(2));
        }
    }
    else if (funcName.compare("strIsEmpty") == 0)
    {
        if (args.size() < 1 || args.at(0).isEmpty())
        {
            ret = "1";
        }
        else
        {
            ret = "0";
        }
    }
    else if (funcName.compare("strLength") == 0)
    {
        if (args.size() >= 1)
        {
            ret = QString("%1").arg(args.at(0).size());
        }
        else
        {
            ret = "0";
        }
    }
    else if (funcName.compare("strListItem") == 0)
    {
        // strListItem("string", '<sep char>', index);
        if (args.size() >= 1)
        {
            if (args.at(0).isEmpty())
            {
                ret = "";
            }

            QString thestr = args.at(0).trimmed();
            QStringList stritems;
            if (args.size() == 1)
            {
                ret = thestr;
            }
            else if (args.size() >= 2)
            {
                QString sep = args.at(1).trimmed();
                if (sep.isEmpty())
                {
                    sep = args.at(1);
                }
                stritems = thestr.split(sep, Qt::SkipEmptyParts);
                if (stritems.size() > 0)
                {
                    if (args.size() == 3)
                    {
                        bool bok;
                        int idx = args.at(2).toInt(&bok);
                        if (bok)
                        {
                            if (idx <= 0)
                            {
                                ret = stritems.first();
                            }
                            else if (idx >= stritems.size()-1)
                            {
                                ret = stritems.last();
                            }
                            else
                            {
                                ret = stritems.at(idx);
                            }
                        }
                        else
                        {
                            return QString("ERROR: strListItem() - argument 3 (<idx>) is invalid!");
                        }
                    }
                    else
                    {
                        ret = stritems.at(0);
                    }
                }
                else
                {
                    ret = thestr;
                }
            }
        }
        else
        {
            return QString("ERROR: Not enough arguments!");
        }
    }
    else if (funcName.compare("strListLength") == 0)
    {
        // strListItem("string", '<sep char>');
        if (args.size() >= 1)
        {
            if (args.at(0).isEmpty())
            {
                ret = "0";
            }

            if (args.size() == 1)
            {
                ret = "1";
            }
            else if (args.size() == 2)
            {
                QString thestr = args.at(0).trimmed();
                QString sep = args.at(1).trimmed();
                if (sep.isEmpty())
                {
                    sep = args.at(1);
                }
                QStringList stritems  = thestr.split(sep, Qt::SkipEmptyParts);
                ret = QString("%1").arg(stritems.length());
            }
            else
            {
                return QString("ERROR: Too many arguments!");
            }
        }
        else
        {
            return QString("ERROR: Not enough arguments!");
        }
    }
    else if (funcName.compare("strSubstring") == 0)
    {
        if (args.size() < 3)
        {
            if (args.size() > 0)
            {
                return args.at(0);
            }

            return QString("ERROR: Not enough arguments!");
        }

        bool bok;
        int pos = QVariant(args.at(1)).toInt(&bok);
        if (!bok)
        {
            return QString("ERROR: Argument #2 is not an integer number!");
        }

        int cnt = QVariant(args.at(2)).toInt(&bok);
        if (!bok)
        {
            return QString("ERROR: Argument #3 is not an integer number!");
        }

        ret = args.at(0).mid(pos, cnt);
    }
    else if (funcName.compare("strCompare") == 0)
    {
        if (args.size() < 2)
        {
            return QString("ERROR: Need two strings to compare!");
        }

        Qt::CaseSensitivity csens = Qt::CaseInsensitive;

        // if the user provided a thrid parameter, we
        // try to make sense of it
        if (args.size() == 3)
        {
            bool bOK;
            int sens = QVariant(args.at(2)).toInt(&bOK);


            if (bOK && sens)
            {
                csens = Qt::CaseSensitive;
            }
        }

        int ret = args.at(0).compare(args.at(1), csens);
        return QString("%1").arg(ret);

    }
    else if (funcName.compare("strContains") == 0)
    {
        if (args.size() < 2 || args.at(1).isEmpty())
        {
            return QString("ERROR: Need a string to look for!");
        }

        Qt::CaseSensitivity csens = Qt::CaseInsensitive;

        // if the user provided a thrid parameter, we
        // try to make sense of it
        if (args.size() == 3)
        {
            bool bOK;
            int sens = QVariant(args.at(2)).toInt(&bOK);


            if (bOK && sens)
            {
                csens = Qt::CaseSensitive;
            }
        }

        bool bcontains = args.at(0).contains(args.at(1), csens);
        return QString("%1").arg(bcontains ? 1 : 0);
    }
    else
    {
        ret = QString("ERROR: Unknown function '%1'").arg(funcName);
    }

    return ret;
}

QStringList
NMModelController::parseQuotedArguments(const QString& args, const QChar &sep)
{
    QStringList retList;

    if (args.isEmpty())
    {
        return retList;
    }

    QList<int> pos;
    // lastComma indicates the last
    // found separator position
    int lastComma = -1;

    for (int i=0; i < args.size(); ++i)
    {
        if (args[i] == '\"')
        {
            pos << i;
        }
        else if (args[i] == sep && pos.size() > 0 && (pos.size() % 2) == 0)
        {
            retList << args.mid(pos.first()+1, pos.last()-pos.first()-1);
            pos.clear();
            lastComma = i;
        }
        // this detects non quoted arguments
        else if (i > 0 && args[i] == sep && pos.size() == 0)
        {
            retList << args.mid(lastComma+1, i-(lastComma+1));
            lastComma = i;
        }
    }

    // don't forget the last quoted arguments
    // (which isn't followed by a comma)
    if (lastComma > 0)
    {
        if (pos.size() > 0 && (pos.size() % 2) == 0)
        {
            retList << args.mid(pos.first()+1, pos.last()-pos.first()-1);
        }
        else
        {
             retList << args.mid(lastComma+1, args.size()-(lastComma+1));
        }
    }
    // support for single argument functions
    else
    {
        if (args.startsWith('\"') && args.endsWith('\"'))
        {
            retList << args.mid(1, args.size()-2);
        }
        else
        {
            retList << args;
        }
    }

    return retList;
}

QString
NMModelController::processStringParameter(const QObject* obj, const QString& str)
{
    QString nested = str;

    // count the number of ParameterExpressions to be evaluated
    // to avoid endless loops
    QStringList innerExp;
    const int maxcount = 15000;

    nested = str;
    innerExp = this->getNextParamExpr(nested);
    int numExp = innerExp.size();

    int count = 0;
    while (numExp > 0)
    {
        if (count >= maxcount)
        {
            NMLogError(<< "The maximum allowed number of "
                       << "expressions per parameter "
                       << "has been exceeded. Note: LUMASS "
                       << "currently only supports a "
                          "recursion depth of 10 for "
                       << "parameter expressions!")

            return QString::fromLatin1("ERROR: Maximum recursion depth exceeded!");
        }

        for (int inner=0; inner < numExp; ++inner)
        {
            QString tStr = innerExp.at(inner);
            tStr = tStr.simplified();
            //tStr.replace(QString(" "), QString(""));

//            QRegularExpression rexexp("((?<open>\\$\\[)*"
//                                         "(?(<open>)|\\b)"
//                                         "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
//                                         "(?<sep1>(?(<open>):|(?>__)))*"
//                                         "(?<arith>(?(<sep1>)|([ ]*(?<opr>[+\\-])?[ ]*(?<sum>[\\d]+))))*"
//                                         "(?<prop>(?(?<!math:|func:)(?(<sep1>)\\g<comp>)|([a-zA-Z0-9_ \\/\\(\\)&%\\|\\>\\!\\=\\<\\-\\+\\*\\^\\?:;.,'\"])*))*"
//                                         "(?<sep2>(?(<prop>)(?(<open>)):))*"
//                                         "(?(<sep2>)(?<idx>[0-9]+)*|([ ]*(?<opr2>[+\\-]+)[ ]*(?<sum2>[\\d]+))*))(?>\\]\\$)*");

            QRegularExpression rexexp("((?<open>\\$\\[)*"
                                            "(?(<open>)|\\b)"
                                            "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
                                            "(?<sep1>(?(<open>):|(?>__)))*"
                                            "(?<arith>(?(<sep1>)|([ ]*(?<opr>[+\\-])?[ ]*(?<sum>[\\d]+))))*"
                                            "(?<prop>(?(?<!math:|func:)(?(<sep1>)\\g<comp>)|([a-zA-Z0-9_ \\\\\\/\\(\\)&%\\|\\>\\!\\=\\<\\-\\+\\*\\^\\?:;.,'\"])*))*"
                                            "(?<sep2>(?(<prop>)(?(<open>):)))*"
                                            "(?(<sep2>)((?<numidx>[0-9]+)(?:\\]\\$|\\$\\[)|(?<stridx>[^\\r\\n\\$\\[\\]]*))|([ ]*(?<opr2>[+\\\\-]+)[ ]*(?<sum2>[\\d]+))*))(?>\\]\\$)*");

            int pos = 0;
            bool bRecognisedExpression = false;
            //while((pos = rex.indexIn(tStr, pos)) != -1)
            QRegularExpressionMatchIterator mit = rexexp.globalMatch(tStr);
            //while (mit.hasNext())
            // we ever only expect to have one match here!
            if (mit.hasNext())
            {
                QRegularExpressionMatch match = mit.next();
                QString wholeText = match.captured(0);

                QStringList m;
                m << match.capturedRef("comp").toString(); // 0
                m << match.capturedRef("prop").toString(); // 1

                QStringRef numidx = match.capturedRef("numidx");
                QStringRef stridx = match.capturedRef("stridx");

                if (!numidx.isEmpty())                     // 2
                {
                    m << numidx.toString();
                }
                else
                {
                    m << stridx.toString();
                }

                //m << match.capturedRef("idx").toString();  // 2

                bool sep1 = match.capturedRef("sep1").toString().isEmpty() ? false : true;
                bool sep2 = match.capturedRef("sep2").toString().isEmpty() ? false : true;

                // in case we've got arithmetics right after the component name
                m << match.capturedRef("opr").toString();  // 3
                m << match.capturedRef("sum").toString();  // 4

                // in case the arithmetic expression is specified after the property name
                m << match.capturedRef("opr2").toString(); // 5
                m << match.capturedRef("sum2").toString(); // 6


                NMDebugAI(<< m.join(" | ").toStdString() << std::endl);
                //NMDebugAI(<< "---------------" << std::endl);
                //pos += rex.matchedLength();

                // --------------------------------------------------------------------------
                // retrieve model component

                NMProcess* procObj = qobject_cast<NMProcess*>(const_cast<QObject*>(obj));
                NMIterableComponent* host = nullptr;

                if (procObj == 0)
                {
                    host = qobject_cast<NMIterableComponent*>(const_cast<QObject*>(obj));
                }
                else if (obj != nullptr)
                {
                    host = qobject_cast<NMIterableComponent*>(obj->parent());
                }

                // eval a math parser expression
                if (m.at(0).compare(QString("math"), Qt::CaseInsensitive) == 0)
                {
                    double resVal;
                    QString resStr = this->evalMuParserExpression(obj, m.at(1), &resVal);
                    if (resStr.startsWith("ERROR", Qt::CaseSensitive))
                    {
                        return resStr;
                    }

                    tStr = resStr;


//                    otb::MultiParser::Pointer parser = otb::MultiParser::New();
//                    try
//                    {
//                        parser->SetExpr(m.at(1).toStdString());
//                        otb::MultiParser::ValueType res = parser->Eval();
//                        tStr = QString("%1").arg(static_cast<double>(res), 0, 'g', 15);
//                    }
//                    catch (mu::ParserError& evalerr)
//                    {
//                        std::stringstream errmsg;
//                        errmsg << "ERROR:" << obj->objectName().toStdString() << std::endl
//                               << "Math expression evaluation: ";
//                        errmsg << std::endl
//                               << "Message:    " << evalerr.GetMsg() << std::endl
//                               << "Formula:    " << evalerr.GetExpr() << std::endl
//                               << "Token:      " << evalerr.GetToken() << std::endl
//                               << "Position:   " << evalerr.GetPos() << std::endl << std::endl;


//                        //NMLogError(<< errmsg.str());
//                        return QString(errmsg.str().c_str());
//                    }
                }
                else if (m.at(0).compare(QString("LUMASS"), Qt::CaseInsensitive) == 0)
                {
                    if (mSettings.find(m.at(1)) != mSettings.end())
                    {
                       tStr = tStr.replace(wholeText, QString("%1").arg(mSettings[m.at(1)].toString()));
                    }
                    else
                    {
                        tStr = tStr.replace(wholeText, QStringLiteral(""));
                        std::stringstream errstr;
                        errstr << "WARNING: Couldn't find LUMASS setting '"
                               << m.at(1).toStdString() << "' - used an empty string instead!";
                        NMLogDebug(<< errstr.str());
                        //return QString(errstr.str().c_str());
                    }
                }
                else if (m.at(0).compare(QString("func"), Qt::CaseInsensitive) == 0)
                {
                    QString funcexpr = m.at(1);
                    int posOpen = funcexpr.indexOf('(');
                    int posClose = funcexpr.lastIndexOf(')');
                    QString funcName = funcexpr.left(posOpen);
                    QString args = funcexpr.mid(posOpen+1, posClose-posOpen-1).trimmed();
                    QStringList argList;
                    if (args.contains('\"'))
                    {
                        argList = this->parseQuotedArguments(args);
                    }
                    else
                    {
                        foreach(QString s, args.split(',', Qt::SkipEmptyParts))
                        {
                            argList << s.trimmed();
                        }
                    }

                    QString ret = this->evalFunc(funcName, argList, host);
                    if (ret.startsWith("ERROR:"))
                    {
                        std::stringstream msg;
                        msg << ret.right(ret.size()-6).toStdString();
                        //NMLogError(<< msg.str());
                        return QString(msg.str().c_str());
                    }

                    tStr = tStr.replace(wholeText, ret);
                }
                else
                {
                    NMModelComponent* mc = this->getComponent(m.at(0));

                    // if the component is specified by userId, we've got to dig a little deeper
                    if (mc == 0)
                    {
                        if (host)
                        {
                            mc = host->findUpstreamComponentByUserId(m.at(0));
                        }
                        else
                        {
                            NMLogWarn(<< obj->objectName().toStdString()<< ": Process not embedded in model component!");
                        }
                    }

                    // -----------------------------------------------------------------------------
                    // retrieve model parameter and process, if applicable
                    if (mc)
                    {
                        NMIterableComponent* ic = 0;
                        QString paramSpec;
                        QVariant modelParam;

                        // we've got only the component defined
                        if (!sep1)
                        {
                            ic = qobject_cast<NMIterableComponent*>(mc);
                            if (ic)
                            {
                                modelParam = QVariant::fromValue(ic->getIterationStep());
                            }
                            else
                            {
                                modelParam = QString::fromLatin1("ERROR: Invalid iteration-step expression!");
                            }
                        }
                        // we've got a property defined
                        else if (sep1 && m.at(1).isEmpty())
                        {
                            modelParam = QString::fromLatin1("ERROR: malformed parameter expresssion: missing property!");
                        }
                        else if (!sep2)
                        {
                            int pstep = 1;
                            if (host->getHostComponent())
                            {
                                pstep = host->getHostComponent()->getIterationStep();
                            }
                            else if (ic->getHostComponent())
                            {
                                pstep = ic->getHostComponent()->getIterationStep();
                            }
                            paramSpec = QString("%1:%2").arg(m.at(1)).arg(pstep);
                        }
                        else if (sep2 && !m.at(2).isEmpty())
                        {
                            paramSpec = QString("%1:%2").arg(m.at(1)).arg(m.at(2));
                        }
                        else
                        {
                            modelParam = QString::fromLatin1("ERROR: malformed parameter expression: missing index!");
                        }


                        // if we get an invalid parameter, we stop processing and
                        // return the error message
                        if (!paramSpec.isEmpty())
                        {
                            modelParam = mc->getModelParameter(paramSpec);
                        }

                        if (   modelParam.type() == QVariant::String
                            && modelParam.toString().startsWith("ERROR")
                           )
                        {
                            return modelParam.toString();
                        }

                        // .........................................................
                        // if the model parameter is of integer type, we allow
                        // some arithemtic on it...

                        if (    (    modelParam.type() == QVariant::Int
                                     ||  modelParam.type() == QVariant::LongLong
                                     ||  modelParam.type() == QVariant::UInt
                                     ||  modelParam.type() == QVariant::ULongLong
                                     )
                                &&  ( (!m.at(3).isEmpty() && !m.at(4).isEmpty())
                                      || (!m.at(5).isEmpty() && !m.at(6).isEmpty())
                                    )
                                )
                        {
                            bool bok;
                            long long delta = 0;
                            const long long t = !m.at(4).isEmpty() ? m.at(4).toLongLong(&bok)
                                                         : !m.at(6).isEmpty() ? m.at(6).toLongLong(&bok)
                                                                      : 0;
                            if (bok)
                            {
                                delta = t;
                            }

                            long long itStep = modelParam.toLongLong(&bok);

                            if (QString::fromLatin1("+").compare(m.at(3)) == 0
                                || QString::fromLatin1("+").compare(m.at(5)) == 0)
                            {
                                // could only bound  this, if we restricted to the use
                                // of SequentialIterComponent here, not quite sure,
                                // we want to do that
                                itStep += delta;
                            }
                            else if (QString::fromLatin1("-").compare(m.at(3)) == 0
                                     || QString::fromLatin1("-").compare(m.at(5)) == 0)
                            {
                                // prevent 'negative' iStep; could occur in 'instantiation phase'
                                // of the pipeline, when the correct step parameter has not
                                // been established yet (thereby always assuming that the
                                // configuration by the user was correct, in which case the
                                // a wrong parameter would be created during the 'link phase'
                                // of the pipeline establishment)

                                if (itStep - delta >= 0)
                                {
                                    itStep -= delta;
                                }
                                else
                                {
                                    NMLogWarn(<< obj->objectName().toStdString() << "::processStringParameter: "
                                              << "Expression based parameter retreival "
                                              << "prevented a NEGATIVE PARAMETER INDEX!!"
                                              << "  Double check whether the correct "
                                              << "parameter was used and the results are OK!");
                                }
                            }

                            tStr = tStr.replace(wholeText, QString::fromLatin1("%1").arg(itStep));
                        }
                        /// ToDo: how do we handle string lists ?
                        // no integer type, so ignore any potential arithemtic
                        else
                        {
                            tStr = tStr.replace(wholeText, QString::fromLatin1("%1").arg(modelParam.toString()));
                        }

                        NMDebugAI(<< "generated parameter: " << tStr.toStdString() << std::endl);
                    }
                    else
                    {
                        // couldn't find the parameter table
                        std::stringstream ssmsg;
                        ssmsg << "ERROR: '"
                              << tStr.toStdString() << "' - component '"
                                 << m.at(0).toStdString() << "' not found!";

                        return QString(ssmsg.str().c_str());
                    }
                }

                // indicate that we indeed identified and processed an expression
                bRecognisedExpression = true;

            }

            // better raise alarm if the expression is invalid (e.g. using double or float formatted
            // increment operands, which could lead to endless loops)
            if (!bRecognisedExpression)
            {
               std::stringstream ssmsg;
               ssmsg << "ERROR: '"
                     << tStr.toStdString() << "' - invalid parameter syntax/type!";

               return QString(ssmsg.str().c_str());
            }

            nested = nested.replace(innerExp.at(inner), tStr);
            ++count;

        } // for

        innerExp = this->getNextParamExpr(nested);
        numExp = innerExp.size();

    } // while

    return nested;
}

void
NMModelController::trackIdConceptRev(const QString &id,
                                     const QString &concept,
                                     const int &rev)
{
    QMap<QString, QMap<QString, int> >::iterator idIter =
            mMapProvIdConRev.find(id);

    if (idIter == mMapProvIdConRev.end())
    {
        QMap<QString, int> mapRev;
        mapRev.insert(concept, rev);
        mMapProvIdConRev.insert(id, mapRev);
    }
    else
    {
        idIter.value().insert(concept, rev);
    }
}

void
NMModelController::writeProv(const QString provLog)
{
    if (!mbLogProv)
    {
        return;
    }

    if (!mProvFile.isOpen())
    {
        NMLogError(<< "Model Controller: Failed writing provenance entry - provenance file is closed!");
        return;
    }

    // --------------------------------------------------------------------------------
    // disect the statement
    int idOpen = provLog.indexOf('(');
    int idSquare1 = provLog.indexOf('[');
    int idSquare2 = provLog.indexOf(']');
    int idComma = provLog.indexOf(',');
    idComma = idComma > 0 ? idComma : provLog.indexOf(')');
    QString concept = provLog.mid(0, idOpen);
    QString id = provLog.mid(idOpen+1, idComma-idOpen-1);

    int idComma2 = provLog.indexOf(',', idComma+1);
    idComma2 = idComma > 0 ? idComma2 : provLog.indexOf(')');
    QString id2 = idComma2 > 0 ? provLog.mid(idComma+1, idComma2-idComma-1) : "-";

    int idComma3 = provLog.indexOf(',', idComma2+1);
    idComma3 = idComma3 > 0 ? idComma3 : provLog.indexOf(')');
    QString id3 = idComma3 > 0 ? provLog.mid(idComma2+1, idComma3-idComma2-1) : "-";

    QString attrs = idSquare1 > 0 ? provLog.mid(idSquare1+1, idSquare2-idSquare1-1) : "";

    QString entityLog;
    QString writeLog;

    QStringList filterOut;
    filterOut << "agent", "actedOnBehalf", "activity", "wasAssociatedWith";

    // the id iterator (i.e. key of the outer map)
    QMap<QString, QMap<QString, int> >::iterator idIter = mMapProvIdConRev.find(id);
    // the concept iterator (i.e. key of the inner map)
    QMap<QString, int>::iterator conIter;

    // check for the combination of id and concept in the 'tracker maps'
    bool bComboFound = false;
    bool bIdFound = false;
    if (idIter != mMapProvIdConRev.end())
    {
        bIdFound = true;
        conIter = idIter.value().find(concept);
        if (conIter != idIter.value().end())
        {
            bComboFound = true;
        }
    }

    // ===================================================================
    // concept depending processing

    // when concept belongs to 'filterOut' list
    // - only allows a single concept with the given id (i.e. entity id or activity id)
    // - tracks the concept id pair
    if (filterOut.contains(concept))
    {
        if (!bComboFound)
        {
            this->trackIdConceptRev(id, concept, 0);
        }
        else
        {
            NMLogDebug(<< "Model Controller: PROV-N issue: '"
                     << concept.toStdString() << "' has already been logged for '"
                     << id.toStdString() << "'!");
            return;
        }
    }
    // ---------------------------------------------------------------------
    // processing entity statements
    else if (concept.compare("entity") == 0)
    {
        if (bComboFound)
        {
            // get entity revision
            int e_rx = conIter.value();

            // check for derived revision
            conIter = idIter.value().find("wasDerivedFrom");
            int g_rx = -1;
            if (conIter != idIter.value().end())
            {
                g_rx = conIter.value();
                if (g_rx > e_rx)
                {
                    writeLog = QString("entity(%1_r%2")
                               .arg(id).arg(g_rx);
                    attrs = "";
                    this->trackIdConceptRev(id, concept, g_rx);
                }
                else
                {
                    NMLogDebug(<< "Model Controller: PROV-N issue: '"
                             << concept.toStdString() << "' has already been logged for '"
                             << id.toStdString() << "'!");
                    return;
                }
            }
            else if (conIter == idIter.value().end())
            {
                NMLogDebug(<< "Model Controller: PROV-N issue: '"
                         << concept.toStdString() << "' has already been logged for '"
                         << id.toStdString() << "'!");
                return;
            }
        }
        else
        {
            this->trackIdConceptRev(id, concept, 0);
        }
    }
    // -------------------------------------------------------------------------------------
    // processing used statements
    else if (concept.compare("used") == 0)
    {
        // id - activity
        // id2 - entity
        // id3 - time

        // have to lookup different id (-> i.e. id2) here
        idIter = mMapProvIdConRev.find(id2);
        if (idIter != mMapProvIdConRev.end())
        {
            conIter = idIter.value().find("entity");
            if (conIter != idIter.value().end())
            {
                int e_rx = conIter.value();

                // used(e, a, t, -)
                if (e_rx > 0)
                {
                    writeLog = QString("used(%1, %2_r%3, %4")
                                .arg(id).arg(id2).arg(e_rx).arg(id3);
                    this->trackIdConceptRev(id2, concept, e_rx);
                }
                else
                {
                    writeLog = QString("used(%1, %2, %3")
                                .arg(id).arg(id2).arg(id3);
                    this->trackIdConceptRev(id2, concept, 0);
                }
            }
            //            else
            //            {
            //                // mmh, there should be an entity present before
            //                // we can use it!
            //                return;
            //            }
        }
        else
        {
            this->trackIdConceptRev(id2, concept, 0);
        }
    }
    // -------------------------------------------------------------------------------------
    else if (concept.compare("wasGeneratedBy") == 0)
    {
        // id = entity
        // id2 = activity

        if (bIdFound)
        {
            // do we already have an entity of that kind logged?
            conIter = idIter.value().find("entity");

            // ... yes -> turn it into 'revision statement'
            if (conIter != idIter.value().end())
            {
                int e_rx = conIter.value();
                if (e_rx == 0)
                {
                    writeLog = QString("wasDerivedFrom(%1_r%2, %3, %4, -, -")
                                .arg(id).arg(e_rx+1).arg(id).arg(id2);
                }
                else if (e_rx > 0)
                {
                    writeLog = QString("wasDerivedFrom(%1_r%2, %3_r%4, %5, -, -")
                                .arg(id).arg(e_rx+1).arg(id).arg(e_rx).arg(id2);
                }
                this->trackIdConceptRev(id, "wasDerivedFrom", e_rx+1);
            }
            // ... no -> sweet, no problem then
            //            else
            //            {
            //                //writeLog = provLog;

            //                // don't need to really track this
            //                //this->trackIdConceptRev(id, concept, 0);
            //            }
        }
        else
        {
            //writeLog = provLog;
            this->trackIdConceptRev(id, concept, 0);
        }
    }
    // -------------------------------------------------------------------------------------
    else if (concept.compare("wasDerivedFrom") == 0)
    {
        // id = entity (derived)
        // id2 = entity (source)
        // id3 = activity

        int e_rx = 0;

        // determine currently present revision ...
        // ... in case we've logged a revision for this entity already
        if (bComboFound)
        {
            e_rx = conIter.value();
        }
        // ... in case this entity is being revised for the first time
        //        else if (bIdFound)
        //        {
        //            e_
        //            conIter = idIter.value().find("entity");
        //            if (conIter != idIter.value().end())
        //            {
        //                e_rx = conIter.value();
        //            }
        //            else
        //            {
        //                NMLogError(<< "PROVENANCE ERROR: Cannot derive from nothing! "
        //                           << "Expected '" << id.toStdString() << "' "
        //                           << "to be present in the log!");
        //                return;
        //            }
        //        }

        if (e_rx == 0)
        {
            writeLog = QString("wasDerivedFrom(%1_r%2, %3, %4, -, -")
                       .arg(id).arg(e_rx+1).arg(id).arg(id3);
        }
        else if (e_rx > 0)
        {
            writeLog = QString("wasDerivedFrom(%1_r%2, %3_r%4, %5, -, -")
                       .arg(id).arg(e_rx+1).arg(id).arg(e_rx).arg(id3);
        }
        this->trackIdConceptRev(id, concept, e_rx+1);

        entityLog = QString("entity(%1_r%2)").arg(id).arg(e_rx+1);
        this->trackIdConceptRev(id, "entity", e_rx+1);

    }

    // -------------------------------------------------------------------------------------
    // append attributes, if applicable,
    // otherwise just close the statememt
    if (writeLog.isEmpty())
    {
        writeLog = provLog;
    }
    else
    {
        if (attrs.isEmpty())
        {
            writeLog = QString("%1)").arg(writeLog);
        }
        else
        {
            writeLog = QString("%1, [%2])").arg(writeLog).arg(attrs);
        }
    }

    QTextStream out(&mProvFile);
    out << '\t' << writeLog;

    if (!writeLog.endsWith("\n"))
    {
        out << '\n';
    }

    if (!entityLog.isEmpty())
    {
        out << '\t' << entityLog << '\n';
    }

//    QString logNewEntity;
//    QStringList watchout;
//    watchout << "wasGeneratedBy" << "wasDerivedFrom" << "entity" << "activity" << "agent";
//    QMap<QString, QString>::iterator idIter = mMapProvIdConcept.find(id);
//    if (watchout.contains(concept) && idIter != mMapProvIdConcept.end())
//    {
//        if (concept.compare("wasGeneratedBy") == 0)
//        {
//            newId = QString("%1_2").arg(id);

//            if (attrs.isEmpty())
//            {
//                writeLog = QString("wasDerivedFrom(%1,%2,%3,-,-,[prov:type='prov:Revision'])\n")
//                           .arg(newId).arg(id).arg(id2);
//            }
//            else
//            {
//                writeLog = QString("wasDerivedFrom(%1,%2,%3,-,-,[prov:type='prov:Revision',%4])\n")
//                           .arg(newId).arg(id).arg(id2).arg(attrs);
//            }
//            logNewEntity = QString("entity(%1)\n").arg(newId);
//            mMapProvIdConcept.insert(newId, "entity");
//        }
//        else if (concept.compare("wasDerivedFrom") == 0)
//        {
//            int underscore = id.lastIndexOf('_');
//            QString baseId = id.left(underscore);
//            QString cnt = id.right(id.size()-underscore-1);
//            bool bok;
//            int num = cnt.toInt(&bok);
//            if (bok)
//            {
//                newId = QString("%1_%2").arg(baseId).arg(++num);
//            }

//            if (attrs.isEmpty())
//            {
//                writeLog = QString("wasDerivedFrom(%1,%2,%3,-,-,[prov:type='prov:Revision'])\n")
//                           .arg(newId).arg(id).arg(id3);
//            }
//            else
//            {
//                // we expect that the 'wasDerivedFrom' had been properly formatted
//                // so that we can just re-use the 'revision' specification
//                writeLog = QString("wasDerivedFrom(%1,%2,%3,-,-,[%4])\n")
//                           .arg(newId).arg(id).arg(id3).arg(attrs);
//            }
//            logNewEntity = QString("entity(%1)\n").arg(newId);
//            mMapProvIdConcept.insert(newId, "entity");
//        }
//        //else if (concept.compare(idIter.value()) != 0)
//        else if (concept.compare(idIter.value()) == 0)
//        {
//            NMLogWarn(<< "Model Controller: PROV-N issue: '"
//                      << concept.toStdString() << "' has already been logged for '"
//                      << id.toStdString() << "'!");
//            return;
//        }
//        //        else
//        //        {
//        //            NMLogWarn(<< "Model Controller: PROV-N issue: '"
//        //                      << concept.toStdString() << "' has already been logged for '"
//        //                      << id.toStdString() << "'!");
//        //            return;
//        //        }

//    }
//    else if (concept.compare("wasDerivedFrom") == 0)
//    {
//        // create companion entity
//        logNewEntity = QString("entity(%1)\n").arg(id);
//        mMapProvIdConcept.insert(id, "entity");
//    }
//    else
//    {
//        mMapProvIdConcept.insert(id, concept);
//    }

//    QTextStream out(&mProvFile);
//    out << '\t' << writeLog;

//    if (!logNewEntity.isEmpty())
//    {
//        out << '\t' << logNewEntity;
//    }
}

QStringList
NMModelController::getProvNAttributes(const QObject *comp)
{
    QStringList provAttr;

    QStringList propNames = getPropertyList(comp);
    foreach(const QString& prop, propNames)
    {
        QVariant pVal = comp->property(prop.toStdString().c_str());
        QString sVal;
        if (pVal.typeName() == "QString")
        {
            sVal = pVal.toString();
        }
        else if (pVal.typeName() == "QStringList")
        {
            sVal = pVal.toStringList().join(',');
        }
        else if (pVal.typeName() == "QList<QStringList>")
        {
            QList<QStringList> lsl = pVal.value<QList<QStringList> >();
            for (int i=0; i < lsl.size(); ++i)
            {
                QStringList sl = lsl.at(i);
                sVal += sl.join(',');
                if (i+1 < lsl.size())
                {
                    sVal += ", ";
                }
            }
        }
        else if (pVal.typeName() == "QList<QList<QStringList> >")
        {
            QList<QList<QStringList> > llsl = pVal.value<QList<QList<QStringList > > >();
            for (int k=0; k < llsl.size(); ++k)
            {
                QList<QStringList> lsl = llsl.at(k);
                for (int i=0; i < lsl.size(); ++i)
                {
                    QStringList sl = lsl.at(i);
                    sVal += sl.join(',');
                    if (i+1 < lsl.size())
                    {
                        sVal += ", ";
                    }
                }
                if (k+1 < llsl.size())
                {
                    sVal += ", ";
                }
            }
        }
        else
        {
            sVal += pVal.toString();
        }

        // remove quotation marks around parameter value,
        // e.g. as for MapKernelScripts, since they lead
        // to formatting errors of the PROV-N
        // file:
        // [... MapKernelScript=""cipx = 0; ...
        // Here, obviously, the PROV-N parser of ProvStore expects
        // a comma after the double quotation marks ...
        sVal = sVal.replace("\"","");

        QString attr = QString("nm:%1=\"%2\"")
                        .arg(prop)
                        .arg(sVal);

        provAttr.push_back(attr);
    }

    return provAttr;
}

void
NMModelController::logProvNComponent(NMModelComponent *comp)
{
    if (comp == nullptr)
    {
        return;
    }
    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
    NMDataComponent* dc = qobject_cast<NMDataComponent*>(comp);

    QStringList args;
    QString idt = QString("nm:%1").arg(comp->objectName());
    args.push_back(idt);
    QStringList attr;
    QStringList othAttr;// = this->getProvNAttributes(comp);
    // process & aggregate component
    if (ic != nullptr)
    {
        // aggregate component
        if (ic->countComponents())
        {
            attr.push_back("nm:type='nm:AggrComp'");
            attr.append(othAttr);
            NMLogProv(NMLogger::NM_PROV_AGENT, args, attr);

            NMModelComponentIterator icIter = ic->getComponentIterator();
            while(!icIter.isAtEnd())
            {
                logProvNComponent(*icIter);
                QStringList delegateAttr;
                QStringList delegateArgs;
                QString responseId = QString("nm:%1").arg(ic->objectName());
                QString delegateId  = QString("nm:%1").arg((*icIter)->objectName());
                delegateArgs << delegateId << responseId;
                NMLogProv(NMLogger::NM_PROV_DELEGATION,
                          delegateArgs,
                          delegateAttr);
                ++icIter;
            }
        }
        // process component
        else if (ic->getProcess() != nullptr)
        {
            attr.push_back("nm:type='nm:Process'");
            attr.append(othAttr);
            //attr.append(this->getProvNAttributes(ic->getProcess()));
            NMLogProv(NMLogger::NM_PROV_AGENT, args, attr);
        }
    }
    // data component
    else if (dc != nullptr)
    {
        attr.push_back("nm:type='nm:DataComp'");
        attr.append(othAttr);
        NMLogProv(NMLogger::NM_PROV_ENTITY, args, attr);
    }
}

QString
NMModelController::evalMuParserExpression(const QObject *obj, const QString& expr, double* resVal)
{
    QString tStr;
    otb::MultiParser::Pointer parser = otb::MultiParser::New();
    try
    {
        parser->SetExpr(expr.toStdString());
        otb::MultiParser::ValueType res = parser->Eval();
        *resVal = static_cast<double>(res);
        tStr = QString("%1").arg(*resVal, 0, 'g', 15);
    }
    catch (mu::ParserError& evalerr)
    {
        std::stringstream errmsg;
        errmsg << "ERROR:";
        if (obj != nullptr)
        {
            errmsg << obj->objectName().toStdString() << std::endl;
        }
        else
        {
            errmsg << "ModelController" << std::endl;
        }

        errmsg << "Math expression evaluation: "     << std::endl
               << "Message:    " << evalerr.GetMsg() << std::endl
               << "Formula:    " << evalerr.GetExpr() << std::endl
               << "Token:      " << evalerr.GetToken() << std::endl
               << "Position:   " << evalerr.GetPos() << std::endl << std::endl;


        //NMLogError(<< errmsg.str());
        *resVal = 0.0;
        tStr = errmsg.str().c_str();
    }

    return tStr;
}

void
NMModelController::startProv(const QString &fn, const QString& compName)
{
    if (!mbLogProv)
    {
        return;
    }

    mProvFileName = fn;
    mProvFile.setFileName(fn);
    if (!mProvFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        NMLogError(<< "Model Controller: Failed creating provenance record: "
                   << mProvFile.errorString().toStdString());
        return;
    }

    //connect(mLogger, SIGNAL(sendProvN(QString)), this, SLOT(writeProv(QString)));
    connect(mLogger, &NMLogger::sendProvN, this, &NMModelController::writeProv);

    this->mMapProvIdConRev.clear();

    QTextStream provF(&mProvFile);
    provF << "document\n";
    provF << "\tprefix nm <https://manaakiwhenua.github.io/LUMASS/docs/mod_structure#general-model-and-aggregate-component-properties>\n";
    provF << "\tprefix img <https://manaakiwhenua.github.io/LUMASS/docs/cref_image_reader>\n";
    provF << "\tprefix db <https://manaakiwhenua.github.io/LUMASS/docs/cref_table_reader>\n";
}

void
NMModelController::endProv()
{
    if (mProvFile.isOpen())
    {
        QTextStream provF(&mProvFile);
        provF << "endDocument";

        mProvFile.flush();
        mProvFile.close();
    }

    //disconnect(mLogger, SIGNAL(sendProvN(QString)), this, SLOT(writeProv(QString)));
    disconnect(mLogger, &NMLogger::sendProvN, this, &NMModelController::writeProv);
}

