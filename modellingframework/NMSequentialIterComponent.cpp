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
    NMIterableComponent::initAttributes();
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
    mIterationStepRun = mIterationStep;
    unsigned int i = mIterationStepRun-1;
    for (; i < this->mNumIterations; ++i)
	{
        emit signalProgress(mIterationStepRun);
        this->componentUpdateLogic(repo, minLevel, maxLevel, i);
        ++mIterationStepRun;
	}
    mIterationStepRun = mIterationStep;

    // reset aggregate components
    //    if (this->mProcess == 0)
    //    {
    //       NMModelComponent* mc = this->getInternalStartComponent();
    //       while(mc)
    //       {
    //           mc->reset();
    //           mc = this->getNextInternalComponent();
    //       }
    //    }
}

