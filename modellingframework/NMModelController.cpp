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
	  mRootComponent(0), mbAbortionRequested(false)
{
	this->setParent(parent);
	this->mModelStarted = QDateTime::currentDateTime();
	this->mModelStopped = this->mModelStarted;
    mLogger = new NMLogger(this);
}

NMModelController::~NMModelController()
{
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

    return modelSettings;
}

void
NMModelController::clearModelSettings(void)
{
    QStringList modelSettings = mSettings.keys();
    QStringList sys;
    sys << "UserModels" << "Workspace" << "LUMASSPath";

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
            retList << args.mid(lastComma+1, i-(lastComma < 0 ? 0 : lastComma-1));
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
             retList << args.mid(lastComma+1, args.size()-lastComma-1);
        }
    }
    else
    {
        retList << args;
    }

    return retList;
}

QString
NMModelController::processStringParameter(const QObject* obj, const QString& str)
{
    QString nested = str;
    QStringList innerExp = NMModelController::getNextParamExpr(nested);
    int numExp = innerExp.size();

    while (numExp > 0)
    {
        for (int inner=0; inner < numExp; ++inner)
        {
            QString tStr = innerExp.at(inner);
            tStr = tStr.simplified();
            //tStr.replace(QString(" "), QString(""));

            QRegularExpression rexexp("((?<open>\\$\\[)*"
                                         "(?(<open>)|\\b)"
                                         "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
                                         "(?<sep1>(?(<open>):|(?>__)))*"
                                         "(?<arith>(?(<sep1>)|([ ]*(?<opr>[+\\-])?[ ]*(?<sum>[\\d]+))))*"
                                         "(?<prop>(?(?<!math:|func:)(?(<sep1>)\\g<comp>)|([a-zA-Z0-9_ \\/\\(\\)&%\\|\\>\\!\\=\\<\\-\\+\\*\\^\\?:;.,'\"])*))*"
                                         "(?<sep2>(?(<prop>)(?(<open>):)))*"
                                         "(?(<sep2>)(?<idx>[0-9]+)|([ ]*(?<opr2>[+\\-]+)[ ]*(?<sum2>[\\d]+))*))(?>\\]\\$)*");
            int pos = 0;
            bool bRecognisedExpression = false;
            //while((pos = rex.indexIn(tStr, pos)) != -1)
            QRegularExpressionMatchIterator mit = rexexp.globalMatch(tStr);
            while (mit.hasNext())
            {
                QRegularExpressionMatch match = mit.next();
                QString wholeText = match.captured(0);

                QStringList m;
                m << match.capturedRef("comp").toString(); // 0
                m << match.capturedRef("prop").toString(); // 1
                m << match.capturedRef("idx").toString();  // 2

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
                        //parser->SetExpr(m.at(2).toStdString());
                        parser->SetExpr(m.at(1).toStdString());
                        otb::MultiParser::ValueType res = parser->Eval();
                        tStr = QString("%1").arg(res);
                    }
                    catch (mu::ParserError& evalerr)
                    {
                        std::stringstream errmsg;
                        errmsg << "ERROR - " << obj->objectName().toStdString()
                               << " Math expression evaluation: ";
                        errmsg << std::endl
                               << "Message:    " << evalerr.GetMsg() << std::endl
                               << "Formula:    " << evalerr.GetExpr() << std::endl
                               << "Token:      " << evalerr.GetToken() << std::endl
                               << "Position:   " << evalerr.GetPos() << std::endl << std::endl;


                        NMLogError(<< errmsg.str());
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
                        errstr << "Couldn't find LUMASS setting '"
                               << m.at(1).toStdString() << "'!";
                        NMLogError(<< errstr.str());
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

                        QVariant modelParam;
                        if (m.at(1).isEmpty())
                        {
                            ic = qobject_cast<NMIterableComponent*>(mc);
                            if (ic)
                            {
                                modelParam = QVariant::fromValue(ic->getIterationStep());
                            }
                            else
                            {
                                modelParam = QVariant::fromValue(1);
                            }
                        }
                        else
                        {
                            QString paramSpec;
                            if (m.at(2).isEmpty())
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
                            else
                            {
                                paramSpec = QString("%1:%2").arg(m.at(1)).arg(m.at(2));
                            }

                            // if we get an invalid parameter, we stop processing and
                            // return the error message
                            modelParam = mc->getModelParameter(paramSpec);
                            if (    modelParam.type() == QVariant::String
                                    && modelParam.toString().startsWith("ERROR")
                                    )
                            {
                                return modelParam.toString();
                            }
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
                        ssmsg << "ERROR - NMModelController::processStringParameter('"
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
               ssmsg << "ERROR - NMModelController::processStringParameter('"
                     << tStr.toStdString() << "' - invalid parameter syntax/type!";

               return QString(ssmsg.str().c_str());
            }

            nested = nested.replace(innerExp.at(inner), tStr);

        } // for

        innerExp = this->getNextParamExpr(nested);
        numExp = innerExp.size();

    } // while

    return nested;
}
