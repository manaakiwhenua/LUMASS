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

#include "NMLumassEngine.h"
#include "NMModelController.h"
#include "NMModelSerialiser.h"
#include "NMModelComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMMosra.h"
#include "MOSORunnable.h"

NMLumassEngine::NMLumassEngine(QObject* parent)
    : QObject(parent),
      mController(nullptr),
	  mMosra(nullptr), 
	  mBMILogger(nullptr),
	  mMode(NM_ENGINE_MODE_UNKNOWN)
{
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(false);

    mController = new NMModelController(this);
    mController->setLogger(mLogger);

    NMSequentialIterComponent* root = new NMSequentialIterComponent();
    root->setObjectName("root");
    root->setDescription("Top level model component managed by the ModelController");
    mController->addComponent(root);
}

NMLumassEngine::~NMLumassEngine()
{
    if (mLogFile.isOpen())
    {
        mLogFile.flush();
        mLogFile.close();
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
    mLogFile.setFileName(fn);
    if (!mLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
		std::stringstream emsg;
		emsg << "Failed creating log file!";
		log("ERROR", emsg.str().c_str());
        return;
    }

    connect(mLogger, SIGNAL(sendLogMsg(QString)), this, SLOT(writeLogMsg(QString)));

    // write the first message
    QString logstart = QString("LUMASS Engine - %1, %2\n")
            .arg(QDate::currentDate().toString())
            .arg(QTime::currentTime().toString());

    this->writeLogMsg(logstart);
	mBMILogger(2, logstart.toStdString().c_str());
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
    if (!mLogFile.isOpen())
    {
        NMErr("NMLumassEngine", << "Failed writing log message - log file is closed!");
		mBMILogger(4, "NMLumassEngine: Failed writing log message - log file is closed!");
        return;
    }

    QTextStream out(&mLogFile);
    out << msg;

}
