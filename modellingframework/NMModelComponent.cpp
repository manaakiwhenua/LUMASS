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
#include "NMModelComponent.h"
#include "NMModelController.h"

#include <string>
#include <iostream>
#include <sstream>
#include "nmlog.h"


NMModelComponent::NMModelComponent(QObject* parent)
{
	this->setParent(parent);
	this->initAttributes();
	ctx = "NMModelComponent";
}

NMModelComponent::NMModelComponent(const NMModelComponent& modelComp)
{
	NMDebugCtx(ctxNMModelComponent, << "...");

	this->initAttributes();

	NMModelComponent& comp = const_cast<NMModelComponent&>(modelComp);
	NMDebugAI(<< "copying '" << comp.objectName().toStdString() << "'" << std::endl);

	this->moveToThread(comp.thread());
	this->setParent(comp.parent());
	this->mTimeLevel = comp.getTimeLevel();
	this->setObjectName(comp.objectName());
	this->mProcessChainStart = comp.getInternalStartComponent();
	this->setUpstreamModelComponent(comp.getUpstreamModelComponent());
	this->setDownstreamModelComponent(comp.getDownstreamModelComponent());

	NMDebugCtx(ctxNMModelComponent, << "done!");
}

void NMModelComponent::initAttributes(void)
{
	this->mNumIterations = 1;
    this->mTimeLevel = 0;
    this->mMaxInternalTimeLevel = 0;
    this->mProcessChainPointer = 0;
    this->mProcessChainStart = 0;
    this->mUpComponent = 0;
    this->mDownComponent = 0;
    this->mProcess = 0;
    this->mHostComponent = 0;
}

NMModelComponent::~NMModelComponent(void)
{
}

void NMModelComponent::setProcess(NMProcess* proc)
{
	NMDebugCtx(ctxNMModelComponent, << "...");

	this->mProcess = proc;
	this->mProcessChainPointer = 0;
	this->mProcessChainStart = 0;
	proc->setParent(0);
	proc->moveToThread(this->thread());
	proc->setParent(this);

	NMDebugCtx(ctxNMModelComponent, << "done!");

	emit NMModelComponentChanged();
}

int NMModelComponent::countComponents(void)
{
	int ret = 0;

	NMModelComponent* comp = this->getInternalStartComponent();

	while(comp != 0)
	{
		++ret;
		comp = this->getNextInternalComponent();
	}

	return ret;
}


void NMModelComponent::addModelComponent(NMModelComponent* comp)
{
	NMDebugCtx(ctxNMModelComponent, << "...");

	if (comp == 0)
		return;

	// if this component has already a process assigned to it
	// we don't add any components
	// ToDo: raise an appropriate exception
	if (this->mProcess != 0)
		return;

	// normally, we maintain components time levels to support
	// loading models from file, however: nested components must
	// not have lower time levels than the parent component;
	if (comp->getTimeLevel() < this->getTimeLevel())
		comp->setTimeLevel(this->getTimeLevel());

	NMDebugAI(<< "parent: '" << this->objectName().toStdString() << "'" << std::endl);
	NMDebugAI(<< "add child:  '" << comp->objectName().toStdString() << "'" << std::endl);
	NMDebugAI(<< "new chain: ");
	comp->setHostComponent(this);

	NMModelComponent* lastComp = this->getInternalStartComponent();
	if (lastComp == 0)
	{
		this->mProcessChainStart = comp;
		NMDebug(<< "'" << comp->objectName().toStdString() << "'" << std::endl);
		NMDebugCtx(ctxNMModelComponent, << "done!");
		return;
	}
	NMDebug(<< " '" << lastComp->objectName().toStdString() << "'");

	while (lastComp->getDownstreamModelComponent() != 0)
	{
		lastComp = this->getNextInternalComponent();
		NMDebug(<< " '" << lastComp->objectName().toStdString() << "'");
	}
	NMDebug(<< "'" << comp->objectName().toStdString() << "'" << std::endl);

	lastComp->setDownstreamModelComponent(comp);
	comp->setUpstreamModelComponent(lastComp);

	emit NMModelComponentChanged();

	NMDebugCtx(ctxNMModelComponent, << "done!");
}

void NMModelComponent::insertModelComponent(NMModelComponent* insertComp,
		const QString& previousComp)
{
	if (this->mProcess != 0)
		return;

	if (insertComp == 0)
		return;

	bool bAtFirst = false;
	if (previousComp.isEmpty())
		bAtFirst = true;

	// check, whether we've got a valid previous component
	NMModelComponent* prevComp = this->findModelComponent(previousComp);
	if (prevComp == 0 && !bAtFirst)
		return;

	if (bAtFirst)
	{
		NMModelComponent* prevFirst = this->mProcessChainStart;
		this->mProcessChainStart = insertComp;
		insertComp->setDownstreamModelComponent(prevFirst);
		prevFirst->setUpstreamModelComponent(insertComp);
	}
	else
	{
		NMModelComponent* prevDownComp = prevComp->getDownstreamModelComponent();

		// connect the component to be inserted with the previous component
		prevComp->setDownstreamModelComponent(insertComp);
		insertComp->setUpstreamModelComponent(prevComp);

		// if the previous component was already the last component in the chain, we don't
		// need to do any up-linking, otherwise we do
		if (prevDownComp != 0)
		{
			insertComp->setDownstreamModelComponent(prevDownComp);
			prevDownComp->setUpstreamModelComponent(insertComp);
		}
	}

	// nested components must not have lower time levels than
	// the parent component
	if (insertComp->getTimeLevel() < this->getTimeLevel())
		insertComp->setTimeLevel(this->getTimeLevel());

	insertComp->setHostComponent(this);

	emit NMModelComponentChanged();
}


NMModelComponent* NMModelComponent::findModelComponent(const QString& compName)
{
	NMModelComponent* curComp = this->getInternalStartComponent();
	while (curComp != 0)
	{
		if (curComp->objectName().compare(compName, Qt::CaseInsensitive) == 0)
			break;
		curComp = this->getNextInternalComponent();
	}

	return curComp;
}


NMModelComponent* NMModelComponent::removeModelComponent(const QString& compName)
{
	NMDebugCtx(ctx, << "...");

	NMModelComponent* comp = this->findModelComponent(compName);
	if (comp == 0)
		return 0;

	NMDebugAI( << this->objectName().toStdString() << ": removing '"
			<< compName.toStdString() << "'" << std::endl);
	string tname = "";
	NMModelComponent* up = comp->getUpstreamModelComponent();
	tname = up != 0 ? up->objectName().toStdString() : "0";
	NMDebugAI( << "   up-comp is " << tname << std::endl);
	NMModelComponent* down = comp->getDownstreamModelComponent();
	tname = down != 0 ? down->objectName().toStdString() : "0";
	NMDebugAI( << "   down-comp is " << tname << std::endl);
	if (up == 0)// && comp == this->mProcessChainStart)
	{
		if (down != 0)
		{
			this->mProcessChainStart = down;
			down->setUpstreamModelComponent(0);
			NMDebugAI( << "   new start comp is '" << down->objectName().toStdString() << "'" << std::endl);
		}
		else
		{
			NMDebugAI( << comp->objectName().toStdString() << " is empty now" << std::endl);
			this->mProcessChainStart = 0;
		}
	}
	else if (up != 0)
	{
		up->setDownstreamModelComponent(down);
		NMDebugAI( << "   new down comp is '" << (down > 0 ? down->objectName().toStdString() : "0") << "'" << std::endl);
		if (down != 0)
		{
			down->setUpstreamModelComponent(up);
			NMDebugAI( << "   new up comp is '" << up->objectName().toStdString() << "'" << std::endl);
		}
	}

	//	if (this->mProcessChainPointer == comp)
	this->mProcessChainPointer = 0;

	// release all ties to the host's remaining
	// components
	comp->setHostComponent(0);
	comp->setUpstreamModelComponent(0);
	comp->setDownstreamModelComponent(0);

	NMDebugCtx(ctx, << "done!");
	emit NMModelComponentChanged();
	return comp;
}

void NMModelComponent::destroySubComponents(QMap<QString, NMModelComponent*>& repo)
{
	if (this->mProcess == 0)
	{
		NMModelComponent* sc = this->getInternalStartComponent();
		NMModelComponent* dc;
		while (sc != 0)
		{
			dc = sc;
			sc = sc->getNextInternalComponent();
			dc->destroySubComponents(repo);
			repo.remove(dc->objectName());
			delete dc;
		}
	}
	this->setInternalStartComponent(0);
}

void NMModelComponent::setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg)
{
	if (this->mProcess != 0)
	{
		this->mProcess->setNthInput(idx, inputImg);
		return;
	}

	NMModelComponent* sc = this->getInternalStartComponent();
	sc->getProcess()->setNthInput(idx, inputImg);
}

NMModelComponent* NMModelComponent::getLastInternalComponent(void)
{
	NMModelComponent* ret = 0;
	NMModelComponent* comp = this->getInternalStartComponent();
	while(comp != 0)
	{
		ret = comp;
		comp = this->getNextInternalComponent();
	}

	return ret;
}

void NMModelComponent::setTimeLevel(short level)
{
	int diff = level - this->mTimeLevel;
	this->changeTimeLevel(diff);
}

void NMModelComponent::changeTimeLevel(int diff)
{
	this->mTimeLevel += diff;
	if (this->mTimeLevel < 0)
		this->mTimeLevel = 0;

	NMModelComponent* comp = this->getInternalStartComponent();
	while (comp != 0)
	{
		comp->changeTimeLevel(diff);
		comp = this->getNextInternalComponent();
	}

	emit NMModelComponentChanged();
}

NMProcess* NMModelComponent::getEndOfTimeLevel(void)
{
	NMDebugCtx(ctx, << "...");

	NMDebugAI(<< this->objectName().toStdString() << ", level #"
			  << this->mTimeLevel << endl);

	NMProcess* proc = 0;
	if (this->getDownstreamModelComponent() != 0)
	{
		if (this->getDownstreamModelComponent()->getTimeLevel() == this->mTimeLevel)
		{
			proc  = this->getDownstreamModelComponent()->getEndOfTimeLevel();
		}
		else
		{
			NMModelComponent* lastComp = this->getLastInternalComponent();
			if (lastComp != 0 && lastComp->getTimeLevel() == this->mTimeLevel)
			{
				if (lastComp->getProcess() != 0)
				{
					proc = lastComp->getProcess();
					NMDebugAI(<< "gotcha! " << proc->objectName().toStdString() << endl);
				}
				else
				{
					NMModelComponent* lastlastComp = lastComp->getLastInternalComponent();
					if (lastlastComp != 0)
					{
						proc = lastlastComp->getEndOfTimeLevel();
					}
				}
			}
		}
	}
	else
	{
		NMModelComponent* lastComp = this->getLastInternalComponent();
		if (lastComp != 0 && lastComp->getTimeLevel() == this->mTimeLevel)
		{
			if (lastComp->getProcess() != 0)
			{
				proc = lastComp->getProcess();
				NMDebugAI(<< "gotcha! " << proc->objectName().toStdString() << endl);
			}
			else
			{
				NMModelComponent* lastlastComp = lastComp->getLastInternalComponent();
				if (lastlastComp != 0)
				{
					proc = lastlastComp->getEndOfTimeLevel();
				}
			}
		}
	}

	NMDebugCtx(ctx, << "done!");
	return proc;
}

NMItkDataObjectWrapper* NMModelComponent::getOutput(unsigned int idx)
{
	// check, whether we've got a 'single' process component
	if (this->mProcess != 0)
	{
		NMDebugAI(<< this->objectName().toStdString()
				<<"->getOutput(" << idx << ")" << std::endl);
		return this->mProcess->getOutput(idx);
	}

	NMProcess* proc = this->getEndOfTimeLevel();
	if (proc == 0)
	{
		NMErr(ctx, << this->objectName().toStdString() <<
				" ")
		return 0;
	}

	NMDebugAI(<< this->objectName().toStdString()
			<<"->getOutput(" << idx << ")" << std::endl);
	return proc->getOutput(idx);
}

NMModelComponent* NMModelComponent::getInternalStartComponent(void)
{
	if (this->mProcessChainStart == 0)
		return 0;

	this->mProcessChainPointer = this->mProcessChainStart;
	return this->mProcessChainPointer;
}

NMModelComponent* NMModelComponent::getNextInternalComponent(void)
{
	//ToDo: throw an exception, if the pointer has not been positioned properly
	if (this->mProcessChainPointer == 0)
		return 0;

	this->mProcessChainPointer = this->mProcessChainPointer->getDownstreamModelComponent();
	return this->mProcessChainPointer;
}

void NMModelComponent::initialiseComponents(void)
{
	NMDebugCtx(ctxNMModelComponent, << "...");

	if (this->mProcess != 0)
	{
		NMDebugAI(<< "instantiate process '" << this->mProcess->objectName().toStdString() << "'" << std::endl);
		this->mProcess->instantiateObject();
		NMDebugCtx(ctxNMModelComponent, << "done!");
		return;
	}

	NMModelComponent* mc = this->getInternalStartComponent();
	while (mc != 0)
	{
		NMDebugAI(<< "initialise sub components '" << mc->objectName().toStdString() << "': ... " << std::endl );
		mc->initialiseComponents();
		mc = this->getNextInternalComponent();
	}

	NMDebugCtx(ctxNMModelComponent, << "done!");
}

void NMModelComponent::linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctxNMModelComponent, << "...");

	if (this->mProcess != 0)
	{
		this->mProcess->linkInPipeline(step, repo);
		NMDebugCtx(ctxNMModelComponent, << "done!");
		return;
	}

	NMModelComponent* mc = this->getInternalStartComponent();
	while (mc != 0)
	{
		NMDebugAI(<< "link '" << mc->objectName().toStdString() << "' in pipeline ..." << std::endl);
		mc->linkComponents(step, repo);
		mc = this->getNextInternalComponent();
	}

	NMDebugCtx(ctxNMModelComponent, << "done!");
}

void NMModelComponent::mapTimeLevels(unsigned int startLevel,
		QMap<unsigned int, QMap<QString, NMModelComponent*> >& timeLevelMap)
{
	QMap<unsigned int, QMap<QString, NMModelComponent*> >::iterator it;
	if (this->mTimeLevel >= startLevel)
	{
		NMDebugAI(<< this->objectName().toStdString() << " is on time level " << this->mTimeLevel << std::endl);
		it = timeLevelMap.find(this->mTimeLevel);
		if (it != timeLevelMap.end())
		{
			it.value().insert(this->objectName(), this);
		}
		else
		{
			QMap<QString, NMModelComponent*> newLevel;
			newLevel.insert(this->objectName(), this);
			timeLevelMap.insert(this->mTimeLevel, newLevel);
		}
	}

	if (this->mTimeLevel == startLevel)
	{
		NMModelComponent* mc = this->getInternalStartComponent();
		while (mc != 0)
		{
			mc->mapTimeLevels(startLevel, timeLevelMap);
			mc = this->getNextInternalComponent();
		}

		//it = timeLevelMap.find(this->mTimeLevel);
		//if (it != timeLevelMap.end())
		//{
		//	it.value().remove(this->objectName());
		//}
	}
}


void NMModelComponent::update(const QMap<QString, NMModelComponent*>& repo)
{
	/**
	 * HERE IS WHAT WE DO for each iteration step
	 * - first we generate a map containing all sub components up to the next higher time level
	 * - call update on all sub components on a higher time level
	 * - link pipeline components and fetch input data for 'disconnected' sub components on the same
	 *   time level as the 'host' component
	 * - identify and update each (executable) component on the host's time level, which is a
	 *   process component and which doesn't serve as input to any of the other components on
	 *   the same time level (minimum time level for the host)
	 */

	NMDebugCtx(ctxNMModelComponent, << "...");
	NMDebugAI(<< "--- " << this->objectName().toStdString() << " ---" << std::endl);

	// check, whether we're supposed to run at all
	NMModelController* controller = qobject_cast<NMModelController*>(this->parent());
	if (controller != 0)
	{
		if (controller->isModelAbortionRequested())
		{
			NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
			NMDebugCtx(ctxNMModelComponent, << "done!");
			return;
		}
	}
	else
	{
		NMErr(ctxNMModelComponent,
				<< "We'd better quit here - there's no controller in charge!" << endl);
		NMDebugCtx(ctxNMModelComponent, << "done!");
		return;
	}

	this->mMapTimeLevelComp.clear();
	this->mapTimeLevels(this->mTimeLevel, this->mMapTimeLevelComp);
	unsigned int maxLevel = 0, minLevel = 0;
	QList<unsigned int> timeLevels = this->mMapTimeLevelComp.keys();
	maxLevel = timeLevels.back();
	minLevel = timeLevels.at(0);
	const QMap<QString, NMModelComponent*>& cmlm = this->mMapTimeLevelComp.value(minLevel);
	QMap<QString, NMModelComponent*>& minLevelMap =
			const_cast<QMap<QString, NMModelComponent*>& >(cmlm);
	minLevelMap.remove(this->objectName());
	if (minLevelMap.size() == 0)
		this->mMapTimeLevelComp.remove(minLevel);
	if (this->mMapTimeLevelComp.size() == 0)
	{
		NMDebugAI(<< "no sub components detected!" << std::endl);
	}

	//// DEBUG DEBUG DEBUG
	//foreach(const unsigned int& level, this->mMapTimeLevelComp.keys())
	//{
	//	NMDebugAI(<< "#" << level << " ... " << endl);
	//	foreach(const QString& name, this->mMapTimeLevelComp.value(level).keys())
	//	{
	//		NMDebugAI(<< "   " << name.toStdString() << endl);
	//	}
	//}
	// DEBUG DEBUG DEBUG

	// execute this components' pipeline
	for (unsigned int i=0; i < this->mNumIterations; ++i)
	{
		if (controller->isModelAbortionRequested())
		{
			NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
			NMDebugCtx(ctxNMModelComponent, << "done!");
			return;
		}

		NMDebugAI(<< "-------------------------------------------------------iteration " << i+1
				<< "/" << this->mNumIterations << std::endl);

		NMModelComponent* tmpComp = 0;

		// call update on all sub components on the next higher time level (does not matter
		// how much higher the nominal time level is, but we only look at the next higher
		// level flowing 'down' to the current level)
		unsigned int level = maxLevel;
		QMap<QString, NMModelComponent*>::const_iterator compIt;
		QMap<unsigned int, QMap<QString, NMModelComponent*> >::const_iterator timeIt;
		for(; level > minLevel; --level)
		{
			tmpComp = this->getInternalStartComponent();
			while (tmpComp != 0 && !controller->isModelAbortionRequested())
			{
				if (tmpComp->getTimeLevel() == level)
				{
					tmpComp->linkComponents(0, repo);
					tmpComp->update(repo);
				}
				tmpComp = this->getNextInternalComponent();
			}
		}

		// traverse all minLevel components and fetch input data from disconnected pipelines
		tmpComp = this->getInternalStartComponent();
		while (tmpComp != 0 && !controller->isModelAbortionRequested())
		{
			if (tmpComp->getTimeLevel() == minLevel)
				tmpComp->linkComponents(i, repo);
			tmpComp = this->getNextInternalComponent();
		}

		// update all executable components on the host's time level, or just call
		// update on this component's process object, if we're not hosting any other components
		//		NMDebugAI(<< this->objectName().toStdString() << ": iteration #" << i << std::endl);
		if (!controller->isModelAbortionRequested())
		{
			if (this->mProcess != 0)
			{
				NMDebugAI(<< "update " << this->objectName().toStdString() << "'s process..." << std::endl);
				if (!this->mProcess->isInitialised())
					this->mProcess->instantiateObject();
				this->mProcess->linkInPipeline(i, repo);
				this->mProcess->update();
			}
			else
			{
				QStringList execs = this->findExecutableComponents(this->mTimeLevel);
				foreach(const QString& cname, execs)
				{
					NMModelComponent* ec = NMModelController::getInstance()->getComponent(cname);
					if (ec == 0)
						continue;

					NMProcess* ep = ec->getProcess();
					if (ep == 0)
						continue;

					if (!ep->isInitialised())
						ep->instantiateObject();
					ep->update();
				}
			}
		}
	}

	NMDebugCtx(ctxNMModelComponent, << "done!");
}

const QStringList
NMModelComponent::findExecutableComponents(unsigned int timeLevel)
{
	// get the list of components for the specified time level
	QMap<QString, NMModelComponent*> levelComps =
			this->mMapTimeLevelComp.value(timeLevel);

	// we initially copy the list of keys
	QStringList execComps = levelComps.keys();

	//NMDebugAI(<< "raw execs: " << execComps.join(" ").toStdString() << endl);

	// now we subsequently remove all components, which are mentioned as input
	// of one of the given level's process components
	QMap<QString, NMModelComponent*>::iterator levelIt =
			levelComps.begin();
	while(levelIt != levelComps.end())
	{
		NMProcess* proc = levelIt.value()->getProcess();
		if (proc != 0)
		{
			QList<QStringList> allInputs = proc->getInputComponents();
			foreach(const QStringList& list, allInputs)
			{
				foreach(const QString& input, list)
				{
					if (execComps.contains(input))
					{
						execComps.removeOne(input);
					}
				}
			}
		}
		else
		{
			// we remove any aggregate components, since we've got all
			// executable sub-components covered already
			// (if everything went to plan ...); this is to avoid double
			// execution of components
			if (execComps.contains(levelIt.key()))
			{
				execComps.removeOne(levelIt.key());
			}
		}
		++levelIt;
	}

	NMDebugAI(<< "executables: " << execComps.join(" ").toStdString() << endl);
	return execComps;
}

void
NMModelComponent::reset(void)
{
	if (this->getProcess() != 0)
	{
		this->getProcess()->reset();
		return;
	}
	else
	{
		NMModelComponent* comp = this->getInternalStartComponent();
		while(comp != 0)
		{
			comp->reset();
			comp = this->getNextInternalComponent();
		}
	}
}

void NMModelComponent::getEndOfPipelineProcess(NMProcess*& endProc)
{
	NMDebugAI(<< this->objectName().toStdString()
			<< ": looking for the end of the pipe" << std::endl);

	NMModelComponent* lastComp = 0;
	NMModelComponent* lastValid = this->mDownComponent;
	NMModelComponent* nextInternal = this->getInternalStartComponent();
	if (lastValid != 0)
	{
		lastComp = lastValid->mDownComponent;
		while (lastComp != 0)
		{
			NMDebugAI(<< "   ... passing "
					<< lastValid->objectName().toStdString()
					<< std::endl);
			lastValid = lastComp;
			lastComp = lastComp->mDownComponent;
		}
		lastValid->getEndOfPipelineProcess(endProc);
	}
	else if (nextInternal != 0)
	{
		nextInternal->getEndOfPipelineProcess(endProc);
	}
	else if (this->mProcess != 0)
	{
		NMDebugAI(<< "-> found "
				<< this->mProcess->objectName().toStdString()
				<< " to be the end" << std::endl)
		endProc = this->mProcess;
	}
}

bool NMModelComponent::isSubComponent(NMModelComponent* comp)
{
	bool ret = false;

	QMap<unsigned int, QMap<QString, NMModelComponent*> >::const_iterator timeIt =
			this->mMapTimeLevelComp.begin();
	QMap<QString, NMModelComponent*>::const_iterator compIt;
	for (; timeIt != this->mMapTimeLevelComp.end(); ++timeIt)
	{
		compIt = timeIt.value().begin();
		for (; compIt != timeIt.value().end(); ++compIt)
		{
			if (compIt.value()->objectName().compare(comp->objectName()) == 0)
			{
				ret = true;
				break;
			}
		}
	}

	return ret;
}
