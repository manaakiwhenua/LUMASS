/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMSequentialIterComponent.cpp
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */

#include "NMSequentialIterComponent.h"
#include "NMDataComponent.h"
#include "NMModelController.h"

const std::string NMSequentialIterComponent::ctx = "NMSequentialIterComponent";

NMSequentialIterComponent::NMSequentialIterComponent(QObject* parent)
	: mNumIterations(1)
{
	this->setParent(parent);
	this->initAttributes();
}

NMSequentialIterComponent::~NMSequentialIterComponent()
{
}

void
NMSequentialIterComponent::setNumIterations(unsigned int numiter)
{
    if (this->mNumIterations != numiter && numiter > 0)
    {
        this->mNumIterations = numiter;
        emit NumIterationsChanged(numiter);
    }
}

void
NMSequentialIterComponent::iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel)
{


	//NMModelController* controller = NMModelController::getInstance();

	for (unsigned int i=0; i < this->mNumIterations; ++i)
	{
		this->componentUpdateLogic(repo, minLevel, maxLevel, i);

//		if (controller->isModelAbortionRequested())
//		{
//			NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
//			NMDebugCtx(ctx, << "done!");
//			return;
//		}
//
//		NMDebugAI(<< "-------------------------------------------------------iteration " << i+1
//				<< "/" << this->mNumIterations << std::endl);
//
//		NMModelComponent* tmpComp = 0;
//
//		// call update on all sub components on the next higher time level (does not matter
//		// how much higher the nominal time level is, but we only look at the next higher
//		// level flowing 'down' to the current level)
//		unsigned int level = maxLevel;
//		QMap<QString, NMModelComponent*>::const_iterator compIt;
//		QMap<unsigned int, QMap<QString, NMModelComponent*> >::const_iterator timeIt;
//		for(; level > minLevel; --level)
//		{
//			tmpComp = this->getInternalStartComponent();
//			while (tmpComp != 0 && !controller->isModelAbortionRequested())
//			{
//				if (tmpComp->getTimeLevel() == level)
//				{
//					tmpComp->linkComponents(0, repo);
//					tmpComp->update(repo);
//				}
//				tmpComp = this->getNextInternalComponent();
//			}
//		}
//
//		// traverse all minLevel components and fetch input data from disconnected pipelines
//		// in case of data components, we call update to fetch the data
//		tmpComp = this->getInternalStartComponent();
//		while (tmpComp != 0 && !controller->isModelAbortionRequested())
//		{
//			if (tmpComp->getTimeLevel() == minLevel)
//			{
//				tmpComp->linkComponents(i, repo);
//				if (qobject_cast<NMDataComponent*>(tmpComp))
//					tmpComp->update(repo);
//			}
//			tmpComp = this->getNextInternalComponent();
//		}
//
//		// update all executable components on the host's time level, or just call
//		// update on this component's process object, if we're not hosting any other components
//		//		NMDebugAI(<< this->objectName().toStdString() << ": iteration #" << i << std::endl);
//		if (!controller->isModelAbortionRequested())
//		{
//			if (this->mProcess != 0)
//			{
//				NMDebugAI(<< "update " << this->objectName().toStdString() << "'s process..." << std::endl);
//				if (!this->mProcess->isInitialised())
//					this->mProcess->instantiateObject();
//				this->mProcess->linkInPipeline(i, repo);
//				this->mProcess->update();
//			}
//			else
//			{
//				QStringList execs = this->findExecutableComponents(this->mTimeLevel);
//				foreach(const QString& cname, execs)
//				{
//					NMIterableComponent* ec = qobject_cast<NMIterableComponent*>(
//							NMModelController::getInstance()->getComponent(cname));
//					if (ec == 0)
//						continue;
//
//					NMProcess* ep = ec->getProcess();
//					if (ep == 0)
//						continue;
//
//					if (!ep->isInitialised())
//						ep->instantiateObject();
//					ep->update();
//				}
//			}
//		}
	}
}

