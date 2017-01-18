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
#include "NMMfwException.h"

const std::string NMSequentialIterComponent::ctx = "NMSequentialIterComponent";

NMSequentialIterComponent::NMSequentialIterComponent(QObject* parent)
    : mNumIterations(1)
{
	this->setParent(parent);
    NMIterableComponent::initAttributes();
    this->mNumIterationsExpression.clear();
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
    unsigned int niter = mNumIterations;

    if (!this->mNumIterationsExpression.isEmpty())
    {
        QString numIterStr = NMModelController::getInstance()->processStringParameter(this, mNumIterationsExpression);
        bool bok = false;
        unsigned int titer = numIterStr.toUInt(&bok);
        if (bok)
        {
            niter = titer;
            // display the current number of iterations on the display
            this->setNumIterations(niter);
        }

        if (!bok || numIterStr.startsWith("ERROR"))
        {
            std::stringstream msg;
            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
            me.setSource(this->objectName().toStdString());
            if (!bok)
            {
                msg << this->objectName().toStdString()
                    << "::iterativeComponentUpdate(): ERROR "
                    << "- invalid NumIterationsExpression '" << numIterStr.toStdString()
                    << "'!" << std::endl;
            }
            else
            {
                msg << numIterStr.toStdString() << std::endl;
            }

            me.setDescription(msg.str());
            emit signalExecutionStopped();
            this->mIsUpdating = false;
            throw me;
        }
    }

    mIterationStepRun = mIterationStep;
    unsigned int i = mIterationStepRun-1;
    for (; i < niter; ++i)
	{
        emit signalProgress(mIterationStepRun);
        this->componentUpdateLogic(repo, minLevel, maxLevel, i);
        ++mIterationStepRun;
	}
    mIterationStepRun = mIterationStep;

}

