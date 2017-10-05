/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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
 * NMExternalExecWrapper.cpp
 *
 *  Created on: 2015-09-24
 *      Author: Alexander Herzig
 */

#include <QVariant>
#include <QByteArray>

//#include "nmlog.h"

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMModelController.h"
#include "NMExternalExecWrapper.h"
#include "NMMfwException.h"


const std::string NMExternalExecWrapper::ctx = "NMExternalExecWrapper";
NMExternalExecWrapper::NMExternalExecWrapper(QObject* parent)
    : mCmdProcess(0)
{
    this->setParent(parent);
}

NMExternalExecWrapper::~NMExternalExecWrapper()
{
    if (mCmdProcess != 0)
    {
        disconnect(mCmdProcess, SIGNAL(readyReadStandardOutput()),
                   this, SLOT(readOutput()));
        delete mCmdProcess;
        mCmdProcess = 0;
    }
}

QSharedPointer<NMItkDataObjectWrapper>
NMExternalExecWrapper::getOutput(unsigned int idx)
{
    QSharedPointer<NMItkDataObjectWrapper> dw(new NMItkDataObjectWrapper());
    return dw;
}

void
NMExternalExecWrapper::instantiateObject(void)
{
    reset();

    this->mbIsInitialised = true;
    this->setObjectName(QString::fromLatin1("NMExternalExecWrapper"));
}


void
NMExternalExecWrapper::setNthInput(unsigned int numInput,
          QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
}

void
NMExternalExecWrapper::reset(void)
{
    NMProcess::reset();

    if (mCmdProcess != 0)
    {
        disconnect(mCmdProcess, SIGNAL(readyReadStandardOutput()),
                   this, SLOT(readOutput()));
        delete mCmdProcess;
        mCmdProcess = 0;
    }

    mCmdProcess = new QProcess();
    mCmdProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(mCmdProcess, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readOutput()));

    mProcOutput.clear();
}

void
NMExternalExecWrapper::abortExecution(void)
{
    if (mCmdProcess != 0)
    {
        mCmdProcess->close();
        mCmdProcess = 0;
    }
    NMLogInfo(<< "Process killed!");
}

void
NMExternalExecWrapper::readOutput(void)
{
    if (mCmdProcess != 0)
    {
        QString baOut = mCmdProcess->readAllStandardOutput();
        NMLogInfoNoNL(<< baOut.toStdString());
    }
    else
    {
        NMLogWarn(<< "External Process is NULL!");
    }
}

void
NMExternalExecWrapper::update(void)
{
    NMDebugCtx(ctx, << "...");

    if (mCmdProcess == 0)
    {
        NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
        e.setSource(this->parent()->objectName().toStdString());
        std::stringstream msg;
        msg << this->objectName().toStdString() << "::update() - "
            << "external process object is NULL!";
        e.setDescription(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    NMDebugAI( << mCurCmd.toStdString() << std::endl);

    mCmdProcess->setProcessEnvironment(mProcEnv);

    QString objName = this->parent()->objectName();
    emit signalExecutionStarted(objName);

    mCmdProcess->start(mCurCmd);
    mCmdProcess->waitForFinished(-1);

    emit signalExecutionStopped(objName);

    this->mbLinked = false;
    NMDebugCtx(ctx, << "done!");
}

void
NMExternalExecWrapper::linkParameters(unsigned int step,
                    const QMap<QString, NMModelComponent*>& repo)
{
    NMDebugCtx(ctx, << "...");

    QVariant qvCmnd = this->getParameter(QString::fromLatin1("Command"));
    if (qvCmnd.type() != QVariant::String)
    {
        NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
        e.setSource(this->parent()->objectName().toStdString());
        std::stringstream msg;
        msg << "'" << this->objectName().toStdString() << "'";
        e.setDescription(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }
    mCurCmd = qvCmnd.toString();


    QVariant qvEnvironment = this->getParameter(QString::fromLatin1("Environment"));
    if (QString::fromLatin1("QStringList").compare(qvEnvironment.typeName()) == 0)
    {
        QStringList temp = qvEnvironment.toStringList();

        mProcEnv = QProcessEnvironment::systemEnvironment();

        foreach (const QString& kv, temp)
        {
            QStringList aList = kv.split('=', QString::SkipEmptyParts);
            if (aList.size() > 1)
            {
                mProcEnv.insert(aList.at(0), aList.at(1));
                NMLogDebug(<< kv.toStdString());
            }
            else
            {
                NMLogWarn(<< ctx << " - skipped invalid key=value pair '"
                          << kv.toStdString() << "'!");
            }
        }
    }

    NMDebugCtx(ctx, << "done!");
}


