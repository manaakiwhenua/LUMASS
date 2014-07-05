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

NMItkDataObjectWrapper*
NMModelController::getOutputFromSource(const QString& inputSrc)
{
	NMItkDataObjectWrapper* w = 0;

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
    int ind = nmlog::nmindent;
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
	catch (std::exception& e)
	{
#ifdef DEBUG
    nmlog::nmindent = ind;
#endif
        NMDebugAI(<< e.what() << endl);
		NMDebugCtx(ctx, << "done!");
	}

#ifdef DEBUG
    nmlog::nmindent = ind;
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
