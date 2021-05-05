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

#include <string>
#include <iostream>
#include <sstream>

#include <QMetaObject>
#include <QMetaProperty>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMModelComponent.h"
#include "NMIterableComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMDataComponent.h"
#include "NMModelController.h"
#include "NMMfwException.h"

const std::string NMModelComponent::ctx = "NMModelComponent";

NMModelComponent::NMModelComponent(QObject* parent)
{
	this->setParent(parent);
	this->initAttributes();
}

void NMModelComponent::initAttributes(void)
{
    this->mIsUpdating = false;
    this->mTimeLevel = 0;
    this->mUpComponent = 0;
    this->mDownComponent = 0;
    this->mHostComponent = 0;
    this->mLogger = 0;
}

NMModelComponent::~NMModelComponent(void)
{
}

void
NMModelComponent::ProcessLogEvent(itk::Object* obj, const itk::EventObject& event)
{
    if (typeid(event) == typeid(itk::NMLogEvent))
    {
        itk::NMLogEvent& le = dynamic_cast<itk::NMLogEvent&>(
                    const_cast<itk::EventObject&>(event));
        if (mLogger)
        {
            QString userID = this->getUserID();
            if (userID.isEmpty())
            {
                userID = this->objectName();
            }

            QString logmsg = QString("%1: %2").arg(userID).arg(le.getLogMsg().c_str());
            mLogger->processLogMsg(le.getLogTime().c_str(),
                                   (NMLogger::LogEventType)le.getLogType(),
                                   logmsg);
        }
    }
}

void
NMModelComponent::processUserID(void)
{
    if (mController == nullptr)
    {
        return;
    }

    QString procID = mController->processStringParameter(this, mUserID);
    if (procID.startsWith("ERROR"))
    {
        NMLogError(<< this->objectName().toStdString() << ":processUserID() failed! "
                   << procID.toStdString());
        return;
    }

    this->setProperty("UserID", QVariant::fromValue(procID));
}

QVariant
NMModelComponent::getModelParameter(const QString &paramSpec)
{
    QVariant param;

    if (paramSpec.isEmpty())
    {
        return param;
    }

    //  <parameterName>:<indexNumber>
    //  NOTE: the index is 1-BASED!!!
    //        in alignment with parameter expressions fetching table
    //        table values
    QStringList specList = paramSpec.split(":", QString::SkipEmptyParts);
    if (specList.size() < 2)
    {
        specList << "1";
    }

    QStringList propList = this->getModelController()->getPropertyList(this);
    NMProcess* proc = 0;
    if (!propList.contains(specList.at(0)))
    {
        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(this);
        if (ic && ic->getProcess() != 0)
        {
            proc = ic->getProcess();
            propList = this->getModelController()->getPropertyList(proc);
        }

        if (!propList.contains(specList.at(0)))
        {
            //            NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
            //            me.setSource(this->objectName().toStdString());
            //            QString msg = QString("%1::getModelParameter(%2) - don't have a parameter '%3'!")
            //                    .arg(this->objectName())
            //                    .arg(paramSpec)
            //                    .arg(specList.at(1));
            //            me.setDescription(msg.toStdString());
            //            throw me;
            param = QString("ERROR - invalid property: %1").arg(specList.at(0));
            return param;
        }
    }

    // if we've got an explict idex given, we just see what we've got
    // (employing the 'use_up' strategy, s. NMProcess)
    bool bok = true;
    int listIdx = specList.at(1).toInt(&bok);

    /// !!!!
    // adjusting the 1-based index to the 0-based map-indices!
    /// !!!!
    --listIdx;

    if (!bok)
    {
        param = QString("ERROR - invalid index: %1").arg(specList.at(1));
        return param;
    }
    else if (listIdx < 0)
    {
        //        NMMfwException me(NMMfwException::NMModelComponent_InvalidParameter);
        //        me.setSource(this->objectName().toStdString());
        //        QString msg = QString("%1::getModelParameter(%2) - index '%3' is invalid!")
        //                .arg(this->objectName())
        //                .arg(paramSpec)
        //                .arg(listIdx);
        //        me.setDescription(msg.toStdString());
        //        throw me;
        param = QString("ERROR - index '%1' is invalid!").arg(listIdx+1);
        return param;
    }

    QVariant paramList;
    if (proc)
    {
        paramList = proc->property(specList.at(0).toStdString().c_str());
    }
    else
    {
        paramList = this->property(specList.at(0).toStdString().c_str());
    }


    if (QString::fromLatin1("QStringList").compare(paramList.typeName()) == 0)
    {
        QStringList tl = paramList.toStringList();
        if (tl.size())
        {
            if (listIdx >= tl.size() && listIdx > 0)
            {
                listIdx = tl.size()-1;
            }
            param = QVariant::fromValue(tl.at(listIdx));
        }
        else
        {
            param = QString("ERROR - property '%1' is NULL!").arg(specList.at(0));
        }
    }
    else if (QString::fromLatin1("QList<QStringList>").compare(paramList.typeName()) == 0)
    {
        QList<QStringList> tll = paramList.value<QList<QStringList> >();
        if (tll.size())
        {
            if (listIdx >= tll.size() && listIdx > 0)
            {
                listIdx = tll.size()-1;
            }
            param = QVariant::fromValue(tll.at(listIdx));
        }
        else
        {
            param = QString("ERROR - property '%1' is NULL!").arg(specList.at(0));
        }
    }
    else
    {
        param = paramList;
    }

    return param;
}

void
NMModelComponent::getUpstreamPipe(QList<QStringList>& hydra,
		QStringList& upstreamPipe, int step)
{
	if (this->mInputs.size() == 0)
		return;

    int compstep = step;

    // what if we are an NMProcess? and have an index policy set?
    NMIterableComponent* weIc = qobject_cast<NMIterableComponent*>(this);
    if (weIc != nullptr && weIc->getProcess() != nullptr)
    {
        compstep = weIc->getProcess()->mapHostIndexToPolicyIndex(
                    compstep, this->mInputs.size());
    }
    else
    {
        compstep = step < 0 ? 0
            : (step > this->mInputs.size()-1 ? this->mInputs.size()-1
                        : step);
    }

    //	if (compstep != step)
    //	{
    //		// ToDo: this needs to be adjusted as soon as
    //		// we implement the choice of how to map
    //		// step size to input list index (i.e. use_up, cycle, strict step);
    //		// for now, we go for the NM_USE_UP policy
    //        NMDebugAI(<< "mind you, we've adjusted the step parameter "
    //                  << " from " << step << " to " << compstep << " to select"
    //				<< " an input component from '" << this->objectName().toStdString()
    //				<< "'" << std::endl);
    //		//compstep = this->mInputs.size()-1;
    //	}

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
        // make sure that we strip off any input index, e.g. MapAlgebra:1, should
        // be turned into MapAlgebra
        QString inName = this->getModelController()->getComponentNameFromInputSpec(in);

        //NMDebugAI(<< this->objectName().toStdString()
		//		<< "::getUpstreamPipe: investigating input '"
		//		<< in.toStdString() << "' ..." << endl);

		// when we've got a cyclic relationship (e.g. for iteratively
		// updating data buffer components), we have to make sure
		// we don't include the root (executable) element again
		// in the same pipeline
        if (upstreamPipe.contains(inName))
		{
            //NMDebug(<< endl);
			continue;
		}


        // we follow inputs upstream as long as they are sitting outside this component's host or
        // share the same time level as this component
        // -> and are not a subcomponent of this one

        bool bGoUp = false;
        bool bShareHost = false;
        NMModelComponent* comp = this->getModelController()->getComponent(inName);
        NMDataComponent* dataComp = qobject_cast<NMDataComponent*>(comp);
        if (comp != nullptr && dataComp == nullptr)
        {
            if (this->getHostComponent()->objectName().compare(comp->getHostComponent()->objectName()) == 0)
            {
                bShareHost = true;
                if (this->getTimeLevel() == comp->getTimeLevel())
                {
                    bGoUp = true;
                }
            }
            else
            {
                if (weIc)
                {
                    if (!weIc->isSubComponent(comp))
                    {
                        bGoUp = true;
                    }
                }
                else
                {
                    bGoUp = true;
                }
            }
        }


        //if (comp != 0 && comp->getTimeLevel() == this->getTimeLevel())
        if (bGoUp)
		{
			// here, we distinguish between components hosting an itk::Process-derived
			// process object, which can be piped together with other itk::Process-derived
			// objects, and components, which host either any other kind of non-pipeable
			// process components, or components of any other kind, such as group
			// components or data components; thus we're looking for an itk::Process object
			NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);

            // determine the relevant step parameter for the input component:
            // - if input and this component share the host, we use the stepping
            //   determined by the shared host, otherwise
            // - if the input component has different host than this component,
            //   we use the input's host component stepping parameter
            //   NOTE: if the input component is not a direct subcomponent of this
            //   component's host, the input's stepping parameter being used will
            //   most likely be 0
            int instep = bShareHost ? step : ic != nullptr ? ic->getIterationStep()-1 : step;

            // 'partaking' of an input process in a pipeline
            //  can be conditioned with the NumIterationsExpression
            NMSequentialIterComponent* sic = ic != nullptr ? qobject_cast<NMSequentialIterComponent*>(ic) : nullptr;
            if (    sic != nullptr
                 && sic->evalNumIterationsExpression(instep+1) == 0
               )
            {
                bGoUp = false;
            }

            if (bGoUp)
            {
                if (ic != nullptr && ic->getProcess() != nullptr)           // && ic->getProcess()->getInternalProc() != 0)
                {
                    if (ic->getProcess()->getInternalProc() == nullptr)
                    {
                        ic->getProcess()->instantiateObject();
                    }

                        //}


                    //if (    ic != nullptr
                    //     && ic->getProcess() != nullptr
                    //     && ic->getProcess()->getInternalProc() != nullptr
                    //   )
                    //{
                    // add the process component to the pipeline, and
                    // investigate its inputs using the passed step parameter
                    // of the calling component
                    upstreamPipe.push_front(ic->objectName());
                    ic->getUpstreamPipe(hydra, upstreamPipe, instep);
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

                    // just have a look, whether this component has no inputs,
                    // as for example a non itk::Process-based Process component,
                    // such as TableReader, which
                    // still provides an itk::DataObject as input to a 'normal'
                    // process component
                    if (comp->getInputs().size() == 0)
                    {
                        bnotexplored = false;
                    }


                    if (bnotexplored)
                    {
                        QStringList upstreamUpstreamPipe;
                        upstreamUpstreamPipe.push_front(comp->objectName());
                        comp->getUpstreamPipe(hydra, upstreamUpstreamPipe, instep);
                        hydra.push_back(upstreamUpstreamPipe);
                    }
                }
            }
		}
	}
}

void NMModelComponent::setTimeLevel(short level)
{
    int maxdiff = mTimeLevel * -1;
    int diff = level - this->mTimeLevel;

    if (this->mHostComponent != 0)
    {
        maxdiff = (this->mTimeLevel - mHostComponent->getTimeLevel()) * -1;
    }

    // make sure we're at least at the same time level
    // as our host
    diff = diff < maxdiff ? maxdiff : diff;
    this->changeTimeLevel(diff);
}

void NMModelComponent::changeTimeLevel(int diff)
{
	this->mTimeLevel += diff;
	if (this->mTimeLevel < 0)
		this->mTimeLevel = 0;

    //emit NMModelComponentChanged();
    emit TimeLevelChanged(mTimeLevel);
    emit nmChanged();
}

void
NMModelComponent::setDescription(QString descr)
{
	this->mDescription = descr;
    //emit NMModelComponentChanged();
    emit nmChanged();
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
    QString oldID = mUserID;

    // check, whether string is fit for purpose
    // here we apply the muParser character set for
    // names, as we want to use the the UserID, amongst others,
    // in expressions subject to be parsed by muParser
    // allowed set of characters:
    // "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    QRegExp idchars("[a-zA-Z]*[_a-zA-Z\\d]*");
    if (idchars.indexIn(uid) >= 0)
    {
        mUserID = uid;
        emit NMModelComponentChanged();
        emit nmChanged();
        emit ComponentUserIDChanged(oldID, uid);
    }
    else
    {
        NMMfwException e(NMMfwException::NMModelComponent_InvalidUserID);
        e.setSource(this->objectName().toStdString());
        QString msg = QString(tr("%1: Invalid UserID: '%2'!"))
                .arg(this->objectName()).arg(uid);
        e.setDescription(msg.toStdString());
        throw e;
    }
}
