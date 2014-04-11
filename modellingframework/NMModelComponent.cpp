 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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
#include "NMIterableComponent.h"
#include "NMModelController.h"
#include "NMMfwException.h"

#include <string>
#include <iostream>
#include <sstream>
#include "nmlog.h"

const std::string NMModelComponent::ctx = "NMModelComponent";

NMModelComponent::NMModelComponent(QObject* parent)
{
	this->setParent(parent);
	this->initAttributes();
}

void NMModelComponent::initAttributes(void)
{
    this->mTimeLevel = 0;
    this->mUpComponent = 0;
    this->mDownComponent = 0;
    this->mHostComponent = 0;
}

NMModelComponent::~NMModelComponent(void)
{
}

void
NMModelComponent::getUpstreamPipe(QList<QStringList>& hydra,
		QStringList& upstreamPipe, int step)
{
	if (this->mInputs.size() == 0)
		return;

	int compstep = step < 0 ? 0
			: (step > this->mInputs.size()-1 ? this->mInputs.size()-1
					: step);
	if (compstep != step)
	{
		// ToDo: this needs to be adjusted as soon as
		// we implement the choice of how to map
		// step size to input list index (i.e. use_up, cycle, strict step);
		// for now, we go for the NM_USE_UP policy
		NMDebugAI(<< "mind you, we've adjusted the step parameter to select"
				<< " an input component from '" << this->objectName().toStdString()
				<< "'" << std::endl);
		//compstep = this->mInputs.size()-1;
	}

	QStringList inputs = this->mInputs.at(compstep);
	if (inputs.size() == 0)
	{
		//NMDebugAI(<< this->objectName().toStdString()
		//		<< "::getUpstreamPipe: step #" << compstep
		//		<< " has no inputs ..." << endl);

		return;
	}


	foreach(const QString& in, inputs)
	{
		//NMDebugAI(<< this->objectName().toStdString()
		//		<< "::getUpstreamPipe: investigating input '"
		//		<< in.toStdString() << "' ..." << endl);

		// when we've got a cyclic relationship (e.g. for iteratively
		// updating data buffer components), we have to make sure
		// we don't include the root (executable) element again
		// in the same pipeline
		if (upstreamPipe.contains(in))
		{
			NMDebug(<< endl);
			continue;
		}

		NMModelComponent* comp = NMModelController::getInstance()->getComponent(in);
		if (comp != 0 && comp->getTimeLevel() == this->getTimeLevel())
		{
			// here, we distinguish between components hosting an itk::Process-derived
			// process object, which can be piped together with other itk::Process-derived
			// objects, and components, which host either any other kind of non-pipeable
			// process components, or components of any other kind, such as group
			// components or data components; thus we're looking for an itk::Process object
			NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
			if (ic != 0 && ic->getProcess() != 0 && ic->getProcess()->getInternalProc() != 0)
			{
				//NMDebugAI(<< "... and higher ... " << std::endl);
				upstreamPipe.push_front(ic->objectName());
				ic->getUpstreamPipe(hydra, upstreamPipe, step);
			}
			else
			{
				//NMDebugAI(<< "... stop" << std::endl);
				upstreamPipe.push_front(comp->objectName());

				// if this component hasn't been explored yet, we explore it ...
				// alternatively, in NMIterableComponent::createExecSequence()
				// doubly included sub-pipes could be eliminated rather than
				// preventing double accounting here; however, which way
				// performs better depends ultimately on the structure and
				// complexity of the graph to be parsed (which we don't know
				// in advance); furthermore, doing it here prevents also
				// endless loops in case a 'cyclic' execution graph has been
				// constructed
				bool bnotexplored = true;
				foreach(const QStringList& pipe, hydra)
				{
					if (pipe.last().compare(comp->objectName()) == 0)
					{
						bnotexplored = false;
						break;
					}
				}

				if (bnotexplored)
				{
					QStringList upstreamUpstreamPipe;
					upstreamUpstreamPipe.push_front(comp->objectName());
					comp->getUpstreamPipe(hydra, upstreamUpstreamPipe, step);
					hydra.push_back(upstreamUpstreamPipe);
				}
			}
		}
	}
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

	emit NMModelComponentChanged();
}

void
NMModelComponent::setDescription(QString descr)
{
	this->mDescription = descr;
    emit NMModelComponentChanged();
	emit ComponentDescriptionChanged(descr);
}

void
NMModelComponent::setUserID(const QString& userID)
{
    if (userID.compare(mUserID, Qt::CaseSensitive) == 0)
        return;

    // do what we can to fix 'erroneously' ill-formatted
    // ids
    QString uid = userID.simplified();

    // check, whether string is fit for purpose
    // here we apply the muParser character set for
    // names, as we want to use the the UserID, amongst others,
    // in expressions subject to be parsed by muParser
    // allowed set of characters:
    // "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    QRegExp idchars("[_a-zA-Z\\d]*");
    if (idchars.indexIn(uid) >= 0)
    {
        mUserID = uid;
        emit NMModelComponentChanged();
        emit ComponentUserIDChanged(uid);
    }
    else
    {
        NMMfwException e(NMMfwException::NMModelComponent_InvalidUserID);
        QString msg = QString(tr("%1: Invalid UserID: '%2'!"))
                .arg(this->objectName()).arg(uid);
        e.setMsg(msg.toStdString());
        throw e;
    }
}
