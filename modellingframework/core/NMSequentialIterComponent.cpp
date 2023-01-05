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
#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMSequentialIterComponent.h"
#include "NMDataComponent.h"
#include "NMModelController.h"
#include "NMMfwException.h"

const std::string NMSequentialIterComponent::ctx = "NMSequentialIterComponent";

NMSequentialIterComponent::NMSequentialIterComponent(QObject* parent)
{
	this->setParent(parent);
    NMIterableComponent::initAttributes();
}

NMSequentialIterComponent::~NMSequentialIterComponent()
{
}

void NMSequentialIterComponent::setProcess(NMProcess* proc)
{
//	NMDebugCtx(ctx, << "...");

    this->mProcess = proc;
    this->mProcessChainPointer = nullptr;
    this->mProcessChainStart = nullptr;
    if (this->mProcess)
    {
        if (mLogger)
        {
            this->mProcess->setLogger(mLogger);
        }
        this->mProcess->setParent(0);
        this->mProcess->moveToThread(this->thread());
        this->mProcess->setParent(this);
        this->mProcess->setModelController(this->getModelController());
    }

//	NMDebugCtx(ctx, << "done!");

    emit NMModelComponentChanged();
}

//void
//NMSequentialIterComponent::linkComponents(unsigned int step, const QMap<QString, NMModelComponent *> &repo)
//{
//    NMDebugCtx(this->objectName().toStdString(), << "...");
//
//    this->processUserID();
//    //mOldIterations = mNumIterations;
//    mNumIterations = this->evalNumIterationsExpression(mIterationStep);
//    //NMDebugAI(<< getUserID().toStdString() << ": pre-loop: IterStep=" << getIterationStep() << " niter=" << mNumIterations;)
//    //NMLogDebug(<< getUserID().toStdString() << ": pre-loop: IterStep=" << getIterationStep() << " niter=" << mNumIterations);
//
//    if (this->mProcess != 0)
//    {
//        this->mProcess->linkInPipeline(step, repo);
//        NMDebugCtx(this->objectName().toStdString(), << "done!");
//        return;
//    }
//
//    NMDebugCtx(this->objectName().toStdString(), << "done!");
//}

void
NMSequentialIterComponent::iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel)
{
    //    unsigned int oldNumIter = mNumIterations;
    //    unsigned int niter = evalNumIterationsExpression(mIterationStep);
    //    NMDebugAI(<< getUserID().toStdString() << ": pre-loop: IterStep=" << getIterationStep() << " niter=" << niter;)

    //    if (niter != mNumIterations)
    //    {
    //        this->setNumIterations(niter);
    //    }

    NMModelController* ctrl = this->getModelController();

    unsigned int niter = mNumIterations;
    mIterationStepRun = mIterationStep;
    unsigned int i = mIterationStepRun-1;
    for (; i < niter && !ctrl->isModelAbortionRequested(); ++i)
    {
        emit signalProgress(mIterationStepRun);
        this->componentUpdateLogic(repo, minLevel, maxLevel, i);
        niter = evalNumIterationsExpression(mIterationStepRun+1);
        NMDebugAI(<< this->objectName().toStdString() << ": in-loop: IterStep=" << getIterationStep()
                                                << " i=" << i << " niter=" << niter << std::endl);
        this->setNumIterations(niter);
        ++mIterationStepRun;
    }
    mIterationStepRun = mIterationStep;
    emit signalProgress(mIterationStep);
}

