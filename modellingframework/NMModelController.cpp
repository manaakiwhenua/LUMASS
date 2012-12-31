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
#include "NMModelSerialiser.h"

#include <QFuture>
#include <QtConcurrentRun>

NMModelController::NMModelController(QObject* parent)
	: mbModelIsRunning(false),
	  mRootComponent(0), mbAbortionRequested(false)
{
	this->setParent(parent);
	this->ctx = "NMModelController";
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

		NMModelComponent* comp = this->getComponent(name);
		if (comp != 0)
		{
			NMProcess* proc = comp->getProcess();
			if (proc != 0)
			{
				proc->abortExecution();
			}
		}
		this->mbAbortionRequested = true;
		//this->resetExecutionStack();
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

NMModelComponent*
NMModelController::identifyRootComponent(void)
{
	NMDebugCtx(ctx, << "...");

	NMModelComponent* root = 0;

	QMapIterator<QString, NMModelComponent*> cit(this->mComponentMap);
	while(cit.hasNext())
	{
		cit.next();
		NMDebugAI(<< "checking '" << cit.value()->objectName().toStdString()
				<< "' ...");
		if (cit.value()->getHostComponent() == 0)
		{
			root = cit.value();
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

	this->mRootComponent = this->identifyRootComponent();
	QString name = this->mRootComponent->objectName();
	if (this->mRootComponent == 0)
	{
		NMErr(ctx, << "couldn't find a root component!");
		return;
	}

	NMDebugAI(<< "running model on thread: "
			<< this->thread()->currentThreadId() << endl);

	// model management
	this->mbModelIsRunning = true;
	this->mbAbortionRequested = false;

	this->mModelStarted = QDateTime::currentDateTime();

	// we catch all exceptions thrown by ITK & rasdaman here
	// and just report them for now; note this includes
	// the 'abortion-exception' thrwon by ITK/OTB as response to
	// user-requested model abortion
	try
	{
		this->mRootComponent->update(this->mComponentMap);
	}
	catch (std::exception& e)
	{
		NMDebugAI(<< e.what() << endl);
	}

	this->mModelStopped = QDateTime::currentDateTime();
	int msec = this->mModelStarted.msecsTo(this->mModelStopped);
	int min = msec / 60000;
	double sec = (msec % 60000) / 1000.0;

	QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
	NMDebugAI(<< "Model run took (min:sec): " << elapsedTime.toStdString() << endl);

	this->mbModelIsRunning = false;
	this->mbAbortionRequested = false;
	emit signalIsControllerBusy(false);

	// to be on the safe side, we reset the execution stack and
	// notify all listeners, that those components are no longer
	// running
	this->resetExecutionStack();
	this->resetComponent(name);

	NMDebugCtx(ctx, << "done!");
}

void
NMModelController::resetComponent(const QString& compName)
{
	NMModelComponent* comp = this->getComponent(compName);
	if (comp == 0)
		return;

	comp->reset();

}

QString NMModelController::addComponent(NMModelComponent* comp,
		NMModelComponent* host)
{
	NMDebugCtx(ctx, << "...");

	if (comp != 0 && host != 0)
	{
		NMDebugAI(<< "adding '" << comp->objectName().toStdString() << "' to '"
			     << host->objectName().toStdString() << "' ..." << endl);
	}
	else if (comp != 0)
	{
		NMDebugAI(<< "adding '" << comp->objectName().toStdString() << "' to 'root"
			     << "' ..." << endl);
	}

	if (this->mComponentMap.values().contains(comp))
	{
		NMErr(ctx, << "model component already present in repository!");
		NMDebugCtx(ctx, << "done!");
		return "failed";
	}

	QRegExp re("[0-9]{0,4}$");
	QString cname = comp->objectName();
	QString tname = cname;
	QString numstr;
	unsigned long cnt = 1;
	bool bok;
	NMDebugAI(<< "checking names ..." << endl);
	NMDebugAI(<< tname.toStdString() << endl);
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
		NMDebugAI(<< tname.toStdString() << endl);
	}

	NMDebugAI(<< "insert comp as '" << tname.toStdString() << "'" << endl);
	comp->setParent(0);
	comp->moveToThread(this->thread());
	comp->setObjectName(tname);
	comp->setParent(this);

	this->mComponentMap.insert(tname, comp);

	// check, whether we've go a valid host
	if (host != 0)
	{
		if (this->mComponentMap.keys().contains(host->objectName()))
		{
			host->addModelComponent(comp);
		}
	}

	NMDebugCtx(ctx, << "done!");
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

	NMModelComponent* host = comp->getHostComponent();
	if (host != 0)
		host->removeModelComponent(name);
	comp->destroySubComponents(this->mComponentMap);
	this->mComponentMap.remove(name);
	delete comp;

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
