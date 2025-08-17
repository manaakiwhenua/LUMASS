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
#include <QSet>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMModelController.h"
#include "NMDataComponent.h"
#include "NMSequentialIterComponent.h"
#include "NMMfwException.h"
#include "NMProcessFactory.h"
#include "NMImageReader.h"
#include "nmNetCDFIO.h"
#include "utils/muParser/muParserError.h"

///////////////////////////////////////////////
/// NMModelComponentIterator implementation
///////////////////////////////////////////////

void
NMModelComponentIterator::init(NMIterableComponent* ic)
{
    mIterList.clear();
    mComponent = nullptr;
    mListPos = -1;
    if (ic == nullptr)
    {
        return;
    }

    NMModelComponent* c = ic->getInternalStartComponent();
    while (c != nullptr)
    {
        mIterList.push_back(c);
        c = ic->getNextInternalComponent();
    }

    if (mIterList.size())
    {
        mComponent = mIterList.at(0);
        mListPos = 0;
    }
}

///////////////////////////////////////////////
/// NMIterableComponent implementation
///////////////////////////////////////////////

const std::string NMIterableComponent::ctx = "NMIterableComponent";

NMIterableComponent::NMIterableComponent(QObject* parent)
    : NMModelComponent(parent)
{
    this->setParent(parent);
    this->initAttributes();
}

void NMIterableComponent::initAttributes(void)
{
    NMModelComponent::initAttributes();
    this->mMaxInternalTimeLevel = 0;
    this->mProcessChainPointer = nullptr;
    this->mProcessChainStart = nullptr;
    this->mProcess = nullptr;
    this->mIterationStep = 1;
    this->mIterationStepRun = 1;
    this->mIterationStepExpression.clear();
    this->mNumIterations = 1;
    this->mNumIterationsExpression.clear();
}

NMIterableComponent::~NMIterableComponent(void)
{
}

NMModelComponentIterator
NMIterableComponent::getComponentIterator()
{
    return NMModelComponentIterator(this);
}

void
NMIterableComponent::setModelController(NMModelController *controller)
{
    this->mController = controller;
    if (this->mProcess)
    {
        this->mProcess->setModelController(controller);
    }

    NMModelComponentIterator cit = this->getComponentIterator();
    while (*cit != nullptr)
    {
        (*cit)->setModelController(controller);
        ++cit;
    }
}

void
NMIterableComponent::setLogger(NMLogger* logger)
{
    mLogger = logger;
    if (this->mProcess)
    {
        mProcess->setLogger(logger);
    }
}

//void NMIterableComponent::setProcess(NMProcess* proc)
//{
////	NMDebugCtx(ctx, << "...");

//    this->mProcess = proc;
//    this->mProcessChainPointer = nullptr;
//    this->mProcessChainStart = nullptr;
//    if (this->mProcess)
//    {
//        if (mLogger)
//        {
//            this->mProcess->setLogger(mLogger);
//        }
//        this->mProcess->setParent(0);
//        this->mProcess->moveToThread(this->thread());
//        this->mProcess->setParent(this);
//        this->mProcess->setModelController(this->getModelController());
//    }

////	NMDebugCtx(ctx, << "done!");

//    emit NMModelComponentChanged();
//}

QVariant
NMIterableComponent::getModelParameter(const QString& paramSpec)
{
    QString fullSpec = paramSpec;
    QStringList spec = paramSpec.split(":", Qt::SkipEmptyParts);
    if (spec.size() < 2)
    {
        // convert 1-based iteration steps to 0-based index
        int step = static_cast<int>(this->getIterationStep());
        step = step-1 < 0 ? 0 : step-1;
        fullSpec = QString("%1:%2").arg(paramSpec)
                                   .arg(step);
    }

    return NMModelComponent::getModelParameter(fullSpec);
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

    NMModelComponentIterator cit = this->getComponentIterator();
    while(*cit != nullptr)
    {
        ++ret;
        ++cit;
    }

    return ret;
}


void NMIterableComponent::addModelComponent(NMModelComponent* comp)
{
//	NMDebugCtx(ctx, << "...");

    if (comp == nullptr)
        return;

    // if this component has already a process assigned to it
    // we don't add any components
    // ToDo: raise an appropriate exception
    if (this->mProcess != nullptr)
        return;

    // normally, we maintain components time levels to support
    // loading models from file, however: nested components must
    // not have lower time levels than the parent component;
    if (comp->getTimeLevel() < this->getTimeLevel())
        comp->setTimeLevel(this->getTimeLevel());

    NMDebugAI(<< "parent: '" << this->objectName().toStdString() << "'" << std::endl);
    NMDebugAI(<< "  -> add child:  '" << comp->objectName().toStdString() << "'" << std::endl);
//	NMDebugAI(<< "new chain: ");
    comp->setHostComponent(this);

    NMModelComponentIterator cit = this->getComponentIterator();
    NMModelComponent* lastComp = *cit;
    if (lastComp == nullptr)
    {
        this->mProcessChainStart = comp;
//		NMDebug(<< "'" << comp->objectName().toStdString() << "'" << std::endl);
//		NMDebugCtx(ctx, << "done!");
        return;
    }
//	NMDebug(<< " '" << lastComp->objectName().toStdString() << "'");

    //while (lastComp->getDownstreamModelComponent() != 0)
    ++cit;
    while(*cit != nullptr)
    {
        lastComp = *cit;//this->getNextInternalComponent();
        ++cit;
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
        const QMap<QString, NMModelComponent *> &levelComps,
        unsigned int timeLevel, int step)
{
    //NMDebugCtx(this->objectName().toStdString(), << "...");

    // identify all available ends within this group component with
    // respect to the container's time level
    QStringList deadEnds = this->findExecutableComponents(levelComps,
                timeLevel, step);

    // find for each of those ends the upstream components
    // constituting the pipeline

    // Deprecated
        // until they
        // hit the next upstream non-piped component
        //QList<QStringList> subPipes;
    foreach(const QString de, deadEnds)
    {
        NMModelComponent* comp = this->getModelController()->getComponent(de);
        if (comp == nullptr)
            continue;

        NMIterableComponent* icomp = qobject_cast<NMIterableComponent*>(comp);
        if (icomp != nullptr)
        {
            if (icomp->evalNumIterationsExpression(step) == 0)
            {
                continue;
            }
        }

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
                    this->getModelController()->getComponent(compName));
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

    NMModelController* ctrl = this->getModelController();
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
    if (this->mProcess != nullptr)
        return;

    if (insertComp == nullptr)
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
    NMModelComponentIterator cit = this->getComponentIterator();
    while(*cit != 0)
    {
        if (cit->objectName().compare(compName, Qt::CaseInsensitive) == 0)
        {
            return *cit;
        }
        ++cit;
    }

    return *cit;
}

NMModelComponent*
NMIterableComponent::findComponentByUserId(const QString& userId, const int timeLevel)
{
    NMModelComponentIterator cit = this->getComponentIterator();
    while(*cit != 0)
    {
        if (cit->getUserID().compare(userId, Qt::CaseInsensitive) == 0)
        {
            if (timeLevel >= 0)
            {
                if (cit->getTimeLevel() > timeLevel)
                {
                    return *cit;
                }
            }
            else
            {
                return *cit;
            }
        }
        ++cit;
    }

    return *cit;
}

void
NMIterableComponent::getUpstreamPipelineComponents(QStringList& upstrPipeComps)
{
    foreach(const QStringList& inList, this->getInputs())
    {
        foreach(const QString& input, inList)
        {
            // accounts for name suffixes for input components that provide
            // more than one output component
            //     012345678910
            // eg: BMIModel2:fail
            QString compName = input;
            int suffixPos = input.indexOf(QChar(':'));
            if (suffixPos > 0)
            {
                compName = input.left(suffixPos);
            }
            NMModelComponent* comp = this->mController != nullptr ?
                                     this->mController->getComponent(compName) :
                                     nullptr;
            if (comp != nullptr)
            {
                NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
                if (ic != nullptr)
                {
                    ic->getUpstreamPipelineComponents(upstrPipeComps);
                }
                upstrPipeComps.push_back(compName);
            }
        }
    }
}

NMModelComponent*
NMIterableComponent::findUpstreamComponentByUserId(const QString& userId)
{
    NMIterableComponent* host = this;
    NMModelComponent* mc = nullptr;

    // ... are we the one ?
    if (this->getUserID().compare(userId, Qt::CaseInsensitive) == 0)
    {
        return  this;
    }

    // ... look around ...
    if (this->mProcess != nullptr)
    {
        // STEP 1: investigate upstream pipeline components
        QStringList upstrComps;
        getUpstreamPipelineComponents(upstrComps);
        foreach(const QString& input, upstrComps)
        {
            NMModelComponent* incomp = this->mController != nullptr ?
                                        this->mController->getComponent(input) :
                                        nullptr;
            if (incomp != nullptr && incomp->getUserID().compare(userId, Qt::CaseInsensitive) == 0)
            {
                return incomp;
            }
        }

        // STEP 2: look at sibling components at higher time levels
        host = this->getHostComponent();
        if (host != nullptr)
        {
            mc = host->findComponentByUserId(userId, this->getTimeLevel());
            if (mc == nullptr)
            {
                host = host->getHostComponent();
            }
        }
    }


    // ... look above (i.e. next hierarchy level) ...
    while (host != nullptr && mc == nullptr)
    {
        mc = host->findComponentByUserId(userId);
        if (mc == nullptr)
        {
            host = host->getHostComponent();
        }
    }

    return mc;
}


NMModelComponent* NMIterableComponent::removeModelComponent(const QString& compName)
{
    NMDebugCtx(ctx, << "...");

    NMModelComponent* comp = this->findModelComponent(compName);
    if (comp == nullptr)
        return 0;

    NMDebugAI( << this->objectName().toStdString() << ": removing '"
            << compName.toStdString() << "'" << std::endl);
    string tname = "";
    NMModelComponent* up = comp->getUpstreamModelComponent();
    tname = up != nullptr ? up->objectName().toStdString() : "0";
    NMDebugAI( << "   up-comp is " << tname << std::endl);
    NMModelComponent* down = comp->getDownstreamModelComponent();
    tname = down != nullptr ? down->objectName().toStdString() : "0";
    NMDebugAI( << "   down-comp is " << tname << std::endl);
    if (up == nullptr)// && comp == this->mProcessChainStart)
    {
        if (down != nullptr)
        {
            this->mProcessChainStart = down;
            down->setUpstreamModelComponent(nullptr);
            NMDebugAI( << "   new start comp is '" << down->objectName().toStdString() << "'" << std::endl);
        }
        else
        {
            NMDebugAI( << comp->objectName().toStdString() << " is empty now" << std::endl);
            this->mProcessChainStart = nullptr;
        }
    }
    else if (up != nullptr)
    {
        up->setDownstreamModelComponent(down);
        NMDebugAI( << "   new down comp is '" << (down != nullptr ? down->objectName().toStdString() : "0") << "'" << std::endl);
        if (down != nullptr)
        {
            down->setUpstreamModelComponent(up);
            NMDebugAI( << "   new up comp is '" << up->objectName().toStdString() << "'" << std::endl);
        }
    }

    this->mProcessChainPointer = nullptr;

    // release all ties to the host's remaining
    // components
    comp->setHostComponent(nullptr);
    comp->setUpstreamModelComponent(nullptr);
    comp->setDownstreamModelComponent(nullptr);

    NMDebugCtx(ctx, << "done!");
    emit NMModelComponentChanged();
    return comp;
}

void NMIterableComponent::destroySubComponents(QMap<QString, NMModelComponent*>& repo)
{
    if (this->mProcess == 0)
    {
        NMModelComponentIterator cit = this->getComponentIterator();
        NMIterableComponent* dc = 0;
        while(*cit != 0)
        {
            dc = qobject_cast<NMIterableComponent*>(*cit);
            if (dc != 0)
            {
                dc->destroySubComponents(repo);
                repo.remove(dc->objectName());
                delete dc;
                dc = 0;
            }
            else
            {
                repo.remove(cit->objectName());
                NMModelComponent* sc = *cit;
                delete sc;
                sc = 0;
            }
            ++cit;
        }
    }
    this->setInternalStartComponent(0);
}

void NMIterableComponent::setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg, const QString& name)
{
    if (this->mProcess != 0)
    {
        this->mProcess->setNthInput(idx, inputImg, name);
        return;
    }

    NMModelComponent* sc = this->mProcessChainStart;
    sc->setNthInput(idx, inputImg, name);
}

NMModelComponent* NMIterableComponent::getLastInternalComponent(void)
{
    NMModelComponent* ret = 0;
    NMModelComponentIterator cit = this->getComponentIterator();
    while (*cit != 0)
    {
        ret = *cit;
        ++cit;
    }

    return ret;
}


QSharedPointer<NMItkDataObjectWrapper>
NMIterableComponent::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> ret;
    // check, whether we've got a 'single' process component
    if (this->mProcess != 0)
    {
        NMLogDebug(<< this->objectName().toStdString()
                << "->getOutput(" << idx << ")" << std::endl);

        ret = mProcess->getOutput(idx);
        if (!ret.isNull())
        {
            if (ret->getOTBTab().IsNotNull())
            {
                ret->getOTBTab()->AddObserver(itk::NMLogEvent(), mProcess->getObserver());
            }
        }
        else
        {
            NMLogWarn(<< this->objectName().toStdString() <<
                    ": No output available at idx= " << idx << " !");
        }
        return ret;
    }


    NMLogWarn(<< this->objectName().toStdString() <<
            ": No output available!");
    return ret;

}

QSharedPointer<NMItkDataObjectWrapper>
NMIterableComponent::getOutput(const QString &name)
{
    QSharedPointer<NMItkDataObjectWrapper> ret;
    if (this->mProcess != nullptr)
    {
        ret = mProcess->getOutput(name);
        if (!ret.isNull())
        {
            if (ret->getOTBTab().IsNotNull())
            {
                ret->getOTBTab()->AddObserver(itk::NMLogEvent(), mProcess->getObserver());
            }
        }
        else
        {
            NMLogWarn(<< this->objectName().toStdString() <<
                    ": No output available!");
        }
    }

    NMLogWarn(<< this->objectName().toStdString() << ": No output '"
              << name.toStdString() << "'");

    return ret;
}

QStringList
NMIterableComponent::getOutputNames(void)
{
    QStringList ret;
    if (this->mProcess != nullptr)
    {
        ret = this->mProcess->getOutputNames();
    }
    return ret;
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

    if (this->mProcess != nullptr && this->getTimeLevel() == timeLevel)
    {
        //NMDebugAI(<< "\tinstantiate process '" << this->mProcess->objectName().toStdString() << "'" << std::endl);
        bool binit = true;

        NMSequentialIterComponent* sic = qobject_cast<NMSequentialIterComponent*>(this);
        if (    sic != nullptr
             && sic->evalNumIterationsExpression(1) == 0
           )
        {
            binit = false;
        }

        if (binit)
        {
            this->mProcess->instantiateObject();
            this->mProcess->setLogger(this->mLogger);
        }
        //NMDebugCtx(this->objectName().toStdString(), << "done!");
        return;
    }

    NMModelComponentIterator cit = this->getComponentIterator();
    while (*cit != 0)
    {
        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(*cit);
        if (ic != 0 && ic->getTimeLevel() == timeLevel && ic->countComponents() == 0)
        {
            //NMDebugAI(<< "initialise '" << ic->objectName().toStdString()
            //         << "' ... " << std::endl);
            ic->initialiseComponents(timeLevel);
        }
        ++cit;
    }

    //	NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void
NMIterableComponent::changeTimeLevel(int diff)
{
    // we call the superclass implementation first
    NMModelComponent::changeTimeLevel(diff);

    NMModelComponentIterator cit = this->getComponentIterator();
    while (*cit != 0)
    {
        cit->changeTimeLevel(diff);
        ++cit;
    }

    //emit TimeLevelChanged(this->mTimeLevel);
    //emit NMModelComponentChanged();
}

unsigned int
NMIterableComponent::evalNumIterationsExpression(const unsigned int& step)
{
    unsigned int niter = mNumIterations;

    // note: step is 1-based as mIterationStep
    if (step >= 1
        && this->mNumIterationsExpression.size() != 0)
    {
        // emploly the NM_USE_UP (and continue to use the last one) parameter policy
        int exprIdx = step-1;
        if (exprIdx >= this->mNumIterationsExpression.size())
        {
            exprIdx = this->mNumIterationsExpression.size()-1;
        }

        QString numIterStr = this->getModelController()->processStringParameter(this,
                                                                         mNumIterationsExpression.at(exprIdx));

        // we don't interprete an empty string as error but as
        // a non set parameter
        if (numIterStr.isEmpty())
        {
            return mNumIterations;
        }


        bool bok = false;
        unsigned int titer = numIterStr.toUInt(&bok);
        if (bok)
        {
            niter = titer;
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

            NMModelController::MPICompProg compProg;
            compProg.compName = this->objectName();
            compProg.event = NMModelController::NM_EVENT_EXEC_STOPPED;
            compProg.progress = 0;
            mController->mpiSignalProgress(compProg);

            this->mIsUpdating = false;
            throw me;
        }
    }

    return niter;
}

void
NMIterableComponent::setNumIterations(unsigned int numiter)
{
    if (this->mNumIterations != numiter && numiter > 0)
    {
        this->mNumIterations = numiter;
        emit NumIterationsChanged(numiter);
    }
}

void
NMIterableComponent::setNumIterationsExpression(QStringList iterExpr)
{
    this->mNumIterationsExpression = iterExpr;
    emit NumIterationsExpressionChanged();
}


void NMIterableComponent::linkComponents(unsigned int step,
        const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(this->objectName().toStdString(), << "...");

    this->processUserID();
    //mNumIterations = this->evalNumIterationsExpression(mIterationStep);
    mNumIterations = this->evalNumIterationsExpression(step);
    if (this->mProcess != 0)
    {
        this->mProcess->linkInPipeline(step, repo);
        NMDebugCtx(this->objectName().toStdString(), << "done!");
        return;
    }

    NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void NMIterableComponent::mapTimeLevels(unsigned int startLevel,
        QMap<unsigned int, QMap<QString, NMModelComponent*> >& timeLevelMap)
{
    NMDebugAI(<< this->objectName().toStdString() << "::mapTimeLevels(" << startLevel
              << ", timeLevelMap)..." << endl);

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
        NMModelComponentIterator cit = this->getComponentIterator();
        while (*cit != 0)
        {
            if (cit->getTimeLevel() >= startLevel)
            {
                it = timeLevelMap.find(cit->getTimeLevel());
                if (it != timeLevelMap.end())
                {
                    it.value().insert(cit->objectName(), *cit);
                }
                else
                {
                    QMap<QString, NMModelComponent*> newLevel;
                    newLevel.insert(cit->objectName(), *cit);
                    timeLevelMap.insert(cit->getTimeLevel(), newLevel);
                }
            }
            ++cit;
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

    // prevent chasing our own tail
    if (mIsUpdating)
    {
        NMMfwException ul(NMMfwException::NMModelComponent_RecursiveUpdate);
        ul.setSource(this->objectName().toStdString());
        std::stringstream msg;
        msg << this->objectName().toStdString()
            << " is already updating itself!";
        ul.setDescription(msg.str());
        NMDebugCtx(ctx, << "done!");
        return;
    }
    mIsUpdating = true;


    NMDebugCtx(this->objectName().toStdString(), << "...");
    NMDebugAI(<< "--- " << this->objectName().toStdString() << " ---" << std::endl);

    emit signalExecutionStarted();

    NMModelController::MPICompProg compProg;
    compProg.compName = this->objectName();
    compProg.event = NMModelController::NM_EVENT_EXEC_STARTED;
    compProg.progress = 0;
    mController->mpiSignalProgress(compProg);

    // check, whether we're supposed to run at all
    NMModelController* controller = this->getModelController();//= qobject_cast<NMModelController*>(this->parent());
    if (controller != nullptr)
    {
        if (controller->isModelAbortionRequested())
        {
            NMLogInfo(<< "Model abortion requested!" << endl);
            NMDebugAI(<< "Model abortion requested!" << endl);
            NMDebugCtx(this->objectName().toStdString(), << "done!");
            emit signalExecutionStopped();
            compProg.event = NMModelController::NM_EVENT_EXEC_STOPPED;
            mController->mpiSignalProgress(compProg);
            return;
        }
    }
    else
    {
        NMLogError(<< this->objectName().toStdString()
                << ": We'd better quit here - there's no controller in charge!" << endl);
        NMDebugCtx(this->objectName().toStdString(), << "done!");
        emit signalExecutionStopped();
        compProg.event = NMModelController::NM_EVENT_EXEC_STOPPED;
        mController->mpiSignalProgress(compProg);
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

#ifdef LUMASS_DEBUG
    if (this->mMapTimeLevelComp.size() == 0)
    {
        NMDebugAI(<< "no sub components detected!" << std::endl);
    }

    foreach(const unsigned int& level, this->mMapTimeLevelComp.keys())
    {
        NMDebugAI(<< "#" << level << " ... " << endl);
        foreach(const QString& name, this->mMapTimeLevelComp.value(level).keys())
        {
            NMDebugAI(<< "   " << name.toStdString() << endl);
        }
    }
#endif

    this->iterativeComponentUpdate(repo, minLevel, maxLevel);

    emit signalExecutionStopped();
    compProg.event = NMModelController::NM_EVENT_EXEC_STOPPED;
    mController->mpiSignalProgress(compProg);
    mIsUpdating = false;
    NMDebugCtx(this->objectName().toStdString(), << "done!");
}

void
NMIterableComponent::componentUpdateLogic(const QMap<QString, NMModelComponent*>& repo,
            unsigned int minLevel, unsigned int maxLevel, unsigned int step)
{
    NMDebugCtx(this->objectName().toStdString(), << "...");
    NMModelController* controller = this->getModelController();

    // don't need another variable for step, just confusing
    //unsigned int i = step;

    if (controller->isModelAbortionRequested())
    {
        NMDebugAI(<< "The user elected to abort model execution, so we break out here!" << endl);
        NMDebugCtx(this->objectName().toStdString(), << "done!");
        return;
    }

    NMDebugAI(<< ">>>> START ITERATION #" << step+1 << std::endl);


    // some info we can use later for debug in the event of an exception
    NMModelComponent* comp = 0;
    QString hostName;
    int hostStep;

    bool bThrow = false;
    std::stringstream exStackInfo;
    std::stringstream exDescription;
    std::string exObjName;
    std::string exSource;
    NMMfwException::ExceptionType exceptionType;

    // ==============================================================================
    // PROVENANCE
    QStringList args;
    QStringList attrs;
    QDateTime startTime;
    QDateTime endTime;
    QString actId = QString("nm:%1_Update-%2").arg(this->objectName()).arg(this->getIterationStep());
    QString agId = QString("nm:%1").arg(this->objectName());

    // ==============================================================================
    // UPDATE LOGIC

    // declarations potentially required in catch-block for clean exit under excpetion
    int commRank = 0;
    QMap<int, QPair<QString, MPI_Comm>> mapRankNameSplitComm;

    try
    {

    // =================================== PROCESS COMPONENT =========================
    // when we've got a process component, the process is straightforward
    // and we just link and execute this one
    if (this->mProcess != nullptr)
    {
        exObjName = this->objectName().toStdString();
        NMDebugAI(<< "update " << this->objectName().toStdString() << "'s process..." << std::endl);
        NMLogDebug(<< "update " << this->objectName().toStdString() << "'s process...");
        if (!this->mProcess->isInitialised())
            this->mProcess->instantiateObject();
        this->mProcess->setLogger(mLogger);

        // log provenance
        unsigned int hostStep = 1;
        if (this->getHostComponent() != nullptr)
        {
            hostStep = this->getHostComponent()->getIterationStep();
        }
        actId = QString("nm:%1_Update-%2").arg(this->objectName()).arg(hostStep);

        attrs.clear();// = this->mProcess->getRunTimeParaProvN();

        startTime = QDateTime::currentDateTime();
        this->mProcess->linkInPipeline(step, repo);

        args.clear();
        args << actId << "-";
        NMIterableComponent* host = this->getHostComponent();
        QString hostActProv;
        if (host)
        {
            unsigned int hStep = host->getIterationStep();
            hostActProv = QString("nm:%1_Update-%2")
                            .arg(host->objectName())
                            .arg(hStep);
            args << hostActProv;
        }
        else
        {
            args << "-";
        }
        args << startTime.toString(controller->getSetting("TimeFormat").toString());
        controller->getLogger()->logProvN(NMLogger::NM_PROV_START, args, attrs);


        // execute process / pipeline
        this->mProcess->update();

        // more provenenace
        endTime = QDateTime::currentDateTime();

        args.clear();
        args << actId << "-";
        if (host)
        {
            args << hostActProv;
        }
        else
        {
            args << "-";
        }
        //args << endTime.toString(Qt::ISODate);
        args << endTime.toString(controller->getSetting("TimeFormat").toString());
        attrs.clear();
        controller->getLogger()->logProvN(NMLogger::NM_PROV_END, args, attrs);

        NMDebugCtx(this->objectName().toStdString(), << "done!");


        // log provenance


        //NMLogProv(NMLogger::NM_PROV_ACTIVITY, args, attrs);
        //        controller->getLogger()->logProvN(NMLogger::NM_PROV_ACTIVITY, args, attrs);

        //        args.clear();
        //        args << actId << agId;
        //        controller->getLogger()->logProvN(NMLogger::NM_PROV_ASSOCIATION, args, attrs);
        return;
    }

    // =================================== AGGREGATE COMPONENT =========================
    // provenance
    // we create a new activity for each iteration step of the aggr component
    attrs.clear();
    args.clear();

    args << actId << "-" << "-";
    controller->getLogger()->logProvN(NMLogger::NM_PROV_ACTIVITY, args, attrs);

    attrs.clear();
    args.clear();
    args << actId << agId << "-";
    controller->getLogger()->logProvN(NMLogger::NM_PROV_ASSOCIATION, args, attrs);


    // =========================================================================
    //      Init MPI for parallel proc
    // =========================================================================

    // get responsible comm for this component
    MPI_Comm comm = controller->getNextUpstrMPIComm(this->objectName());

    int _cn_len;
    char _commName[MPI_MAX_OBJECT_NAME];
    ::sprintf(_commName, "MPI_COMM_MULL");
    if (comm != MPI_COMM_NULL)
    {
        MPI_Comm_get_name(comm, _commName, &_cn_len);
    }

    int commProcs = 1;
    int worldRank = 0;
    int worldProcs = 1;

    if (comm != MPI_COMM_NULL)
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
        MPI_Comm_size(MPI_COMM_WORLD, &worldProcs);
        MPI_Comm_size(comm, &commProcs);
        MPI_Comm_rank(comm, &commRank);
    }

    std::stringstream exmsg;
    exmsg << "lr" << commRank << ": step #" << step;
    NMDebugCtx(this->objectName().toStdString(), << exmsg.str());

    //// have all procs on this branch of execution catch up to this point
    NMDebugAI(<< "commRank-#" << commRank << " of comm-#" << _commName << " is running " << this->objectName().toStdString()
              << " on #" << commProcs << " procs..." << endl);

    if (comm != MPI_COMM_NULL)
    {
        NMDebugAI(<< "*** waiting at " << this->objectName().toStdString()
                  << "'s ENTRY barrier prior to task allocation/execution ..." << std::endl);
        MPI_Barrier(comm);
    }
    // =========================================================================

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
        timeIt = this->mMapTimeLevelComp.constFind(level);
        if (timeIt != this->mMapTimeLevelComp.cend())
        {
            this->createExecSequence(execList, timeIt.value(), level, step);
        }

        ///////////////// DEBUG
        // let's have a look what we've got so far ...
        NMDebug(<< endl);
        NMDebugAI(<< "PIPELINES AND EXECUTION ORDER ON LEVEL "
                  << level
                  << " STEP " << this->getIterationStep()
                  <<  " ------------" << endl);
        int cnt = 0;
        foreach(const QStringList& sp, execList)
        {
            NMDebugAI(<< "#" << cnt << ": " << sp.join("-->").toStdString() << endl);
            ++cnt;
        }
        NMDebug(<< endl);
        ////////////////// DEBUG


        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //          INIT PARALLEL EXECUTION OF COMPs ON THE GIVEN TIME LEVEL
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        QStringList rankExecComps;
        QStringList rankRegComps;
        QStringList rankPioWriters;

        QList<QStringList> parallelTasks, sequentialTasks;
        QStringList parallelExe, sequentialExe, parallelWriters;
        QVector<int> parallelWriterProcs;
        QMap<QString, QStringList> mapExePipe;
        QMap<int, std::pair<std::unordered_set<int>, std::string>> mapSplitRanksTasks;
        mapRankNameSplitComm.clear();

        //MPI_Comm splitComm = MPI_COMM_NULL;
        if (commProcs > 1)
        {
            // distinguish between sequential st and parallel tasks pt
            NMDebugAI( << "*** parallel parallel execution of tasks on " << this->objectName().toStdString()
                       << "'s TimeLevel #" << level << std::endl);

            // identify components/pipelines that may utilise
            // more than one process
            foreach(const QStringList& pipe, execList)
            {
                NMModelComponent* comp = controller->getComponent(pipe.last());
                NMIterableComponent* icomp = qobject_cast<NMIterableComponent*>(comp);

                mapExePipe.insert(pipe.last(), pipe);

                /// ToDo: need to look at this at one point!
                /// HAVE IMPLEMENTED THIS INTO ::createExecSequence();
                // we disregard disabled components
                //NMSequentialIterComponent* sicomp = qobject_cast<NMSequentialIterComponent*>(icomp);
                //if (    sicomp->evalNumIterationsExpression(step) == 0
                //    ||  (    sicomp->evalNumIterationsExpression(step) == sicomp->getNumIterations()
                //          && sicomp->getIterationStep() > sicomp->getNumIterations()
                //        )
                //   )
                //{
                //    continue;
                //}


                if (icomp != nullptr && icomp->getProcess() != nullptr)
                {
                    if (icomp->objectName().startsWith(QStringLiteral("ImageWriter")))
                    {
                        QVariant writeProcsVar = icomp->getProcess()->property("WriteProcs");
                        if (writeProcsVar.isValid())
                        {
                            bool bOK;
                            int writeProcs = writeProcsVar.toInt(&bOK);
                            if (bOK)
                            {
                                if (writeProcs > 1)
                                {
                                    parallelTasks.push_back(pipe);
                                    parallelExe.push_back(pipe.last());
                                    parallelWriters.push_back(pipe.last());
                                    parallelWriterProcs.push_back(writeProcs);
                                }
                                else
                                {
                                    sequentialTasks.push_back(pipe);
                                    sequentialExe.push_back(pipe.last());
                                }
                            }
                        }
                    }
                    else
                    {
                        sequentialTasks.push_back(pipe);
                        sequentialExe.push_back(pipe.last());
                    }
                }
                else if (icomp != nullptr && icomp->objectName().startsWith(QStringLiteral("AggrComp")))
                {
                    parallelTasks.push_back(pipe);
                    parallelExe.push_back(pipe.last());
                }
                else
                {
                    sequentialTasks.push_back(pipe);
                    sequentialExe.push_back(pipe.last());
                }
            }


            NMDebugAI(<< "parallelExe: " << parallelExe.join(" ").toStdString());
            NMDebugAI(<< "parallelWriters: " << parallelWriters.join(" ").toStdString());
            NMDebugAI(<< "parallelWriterProcs: ");
            for (int wp=0; wp < parallelWriterProcs.size(); ++wp)
            {
                NMDebug(<< parallelWriterProcs.at(wp) << " ");
            }
            NMDebugAI(<< "sequentialExe: " << sequentialExe.join(" ").toStdString());

            // list / vector of REMAINING exe / writer / write proc demand
            QStringList _parallelExe = parallelExe;
            QStringList _sequentialExe = sequentialExe;
            QStringList _parallelWriters = parallelWriters;
            std::vector<int> _parallelWriterProcs(parallelWriterProcs.begin(), parallelWriterProcs.end());

            const int num_paraTasks = parallelExe.size();
            const int num_seqTasks = sequentialExe.size();
            const int num_totalTasks = num_paraTasks + num_seqTasks;

            std::deque<int> idleProcs;
            for (int r=0; r < commProcs; ++r)
            {
                idleProcs.push_back(r);
            }

            // ========================== TASK ALLOCATION ==================================
            // To execute tasks in parallel, we split the communicator registered with this
            // Aggregate Component into as many communicators as there are tasks that
            // can be executed concurrently given the number of ranks available. As we may
            // have more tasks that could be executed concurrently as available ranks,
            // we may need more than one iteration.
            // While allocating ranks to tasks, we distinguish between sequential and
            // parallel tasks. Parallel tasks are able to utilise more than one process (rank)
            // during their execution. This is the case for Aggregate Components (nested
            // inside this Aggregate Component) and for processing pipelines with a parallel
            // ImageWriter component. While ImageWriter components are actually checked
            // for being configured for parallel writing before allocating more than one
            // rank, Aggregate Components are not checked for containing sub components
            // that could actually be processed in parallel.
            // In each iteration, we're aiming to allocate all available ranks across the
            // available tasks on a given time level. Each task receives it's own communicator
            // identified by its allocated splitID (>= 0). Any ranks not allocated to a sequential
            // or a parallel task are assigned splitID=-1 and MPI_COMM_NULL, respectively.
            // Once all ranks are allocated, the associated tasks are executed. Once ALL tasks
            // have completed, their associated communicators are freed. Any remaining tasks
            // left to be executed on this time level are being dealt with in the next
            // iteration.

            int cnt = 1;
            int splitID = 0;
            int para_done = 0, seq_done = 0, total_done = 0;
            while (commProcs > 0 && total_done < num_totalTasks && cnt <= num_totalTasks)
            {
                MPI_Comm splitComm = MPI_COMM_NULL;
                int total_left = _parallelExe.size() + _sequentialExe.size();

                if (commRank == 0)
                {
                    NMDebug(<< "***>>> ROUND #" << cnt << " - allocating tasks <<<***" << std::endl);
                    NMDebugAI(<< "idle procs: " << idleProcs.size() << ": (");
                    for (int ip=0; ip < idleProcs.size(); ++ip)
                    {
                        NMDebug(<< idleProcs.at(ip) << " ");
                    }
                    NMDebug(<< ")" << std::endl);
                    NMDebugAI(<< "total tasks left: " << total_left << "(" << num_totalTasks << ")" << std::endl);
                    NMDebugAI(<< "seq tasks left:   " << _sequentialExe.size() << "(" << num_seqTasks << ")" << std::endl);
                    NMDebugAI(<< "para tasks left:  " << _parallelExe.size()  << "(" << num_paraTasks << ")" << std::endl);
                    NMDebugAI(<< "procs available:  " << idleProcs.size() << "(" << commProcs << ")" << std::endl);
                    NMDebug(<< std::endl);

                    NMDebugAI(<< "***>>> sequential tasks <<<***" << std::endl);
                }

                // -------------------------- SEQUENTIAL ----------------------------------------
                if (commRank == 0 && seq_done >= num_seqTasks)
                {
                    NMDebugAI(<< "     nothing to be done here!   " << std::endl << std::endl);
                }

                while (idleProcs.size() > 0 && seq_done < num_seqTasks)
                {
                    std::pair<std::unordered_set<int>, std::string> seq_taskAdmin = {{}, ""};
                    const QString sexe = _sequentialExe.front();
                    seq_taskAdmin.second = sexe.toStdString();
                    _sequentialExe.pop_front();

                    const int seqRank = idleProcs.front();
                    seq_taskAdmin.first.insert(seqRank);
                    idleProcs.pop_front();

                    if (seqRank == commRank)
                    {
                        std::string seqSplitCommName = seq_taskAdmin.second + "-" + std::to_string(splitID);
                        NMDebugAI(<< "*** MPI_Comm_split(comm, "<< splitID << ", " << seqRank
                                       << ", " << seqSplitCommName << ")" << std::endl);
                        MPI_Comm_split(comm, splitID, seqRank, &splitComm);
                        MPI_Comm_set_name(splitComm, seqSplitCommName.c_str());
                        mController->registerParallelGroup(sexe, splitComm);
                        mapRankNameSplitComm.insert(commRank, qMakePair(sexe, splitComm));
                    }
                    mapSplitRanksTasks.insert(splitID, seq_taskAdmin);

                    ++splitID;
                    ++seq_done;
                    ++total_done;
                }


                // -------------------------- PARALLEL ----------------------------------------
                // calc resources for parallel tasks
                int procs_left = idleProcs.size();
                int pprocs_per_task = 1, __pprocs_per_task = 0;
                if (_parallelExe.size() > 0 && procs_left > _parallelExe.size())
                {
                    pprocs_per_task = std::floor(procs_left / double(_parallelExe.size()));
                }
                int rest_procs = procs_left - pprocs_per_task * _parallelExe.size();

                if (commRank == 0)
                {
                    NMDebug(<< std::endl);
                    NMDebugAI(<< "procs available:  " << procs_left << "(" << commProcs << ")" << std::endl);
                    NMDebugAI(<< "pprocs per ptask: " << pprocs_per_task << std::endl);
                    NMDebugAI(<< "rest_procs:       " << rest_procs << std::endl);
                    NMDebug(<< std::endl);
                    NMDebugAI(<< "***>>> parallel tasks <<<***" << std::endl);
                }

                //const int para_left = _parallelExe.size();
                for (int p=0, rp=0; p < procs_left && para_done < num_paraTasks; p += __pprocs_per_task, ++rp, ++para_done, ++total_done, ++splitID)
                {
                    __pprocs_per_task = pprocs_per_task;
                    if (rp < rest_procs)
                    {
                        ++__pprocs_per_task;
                        NMDebugAI(<< "*** parallel task with splitID=" << splitID
                                  << " receives 1 extra process!" << std::endl);
                    }

                    std::pair<std::unordered_set<int>, std::string> taskAdmin = {{}, ""};
                    const QString pexe = _parallelExe.front();
                    taskAdmin.second = pexe.toStdString();
                    _parallelExe.pop_front();
                    // only allocate the configured WriteProcs to parallel writer
                    const int pw_id = _parallelWriters.indexOf(pexe);
                    if (pw_id >= 0)
                    {
                        __pprocs_per_task = std::min(_parallelWriterProcs.at(pw_id), __pprocs_per_task);
                        rest_procs += __pprocs_per_task - pprocs_per_task;
                    }

                    for (int pp=0; pp < __pprocs_per_task; ++pp)
                    {
                        const int procRank = idleProcs.front();
                        taskAdmin.first.insert(procRank);
                        idleProcs.pop_front();

                        // if current rank is part of the crew running this task, it needs to call MPI_Comm_split for registration
                        if (commRank == procRank)
                        {
                            std::string paraCommName = taskAdmin.second + "-" + std::to_string(splitID);
                            NMDebugAI(<< "*** MPI_Comm_split(comm, "<< splitID << ", " << commRank
                                           << ", " << paraCommName << ")" << std::endl);
                            MPI_Comm_split(comm, splitID, commRank, &splitComm);
                            MPI_Comm_set_name(splitComm, paraCommName.c_str());
                            mController->registerParallelGroup(pexe, splitComm);
                            mapRankNameSplitComm.insert(commRank, qMakePair(pexe, splitComm));
                        }
                    }
                    mapSplitRanksTasks.insert(splitID, taskAdmin);
                }


                // make sure we've assigned all ranks to a split communicator
                while(idleProcs.size() > 0)
                {
                    const int procRank = idleProcs.front();
                    idleProcs.pop_front();
                    if (commRank == procRank)
                    {
                        NMDebugAI(<< "*** MPI_Comm_split(comm, "<< -1 << ", " << commRank
                                       << ", MPI_COMM_NULL)" << std::endl);
                        MPI_Comm_split(comm, -1, commRank, &splitComm);
                    }
                }

                // ========================== TASK EXECUTION ==================================
                 if (commRank == 0)
                {
                    NMDebug(<< std::endl);
                    NMDebugAI(<< "***>>> running tasks <<<***" << std::endl);
                }

                // run tasks
                auto srt_it = mapSplitRanksTasks.cbegin();
                for (; srt_it != mapSplitRanksTasks.cend(); ++srt_it)
                {
                    auto procIt = srt_it.value().first.cbegin();
                    for (; procIt != srt_it.value().first.cend(); ++procIt)
                    {
                        // if commRank is part of the crew running this, by all means chip in!!
                        if (*procIt == commRank)
                        {
                            // find the pipeline for the task
                            auto pipeit = mapExePipe.constFind(srt_it.value().second.c_str());
                            if (pipeit == mapExePipe.constEnd())
                            {
                                NMMfwException ex(NMMfwException::NMModelComponent_InvalidParameter);
                                ex.setSource(srt_it.value().second);
                                QString msg = QString("Executable component '%1' couldn't be mapped to a valid task!")
                                                .arg(srt_it.value().second.c_str());
                                ex.setDescription(msg.toStdString());
                                throw ex;
                            }

                            NMDebugAI(<< "*** r" << commRank << ":  is running: " << srt_it.value().second << std::endl);
                            QStringList exePipe = pipeit.value();
                            bool bParallelWriter = false;
                            if (    parallelWriters.contains(srt_it.value().second.c_str())
                                 && srt_it.value().first.size() > 1
                               )
                            {
                                bParallelWriter = true;
                            }
                            executeTask(exePipe, bParallelWriter, repo, step, commRank, worldRank);
                        }
                    }
                }
                if (commRank == 0)
                {
                    NMDebug(<< std::endl);
                }

                // ========================== CLEAN UP ==================================
                //MPI_Barrier(comm);

                auto rsc_it = mapRankNameSplitComm.cbegin();
                for (; rsc_it != mapRankNameSplitComm.cend(); ++rsc_it)
                {
                    if (rsc_it.key() == commRank)
                    {
                        MPI_Comm _rsc = rsc_it.value().second;
                        if (_rsc != MPI_COMM_NULL)
                        {
                            mController->deregisterParallelGroup(rsc_it.value().first);
                            int cn_len;
                            char cname[MPI_MAX_OBJECT_NAME];
                            MPI_Comm_get_name(_rsc, cname, &cn_len);
                            MPI_Comm_free(&_rsc);
                            NMDebugAI(<< "freed splitComm=" << cname << std::endl);
                        }
                    }
                }


                // resume
                if (commRank == 0)
                {
                    NMDebugAI(<< "total tasks left: " << (_sequentialExe.size() + _parallelExe.size())
                              << "(" << num_totalTasks << ")" << std::endl);
                    NMDebugAI(<< "seq tasks left:   " << _sequentialExe.size() << "(" << num_seqTasks << ")" << std::endl);
                    NMDebugAI(<< "para tasks left:  " << _parallelExe.size()  << "(" << num_paraTasks << ")" << std::endl);
                    NMDebug(<< std::endl);
                }

                // re-fill idle procs
                for (int ap=0; ap < commProcs; ++ap)
                {
                    idleProcs.push_back(ap);
                }

                // empty the task management map
                mapSplitRanksTasks.clear();
                mapRankNameSplitComm.clear();

                // reset split count
                splitID = 0;
                ++cnt;

                // wait for everybody to have completed their tasks + clean-up before starting the next round
                NMDebugAI(<< "*** waiting at " << this->objectName().toStdString()
                          << "'s TimeLevel #" << level << " parallel task execution loop-end barrier ..." << std::endl);
                MPI_Barrier(comm);
            }
 /*

            // allocate processes to tasks

            int rankId = 0;
            int taskId = 0;
            int splitId = 0;
            QSet<int> setRanks;
            int bOnlyParallel = false;
            const int ntasks = execList.size();
            const int nsplits = std::min(ntasks, commProcs);


            // DEBUG
            //if (commRank == 0)
#ifdef LUMASS_DEBUG
            {
                std::cout << endl << "execList: ";
                for(int t=0; t < execList.size(); ++t)
                {
                    const QString& comp = execList.at(t).last();
                    std::cout << t << ":" << comp.toStdString() << " ";
                }
                std::cout << endl;

                std::cout << "commProcs=" << commProcs
                          << " ntasks=" << ntasks
                          << " nsplits=" << nsplits
                          << endl << endl;
            }
            // DEBUG
#endif


            while (rankId < commProcs && taskId < ntasks)
            {
                QString execComp = execList.at(taskId).last();

                QPair<QVector<int>, QVector<int>> pairRT;
                if (mapSplitRanksTasks.find(splitId) == mapSplitRanksTasks.end())
                {
                    pairRT = QPair<QVector<int>, QVector<int>>(QVector<int>(), QVector<int>());
                }
                else
                {
                    pairRT = mapSplitRanksTasks[splitId];
                }

                // If nprocs > ntasks and pt > 0,
                // allocate remaining procs to parallel tasks only,
                // i.e. if applicable, skip this task and/or split (colour)
                // and look at the next one.
                // If there are only sequential tasks, assign a split (colour)
                // of -1 to indicate that those ranks are not required here
                if (bOnlyParallel)
                {
                    if (parallelTasks.size() > 0)
                    {
                        const int _taskId = taskId;
                        const int _splitId = splitId;

                        // try the next task if this one is not parallel
                        while(!parallelExe.contains(execList.at(taskId).last()))
                        {
                            taskId = taskId < ntasks-1 ? taskId+1 : 0;
                            const QString _ec = execList.at(taskId).last();
                            {
                                // for parallel writers ensure that we don't allocte
                                // more ranks to it than configured by the user!
                                const int _id = parallelWriters.indexOf(_ec);
                                if (_id >= 0)
                                {
                                    if (rankId > parallelWriterProcs.at(_id)-1)
                                    {
                                        wulog(-1, "lr" << commRank << ": " << this->objectName().toStdString()
                                              << ": splitId=" << splitId << " rankId=" << rankId
                                              << "taskId #" << taskId << " (parallelWriter) at max rank allocations - skip this rank!");
                                        continue;
                                    }
                                }
                            }
                            execComp = execList.at(taskId).last();
                        }

                        // if the task skip to a parellel 'split' if this one isn't one
                        while(pairRT.second.size() > 0 && sequentialExe.contains(execList.at(pairRT.second[0]).last()))
                        {
                            splitId = splitId < nsplits-1 ? splitId+1 : 0;
                            if (mapSplitRanksTasks.constFind(splitId) == mapSplitRanksTasks.cend())
                            {
                                break;
                            }
                            pairRT = mapSplitRanksTasks[splitId];
                        }

                        // make sure, we don't allocate more ranks to a parallel writer
                        // than requested by the user!
                        const int __id = parallelWriters.indexOf(execComp);
                        if (__id >= 0)
                        {
                            if (rankId > parallelWriterProcs.at(__id)-1)
                            {
                                splitId = -1;
                                wulog(-1, "lr" << commRank << ": " << this->objectName().toStdString()
                                      << " rankId=" << rankId << ": splitId #-1 as the only remaining parallel task "
                                      << " (parallelWriter) is at max rank allocation already!");
                            }
                        }
                    }
                    else
                    {
                        splitId = -1;
                    }
                }

                // only call MPI_Comm_split ONCE for each rank!
                if (!setRanks.contains(rankId))
                {
                    if (commRank == rankId)
                    {
                        wulog(-1, "lr" << commRank << ": " << this->objectName().toStdString()
                              << ": splitId=" << splitId << " rankId=" << rankId);
                        MPI_Comm_split(comm, splitId, rankId, &splitComm);
                        std::string cname = execComp.toStdString() + "-" + std::to_string(splitId);
                        MPI_Comm_set_name(splitComm, cname.c_str());

                        if (splitId >= 0)
                        {
                            QString regCompName = execComp;
                            if (execComp.indexOf(QStringLiteral("ImageWriter")) >= 0)
                            {
                                regCompName = this->mController->getComponent(execComp)->getHostComponent()->objectName();
                            }
                            wulog(-1, "lr" << commRank << ": " << this->objectName().toStdString()
                                  << ": splitId=" << splitId << " rankId=" << rankId
                                  << " registers comm=" << cname << " with " << regCompName.toStdString());
                            controller->registerParallelGroup(regCompName, splitComm);
                            rankExecComps << execComp;
                            rankRegComps << rankRegComps;

                            if (parallelWriters.contains(execComp))
                            {
                                rankPioWriters << execComp;
                            }
                        }
                    }
                    setRanks << rankId;
                }

                if (!pairRT.first.contains(rankId))
                {
                    pairRT.first.push_back(rankId);
                }
                if (!pairRT.second.contains(taskId))
                {
                    pairRT.second.push_back(taskId);
                }

                // only log 'active' splits
                if (splitId >= 0)
                {
                    mapSplitRanksTasks[splitId] = pairRT;

                    // we can have more than one task for a given process / rank
                    // if commProcs < ntasks
                    if (pairRT.first.contains(commRank) && commRank == rankId)
                    {
                        if (pairRT.second.size() > 0 && !rankExecComps.contains(execComp))
                        {
                            int cn_len;
                            char comm_name[MPI_MAX_OBJECT_NAME];
                            QString execComp = execList.at(pairRT.second[0]).last();
                            QString regCompName = execComp;
                            if (execComp.indexOf(QStringLiteral("ImageWriter")) >= 0)
                            {
                                regCompName = this->mController->getComponent(execComp)->getHostComponent()->objectName();
                            }
                            MPI_Comm regComm = controller->getNextUpstrMPIComm(execComp);
                            if (regComm != MPI_COMM_NULL)
                            {
                                MPI_Comm_get_name(comm, comm_name, &cn_len);
                            }
                            else
                            {
                                ::sprintf(comm_name, "MPI_COMM_NULL");
                            }

                            wulog(-1, "lr" << commRank << ": " << this->objectName().toStdString()
                                  << ": splitId=" << splitId << " rankId=" << rankId
                                  << " also registers comm=" << comm_name << " also with "
                                  << execComp.toStdString());

                            controller->registerParallelGroup(regCompName, regComm);
                            rankExecComps << execComp;
                        }
                    }
                }

                // if commProcs > ntasks, spare procs are only
                // allocated to (potentially) parallel tasks
                if (commProcs > ntasks)
                {
                    if (splitId == nsplits-1)
                    {
                        bOnlyParallel = true;
                    }

                    taskId = taskId < ntasks-1 ? taskId+1 : 0;
                    ++rankId;
                }
                else
                {
                    rankId = rankId < commProcs-1 ? rankId+1 : 0;
                    ++taskId;
                }
                splitId = splitId >=0 ? splitId < nsplits-1 ? splitId+1 : 0 : splitId;
            }

            // ------ DEBUG -------
            wulog(-1, "lr" << commRank << ": <<" << this->objectName().toStdString() << ">> Allocation RESULT ... >>")
            QMap<int, QPair<QVector<int>, QVector<int>>>::const_iterator ci = mapSplitRanksTasks.begin();
            while(ci != mapSplitRanksTasks.cend())
            {
                std::string rv, tv;
                vstr(ci.value().first, " ", rv)
                vstr(ci.value().second, " ", tv)
                wulog(-1, "<<" << this->objectName().toStdString() << ">> mapSRT=(" << ci.key() << ", <" << rv << "," << tv << ">)")
                ++ci;
            }

            std::string rec;
            slstr(rankExecComps, " ", rec)
            wulog(-1, "lr" << commRank << ": <<" << this->objectName().toStdString() << ">> RankExecComps: " << rec)
            // ------ DEBUG -------

            */
        }
        else
        {
            // ... task distribution is easy with just one process!
            //QPair<QVector<int>, QVector<int>> ranksTasks;
//            QPair<QVector<int>, QSet<QString>> ranksTasks;
//            ranksTasks.first.push_back(commRank);
//            for (int ft=0; ft < execList.size(); ++ft)
//            {
//                //ranksTasks.second.push_back(ft);
//                ranksTasks.second.insert(execList.at(ft).last());
//                rankExecComps << execList.at(ft).last();
//                rankRegComps << execList.at(ft).last();
//            }
//            mapSplitRanksTasks[0] = ranksTasks;

//            sequentialTasks = execList;

              NMDebugAI( << "*** sequential execution of tasks on " << this->objectName().toStdString()
                       << "'s TimeLevel #" << level << std::endl);

              for (int t=0; t < execList.size(); ++t)
              {
                  std::pair<std::unordered_set<int>, std::string> taskAdmin = {{commRank}, execList.at(t).last().toStdString()};
                  mapSplitRanksTasks.insert(t, taskAdmin);

                  executeTask(execList.at(t), false, repo, step, commRank, worldRank);
              }
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //          PROCESS COMPS and PIPES sequential or parallel
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
        foreach(const QStringList& pipeline, execList)
        {
            // skip this pipeline, if it's not this rank's business!
            if (!rankExecComps.contains(pipeline.last()))
            {
                wulog(-1, "lr" << commRank << ": >> skip " << pipeline.last().toStdString());
                continue;
            }

            // for each pipeline, we first link each individual component
            // (from head to toe), before we finally call update on the
            // last (i.e. executable) component of the pipeline
            std::vector<otb::NetCDFIO::Pointer> parallelReaders;

            comp = nullptr;
            for (int c=0; c < pipeline.size(); ++c)
            {
                QString in = pipeline.at(c);
                comp = controller->getComponent(in);
                if (comp == 0)
                {
                    NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
                    e.setSource(in.toStdString());
                    std::stringstream msg;
                    msg << "'" << in.toStdString() << "'";
                    e.setDescription(msg.str());
                    NMDebugCtx(this->objectName().toStdString(), << "done!");
                    emit signalExecutionStopped();
                    throw e;
                }
                // link component
                comp->linkComponents(step, repo);

                // if comp is a reader in a parallel write pipeline and
                // if comp is reading a netcdf file, initiate parallel read!
                if (rankPioWriters.contains(pipeline.last()))
                {
                    NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
                    if (ic != nullptr && ic->objectName().startsWith("ImageReader"))
                    {
                        NMImageReader* reader = qobject_cast<NMImageReader*>(ic->getProcess());
                        if (reader != nullptr)
                        {
                            otb::ImageIOBase* bio = const_cast<otb::ImageIOBase*>(reader->getImageIOBase());
                            otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(bio);

                            if (nio.GetPointer() != nullptr)
                            {
wulog(-1, "lr" << commRank << ": '" << nio->GetFileName() << "' needs opening in parallel mode ...!");
                                MPI_Comm niopioComm = this->mController->getNextUpstrMPIComm(comp->objectName());
                                MPI_Info info = MPI_INFO_NULL;
                                bool bpio = nio->InitParallelIO(niopioComm, info, false);
wulog(-1, "lr" << commRank << ": init parallel IO " << (bpio ? " successful!" : " failed!"));
                                parallelReaders.push_back(nio);
                            }
                        }
                    }
                }


                // gather some info, we could use for debugging purposes in case
                // the execution fails
                hostName = QStringLiteral("Unknown");
                hostStep = -1;
                if (comp->getHostComponent())
                {
                    hostName = comp->getHostComponent()->objectName();
                    hostStep = comp->getHostComponent()->getIterationStep();
                }

                NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
                NMProcess* pc = ic == nullptr ? nullptr : ic->getProcess();

                // log provenance
                args.clear();
                attrs = controller->getProvNAttributes(comp);

                QString respId = QString("nm:%1").arg(this->objectName());


                actId = QString("nm:%1_Update-%2").arg(comp->objectName()).arg(this->getIterationStep());
                agId = QString("nm:%1").arg(comp->objectName());


                args << agId;
                controller->getLogger()->logProvN(NMLogger::NM_PROV_AGENT, args, attrs);

                attrs.clear();
                args.clear();
                args << agId << respId << "-";
                controller->getLogger()->logProvN(NMLogger::NM_PROV_DELEGATION, args, attrs);

                attrs.clear();
                if (pc != nullptr)
                {
                    attrs.append(pc->getRunTimeParaProvN());
                }
                args.clear();
                args << actId << "-" << "-";
                controller->getLogger()->logProvN(NMLogger::NM_PROV_ACTIVITY, args, attrs);

                attrs.clear();
                args.clear();
                args << actId << agId << "-";
                controller->getLogger()->logProvN(NMLogger::NM_PROV_ASSOCIATION, args, attrs);
            }

            // calling update on the last component of the pipeline
            // (the most downstream)
            if (!controller->isModelAbortionRequested())
            {
                wulog(-1, "lr" << commRank << ": >> " << pipeline.last().toStdString() << "::update() ...");
                comp->update(repo);

                // clase parallel readers, if any
                for (int pr=0; pr < parallelReaders.size(); ++pr)
                {
                    parallelReaders.at(pr)->FinaliseParallelIO();
                }
            }
            else
            {
                NMDebugAI(<< ">>>> END ITERATION #" << step+1 << std::endl);
                NMDebugCtx(this->objectName().toStdString(), << "done!");
                return;
            }

            // release resources
            foreach (const QString in, pipeline)
            {
                comp = controller->getComponent(in);
                NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
                if (ic && ic->getProcess() != 0)
                {
                    ic->getProcess()->reset();
                }
            }
        }

        // Wait for all ranks associated with this aggregate component.
        // Note that this includes all ranks belonging to splitComm that
        // actually did some work on this time level.
        if (comm != MPI_COMM_NULL)
        {
            wulog(-1, "lr" << commRank << ": >> waiting at " << this->objectName().toStdString()
                  << "'s iteration COMM barrier ... ");
            MPI_Barrier(comm);
        }

        // Now that all ranks have finished, we can
        // de-register the splitComm communicators and free them.
        if (splitComm != MPI_COMM_NULL)
        {
            foreach(const QString& rec, rankExecComps)
            {
                this->mController->deregisterParallelGroup(rec);
            }
            wulog(-1, "lr" << commRank << ": >> waiting at " << this->objectName().toStdString()
                  << "'s time level SPLIT barrier ... ");
            MPI_Comm_free(&splitComm);
        }

*/




    }// end time-level loop

    NMDebugAI(<< ">>>> END ITERATION #" << step+1 << std::endl);
    NMDebugCtx(this->objectName().toStdString(), << "done!");

    }// end try-block
    catch (mu::ParserError& evalerr)
    {
        std::stringstream errmsg;
        errmsg << std::endl
               << "Message:    " << evalerr.GetMsg() << std::endl
               << "Formula:    " << evalerr.GetExpr() << std::endl
               << "Token:      " << evalerr.GetToken() << std::endl
               << "Position:   " << evalerr.GetPos() << std::endl << std::endl;

        exStackInfo << (exObjName.empty() ? hostName.toStdString() : exObjName) << " step #" << hostStep << ": "
            << (comp == 0 ? "NULL-Comp" : comp->objectName().toStdString()) << " step #" << step+1;
        exDescription << errmsg.str();
        exSource = (exObjName.empty() ? hostName.toStdString() : exObjName);
        exceptionType = NMMfwException::NMProcess_ExecutionError;
        bThrow = true;
    }
    catch (itk::ExceptionObject& err)
    {
        exStackInfo << (exObjName.empty() ? hostName.toStdString() : exObjName) << " step #" << hostStep << ": "
            << (comp == 0 ? "NULL-Comp" : comp->objectName().toStdString()) << " step #" << step+1;
        exSource = (exObjName.empty() ? hostName.toStdString() : exObjName);
        exDescription << err.GetDescription();
        bThrow = true;
    }
    catch (NMMfwException& nmerr)
    {
        if (nmerr.getExecStackInfo().empty())
        {
            exStackInfo << (exObjName.empty() ? hostName.toStdString() : exObjName) << " step #" << hostStep << ": "
                << (comp == 0 ? "NULL-Comp" : comp->objectName().toStdString()) << " step #" << step+1;
        }
        else
        {
            exStackInfo << nmerr.getExecStackInfo();
        }

        if (nmerr.getSource().empty())
        {
            exSource = (exObjName.empty() ? hostName.toStdString() : exObjName);
        }
        else
        {
            exSource = nmerr.getSource();
        }
        exDescription << nmerr.getDescription();
        exceptionType = nmerr.getType();
        bThrow = true;
    }
    catch (std::exception& e)
    {
        exStackInfo << (exObjName.empty() ? hostName.toStdString() : exObjName)
                    << " step #" << hostStep << ": "
                    << (comp == 0 ? "NULL-Comp" : comp->objectName().toStdString())
                    << " step #" << step+1;
        exDescription << e.what();
        bThrow = true;
    }
    if (bThrow)
    {
        NMMfwException mfwe;
        mfwe.setType(exceptionType);
        mfwe.setDescription(exDescription.str());
        mfwe.setExecStackInfo(exStackInfo.str());
        mfwe.setSource(exSource);
        NMErr(ctx, << mfwe.what());

        // clear any allocated splitComms
        auto rsc_it = mapRankNameSplitComm.cbegin();
        for (; rsc_it != mapRankNameSplitComm.cend(); ++rsc_it)
        {
            if (rsc_it.key() == commRank)
            {
                MPI_Comm _rsc = rsc_it.value().second;
                if (_rsc != MPI_COMM_NULL)
                {
                    mController->deregisterParallelGroup(rsc_it.value().first);
                    int cn_len;
                    char cname[MPI_MAX_OBJECT_NAME];
                    MPI_Comm_get_name(_rsc, cname, &cn_len);
                    MPI_Comm_free(&_rsc);
                    NMDebugAI(<< "freed splitComm=" << cname << std::endl);
                }
            }
        }


        NMDebugCtx(this->objectName().toStdString(), << "done!");

        NMModelController::MPICompProg compProg;
        compProg.compName = this->objectName();
        compProg.event = NMModelController::NM_EVENT_EXEC_ABORTED;
        compProg.progress = 0;
        mController->mpiSignalProgress(compProg);

        emit signalExecutionStopped();
        mIsUpdating = false;
        throw mfwe;
    }
}

void
NMIterableComponent::executeTask(const QStringList pipeline,
                                 bool bParallelWriter,
                                 const QMap<QString, NMModelComponent *> &repo,
                                 unsigned int step, int rank, int worldRank)
{
    //// skip this pipeline, if it's not this rank's business!
    //if (!rankExecComps.contains(pipeline.last()))
    //{
    //    wulog(-1, "lr" << commRank << ": >> skip " << pipeline.last().toStdString());
    //    continue;
    //}

    // for each pipeline, we first link each individual component
    // (from head to toe), before we finally call update on the
    // last (i.e. executable) component of the pipeline
    std::vector<otb::NetCDFIO::Pointer> parallelReaders;

    NMModelController* controller = mController;
    NMModelComponent* comp = nullptr;
    for (int c=0; c < pipeline.size(); ++c)
    {
        QString in = pipeline.at(c);
        comp = controller->getComponent(in);
        if (comp == 0)
        {
            NMMfwException e(NMMfwException::NMModelController_UnregisteredModelComponent);
            e.setSource(in.toStdString());
            std::stringstream msg;
            msg << "'" << in.toStdString() << "'";
            e.setDescription(msg.str());
            NMDebugCtx(this->objectName().toStdString(), << "done!");
            emit signalExecutionStopped();
            throw e;
        }
        // link component
        comp->linkComponents(step, repo);

        // if comp is a reader in a parallel write pipeline and
        // if comp is reading a netcdf file, initiate parallel read!
        //if (rankPioWriters.contains(pipeline.last()))
        if (bParallelWriter)
        {
            NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
            if (ic != nullptr && ic->objectName().startsWith("ImageReader"))
            {
                NMImageReader* reader = qobject_cast<NMImageReader*>(ic->getProcess());
                if (reader != nullptr)
                {
                    otb::ImageIOBase* bio = const_cast<otb::ImageIOBase*>(reader->getImageIOBase());
                    otb::NetCDFIO::Pointer nio = dynamic_cast<otb::NetCDFIO*>(bio);

                    if (nio.GetPointer() != nullptr)
                    {
wulog(-1, "lr" << rank << ": '" << nio->GetFileName() << "' needs opening in parallel mode ...!");
                        MPI_Comm niopioComm = this->mController->getNextUpstrMPIComm(comp->objectName());
                        MPI_Info info = MPI_INFO_NULL;
                        bool bpio = nio->InitParallelIO(niopioComm, info, false);
wulog(-1, "lr" << rank << ": init parallel IO " << (bpio ? " successful!" : " failed!"));
                        parallelReaders.push_back(nio);
                    }
                }
            }
        }


        // gather some info, we could use for debugging purposes in case
        // the execution fails
        QString hostName = QStringLiteral("Unknown");
        int hostStep = -1;
        if (comp->getHostComponent())
        {
            hostName = comp->getHostComponent()->objectName();
            hostStep = comp->getHostComponent()->getIterationStep();
        }

        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
        NMProcess* pc = ic == nullptr ? nullptr : ic->getProcess();

        // log provenance
        QStringList args;
        QStringList attrs = controller->getProvNAttributes(comp);

        QString respId = QString("nm:%1").arg(this->objectName());


        QString actId = QString("nm:%1_Update-%2").arg(comp->objectName()).arg(this->getIterationStep());
        QString agId = QString("nm:%1").arg(comp->objectName());


        args << agId;
        controller->getLogger()->logProvN(NMLogger::NM_PROV_AGENT, args, attrs);

        attrs.clear();
        args.clear();
        args << agId << respId << "-";
        controller->getLogger()->logProvN(NMLogger::NM_PROV_DELEGATION, args, attrs);

        attrs.clear();
        if (pc != nullptr)
        {
            attrs.append(pc->getRunTimeParaProvN());
        }
        args.clear();
        args << actId << "-" << "-";
        controller->getLogger()->logProvN(NMLogger::NM_PROV_ACTIVITY, args, attrs);

        attrs.clear();
        args.clear();
        args << actId << agId << "-";
        controller->getLogger()->logProvN(NMLogger::NM_PROV_ASSOCIATION, args, attrs);
    }

    // calling update on the last component of the pipeline
    // (the most downstream)
    if (!controller->isModelAbortionRequested())
    {
        wulog(-1, "lr" << rank << ": >> " << pipeline.last().toStdString() << "::update() ...");
        comp->update(repo);

        // clase parallel readers, if any
        for (int pr=0; pr < parallelReaders.size(); ++pr)
        {
            parallelReaders.at(pr)->FinaliseParallelIO();
        }
    }
    else
    {
        NMDebugAI(<< ">>>> END ITERATION #" << step+1 << std::endl);
        NMDebugCtx(this->objectName().toStdString(), << "done!");
        return;
    }

    // release resources
    foreach (const QString in, pipeline)
    {
        comp = controller->getComponent(in);
        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
        if (ic && ic->getProcess() != 0)
        {
            ic->getProcess()->reset();
        }
    }
}

const QStringList
NMIterableComponent::findExecutableComponents(
        const QMap<QString, NMModelComponent*>& levelComps,
        unsigned int timeLevel, int step)
{
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

    NMModelController* ctrl = this->getModelController();

    // now we subsequently remove all components, which are mentioned as input
    // of one of the given level's other components;
    // we also look for any data components we can find, in case
    // we haven't got 'dead ends' but a cyclic relationship whereas a
    // data buffer component is updated subsequently
    QStringList dataBuffers;

    QMap<QString, NMModelComponent*>::const_iterator inputIt =
            levelComps.constBegin();  
    for (; inputIt != levelComps.constEnd(); ++inputIt)
    {
        QString input = inputIt.key();

        // we only execute 'sink' processes, DataBuffers, and
        // aggregate components, so if you're not one of them,
        // we take you off the list
        if (    !input.startsWith(QString::fromLatin1("DataBuffer"))
            &&  !input.startsWith(QString::fromLatin1("AggrComp"))
            &&  !NMProcessFactory::instance().isSink(input)
           )
        {
            execComps.removeOne(input);
            NMDebugAI(<< "removed non-executable '" << inputIt.key().toStdString() << "' from executables"
                      << std::endl);
            continue;
        }

        // make list of data buffers
        NMDataComponent* buf = qobject_cast<NMDataComponent*>(inputIt.value());
        if (buf != nullptr)
        {
            dataBuffers.push_back(buf->objectName());
        }

        // check whether current component 'input' is an input to another
        // component on the candidate list (ie comps on this time level);
        // if yes, remove 'input' from exec list
        QMap<QString, NMModelComponent*>::const_iterator testIt =
                levelComps.constBegin();
        for (; testIt != levelComps.constEnd(); ++testIt)
        {
            const QList<QStringList>& _icInputsList = testIt.value()->getInputs();
            NMIterableComponent* _ic = qobject_cast<NMIterableComponent*>(testIt.value());

            QStringList _icInputs;
            if (_icInputsList.size()-1 >= step)
            {
                // apply "NM_USE_UP" index policy
                int _step = step;
                if (_step > _icInputsList.size()-1)
                {
                    _step = _icInputsList.size()-1;
                }

                if (_ic != nullptr)
                {
                    if (_ic->getProcess() != nullptr)
                    {
                        _step = _ic->getProcess()->mapHostIndexToPolicyIndex(_step, _icInputsList.size());
                    }
                }

                _icInputs = _icInputsList.at(_step);
                if (_icInputs.contains(input))
                {
                    execComps.removeOne(input);
                    NMDebugAI(<< "removed non-executable '" << input.toStdString() << "' from executables"
                              << std::endl);
                    break;
                }
            }
        }
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
    this->mIsUpdating = false;
    if (this->getProcess() != 0)
    {
        this->getProcess()->reset();
        return;
    }
    else
    {
        NMModelComponentIterator cit = this->getComponentIterator();
        while (*cit != 0)
        {
            cit->reset();
            ++cit;
        }
    }
}

void
NMIterableComponent::setIterationStep(unsigned int step)
{
    this->mIterationStep = step == 0 ? 1 : step;
    this->mIterationStepRun = this->mIterationStep;
    emit signalProgress(mIterationStep);
}

unsigned int
NMIterableComponent::getIterationStep(void)
{
    if (this->getModelController()->isModelRunning())
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

    NMModelComponentIterator cit = this->getComponentIterator();
    while (*cit != 0 && !bIsSub)
    {
        if (comp->objectName().compare(cit->objectName()) == 0)
        {
            bIsSub = true;
            break;
        }

        NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(*cit);
        if (ic)
        {
            bIsSub = ic->isSubComponent(comp);
        }
        ++cit;
    }

    return bIsSub;
}
