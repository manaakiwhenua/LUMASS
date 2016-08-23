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

#include "NMModelController.h"
#include "NMIterableComponent.h"
#include "NMMfwException.h"

#include <QFuture>
#include <QtConcurrentRun>

const std::string NMModelController::ctx = "NMModelController";

NMModelController::NMModelController(QObject* parent)
	: mbModelIsRunning(false),
	  mRootComponent(0), mbAbortionRequested(false)
{
	this->setParent(parent);
	this->mModelStarted = QDateTime::currentDateTime();
	this->mModelStopped = this->mModelStarted;
}

NMModelController::~NMModelController()
{
}

NMModelController*
NMModelController::getInstance(void)
{
	static NMModelController controller;
	return &controller;
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
			NMErr(ctx, << "failed to interpret input source parameter"
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

void
NMModelController::executeModel(const QString& compName)
{
	NMDebugCtx(ctx, << "...");

	if (this->mbModelIsRunning)
	{
		NMDebugAI(<< "There is already a model running! "
				<< "Be patient and try later!" << endl);
		NMDebugCtx(ctx, << "done!");
		return;
	}

	//this->mRootComponent = this->identifyRootComponent();
	//QString name = this->mRootComponent->objectName();
	NMModelComponent* comp = this->getComponent(compName);
	if (comp == 0)
	{
		NMErr(ctx, << "couldn't find '"
				<< compName.toStdString() << "'!");
		NMDebugCtx(ctx, << "done!");
		return;
	}

	// we only execute 'iterable / executable' components
//	NMIterableComponent* icomp =
//			qobject_cast<NMIterableComponent*>(comp);
//	if (icomp == 0)
//	{
//		NMErr(ctx, << "component '" << compName.toStdString()
//				<< "' is of type '" << comp->metaObject()->className()
//				<< "' which is non-executable!");
//		return;
//	}

	// we reset all the components
	// (and thereby delete all data buffers)
	this->resetComponent(compName);


	NMDebugAI(<< "running model on thread: "
			<< this->thread()->currentThreadId() << endl);

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
        NMDebugAI(<< nmerr.what() << std::endl);
        NMDebugCtx(ctx, << "done!");
    }
    catch (std::exception& e)
	{
#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif
        NMDebugAI(<< e.what() << endl);
		NMDebugCtx(ctx, << "done!");
	}


#ifdef DEBUG
#ifndef _WIN32
    nmlog::nmindent = ind;
#endif
#endif

	this->mModelStopped = QDateTime::currentDateTime();
	int msec = this->mModelStarted.msecsTo(this->mModelStopped);
	int min = msec / 60000;
	double sec = (msec % 60000) / 1000.0;

	QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
	NMDebugAI(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);
	NMMsg(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);

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
	NMModelComponent* comp = this->getComponent(compName);
	if (comp == 0)
	{
		NMErr(ctx, << "couldn't find '"
				<< compName.toStdString() << "'!");
		return;
	}

//	NMDebugAI(<< "resetting component '" << compName.toStdString()
//			  << "'" << endl);

	comp->reset();
//	NMDebugCtx(ctx, << "done!");
}

QList<NMModelComponent*>
NMModelController::getComponents(const QString &userId)
{
    QList<NMModelComponent*> ret;

    QMultiMap<QString, NMModelComponent*>::const_iterator it =
            mUserIdMap.cbegin();
    while (it != mUserIdMap.end())
    {
        if (it.key().compare(userId) == 0)
        {
            ret << it.value();
        }
        ++it;
    }

    return ret;
}

QString NMModelController::addComponent(NMModelComponent* comp,
		NMModelComponent* host)
{
    //	NMDebugCtx(ctx, << "...");

	NMIterableComponent* ihost = 0;
	if (host != 0)
	{
		ihost = qobject_cast<NMIterableComponent*>(host);
	}

    //	if (comp != 0 && ihost != 0)
    //	{
    //		NMDebugAI(<< "adding '" << comp->objectName().toStdString() << "' to '"
    //			     << host->objectName().toStdString() << "' ..." << endl);
    //	}
    //	else if (comp != 0)
    //	{
    //		NMDebugAI(<< "adding '" << comp->objectName().toStdString() << "' to 'root"
    //			     << "' ..." << endl);
    //	}

	if (this->mComponentMap.values().contains(comp))
	{
		NMErr(ctx, << "model component already present in repository!");
        //		NMDebugCtx(ctx, << "done!");
		return "failed";
	}

	QRegExp re("[0-9]{0,4}$");
	QString cname = comp->objectName();
	QString tname = cname;
	QString numstr;
	unsigned long cnt = 1;
	bool bok;
    //	NMDebugAI(<< "checking names ..." << endl);
    //	NMDebugAI(<< tname.toStdString() << endl);
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
        //		NMDebugAI(<< tname.toStdString() << endl);
	}

    //	NMDebugAI(<< "insert comp as '" << tname.toStdString() << "'" << endl);
	comp->setParent(0);
	comp->moveToThread(this->thread());
	comp->setObjectName(tname);
	comp->setParent(this);

	this->mComponentMap.insert(tname, comp);
    this->mUserIdMap.insertMulti(comp->getUserID(), comp);
    connect(comp, SIGNAL(ComponentUserIDChanged(QString)),
            this, SLOT(setUserId(QString)));

	// check, whether we've go a valid host
	if (ihost != 0)
	{
		if (this->mComponentMap.keys().contains(host->objectName()))
		{
			ihost->addModelComponent(comp);
		}
	}

    //	NMDebugCtx(ctx, << "done!");
	return tname;
}

void NMModelController::setUserId(const QString& userId)
{
    NMModelComponent* comp = qobject_cast<NMModelComponent*>(this->sender());
    if (comp == 0)
    {
        return;
    }

    QMap<QString, NMModelComponent*>::iterator it =
            mUserIdMap.find(userId, comp);

    if (it != mUserIdMap.end())
    {
        mUserIdMap.remove(it.key(), comp);
    }

    mUserIdMap.insert(userId, comp);
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
		NMErr(ctx, << "component '" << name.toStdString() << "' is not controlled by this "
				<< "controller!");
		return false;
	}

	NMIterableComponent* host = comp->getHostComponent();
	if (host != 0)
		host->removeModelComponent(name);

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
            tStr.replace(QString(" "), QString(""));
            QRegExp rex("\\$\\[([a-zA-Z]+[a-zA-Z_\\d]*){1,1}(?::([a-zA-Z]+[a-zA-Z_\\d]*))?(?::(\\d*))?([\\+-]?)(\\d*)\\]\\$");
            int pos = 0;
            while((pos = rex.indexIn(tStr, pos)) != -1)
            {
                // 0: whole captured text
                // 1: component name  | or user Id
                // 2: [: property name]
                // 3: [: property index]
                // 4: [operator]
                // 5: [integer number]
                QStringList m = rex.capturedTexts();
                NMDebugAI(<< m.join(" | ").toStdString() << std::endl);
                //NMDebugAI(<< "---------------" << std::endl);
                pos += rex.matchedLength();

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

                NMModelComponent* mc = NMModelController::getInstance()->getComponent(m.at(1));

                // if the component is specified by userId, we've got to dig a little deeper
                if (mc == 0)
                {
                    if (host)
                    {
                        mc = host->findUpstreamComponentByUserId(m.at(1));
                    }
                    else
                    {
                        NMWarn(obj->objectName().toStdString(), "Process not embedded in model component!");
                    }
                }

                // -----------------------------------------------------------------------------
                // retrieve model parameter and process, if applicable
                if (mc)
                {
                    NMIterableComponent* ic = 0;

                    QVariant modelParam;
                    if (m.at(2).isEmpty())
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
                        if (m.at(3).isEmpty())
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
                            paramSpec = QString("%1:%2").arg(m.at(2)).arg(pstep);
                        }
                        else
                        {
                            paramSpec = QString("%1:%2").arg(m.at(2)).arg(m.at(3));
                        }
                        modelParam = mc->getModelParameter(paramSpec);
                    }

                    // .........................................................
                    // if the model parameter is of integer type, we allow
                    // some arithemtic on it...

                    if (    (    modelParam.type() == QVariant::Int
                                 ||  modelParam.type() == QVariant::LongLong
                                 ||  modelParam.type() == QVariant::UInt
                                 ||  modelParam.type() == QVariant::ULongLong
                                 )
                            &&  !m.at(4).isEmpty() && !m.at(5).isEmpty()
                            )
                    {
                        bool bok;
                        long long delta = 0;
                        const long long t = m.at(5).toLongLong(&bok);
                        if (bok)
                        {
                            delta = t;
                        }

                        //int itStep = ic->getIterationStep();
                        long long itStep = modelParam.toLongLong(&bok);

                        if (QString::fromLatin1("+").compare(m.at(4)) == 0)
                        {
                            // could only bound  this, if we restricted to the use
                            // of SequentialIterComponent here, not quite sure,
                            // we want to do that
                            itStep += delta;
                        }
                        else if (QString::fromLatin1("-").compare(m.at(4)) == 0)
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
                                NMWarn(obj->objectName().toStdString(),
                                       << "Expression based parameter retreival "
                                       << "prevented a NEGATIVE PARAMETER INDEX!!"
                                       << "  Double check whether the correct "
                                       << "parameter was used and the results are OK!");
                            }
                        }

                        tStr = tStr.replace(m.at(0), QString::fromLatin1("%1").arg(itStep));
                    }
                    /// ToDo: how do we handle string lists ?
                    // no integer type, so ignore any potential arithemtic
                    else
                    {
                        tStr = tStr.replace(m.at(0), QString::fromLatin1("%1").arg(modelParam.toString()));
                    }

                    NMDebugAI(<< "generated parameter: " << tStr.toStdString() << std::endl);
                }
                else
                {
                    // couldn't find the parameter table
                    NMErr(ctxNMProcess, << "Failed to find component '"
                          << m.at(1).toStdString() << "'!");
                    return tStr;
                }
            }

            nested = nested.replace(innerExp.at(inner), tStr);

        } // for

        innerExp = NMModelController::getNextParamExpr(nested);
        numExp = innerExp.size();

    } // while

    return nested;
}
