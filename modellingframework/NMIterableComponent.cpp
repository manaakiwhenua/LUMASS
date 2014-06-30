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
#include "NMIterableComponent.h"

#include <string>
#include <iostream>
#include <sstream>
#include "nmlog.h"

#include "NMModelController.h"
#include "NMDataComponent.h"
#include "NMMfwException.h"
#include "NMProcessFactory.h"

const std::string NMIterableComponent::ctx = "NMIterableComponent";

NMIterableComponent::NMIterableComponent(QObject* parent)
{
	this->setParent(parent);
	this->initAttributes();
}

void NMIterableComponent::initAttributes(void)
{
	NMModelComponent::initAttributes();
	this->mMaxInternalTimeLevel = 0;
    this->mProcessChainPointer = 0;
    this->mProcessChainStart = 0;
    this->mProcess = 0;
    this->mIterationStep = 1;
    this->mIterationStepRun = 1;
}

NMIterableComponent::~NMIterableComponent(void)
{
}

void NMIterableComponent::setProcess(NMProcess* proc)
{
//	NMDebugCtx(ctx, << "...");

	this->mProcess = proc;
	this->mProcessChainPointer = 0;
	this->mProcessChainStart = 0;
	proc->setParent(0);
	proc->moveToThread(this->thread());
	proc->setParent(this);

//	NMDebugCtx(ctx, << "done!");

	emit NMModelComponentChanged();
}

void
NMIterableComponent::setInputs(const QList<QStringList>& inputs)
{
	this->mInputs = inputs;
	if (this->mProcess)
		this->mProcess->setInputComponents(inputs);

	emit NMModelComponentChanged();
}

const QList<QStringList>
NMIterableComponent::getInputs(void)
{
	if (this->mProcess)
		return this->mProcess->getInputComponents();
	else
		return this->mInputs;
}


int NMIterableComponent::countComponents(void)
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


void NMIterableComponent::addModelComponent(NMModelComponent* comp)
{
//	NMDebugCtx(ctx, << "...");

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

//	NMDebugAI(<< "parent: '" << this->objectName().toStdString() << "'" << std::endl);
//	NMDebugAI(<< "add child:  '" << comp->objectName().toStdString() << "'" << std::endl);
//	NMDebugAI(<< "new chain: ");
	comp->setHostComponent(this);

	NMModelComponent* lastComp = this->getInternalStartComponent();
	if (lastComp == 0)
	{
		this->mProcessChainStart = comp;
//		NMDebug(<< "'" << comp->objectName().toStdString() << "'" << std::endl);
//		NMDebugCtx(ctx, << "done!");
		return;
	}
//	NMDebug(<< " '" << lastComp->objectName().toStdString() << "'");

	while (lastComp->getDownstreamModelComponent() != 0)
	{
		lastComp = this->getNextInternalComponent();
//		NMDebug(<< " '" << lastComp->objectName().toStdString() << "'");
	}
//	NMDebug(<< "'" << comp->objectName().toStdString() << "'" << std::endl);

	lastComp->setDownstreamModelComponent(comp);
	comp->setUpstreamModelComponent(lastComp);

	emit NMModelComponentChanged();

//	NMDebugCtx(ctx, << "done!");
}


void
NMIterableComponent::createExecSequence(QList<QStringList>& execList,
		unsigned int timeLevel, int step)
{
	//NMDebugCtx(this->objectName().toStdString(), << "...");

	// identify all available ends within this group component with
	// respect to the container's time level
	QStringList deadEnds = this->findExecutableComponents(timeLevel, step);

	// find for each of those ends the upstream pipeline until they
	// hit the next upstream non-piped component
	//QList<QStringList> subPipes;
	foreach(const QString de, deadEnds)
	{
		NMModelComponent* comp = NMModelController::getInstance()->getComponent(de);
		if (comp == 0)
			continue;

		QStringList upstreamPipe;
		upstreamPipe.push_front(comp->objectName());
		comp->getUpstreamPipe(execList, upstreamPipe, step);
		execList.push_back(upstreamPipe);
	}

	//NMDebugCtx(this->objectName().toStdString(), << "done!");
}

int
NMIterableComponent::isInExecList(const QList<QStringList>& execList,
		const QString& compName)
{
	int foundyou = -1;
	int cnt = 0;
	foreach(const QStringList& lst, execList)
	{
		if (lst.contains(compName))
		{
			foundyou = cnt;
			break;
		}
		++cnt;
	}
	return foundyou;
}

int
NMIterableComponent::isCompExecComp(const QList<QStringList>& execList,
		const QString& compName)
{
	int idx = -1;

	int cnt = 0;
	foreach (const QStringList lst, execList)
	{
		if (lst.last().compare(compName) == 0)
		{
			NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(
					NMModelController::getInstance()->getComponent(compName));
			if (ic != 0 && ic->getProcess() != 0)
			{
				idx = cnt;
				break;
			}
		}
		++cnt;
	}

	return idx;
}

std::vector<int>
NMIterableComponent::findSourceComp(const QList<QStringList>& execList,
		const QString& compName)
{
	std::vector<int> indices;
	indices.push_back(-1);
	indices.push_back(-1);

	int cnt_outer = 0;
	int cnt_inner = 0;
	bool bfound = false;
	foreach(const QStringList& level, execList)
	{
		foreach(const QString& part, level)
		{
			if (part.compare(compName) == 0)
			{
				indices[1] = cnt_inner;
				break;
			}

			++cnt_inner;
		}

		if (indices[1] != -1)
		{
			indices[0] = cnt_outer;
			break;
		}

		++cnt_outer;
	}

	return indices;
}

std::vector<int>
NMIterableComponent::findTargetComp(const QList<QStringList>& execList,
		const QString& compName, int step)
{
	NMDebugCtx(ctx, << "...");

	NMDebugAI(<< "looking for a target component for '"
			<< compName.toStdString() << "' on iter #" << step << endl);

	std::vector<int> indices;
	indices.push_back(-1);
	indices.push_back(-1);

	NMModelController* ctrl = NMModelController::getInstance();
	NMModelComponent* receiver = 0;
	int cnt_inner = 0;
	int cnt_outer = 0;
	foreach(const QStringList& lst, execList)
	{
		NMDebugAI(<< "looking at execSequence pos #"
				<< cnt_outer << endl);
		foreach(const QString& s, lst)
		{
			NMDebugAI(<< "   looking at comp '"
					<< s.toStdString() << "'" << endl);
			receiver = ctrl->getComponent(s);
			if (receiver == 0)
			{
				++cnt_inner;
				continue;
			}

			QList<QStringList> inputs = receiver->getInputs();
			if (inputs.size() == 0)
			{
				++cnt_inner;
				continue;
			}

			QStringList theList;
			if (step < inputs.size())
				theList = inputs.at(step);

			if (theList.contains(compName))
			{
				NMDebugAI( << "   found at #" <<
						cnt_inner << endl);
				indices[1] = cnt_inner;
				break;
			}

			++cnt_inner;
		}

		if (indices[1] != -1)
		{
			indices[0] = cnt_outer;
			break;
		}

		++cnt_outer;
	}

	NMDebugCtx(ctx, << "done!");
	return indices;
}

void NMIterableComponent::insertModelComponent(NMModelComponent* insertComp,
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


NMModelComponent* NMIterableComponent::findModelComponent(const QString& compName)
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


NMModelComponent* NMIterableComponent::removeModelComponent(const QString& compName)
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

void NMIterableComponent::destroySubComponents(QMap<QString, NMModelComponent*>& repo)
{
	if (this->mProcess == 0)
	{
		NMModelComponent* sc = this->getInternalStartComponent();
        NMIterableComponent* dc = 0;
		while (sc != 0)
		{
            dc = static_cast<NMIterableComponent*>(sc);
			if (dc != 0)
            {
				dc->destroySubComponents(repo);
                repo.remove(dc->objectName());
                delete dc;
                dc = 0;
            }

            sc = this->getNextInternalComponent();
            if (sc != 0)
            {
                repo.remove(sc->objectName());
                delete sc;
                sc = 0;
            }
		}
	}
	this->setInternalStartComponent(0);
}

void NMIterableComponent::setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg)
{
	if (this->mProcess != 0)
	{
		this->mProcess->setNthInput(idx, inputImg);
		return;
	}

	NMModelComponent* sc = this->getInternalStartComponent();
	sc->setNthInput(idx, inputImg);
}

NMModelComponent* NMIterableComponent::getLastInternalComponent(void)
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

//void NMModelComponent::setTimeLevel(short level)
//{
//	int diff = level - this->mTimeLevel;
//	this->changeTimeLevel(diff);
//}
//
//void NMModelComponent::changeTimeLevel(int diff)
//{
//	this->mTimeLevel += diff;
//	if (this->mTimeLevel < 0)
//		this->mTimeLevel = 0;
//
//	NMModelComponent* comp = this->getInternalStartComponent();
//	while (comp != 0)
//	{
//		comp->changeTimeLevel(diff);
//		comp = this->getNextInternalComponent();
//	}
//
//	emit NMModelComponentChanged();
//}

/*
NMModelComponent* NMIterableComponent::getEndOfTimeLevel(void)
{
	NMDebugCtx(ctx, << "...");

	// we go right to the end of this component's time level
	// and check; when we've got a non-iterable component, we
	// return the very component, otherwise, we ask for the
	// last component's end of time level


	NMDebugAI(<< this->objectName().toStdString() << ", level #"
			  << this->mTimeLevel << endl);

	NMModelComponent* ret = 0;
	NMModelComponent* tmc = this;
	NMModelComponent* lmc = 0;
	while(tmc != 0 && tmc->getTimeLevel() == this->mTimeLevel)
	{
		lmc = tmc;
		tmc = lmc->getDownstreamModelComponent();
	}

	NMIterableComponent* itmc = qobject_cast<NMIterableComponent*>(lmc);
	if (itmc != 0)
	{
		ret = itmc->getEndOfTimeLevel();
	}
	else
		ret = lmc;

	//NMProcess* proc = 0;
	//if (this->getDownstreamModelComponent() != 0)
	//{
	//	NMIterableComponent* dmc = qobject_cast<NMIterableComponent*>(this->getDownstreamModelComponent());
	//	if (dmc->getTimeLevel() == this->mTimeLevel)
	//	{
	//		proc  = dmc->getEndOfTimeLevel();
	//	}
	//	else
	//	{
	//		NMIterableComponent* lastComp =
	//				qobject_cast<NMIterableComponent*>(this->getLastInternalComponent());
	//		if (lastComp != 0 && lastComp->getTimeLevel() == this->mTimeLevel)
	//		{
	//			if (lastComp->getProcess() != 0)
	//			{
	//				proc = lastComp->getProcess();
	//				NMDebugAI(<< "gotcha! " << proc->objectName().toStdString() << endl);
	//			}
	//			else
	//			{
	//				NMIterableComponent* lastlastComp =
	//						qobject_cast<NMIterableComponent*>(lastComp->getLastInternalComponent());
	//				if (lastlastComp != 0)
	//				{
	//					proc = lastlastComp->getEndOfTimeLevel();
	//				}
	//			}
	//		}
	//	}
	//}
	//else
	//{
	//	NMIterableComponent* lastComp =
	//			qobject_cast<NMIterableComponent*>(this->getLastInternalComponent());
	//	if (lastComp != 0 && lastComp->getTimeLevel() == this->mTimeLevel)
	//	{
	//		if (lastComp->getProcess() != 0)
	//		{
	//			proc = lastComp->getProcess();
	//			NMDebugAI(<< "gotcha! " << proc->objectName().toStdString() << endl);
	//		}
	//		else
	//		{
	//			NMIterableComponent* lastlastComp =
	//					qobject_cast<NMIterableComponent*>(lastComp->getLastInternalComponent());
	//			if (lastlastComp != 0)
	//			{
	//				proc = lastlastComp->getEndOfTimeLevel();
	//			}
	//		}
	//	}
	//}

	NMDebugCtx(ctx, << "done!");
	return ret;
}
*/

NMItkDataObjectWrapper*
NMIterableComponent::getOutput(unsigned int idx)
{
	// check, whether we've got a 'single' process component
	if (this->mProcess != 0)
	{
		NMDebugAI(<< this->objectName().toStdString()
				<<"->getOutput(" << idx << ")" << std::endl);
		return this->mProcess->getOutput(idx);
	}

	// ToDo: double check this approach!
//	NMModelComponent* eotComp = this->getEndOfTimeLevel();
//	if (eotComp != 0)
//	{
//		return eotComp->getOutput(idx);
//	}

	NMErr(this->objectName().toStdString(), << this->objectName().toStdString() <<
			" - Couldn't fetch any output!");
	return 0;

}

NMModelComponent* NMIterableComponent::getInternalStartComponent(void)
{
	if (this->mProcessChainStart == 0)
		return 0;

	this->mProcessChainPointer = this->mProcessChainStart;
	return this->mProcessChainPointer;
}

NMModelComponent* NMIterableComponent::getNextInternalComponent(void)
{
	//ToDo: throw an exception, if the pointer has not been positioned properly
	if (this->mProcessChainPointer == 0)
		return 0;

	this->mProcessChainPointer = this->mProcessChainPointer->getDownstreamModelComponent();
	return this->mProcessChainPointer;
}

void NMIterableComponent::initialiseComponents(unsigned int timeLevel)
{
//	NMDebugCtx(this->objectName().toStdString(), << "...");

	if (this->mProcess != 0 && this->getTimeLevel() == timeLevel)
	{
//		NMDebugAI(<< "instantiate process '" << this->mProcess->objectName().toStdString() << "'" << std::endl);
		this->mProcess->instantiateObject();
//		NMDebugCtx(this->objectName().toStdString(), << "done!");
		return;
	}

	NMModelComponent* mc = this->getInternalStartComponent();
	while (mc != 0)
	{
		NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(mc);
		if (ic != 0 && ic->getTimeLevel() == timeLevel)
		{
//			NMDebugAI(<< "initialise '" << mc->objectName().toStdString()
//					 << "' ... " << std::endl );
			ic->initialiseComponents(timeLevel);
		}
		mc = this->getNextInternalComponent();
	}

//	NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void
NMIterableComponent::changeTimeLevel(int diff)
{
	// we call the superclass implementation first
	NMModelComponent::changeTimeLevel(diff);

	NMModelComponent* comp = this->getInternalStartComponent();
	while (comp != 0)
	{
		comp->changeTimeLevel(diff);
		comp = this->getNextInternalComponent();
	}

    //emit TimeLevelChanged(this->mTimeLevel);
    //emit NMModelComponentChanged();
}


void NMIterableComponent::linkComponents(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(this->objectName().toStdString(), << "...");

	if (this->mProcess != 0)
	{
		this->mProcess->linkInPipeline(step, repo);
		NMDebugCtx(this->objectName().toStdString(), << "done!");
		return;
	}

	//NMModelComponent* mc = this->getInternalStartComponent();
	//while (mc != 0)
	//{
	//	NMDebugAI(<< "link '" << mc->objectName().toStdString() << "' in pipeline ..." << std::endl);
	//	mc->linkComponents(step, repo);
	//	mc = this->getNextInternalComponent();
	//}

	NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void NMIterableComponent::mapTimeLevels(unsigned int startLevel,
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
//			NMIterableComponent* ic =
//					qobject_cast<NMIterableComponent*>(mc);
//			if (ic != 0)
//			{
//				ic->mapTimeLevels(startLevel, timeLevelMap);
//			}
//			else
			{
				if (mc->getTimeLevel() >= startLevel)
				{
					it = timeLevelMap.find(mc->getTimeLevel());
					if (it != timeLevelMap.end())
					{
						it.value().insert(mc->objectName(), mc);
					}
					else
					{
						QMap<QString, NMModelComponent*> newLevel;
						newLevel.insert(mc->objectName(), mc);
						timeLevelMap.insert(mc->getTimeLevel(), newLevel);
					}
				}
			}
			mc = this->getNextInternalComponent();
		}
	}
}


void NMIterableComponent::update(const QMap<QString, NMModelComponent*>& repo)
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

	NMDebugCtx(this->objectName().toStdString(), << "...");
	NMDebugAI(<< "--- " << this->objectName().toStdString() << " ---" << std::endl);

    emit signalExecutionStarted();

	// check, whether we're supposed to run at all
	NMModelController* controller = qobject_cast<NMModelController*>(this->parent());
	if (controller != 0)
	{
		if (controller->isModelAbortionRequested())
		{
			NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
			NMDebugCtx(this->objectName().toStdString(), << "done!");
            emit signalExecutionStopped();
			return;
		}
	}
	else
	{
		NMErr(this->objectName().toStdString(),
				<< "We'd better quit here - there's no controller in charge!" << endl);
		NMDebugCtx(this->objectName().toStdString(), << "done!");
        emit signalExecutionStopped();
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

    // DEBUG DEBUG DEBUG
    foreach(const unsigned int& level, this->mMapTimeLevelComp.keys())
    {
        NMDebugAI(<< "#" << level << " ... " << endl);
        foreach(const QString& name, this->mMapTimeLevelComp.value(level).keys())
        {
            NMDebugAI(<< "   " << name.toStdString() << endl);
        }
    }
    // DEBUG DEBUG DEBUG

	// execute this components' pipeline
//    try
//    {
        this->iterativeComponentUpdate(repo, minLevel, maxLevel);
//    }
//    catch (std::exception& e)
//    {
//        emit signalExecutionStopped();
//        NMDebugCtx(this->objectName().toStdString(), << "done!");
//        throw e;
//    }

    emit signalExecutionStopped();
	NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void
NMIterableComponent::componentUpdateLogic(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel, unsigned int step)
{
	NMDebugCtx(this->objectName().toStdString(), << "...");
	NMModelController* controller = NMModelController::getInstance();
	unsigned int i = step;

	if (controller->isModelAbortionRequested())
	{
		NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
		NMDebugCtx(this->objectName().toStdString(), << "done!");
		return;
	}

	NMDebugAI(<< ">>>> START ITERATION #" << i+1 << std::endl);

    try
    {

	// when we've got a process component, the process is straightforward
	// and we just link and execute this one
	if (this->mProcess != 0)
	{
		NMDebugAI(<< "update " << this->objectName().toStdString() << "'s process..." << std::endl);
		if (!this->mProcess->isInitialised())
			this->mProcess->instantiateObject();
		this->mProcess->linkInPipeline(i, repo);
        this->mProcess->update();
		NMDebugCtx(this->objectName().toStdString(), << "done!");
		return;
	}


	// we identify and execute processing pipelines and components
	// on each individual time level separately, from the highest
	// timelevel to the  lowest timelevel
	QMap<QString, NMModelComponent*>::const_iterator compIt;
	QMap<unsigned int, QMap<QString, NMModelComponent*> >::const_iterator timeIt;
	for (long level = (long)maxLevel;
		 level > -1 && level >= minLevel && !controller->isModelAbortionRequested();
		 --level)
	{
		// we initialise all involved components, so we can actually
		// identify, which one is an ITK/OTB-derived process component
		// or  not
		NMDebugAI(<< "initialising process components on level "
				  << level << " ..." << endl);
		this->initialiseComponents((unsigned int)level);

		QList<QStringList> execList;
		this->createExecSequence(execList, level, step);

		///////////////// DEBUG
		// let's have a look what we've got so far ...
		NMDebug(<< endl);
		NMDebugAI(<< "PIPELINES AND EXECUTION ORDER ON LEVEL "
                  << level << " ------------" << endl);
		int cnt = 0;
		foreach(const QStringList& sp, execList)
		{
			NMDebugAI(<< "#" << cnt << ": " << sp.join("-->").toStdString() << endl);
			++cnt;
		}
		NMDebug(<< endl);
		////////////////// DEBUG

        // for each pipeline, we first link each individual component
		// (from head to toe), before we finally call update on the
		// last (i.e. executable) component of the pipeline
		foreach(const QStringList& pipeline, execList)
		{
			NMModelComponent* comp = 0;
			foreach(const QString in, pipeline)
			{
				comp = controller->getComponent(in);
				if (comp == 0)
				{
					NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
					std::stringstream msg;
					msg << "'" << in.toStdString() << "'";
					e.setMsg(msg.str());
					throw e;
				}
                comp->linkComponents(step, repo);
			}

			// calling update on the last component of the pipeline
			// (the most downstream)
			if (!controller->isModelAbortionRequested())
			{
				NMDebugAI(<< "calling " << pipeline.last().toStdString()
						<< "::update() ..." << endl);
                comp->update(repo);
			}
			else
			{
				NMDebugAI(<< ">>>> END ITERATION #" << i+1 << std::endl);
				NMDebugCtx(this->objectName().toStdString(), << "done!");
				return;
			}
		}
	}

    NMDebugAI(<< ">>>> END ITERATION #" << i+1 << std::endl);
    NMDebugCtx(this->objectName().toStdString(), << "done!");

    }
    catch (std::exception& e)
    {
        NMMfwException re(NMMfwException::Unspecified);
        re.setMsg(e.what());
        NMDebugCtx(this->objectName().toStdString(), << "done!");
        emit signalExecutionStopped();
        throw re;
    }
}

const QStringList
NMIterableComponent::findExecutableComponents(unsigned int timeLevel,
		int step)
{
	// get the list of components for the specified time level
	QMap<QString, NMModelComponent*> levelComps =
			this->mMapTimeLevelComp.value(timeLevel);

	// we initially copy the list of keys
	QStringList execComps = levelComps.keys();

	NMDebugAI(<< "looking for executables amongst level "
			  << timeLevel << " components ... " << std::endl);
	NMDebugAI( << "... " << execComps.join(" ").toStdString() << std::endl);

	// remove the calling component itself
	if (execComps.contains(this->objectName()))
	{
		execComps.removeOne(this->objectName());
	}

	NMModelController* ctrl = NMModelController::getInstance();

	//NMDebugAI(<< "raw execs: " << execComps.join(" ").toStdString() << endl);

	// now we subsequently remove all components, which are mentioned as input
	// of one of the given level's other components;
    // we also look for any data components we can find, in case
	// we haven't got 'dead ends' but a cyclic relationship whereas a
	// data buffer component is updated subsequently
	QStringList dataBuffers;

	QMap<QString, NMModelComponent*>::iterator levelIt =
			levelComps.begin();
	while(levelIt != levelComps.end())
	{
        // we only execute 'non-sink' processes, DataBuffers or
        // aggregate components
        QString cn = levelIt.key();
        if (    !cn.startsWith(QString::fromLatin1("DataBuffer"))
            &&  !cn.startsWith(QString::fromLatin1("AggrComp"))
            &&  !NMProcessFactory::instance().isSink(cn)
           )
        {
            execComps.removeOne(levelIt.key());
            NMDebugAI(<< "removed non-executable '" << levelIt.key().toStdString() << "' from executables"
                      << std::endl);
            ++levelIt;
            continue;
        }


        NMDataComponent* buf = qobject_cast<NMDataComponent*>(levelIt.value());
		if (buf != 0)
		{
			dataBuffers.push_back(buf->objectName());
		}

		QList<QStringList> allInputs = levelIt.value()->getInputs();
		if (allInputs.size() == 0)
		{
			++levelIt;
			continue;
		}

        // determine step to determine the input link for this component

        // here: conventional approach
        if (step > allInputs.size()-1)
        {
            // ToDo: this might need to be adjusted when we
            // introduce the choice between 'use_up | cyle | <other>'
            // for now, we make it NM_USE_UP
            step = allInputs.size()-1;
        }

        // in case we're looking at a process component, we account for its index policy
        // (i.e. USE_UP | CYCLIC | SYNC_WITH_HOST)
        // and for its current iteration step
        NMIterableComponent* procComp = qobject_cast<NMIterableComponent*>(levelIt.value());
        if (procComp)
        {
            if (procComp->getProcess())
            {
                step = procComp->getProcess()->mapHostIndexToPolicyIndex(step, allInputs.size());
            }
        }

		QStringList inputs = allInputs.at(step);
		foreach(const QString& in, inputs)
		{
//            NMModelComponent* c = ctrl->getComponent(in);
//            NMModelComponent* hc = c ? c->getHostComponent() : 0;

            if (execComps.contains(in))
			{
				execComps.removeOne(in);
			}
            //experimental
            // we only account for those components, which are
            // actually direct children of THIS iterable
            // component; all nested components are run
            //
//            else if (hc != 0 && hc != this)
//            {
//                execComps.removeOne(in);
//            }
		}

		++levelIt;
	}

	// if the execComps list is empty, we shall just return all data buffers (=stock component)
	// which could be used to iteratively calculate its status whereby the status itself
	// is input to some sort of calculation, hence the above applied definition of
	// 'executable' doesn't hold any more. Potentially, this could also include any
	// other type of 'non-piped' components, but so far, we've got only the data
	// buffers....
	if (execComps.size() == 0)
	{
		NMDebugAI(<< "executables: " << dataBuffers.join(" ").toStdString() << endl);
		return dataBuffers;
	}
	else
	{
		NMDebugAI(<< "executables: " << execComps.join(" ").toStdString() << endl);
		return execComps;
	}
}

void
NMIterableComponent::reset(void)
{
    this->mIterationStepRun = this->mIterationStep;
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

void
NMIterableComponent::setIterationStep(unsigned int step)
{
    this->mIterationStep = step == 0 ? 1 : step;
    this->mIterationStepRun = this->mIterationStep;
}

unsigned int
NMIterableComponent::getIterationStep(void)
{
    if (NMModelController::getInstance()->isModelRunning())
        return this->mIterationStepRun;
    else
        return this->mIterationStep;
}

/*
void NMIterableComponent::getEndOfPipelineProcess(NMProcess*& endProc)
{
	NMDebugAI(<< this->objectName().toStdString()
			<< ": looking for the end of the pipe" << std::endl);

	NMIterableComponent* ic = 0;
	NMModelComponent* lastComp = 0;
	NMModelComponent* lastValid = this->mDownComponent;
	NMModelComponent* nextInternal = this->getInternalStartComponent();
	if (lastValid != 0)
	{
		lastComp = lastValid->getDownstreamModelComponent();
		while (lastComp != 0)
		{
			NMDebugAI(<< "   ... passing "
					<< lastValid->objectName().toStdString()
					<< std::endl);
			lastValid = lastComp;
			lastComp = lastComp->getDownstreamModelComponent();
		}
		ic = qobject_cast<NMIterableComponent*>(lastValid);
		if (ic != 0)
			ic->getEndOfPipelineProcess(endProc);
	}
	else if (nextInternal != 0)
	{
		ic = qobject_cast<NMIterableComponent*>(nextInternal);
		if (ic != 0)
			ic->getEndOfPipelineProcess(endProc);
	}
	else if (this->mProcess != 0)
	{
		NMDebugAI(<< "-> found "
				<< this->mProcess->objectName().toStdString()
				<< " to be the end" << std::endl)
		endProc = this->mProcess;
	}
}
*/

bool NMIterableComponent::isSubComponent(NMModelComponent* comp)
{
    bool bIsSub = false;

/// this commented code below doesn't work any more, since we now only map
/// time levels within the bounds of an IterableComponent

//	QMap<unsigned int, QMap<QString, NMModelComponent*> >::const_iterator timeIt =
//			this->mMapTimeLevelComp.begin();
//	QMap<QString, NMModelComponent*>::const_iterator compIt;
//	for (; timeIt != this->mMapTimeLevelComp.end(); ++timeIt)
//	{
//		compIt = timeIt.value().begin();
//		for (; compIt != timeIt.value().end(); ++compIt)
//		{
//			if (compIt.value()->objectName().compare(comp->objectName()) == 0)
//			{
//				ret = true;
//				break;
//			}
//		}
//	}

    NMModelComponent* c = this->getInternalStartComponent();
    while(c != 0 && !bIsSub)
    {
        if (comp->objectName().compare(c->objectName()) == 0)
        {
            bIsSub = true;
            break;
        }

        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(c);
        if (ic)
        {
            bIsSub = ic->isSubComponent(comp);
        }
        c = this->getNextInternalComponent();
    }

    return bIsSub;
}
