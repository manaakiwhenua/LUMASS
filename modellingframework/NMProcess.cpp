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
 * NMProcess.cpp
 *
 *  Created on: 16/04/2012
 *      Author: alex
 */

#include "NMProcess.h"
#include "NMModelComponent.h"
#include "NMModelController.h"

#include <QDateTime>

#include "otbImage.h"
#include "itkImageIOBase.h"

NMProcess::NMProcess(QObject *parent)
	: mbAbortExecution(false), mbLinked(false)
{
	this->mInputComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mOutputComponentType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mNMComponentType = NMItkDataObjectWrapper::NM_UNKNOWN;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mbIsInitialised = false;
	this->mParameterHandling = NM_SYNC_WITH_HOST;
	this->mParamPos = 0;
	this->mProgress = 0.0;
}

NMProcess::~NMProcess()
{
	// TODO Auto-generated destructor stub
}

void
NMProcess::linkInPipeline(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(this->parent()->objectName().toStdString(), << "...");

	if (mbLinked)
	{
		NMDebugAI(<< "seems we've been linked already without being executed!" << endl);
		NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
		return;
	}

	if (!this->isInitialised())
		this->instantiateObject();

	// in case we've got an itk::ProcessObject, we
	// add an observer for progress report
	if (this->mOtbProcess.IsNotNull())
	{
		mObserver = ObserverType::New();
		mObserver->SetCallbackFunction(this,
				&NMProcess::UpdateProgressInfo);
		this->mOtbProcess->AddObserver(itk::ProgressEvent(), mObserver);
		this->mOtbProcess->AddObserver(itk::StartEvent(), mObserver);
		this->mOtbProcess->AddObserver(itk::EndEvent(), mObserver);
		this->mOtbProcess->AddObserver(itk::AbortEvent(), mObserver);
	}

	this->linkInputs(step, repo);
	this->linkParameters(step, repo);

	//NMDebugAI( << " mbLinked = true" << endl);
	this->mbLinked = true;

	NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

void
NMProcess::linkParameters(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
	// this should really be implemented in subclasses;
	// this stub is only provided to avoid stubs within subclasses
	// have to be implemented in any case, even if this functionality is not required
}

void NMProcess::removeInputComponent(const QString& input)
{
	for (unsigned int i=0; i < this->mInputComponents.size(); ++i)
	{
		QStringList sl = this->mInputComponents.at(i);
		if (sl.contains(input, Qt::CaseInsensitive))
		{
			sl.removeOne(input);
			this->mInputComponents[i] = sl;
			break;
		}
	}
}

void NMProcess::linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(ctxNMProcess, << "...");

	if (this->mInputComponents.size() == 0)
	{
		NMDebugAI(<< "no inputs for " << this->objectName().toStdString() << std::endl);
		NMDebugCtx(ctxNMProcess, << "done!");
		return;
	}

	// set the step parameter according to the ParameterHandling mode set for this process
	switch(this->mParameterHandling)
	{
	case NM_USE_UP:
		if (this->mParamPos < this->mInputComponents.size())
		{
			step = this->mParamPos;
			// this is better done in the update function, so that it only
			// get's incremented after an actual processing step
			++this->mParamPos;
		}
		else if (this->mParamPos >= this->mInputComponents.size())
		{
			this->mParamPos = this->mInputComponents.size()-1;
			step = this->mParamPos;
		}
		break;
	case NM_CYCLE:
		if (this->mParamPos < this->mInputComponents.size())
		{
			step = this->mParamPos;
			++this->mParamPos;
		}
		else if (this->mParamPos >= this->mInputComponents.size())
		{
			step = 0;
			this->mParamPos = 1;
		}
		break;
	case NM_SYNC_WITH_HOST:
		if (step < this->mInputComponents.size())
		{
			this->mParamPos = step;
		}
		else
		{
			step = 0;
			this->mParamPos = 0;
			NMErr(ctxNMProcess, << "mParamPos and host's step out of sync!! Set step / mParamPos = 0");
		}

		break;
	}

	NMModelComponent* parentComp = qobject_cast<NMModelComponent*>(this->parent());
	QString targetName = parentComp->objectName();

	// set the input images
	QString inputCompName;
	NMDebugAI(<< "ParameterPos: #" << step << std::endl);
	if (step < this->mInputComponents.size())
	{

		for (unsigned int ii = 0;
				ii < this->mInputComponents.at(step).size();
				++ii)
		{
			// find the input component
			inputCompName = this->mInputComponents.at(step).at(
					ii);
			QMap<QString, NMModelComponent*>::const_iterator it = repo.find(
					inputCompName);
			if (it != repo.end())
			{
				NMModelComponent*& ic =
						const_cast<NMModelComponent*&>(it.value());
				// dont' link everything ...
				if (//parentComp->objectName().compare(ic->objectName()) != 0 &&
					parentComp->getTimeLevel() == ic->getTimeLevel()        &&
					//parentComp->isSubComponent(ic)					        &&
				    parentComp->getNumIterations() == 1
				   )
				{
					NMDebugAI(<< targetName.toStdString() << " <-(" << ii << ")- "
							<< ic->objectName().toStdString() << " ... " << std::endl);
					this->setNthInput(ii, ic->getOutput());
				}
				else
				{
					NMDebugAI(<< parentComp->objectName().toStdString() << "(" << ii
							<< ") <-||- " << ic->objectName().toStdString() << " ... "
							<< std::endl);


					// ToDo - check whether we really want to update here,
					// or whether we want the user to design its model properly
					ic->getProcess()->update();

					NMItkDataObjectWrapper* dw = new NMItkDataObjectWrapper(
							*ic->getOutput());
					if (dw->getDataObject() != 0)
					{
						NMDebug(<< "ok!" << std::endl);
						dw->setParent(parentComp);
						dw->getDataObject()->DisconnectPipeline();
						this->setNthInput(ii, dw);
					}
					else
					{
						NMDebug(<< "failed!" << std::endl);
					}
				}
				//this->mOtbProcess->Modified();
			}
			else
			{
				NMErr(ctxNMProcess, << "required input component not found!");
				return;
			}
		}
	}
	else
	{
		NMDebugAI(<< "no other input for step #" << step << " - nothing to do ..." << std::endl);
	}

	NMDebugCtx(ctxNMProcess, << "done!");
}

/**
 * Get the value of mInputComponentType
 * \return the value of mInputComponentType
 */
itk::ImageIOBase::IOComponentType
NMProcess::getInputComponentType(void)
{
    return this->mInputComponentType;
}

void NMProcess::reset(void)
{
	NMDebugCtx(this->parent()->objectName().toStdString(), << "...");
	this->mParamPos = 0;
	this->mbIsInitialised = false;
	NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

void NMProcess::update(void)
{
	NMDebugCtx(ctxNMProcess, << "...");

	if (this->mbIsInitialised && this->mOtbProcess.IsNotNull())
	{
		this->mOtbProcess->Update();
	}
	else
	{
		NMErr(ctxNMProcess,
				<< "Update failed! Either the process object is not initialised or"
				<< " itk::ProcessObject is NULL and ::update() is not re-implemented!");
	}
	this->mbLinked = false;

	NMDebugAI(<< "update value for: mParamPos=" << this->mParamPos << endl);
	NMDebugCtx(ctxNMProcess, << "done!");
}

void
NMProcess::abortExecution(void)
{
	this->mbAbortExecution = true;
}

void
NMProcess::setInputComponents(QList<QStringList> inputComponents)
{
	this->mInputComponents = inputComponents;
	emit signalInputChanged(inputComponents);
}

void
NMProcess::UpdateProgressInfo(itk::Object* obj,
		const itk::EventObject& event)
{
	itk::Object* ncobj = const_cast<itk::Object*>(obj);
	itk::ProcessObject* proc = dynamic_cast<itk::ProcessObject*>(ncobj);
	if (proc == 0)
		return;

	if (this->mbAbortExecution)
		proc->AbortGenerateDataOn();

	if (typeid(event) == typeid(itk::ProgressEvent))
	{
		emit signalProgress((float)(proc->GetProgress() * 100.0));
	}
	else if (typeid(event) == typeid(itk::StartEvent))
	{
		emit signalExecutionStarted(this->parent()->objectName());
	}
	else if (typeid(event) == typeid(itk::EndEvent))
	{
		emit signalExecutionStopped(this->parent()->objectName());
		emit signalProgress(0);
		this->mOtbProcess->SetAbortGenerateData(false);
		this->mbAbortExecution = false;
	}
	else if (typeid(event) == typeid(itk::AbortEvent))
	{
		emit signalExecutionStopped(this->parent()->objectName());
		emit signalProgress(0);
		this->mOtbProcess->ResetPipeline();
		this->mOtbProcess->SetAbortGenerateData(false);
		this->mbAbortExecution = false;
	}

	// regardless of what event made this method been called, it indicates that
	// the process has been executed, which means for the next iteration, new
	// parameters have potentially be linked in again
	//NMDebugAI(<< this->parent()->objectName().toStdString()
	//		  << "::UpdateProgressInfo(): mbLinked = false" << endl);
	this->mbLinked = false;
}

NMItkDataObjectWrapper::NMComponentType
NMProcess::getInputNMComponentType()
{
	NMItkDataObjectWrapper::NMComponentType nmtype;
	switch (this->mInputComponentType)
	{
		case itk::ImageIOBase::UCHAR : nmtype = NMItkDataObjectWrapper::NM_UCHAR	; break;
		case itk::ImageIOBase::CHAR	 : nmtype = NMItkDataObjectWrapper::NM_CHAR	; break;
		case itk::ImageIOBase::USHORT: nmtype = NMItkDataObjectWrapper::NM_USHORT; break;
		case itk::ImageIOBase::SHORT : nmtype = NMItkDataObjectWrapper::NM_SHORT	; break;
		case itk::ImageIOBase::UINT	 : nmtype = NMItkDataObjectWrapper::NM_UINT	; break;
		case itk::ImageIOBase::INT	 : nmtype =	NMItkDataObjectWrapper::NM_INT	;	break;
		case itk::ImageIOBase::ULONG : nmtype =	NMItkDataObjectWrapper::NM_ULONG;    break;
		case itk::ImageIOBase::LONG	 : nmtype =	NMItkDataObjectWrapper::NM_LONG	;   break;
		case itk::ImageIOBase::FLOAT : nmtype =	NMItkDataObjectWrapper::NM_FLOAT; break;
		case itk::ImageIOBase::DOUBLE: nmtype =	NMItkDataObjectWrapper::NM_DOUBLE;	break;
	default: nmtype = NMItkDataObjectWrapper::NM_UNKNOWN; break;
	}

	return nmtype;
}

NMItkDataObjectWrapper::NMComponentType
NMProcess::getOutputNMComponentType()
{
	NMItkDataObjectWrapper::NMComponentType nmtype;
	switch (this->mOutputComponentType)
	{
		case itk::ImageIOBase::UCHAR	 : nmtype = NMItkDataObjectWrapper::NM_UCHAR	; break;
		case itk::ImageIOBase::CHAR		 : nmtype = NMItkDataObjectWrapper::NM_CHAR	; break;
		case itk::ImageIOBase::USHORT	 : nmtype = NMItkDataObjectWrapper::NM_USHORT; break;
		case itk::ImageIOBase::SHORT	 : nmtype = NMItkDataObjectWrapper::NM_SHORT	; break;
		case itk::ImageIOBase::UINT		 : nmtype = NMItkDataObjectWrapper::NM_UINT	; break;
		case itk::ImageIOBase::INT	 : nmtype =	NMItkDataObjectWrapper::NM_INT	;	break;
		case itk::ImageIOBase::ULONG : nmtype =	NMItkDataObjectWrapper::NM_ULONG;    break;
		case itk::ImageIOBase::LONG	 : nmtype =	NMItkDataObjectWrapper::NM_LONG	;   break;
		case itk::ImageIOBase::FLOAT : nmtype =	NMItkDataObjectWrapper::NM_FLOAT; break;
		case itk::ImageIOBase::DOUBLE: nmtype =	NMItkDataObjectWrapper::NM_DOUBLE;	break;
		default: nmtype = NMItkDataObjectWrapper::NM_UNKNOWN; break;
	}

	return nmtype;
}

void
NMProcess::setInputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype)
{
	itk::ImageIOBase::IOComponentType type;
	switch (nmtype)
	{
		case NMItkDataObjectWrapper::NM_UCHAR: type = itk::ImageIOBase::UCHAR; break;
		case NMItkDataObjectWrapper::NM_CHAR: type = itk::ImageIOBase::CHAR; break;
		case NMItkDataObjectWrapper::NM_USHORT: type = itk::ImageIOBase::USHORT; break;
		case NMItkDataObjectWrapper::NM_SHORT: type = itk::ImageIOBase::SHORT; break;
		case NMItkDataObjectWrapper::NM_UINT: type = itk::ImageIOBase::UINT; break;
		case NMItkDataObjectWrapper::NM_INT: type = itk::ImageIOBase::INT; break;
		case NMItkDataObjectWrapper::NM_ULONG: type = itk::ImageIOBase::ULONG; break;
		case NMItkDataObjectWrapper::NM_LONG: type = itk::ImageIOBase::LONG; break;
		case NMItkDataObjectWrapper::NM_FLOAT: type = itk::ImageIOBase::FLOAT; break;
		case NMItkDataObjectWrapper::NM_DOUBLE: type = itk::ImageIOBase::DOUBLE; break;
		default: type = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

	this->mInputComponentType = type;
	this->mbIsInitialised = false;
}

void
NMProcess::setOutputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype)
{
	itk::ImageIOBase::IOComponentType type;
	switch (nmtype)
	{
		case NMItkDataObjectWrapper::NM_UCHAR: type = itk::ImageIOBase::UCHAR; break;
		case NMItkDataObjectWrapper::NM_CHAR: type = itk::ImageIOBase::CHAR; break;
		case NMItkDataObjectWrapper::NM_USHORT: type = itk::ImageIOBase::USHORT; break;
		case NMItkDataObjectWrapper::NM_SHORT: type = itk::ImageIOBase::SHORT; break;
		case NMItkDataObjectWrapper::NM_UINT: type = itk::ImageIOBase::UINT; break;
		case NMItkDataObjectWrapper::NM_INT: type = itk::ImageIOBase::INT; break;
		case NMItkDataObjectWrapper::NM_ULONG: type = itk::ImageIOBase::ULONG; break;
		case NMItkDataObjectWrapper::NM_LONG: type = itk::ImageIOBase::LONG; break;
		case NMItkDataObjectWrapper::NM_FLOAT: type = itk::ImageIOBase::FLOAT; break;
		case NMItkDataObjectWrapper::NM_DOUBLE: type = itk::ImageIOBase::DOUBLE; break;
		default: type = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

	this->mOutputComponentType = type;
	this->mbIsInitialised = false;
}


/**
 * Get the value of mOutputComponentType
 * \return the value of mOutputComponentType
 */
itk::ImageIOBase::IOComponentType NMProcess::getOutputComponentType(void)
{
    return this->mOutputComponentType;
}

