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
* NMSQLProcessor.cpp
*
*  Created on: 22/10/2015
*  Author: Alexander Herzig
*
*  Source of Inspiration:
*/

#include "NMSQLProcessor.h"

const std::string NMSQLProcessor::ctx = "NMSQLProcessor";

NMSQLProcessor::NMSQLProcessor(QObject *parent)
    :NMProcess(parent)
{

}

NMSQLProcessor::~NMSQLProcessor()
{
}

void
NMSQLProcessor::setNthInput(unsigned int numInput,
        QSharedPointer<NMItkDataObjectWrapper> img)
{
    while (numInput >= mInputDataWrapper.size())
    {
        QSharedPointer<NMItkDataObjectWrapper> dw(new NMItkDataObjectWrapper());
        mInputDataWrapper.append(dw);
    }

    mInputDataWrapper.replace(numInput, img);
}


QSharedPointer<NMItkDataObjectWrapper>
NMSQLProcessor::getOutput(unsigned int idx)
{
    if (idx < mOutputDataWrapper.size())
    {
        return mOutputDataWrapper.at(idx);
    }

    return QSharedPointer<NMItkDataObjectWrapper>(new NMItkDataObjectWrapper());

}

void
NMSQLProcessor::update()
{
    std::string reportName = ctx;
    if (parent() != 0)
    {
        reportName = parent()->objectName().toStdString();
    }


    NMDebugCtx(reportName, << "...");
    if (!this->mbLinked)
    {
        NMDebugCtx(reportName, << "done!");
        return;
    }

    //// now we're doing something ....
    emit signalExecutionStarted(QString(reportName.c_str()));

    QVariant qvStmts = this->getParameter("SQLStmt");
    QStringList stmts;
    if (qvStmts.isValid())
    {
        stmts = qvStmts.toStringList();
    }

    foreach(const QString& s, stmts)
    {
        NMDebugAI(<< s.toStdString() << std::endl);
    }


    emit signalExecutionStopped(QString(reportName.c_str()));
    //// and now we're finished!

    this->mMTime = QDateTime::currentDateTimeUtc();
    NMDebugAI(<< "modified at: "
              << mMTime.toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString() << std::endl);

    this->mbLinked = false;
    NMDebugCtx(reportName, << "done!");
}


void
NMSQLProcessor::linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo)
{
    // anything to do here
}


