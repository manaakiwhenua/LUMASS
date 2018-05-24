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

#include <QFuture>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QString>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#include "NMModelController.h"
#include "NMIterableComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMParameterTable.h"
#include "NMDataComponent.h"
#include "NMMfwException.h"
#include "NMImageReader.h"
#include "NMTableReader.h"

#include "otbMultiParser.h"

const std::string NMModelController::ctx = "NMModelController";

NMModelController::NMModelController(QObject* parent)
	: mbModelIsRunning(false),
      mRootComponent(0), mbAbortionRequested(false),
      mbLogProv(false)
{
	this->setParent(parent);
	this->mModelStarted = QDateTime::currentDateTime();
	this->mModelStopped = this->mModelStarted;
    mLogger = new NMLogger(this);
}

NMModelController::~NMModelController()
{
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
	QStringList inputSrcParams = inputSrc.split(":", QString::SkipEmptyParts);
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

bool
NMModelController::isModelRunning(void)
{
	return this->mbModelIsRunning;
}

void
NMModelController::reportExecutionStopped(const QString & compName)
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
NMModelController::reportExecutionStarted(const QString & compName)
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

//        NMLogInfo(<< "ModelController: Model '" << comp->objectName().toStdString()
//                  << "' has been requested to abort execution at the next opportunity!");
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
NMModelController::deleteLater(QStringList compNames)
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
    sys << "UserModels" << "Workspace" << "LUMASSPath" << "TimeFormat";

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
NMModelController::updateSettings(const QString& key, QVariant value)
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
NMModelController::executeModel(const QString& compName)
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

    if (mbLogProv)
    {
        QString provFN = QString("%1/%2_%3.provn")
                         .arg(this->getSetting("Workspace").toString())
                         .arg(userID)
                         .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        provFN = provFN.replace(":", "");
        provFN = provFN.replace("-", "");
        startProv(provFN, comp->objectName());

        //this->logProvNComponent(comp);
    }

    // ================================================
    // we reset all the components
	// (and thereby delete all data buffers)
	this->resetComponent(compName);

	NMDebugAI(<< "running model on thread: "
			<< this->thread()->currentThreadId() << endl);

    msg = QString("Model Controller: Executing model %1 ...").arg(userID);
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(),
                           NMLogger::NM_LOG_INFO,
                           msg);

	// model management
	this->mbModelIsRunning = true;
	this->mbAbortionRequested = false;

	this->mModelStarted = QDateTime::currentDateTime();

#ifdef DEBUG
#ifndef _WIN32
    int ind = nmlog::nmindent;
#else
	int ind = 2;
#endif
#endif

    emit signalModelStarted();

	// we catch all exceptions thrown by ITK/OTB, rasdaman
	// and the LUMASS MFW components
	// and just report them for now; note this includes
	// the 'abortion-exception' thrown by ITK/OTB as response to
	// user-requested model abortion
	try
	{
		comp->update(this->mComponentMap);
	}
    catch (NMMfwException& nmerr)
    {
#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif
        NMLogError(<< "Model Controller: " << nmerr.what());
        NMDebugCtx(ctx, << "done!");
    }
    catch (itk::ExceptionObject& ieo)
    {
#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif
        NMLogError(<< "Model Controller: " << ieo.what());
        NMDebugCtx(ctx, << "done!");
    }
    catch (std::exception& e)
	{
#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif
        NMLogError(<< "Model Controller: " << e.what());
		NMDebugCtx(ctx, << "done!");
	}


#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif

    emit signalModelStopped();

	this->mModelStopped = QDateTime::currentDateTime();
	int msec = this->mModelStarted.msecsTo(this->mModelStopped);
	int min = msec / 60000;
	double sec = (msec % 60000) / 1000.0;

	QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
    //NMDebugAI(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);
	NMMsg(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);
    NMLogInfo(<< "Model Controller: Model stopped after (min:sec): " << elapsedTime.toStdString());

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

	NMDebugCtx(ctx, << "done!");
}

void
NMModelController::resetComponent(const QString& compName)
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

    if (comp == 0)
    {
        NMLogError(<< ctx << ": cannot add NULL component to model!");
        return "failed";
    }

    NMIterableComponent* ihost = 0;
	if (host != 0)
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

	comp->setParent(0);
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

void NMModelController::setUserId(const QString& oldId, const QString& newId)
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
	if (this->mComponentMap.keys().contains(compName))
		ret = true;
	else
		ret = false;

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

    otb::AttributeTable::Pointer tab;
    // data component
    if (dc)
    {
        // update the data component
        QSharedPointer<NMItkDataObjectWrapper> dw = dc->getOutput(0);
        if (!dw.isNull())
        {
            tab = dw->getOTBTab();
        }
        else
        {
            this->executeModel(dc->objectName());
            QSharedPointer<NMItkDataObjectWrapper> dwupd = dc->getOutput(0);
            if (!dwupd.isNull())
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
                this->executeModel(ic->objectName());
                QSharedPointer<NMItkDataObjectWrapper> dw = ic->getOutput(0);
                if (!dw.isNull())
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

QString
NMModelController::evalFunc(const QString& funcName, const QStringList& args)
{
    QString ret;
    if (funcName.compare("isFile") == 0)
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
    int maxcount = 0;
    QString nested = str;

    // count the number of ParameterExpressions to be evaluated
    // to avoid endless loops
    QStringList innerExp = this->getNextParamExpr(nested);
    while (innerExp.size() > 0)
    {
        foreach(const QString& s, innerExp)
        {
            nested = nested.replace(s, QString(""));
            ++maxcount;
        }
        innerExp = this->getNextParamExpr(nested);
    }
    /// ToDo: testing!
    maxcount *= 11;

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
                                            "(?<prop>(?(?<!math:|func:)(?(<sep1>)\\g<comp>)|([a-zA-Z0-9_ \\/\\(\\)&%\\|\\>\\!\\=\\<\\-\\+\\*\\^\\?:;.,'\"])*))*"
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
                NMIterableComponent* host = 0;

                if (procObj == 0)
                {
                    host = qobject_cast<NMIterableComponent*>(const_cast<QObject*>(obj));
                }
                else
                {
                    host = qobject_cast<NMIterableComponent*>(obj->parent());
                }

                // eval a math parser expression
                if (m.at(0).compare(QString("math"), Qt::CaseInsensitive) == 0)
                {
                    otb::MultiParser::Pointer parser = otb::MultiParser::New();
                    try
                    {
                        parser->SetExpr(m.at(1).toStdString());
                        otb::MultiParser::ValueType res = parser->Eval();
                        tStr = QString("%1").arg(res);
                    }
                    catch (mu::ParserError& evalerr)
                    {
                        std::stringstream errmsg;
                        errmsg << "ERROR:" << obj->objectName().toStdString() << std::endl
                               << "Math expression evaluation: ";
                        errmsg << std::endl
                               << "Message:    " << evalerr.GetMsg() << std::endl
                               << "Formula:    " << evalerr.GetExpr() << std::endl
                               << "Token:      " << evalerr.GetToken() << std::endl
                               << "Position:   " << evalerr.GetPos() << std::endl << std::endl;


                        //NMLogError(<< errmsg.str());
                        return QString(errmsg.str().c_str());
                    }
                }
                else if (m.at(0).compare(QString("LUMASS"), Qt::CaseInsensitive) == 0)
                {
                    if (mSettings.find(m.at(1)) != mSettings.end())
                    {
                       tStr = tStr.replace(wholeText, QString("%1").arg(mSettings[m.at(1)].toString()));
                    }
                    else
                    {
                        std::stringstream errstr;
                        errstr << "ERROR: Couldn't find LUMASS setting '"
                               << m.at(1).toStdString() << "'!";
                        //NMLogError(<< errstr.str());
                        return QString(errstr.str().c_str());
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
                        foreach(QString s, args.split(',', QString::SkipEmptyParts))
                        {
                            argList << s.trimmed();
                        }
                    }

                    QString ret = this->evalFunc(funcName, argList);
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
NMModelController::writeProv(const QString &provLog)
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
        NMLogError(<< "Model Controller: Failed creating provenance record!)");
        return;
    }

    connect(mLogger, SIGNAL(sendProvN(QString)), this, SLOT(writeProv(QString)));

    this->mMapProvIdConRev.clear();

    QTextStream provF(&mProvFile);
    provF << "document\n";
    provF << "\tprefix nm <https://bitbucket.org/landcareresearch/lumass/nm>\n";
    provF << "\tprefix img <https://bitbucket.org/landcareresearch/lumass/img>\n";
    provF << "\tprefix db <https://bitbucket.org/landcareresearch/lumass/db>\n";
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

    disconnect(mLogger, SIGNAL(sendProvN(QString)), this, SLOT(writeProv(QString)));
}

