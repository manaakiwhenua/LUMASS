 /******************************************************************************
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 *
 * This programs distributed in the hope that it will be useful,
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
#include "NMMfwException.h"
#include "NMSequentialIterComponent.h"
#include "NMDataComponent.h"
#include "NMModelController.h"

#include <QDateTime>

#include "otbImage.h"
#include "otbImageIOBase.h"


NMProcess::NMProcess(QObject *parent)
    : mbAbortExecution(false), mbLinked(false)
{
	this->mInputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mOutputComponentType = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	this->mNMComponentType = NMItkDataObjectWrapper::NM_UNKNOWN;
	this->mInputNumDimensions = 2;
	this->mOutputNumDimensions = 2;
	this->mInputNumBands = 1;
	this->mOutputNumBands = 1;
	this->mbIsInitialised = false;
    this->mParameterHandling = NM_USE_UP;
	this->mParamPos = 0;
	this->mProgress = 0.0;
	this->mMTime.setMSecsSinceEpoch(0);
    this->mIsSink = false;
    this->mStepIndex = 0;
}

NMProcess::~NMProcess()
{
	// TODO Auto-generated destructor stub
}

void
NMProcess::linkInPipeline(unsigned int step,
		const QMap<QString, NMModelComponent*>& repo)
{
    NMIterableComponent* hostComp = qobject_cast<NMIterableComponent*>(this->parent());
    if (hostComp == 0)
    {
        NMMfwException nme(NMMfwException::NMProcess_UninitialisedProcessObject);
        nme.setMsg("NMProcess Object not embedded in NMIterableComponent!");
        throw nme;
        return;
    }

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
        if (mIsSink)
        {
            this->mOtbProcess->ReleaseDataFlagOn();
        }
	}

    // ATTENTION:
    // now we use the host's step parameter to determine which parameter
    // to read in, rather than step parameter from the calling process;
    // furthermore, if the host's step is param is zero (i.e. IterationStep == 1)
    // we take the one from the host's host component, since most of the time
    // iteration takes place one level a above the process' direct host
    step = hostComp->getIterationStep()-1;
    if (step == 0)
    {
        NMIterableComponent* hostHost = hostComp->getHostComponent();
        if (hostHost)
        {
            step = hostHost->getIterationStep()-1;
        }
    }
    mStepIndex = step;

    this->linkParameters(step, repo);
    this->linkInputs(step, repo);

#ifdef DEBUG
    if (this->mOtbProcess.IsNotNull())
    {
//        this->mOtbProcess->Print(std::cout, itk::Indent(nmlog::nmindent));
    }
#endif

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
    emit NMProcessChanged();
    emit nmChanged();
}

unsigned short
NMProcess::mapHostIndexToPolicyIndex(unsigned short step,
		unsigned short size)
{
    // attention, this relies on ::linkInPipeline() having been
    // called already when this function is called
    step = this->mbLinked ? this->mStepIndex : step;

	unsigned short idx = 0;
	if (size == 0)
		return idx;

	switch(this->mParameterHandling)
	{
	case NM_USE_UP:
		if (step > size-1)
			idx = size-1;
		else
			idx = step;
		break;
	case NM_CYCLE:
	case NM_SYNC_WITH_HOST:
		unsigned short num = (step+1) % size;
		if (num == 0)
			idx = size-1;
		else
			idx = num-1;
		break;
	}
	return idx;
}


QVariant
NMProcess::getParameter(const QString& property)
{
    NMDebugCtx(this->objectName().toStdString(), << "...");

    QVariant ret;
    QVariant propVal = this->property(property.toStdString().c_str());

    if (QString::fromLatin1("QStringList").compare(propVal.typeName()) == 0)
    {
        QString retStr;
        QStringList lst = propVal.toStringList();
        if (lst.size())
        {
            int step = this->mapHostIndexToPolicyIndex(mStepIndex, lst.size());
            retStr = lst.at(step);
            NMDebugAI( << step << " = mapToHostPolicyIndex(" << mStepIndex
                       << ", " << lst.size() << ")" << std::endl);
            NMDebugAI( << "input parameter to by analysed: " << retStr.toStdString() << std::endl);

            QString tStr = retStr;
            //            QRegExp rex("\\$([a-zA-Z]+\\d*)([\\+-]?)(\\d*)\\$");
            //            int pos = 0;
            //            while((pos = rex.indexIn(retStr, pos)) != -1)
            //            {
            //                // 0: whole captured text
            //                // 1: component name
            //                // 2: operator
            //                // 3: integer number
            //                QStringList m = rex.capturedTexts();
            //                NMDebugAI(<< m.join(" | ").toStdString() << std::endl);
            //                //NMDebugAI(<< "---------------" << std::endl);
            //                pos += rex.matchedLength();

            //                NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(
            //                            NMModelController::getInstance()->getComponent(m.at(1)));
            //                if (ic)
            //                {
            //                    bool bok;
            //                    int delta = 0;
            //                    if (!m.at(3).isEmpty())
            //                    {
            //                        const int t = m.at(3).toInt(&bok);
            //                        if (bok)
            //                            delta = t;
            //                    }

            //                    int itStep = ic->getIterationStep();
            //                    if (QString::fromLatin1("+").compare(m.at(2)) == 0)
            //                    {
            //                        // could only bound  this, if we restricted to the use
            //                        // of SequentialIterComponent here, not quite sure,
            //                        // we want to do that
            //                        itStep += delta;
            //                    }
            //                    else if (QString::fromLatin1("-").compare(m.at(2)) == 0)
            //                    {
            //                        // prevent 'negative' iStep; could occur in 'instantiation phase'
            //                        // of the pipeline, when the correct step parameter has not
            //                        // been established yet (thereby always assuming that the
            //                        // configuration by the user was correct, in which case the
            //                        // a wrong parameter would be created during the 'link phase'
            //                        // of the pipeline establishment)

            //                        if (itStep - delta >= 0)
            //                        {
            //                            itStep -= delta;
            //                        }
            //                        else
            //                        {
            //                            NMWarn(this->objectName().toStdString(),
            //                                   << "Expression based parameter retreival "
            //                                   << "prevented a NEGATIVE PARAMETER INDEX!!"
            //                                   << "  Double check whether the correct "
            //                                   << "parameter was used and the results are OK!");
            //                        }
            //                    }

            //                    tStr = tStr.replace(m.at(0), QString::fromLatin1("%1").arg(itStep));
            //                    NMDebugAI(<< "generated parameter: " << tStr.toStdString() << std::endl);
            //                }
            //            }
            //retStr = tStr;
            retStr = processStringParameter(tStr);
        }
        ret = QVariant::fromValue(retStr);
    }
    else if (QString::fromLatin1("QList<QStringList>").compare(propVal.typeName()) == 0)
    {
        QList<QStringList> wholeParam = propVal.value<QList<QStringList> >();
        int step = this->mapHostIndexToPolicyIndex(mStepIndex, wholeParam.size());
        QStringList curList = wholeParam.at(step);
        for (int i=0; i < curList.size(); ++i)
        {
            curList.replace(i, this->processStringParameter(curList.at(i)));
        }
        ret = QVariant::fromValue(curList);
    }
    // further impl for QList<QStringList> and QList<QList<QStringList> >

    NMDebugCtx(this->objectName().toStdString(), << "done!");
    return ret;
}

QString
NMProcess::processStringParameter(const QString& str)
{
    QString tStr = str;

    QRegExp rex("\\$([a-zA-Z]+\\d*)([\\+-]?)(\\d*)\\$");
    int pos = 0;
    while((pos = rex.indexIn(str, pos)) != -1)
    {
        // 0: whole captured text
        // 1: component name
        // 2: operator
        // 3: integer number
        QStringList m = rex.capturedTexts();
        NMDebugAI(<< m.join(" | ").toStdString() << std::endl);
        //NMDebugAI(<< "---------------" << std::endl);
        pos += rex.matchedLength();

        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(
                    NMModelController::getInstance()->getComponent(m.at(1)));
        if (ic)
        {
            bool bok;
            int delta = 0;
            if (!m.at(3).isEmpty())
            {
                const int t = m.at(3).toInt(&bok);
                if (bok)
                    delta = t;
            }

            int itStep = ic->getIterationStep();
            if (QString::fromLatin1("+").compare(m.at(2)) == 0)
            {
                // could only bound  this, if we restricted to the use
                // of SequentialIterComponent here, not quite sure,
                // we want to do that
                itStep += delta;
            }
            else if (QString::fromLatin1("-").compare(m.at(2)) == 0)
            {
                // prevent 'negative' iStep; could occur in 'instantiation phase'
                // of the pipeline, when the correct step parameter has not
                // been established yet (thereby always assuming that the
                // configuration by the user was correct, in which case the
                // a wrong parameter would be created during the 'link phase'
                // of the pipeline establishment)

                if (itStep - delta >= 0)
                {
                    itStep -= delta;
                }
                else
                {
                    NMWarn(this->objectName().toStdString(),
                           << "Expression based parameter retreival "
                           << "prevented a NEGATIVE PARAMETER INDEX!!"
                           << "  Double check whether the correct "
                           << "parameter was used and the results are OK!");
                }
            }

            tStr = tStr.replace(m.at(0), QString::fromLatin1("%1").arg(itStep));
            NMDebugAI(<< "generated parameter: " << tStr.toStdString() << std::endl);
        }
    }

    return tStr;
}

void NMProcess::linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
	NMDebugCtx(this->parent()->objectName().toStdString(), << "...");

	NMDebugAI(<< "step #" << step << std::endl);

	unsigned int inputstep = step;
	if (this->mInputComponents.size() == 0)
	{
		NMDebugAI(<< "no inputs for " << this->objectName().toStdString() << std::endl);
		NMDebugCtx(ctxNMProcess, << "done!");
		return;
	}
    step = this->mapHostIndexToPolicyIndex(inputstep, this->mInputComponents.size());

	NMIterableComponent* parentComp = qobject_cast<NMIterableComponent*>(this->parent());
	NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(this->parent());
	int parentIter = 2;
	if (sic != 0)
		parentIter = sic->getNumIterations();
	QString targetName = parentComp->objectName();

	// set the input images
	int outIdx = 0;
	QString inputSrc;
	QStringList inputSrcParams;
	QString inputCompName;
	bool bOK;
	NMDebugAI(<< "ParameterPos: #" << step << std::endl);
	if (step < this->mInputComponents.size())
	{

		for (unsigned int ii = 0;
				ii < this->mInputComponents.at(step).size();
				++ii)
		{
            // (re-)set default ouput index of input component
            outIdx = 0;
            // parse the input source string
			inputSrc = this->mInputComponents.at(step).at(
					ii);
			inputSrcParams = inputSrc.split(":", QString::SkipEmptyParts);
			inputCompName = inputSrcParams.at(0);
			if (inputSrcParams.size() == 2)
			{
				outIdx = inputSrcParams.at(1).toInt(&bOK);
				if (!bOK)
				{
					NMErr(ctxNMProcess, << "failed to interpret input source parameter"
							<< "'" << inputSrc.toStdString() << "'");
				}
			}

			// find the input component
			QMap<QString, NMModelComponent*>::const_iterator it = repo.find(
					inputCompName);
			if (it != repo.end())
			{
				NMModelComponent*& ic =
						const_cast<NMModelComponent*&>(it.value());
                //NMIterableComponent* procComp = qobject_cast<NMIterableComponent*>(ic);
                //NMDataComponent* dataComp = qobject_cast<NMDataComponent*>(ic);

                // note: we're linking anything here! It means the user is responsible for deciding
                //       whether or not it is a good idea to build a pipeline across timelevels or
                //       not! -> gives greater flexibility
                NMDebugAI(<< targetName.toStdString() << " <-(" << ii << ")- "
                          << ic->objectName().toStdString()
                          << "[" << outIdx << "] ... " << std::endl);

                // when we've got a process component here, we give it the
                // opportunity to put itself in order and link in
                //                if (procComp != 0 && !procComp->getProcess()->isInitialised())
                //                {
                //                    // since we're on the same time level, we can safely ask
                //                    // the input process to link itself into the pipeline
                //                    procComp->getProcess()->linkInPipeline(inputstep, repo);
                //                }
                //                else //if (dataComp == 0)
                ic->linkComponents(inputstep, repo);


                QSharedPointer<NMItkDataObjectWrapper> iw = ic->getOutput(outIdx);
                if (iw.isNull() || iw->getDataObject() == 0)
                {
                    NMMfwException e(NMMfwException::NMProcess_UninitialisedDataObject);
                    stringstream ss;
                    ss << ic->objectName().toStdString() << "::getOutput("
                       << outIdx << ") has not been initialised!" << endl;
                    e.setMsg(ss.str());
                    NMDebugCtx(ctxNMProcess, << "done!");
                    throw e;
                }

                // since we're on the same time level, we're in pipeline here
                // connect output to input
                this->setNthInput(ii, iw);//ic->getOutput(outIdx));
                NMDebugAI(<< "input #" << ii << ": " << inputSrc.toStdString() << std::endl);

                // note: this method does nothing, if it is not reimplemented by the subclass
                this->setRAT(ii, iw);
            }
            else
            {
                NMMfwException e(NMMfwException::NMProcess_InvalidInput);
                stringstream ss;
                ss << this->parent()->objectName().toStdString() << ": The specified "
                   << "input component '" << inputCompName.toStdString()
                   << "' couldn't be found! Check the property settings." << endl;
                e.setMsg(ss.str());
                NMDebugCtx(ctxNMProcess, << "done!");
                throw e;
            }
		}
	}
	else
	{
		NMDebugAI(<< "no other input for step #" << step << " - nothing to do ..." << std::endl);
	}

	NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

/**
 * Get the value of mInputComponentType
 * \return the value of mInputComponentType
 */
otb::ImageIOBase::IOComponentType
NMProcess::getInputComponentType(void)
{
    return this->mInputComponentType;
}

void NMProcess::reset(void)
{
    //NMDebugCtx(this->parent()->objectName().toStdString(), << "...");
	this->mParamPos = 0;
	this->mbIsInitialised = false;
	this->mbLinked = false;
	this->mMTime.setMSecsSinceEpoch(0);
	this->mOtbProcess = 0;
    this->mStepIndex = 0;
    //NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
}

void NMProcess::update(void)
{
	NMDebugCtx(this->parent()->objectName().toStdString(), << "...");

	if (this->mbIsInitialised && this->mOtbProcess.IsNotNull())
	{
        try
        {
            this->mOtbProcess->Update();
        }
        catch (std::exception& err)//itk::ExceptionObject& err)
        {
            NMMfwException rerr(NMMfwException::NMProcess_ExecutionError);

            NMIterableComponent* pc = qobject_cast<NMIterableComponent*>(this->parent());
            if (pc)
            {
                NMIterableComponent* hc = pc->getHostComponent();

                std::stringstream msg;
                msg << hc->objectName().toStdString() << " step #" << hc->getIterationStep() << ": "
                    << pc->objectName().toStdString() << " step #" << pc->getIterationStep()
                    << ": " << err.what();
                rerr.setMsg(msg.str());
            }
            else
            {
                rerr.setMsg(err.what());
            }
            //NMErr(this->parent()->objectName().toStdString(), << msg.str());
            NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
            emit signalExecutionStopped(this->parent()->objectName());
            emit signalProgress(0);

            throw rerr;
        }

		this->mMTime = QDateTime::currentDateTimeUtc();
		NMDebugAI(<< "modified at: "
				  << mMTime.toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString() << std::endl);
	}
	else
	{
		NMErr(this->parent()->objectName().toStdString(),
				<< "Update failed! Either the process object is not initialised or"
				<< " itk::ProcessObject is NULL and ::update() is not re-implemented!");
	}
	this->mbLinked = false;

	//NMDebugAI(<< "update value for: mParamPos=" << this->mParamPos << endl);
	NMDebugCtx(this->parent()->objectName().toStdString(), << "done!");
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
    //emit NMProcessChanged();
    emit nmChanged();
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

    QString objName = this->objectName();
    if (this->parent())
    {
        objName = this->parent()->objectName();
    }

	if (this->mbAbortExecution)
		proc->AbortGenerateDataOn();

	if (typeid(event) == typeid(itk::ProgressEvent))
	{
		emit signalProgress((float)(proc->GetProgress() * 100.0));
	}
	else if (typeid(event) == typeid(itk::StartEvent))
	{
        emit signalExecutionStarted(objName);
	}
	else if (typeid(event) == typeid(itk::EndEvent))
	{
        emit signalExecutionStopped(objName);
		emit signalProgress(0);
		this->mOtbProcess->SetAbortGenerateData(false);
		this->mbAbortExecution = false;
	}
	else if (typeid(event) == typeid(itk::AbortEvent))
	{
        emit signalExecutionStopped(objName);
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

void
NMProcess::setImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw)
{
    this->setInputImgTypeSpec(dw);
    this->setOutputImgTypeSpec(dw);
}

void
NMProcess::setInputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw)
{
    if (dw.isNull())
        return;

    this->setInputNMComponentType(dw->getNMComponentType());
    this->setInputNumBands(dw->getNumBands());
    this->setInputNumDimensions(dw->getNumDimensions());
}

void
NMProcess::setOutputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw)
{
    if (dw.isNull())
        return;

    this->setOutputNMComponentType(dw->getNMComponentType());
    this->setOutputNumBands(dw->getNumBands());
    this->setOutputNumDimensions(dw->getNumDimensions());
}

NMItkDataObjectWrapper::NMComponentType
NMProcess::getInputNMComponentType()
{
	NMItkDataObjectWrapper::NMComponentType nmtype;
	switch (this->mInputComponentType)
	{
		case otb::ImageIOBase::UCHAR : nmtype = NMItkDataObjectWrapper::NM_UCHAR	; break;
		case otb::ImageIOBase::CHAR	 : nmtype = NMItkDataObjectWrapper::NM_CHAR	; break;
		case otb::ImageIOBase::USHORT: nmtype = NMItkDataObjectWrapper::NM_USHORT; break;
		case otb::ImageIOBase::SHORT : nmtype = NMItkDataObjectWrapper::NM_SHORT	; break;
		case otb::ImageIOBase::UINT	 : nmtype = NMItkDataObjectWrapper::NM_UINT	; break;
		case otb::ImageIOBase::INT	 : nmtype =	NMItkDataObjectWrapper::NM_INT	;	break;
		case otb::ImageIOBase::ULONG : nmtype =	NMItkDataObjectWrapper::NM_ULONG;    break;
		case otb::ImageIOBase::LONG	 : nmtype =	NMItkDataObjectWrapper::NM_LONG	;   break;
		case otb::ImageIOBase::FLOAT : nmtype =	NMItkDataObjectWrapper::NM_FLOAT; break;
		case otb::ImageIOBase::DOUBLE: nmtype =	NMItkDataObjectWrapper::NM_DOUBLE;	break;
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
		case otb::ImageIOBase::UCHAR	 : nmtype = NMItkDataObjectWrapper::NM_UCHAR	; break;
		case otb::ImageIOBase::CHAR		 : nmtype = NMItkDataObjectWrapper::NM_CHAR	; break;
		case otb::ImageIOBase::USHORT	 : nmtype = NMItkDataObjectWrapper::NM_USHORT; break;
		case otb::ImageIOBase::SHORT	 : nmtype = NMItkDataObjectWrapper::NM_SHORT	; break;
		case otb::ImageIOBase::UINT		 : nmtype = NMItkDataObjectWrapper::NM_UINT	; break;
		case otb::ImageIOBase::INT	 : nmtype =	NMItkDataObjectWrapper::NM_INT	;	break;
		case otb::ImageIOBase::ULONG : nmtype =	NMItkDataObjectWrapper::NM_ULONG;    break;
		case otb::ImageIOBase::LONG	 : nmtype =	NMItkDataObjectWrapper::NM_LONG	;   break;
		case otb::ImageIOBase::FLOAT : nmtype =	NMItkDataObjectWrapper::NM_FLOAT; break;
		case otb::ImageIOBase::DOUBLE: nmtype =	NMItkDataObjectWrapper::NM_DOUBLE;	break;
		default: nmtype = NMItkDataObjectWrapper::NM_UNKNOWN; break;
	}

	return nmtype;
}

void
NMProcess::setInputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype)
{
	otb::ImageIOBase::IOComponentType type;
	switch (nmtype)
	{
		case NMItkDataObjectWrapper::NM_UCHAR: type = otb::ImageIOBase::UCHAR; break;
		case NMItkDataObjectWrapper::NM_CHAR: type = otb::ImageIOBase::CHAR; break;
		case NMItkDataObjectWrapper::NM_USHORT: type = otb::ImageIOBase::USHORT; break;
		case NMItkDataObjectWrapper::NM_SHORT: type = otb::ImageIOBase::SHORT; break;
		case NMItkDataObjectWrapper::NM_UINT: type = otb::ImageIOBase::UINT; break;
		case NMItkDataObjectWrapper::NM_INT: type = otb::ImageIOBase::INT; break;
		case NMItkDataObjectWrapper::NM_ULONG: type = otb::ImageIOBase::ULONG; break;
		case NMItkDataObjectWrapper::NM_LONG: type = otb::ImageIOBase::LONG; break;
		case NMItkDataObjectWrapper::NM_FLOAT: type = otb::ImageIOBase::FLOAT; break;
		case NMItkDataObjectWrapper::NM_DOUBLE: type = otb::ImageIOBase::DOUBLE; break;
		default: type = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

    if (this->mInputComponentType != type)
    {
        this->mInputComponentType = type;
        this->mbIsInitialised = false;
        emit NMProcessChanged();
        emit nmChanged();
    }
}

void
NMProcess::setOutputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype)
{
	otb::ImageIOBase::IOComponentType type;
	switch (nmtype)
	{
		case NMItkDataObjectWrapper::NM_UCHAR: type = otb::ImageIOBase::UCHAR; break;
		case NMItkDataObjectWrapper::NM_CHAR: type = otb::ImageIOBase::CHAR; break;
		case NMItkDataObjectWrapper::NM_USHORT: type = otb::ImageIOBase::USHORT; break;
		case NMItkDataObjectWrapper::NM_SHORT: type = otb::ImageIOBase::SHORT; break;
		case NMItkDataObjectWrapper::NM_UINT: type = otb::ImageIOBase::UINT; break;
		case NMItkDataObjectWrapper::NM_INT: type = otb::ImageIOBase::INT; break;
		case NMItkDataObjectWrapper::NM_ULONG: type = otb::ImageIOBase::ULONG; break;
		case NMItkDataObjectWrapper::NM_LONG: type = otb::ImageIOBase::LONG; break;
		case NMItkDataObjectWrapper::NM_FLOAT: type = otb::ImageIOBase::FLOAT; break;
		case NMItkDataObjectWrapper::NM_DOUBLE: type = otb::ImageIOBase::DOUBLE; break;
		default: type = otb::ImageIOBase::UNKNOWNCOMPONENTTYPE; break;
	}

    if (this->mOutputComponentType != type)
    {
        this->mOutputComponentType = type;
        this->mbIsInitialised = false;
        emit NMProcessChanged();
        emit nmChanged();
    }
}


/**
 * Get the value of mOutputComponentType
 * \return the value of mOutputComponentType
 */
otb::ImageIOBase::IOComponentType NMProcess::getOutputComponentType(void)
{
    return this->mOutputComponentType;
}

