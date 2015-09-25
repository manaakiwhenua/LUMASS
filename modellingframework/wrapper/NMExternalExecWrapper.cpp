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

#include "nmlog.h"
#include "NMExternalExecWrapper.h"
#include "NMMfwException.h"

#include <QVariant>
#include <QSharedPointer>

const std::string NMExternalExecWrapper::ctx = "NMExternalExecWrapper";
NMExternalExecWrapper::NMExternalExecWrapper(QObject* parent)
{
    this->setParent(parent);
}

NMExternalExecWrapper::~NMExternalExecWrapper()
{
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

    this->mbIsInitialised = true;
    this->setObjectName(QString::fromLatin1("NMExternalExecWrapper"));
}


void
NMExternalExecWrapper::setNthInput(unsigned int numInput,
          QSharedPointer<NMItkDataObjectWrapper> imgWrapper)
{
}

void
NMExternalExecWrapper::update(void)
{
    NMDebugCtx(ctx, << "...");

    //    NMDebugAI( << "running: " << mCurCmd.toStdString()
    //               << " " << mCurArgs.toStdString());

    QString objName = this->parent()->objectName();
    emit signalExecutionStarted(objName);

    QProcess::execute(mCurCmd, mCurArgList);

    emit signalExecutionStopped(objName);

    //NMDebugAI( << "finished!");


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
        std::stringstream msg;
        msg << "'" << this->objectName().toStdString() << "'";
        e.setMsg(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    mCurCmd = qvCmnd.toString();
    //mCmdProcess.setProgram(sCmd);

    QVariant qvArguments = this->getParameter(QString::fromLatin1("Arguments"));
    if (qvArguments.type() != QVariant::String)
    {
        NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
        std::stringstream msg;
        msg << "'" << this->objectName().toStdString() << "'";
        e.setMsg(msg.str());
        NMDebugCtx(ctx, << "done!");
        throw e;
    }

    mCurArgs = qvArguments.toString();
    mCurArgList = mCurArgs.split(' ');
    mCmdProcess.setArguments(mCurArgList);

    NMDebugCtx(ctx, << "done!");
}


