/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
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
#include "NMLogger.h"

NMLogger::NMLogger(QObject *parent)
    : QObject(parent), mbHtml(false), mLogLevel(NM_LOG_INFO)
{
}

void
NMLogger::logProvN(const NMProvConcept &concept,
                   const QStringList& args,
                   QStringList& attr)
{
    QString msg;

    switch(concept)
    {
    case NM_PROV_ENTITY:
        msg = "entity(";
        break;
    case NM_PROV_ACTIVITY:
        msg = "activity(";
        break;
    case NM_PROV_GENERATION:
        msg = "wasGeneratedBy(";
        break;
    case NM_PROV_USAGE:
        msg = "used(";
        break;
    case NM_PROV_COMMUNICATION:
        msg = "wasInformedBy(";
        break;
    case NM_PROV_START:
        msg = "wasStartedBy(";
        break;
    case NM_PROV_END:
        msg = "wasEndedBy(";
        break;
    case NM_PROV_INVALIDATION:
        msg = "wasInvalidatedBy(";
        break;
    case NM_PROV_DERIVATION:
        msg = "wasDerivedFrom(";
        break;
    case NM_PROV_AGENT:
        msg = "agent(";
        attr.prepend(QString::fromLatin1("prov:type='prov:SoftwareAgent'"));
        break;
    case NM_PROV_ATTRIBUTION:
        msg = "wasAttributedTo(";
        break;
    case NM_PROV_ASSOCIATION:
        msg = "wasAssociatedWith(";
        attr.prepend(QString::fromLatin1("prov:role=\"operator\""));
        break;
    case NM_PROV_DELEGATION:
        msg = "actedOnBehalfOf(";
        attr.prepend(QString::fromLatin1("prov:type=\"delegation\""));
        break;
    case NM_PROV_COLLECTION:
        // must be of type prov:type='prov:Collection'
        msg = "entity(";
        attr.prepend(QString::fromLatin1("prov:type='prov:Collection'"));
        break;
    case NM_PROV_MEMBERSHIP:
        msg = "hadMember(";
        break;
    default:
        msg = "unknown(";
    }

    msg += args.join(',');

    if (attr.size() > 0)
    {
        msg += ",";
        msg += "[";
        msg += attr.join(',');
        msg += "]";
    }

    // removing any new line / carriage return new line
    // characters, since they lead to formatting errors
    // of the PROV-N file; basically there are no
    // such characters allowed in the file
    msg = msg.replace('\n', ' ');
    msg = msg.replace("\r\n", " ");

    msg += ")\n";

    sendProvN(msg);
}

void
NMLogger::processLogMsg(const QString &time, LogEventType type, const QString &msg,
                        bool bForceNewLine)
{
    if (    (int)type < (int)mLogLevel
        ||  msg.isEmpty())
    {
        return;
    }

    QString logmsg = msg;
    // each message its own line unless we specifiy bForceNewLine = false!
    if (bForceNewLine && msg.at(msg.size()-1) != '\n')
    {
        logmsg = QString("%1 \n").arg(msg);
    }

    if (mbHtml)
    {
        logmsg = logmsg.replace(QString("\r\n"), QString("<br>"));
        logmsg = logmsg.replace(QChar('\n'), QString("<br>"));
        logmsg = logmsg.replace(QChar('\r'), QString("<br>"));

        switch(type)
        {
            case NM_LOG_INFO:
                if (!bForceNewLine)
                {
                    logmsg = QString("%1 ").arg(logmsg);
                }
                else
                {
                    logmsg = QString("%1 <b>INFO</b>: %2").arg(time).arg(logmsg);
                }
                break;
            case NM_LOG_WARN:
                logmsg = QString("%1 <b><font color=\"blue\">WARNING</font></b>: %2").arg(time).arg(logmsg);
                break;
            case NM_LOG_ERROR:
                logmsg = QString("%1 <b><font color=\"red\">ERROR</font></b>: %2").arg(time).arg(logmsg);
                break;
            case NM_LOG_DEBUG:
                if (!bForceNewLine)
                {
                    logmsg = QString("%1 ").arg(logmsg);
                }
                else
                {
                    logmsg = QString("%1 DEBUG: %2").arg(time).arg(logmsg);
                }
                break;
        }
    }
    else
    {
        switch(type)
        {
            case NM_LOG_INFO:
                if (!bForceNewLine)
                {
                    logmsg = QString("%1 ").arg(logmsg);
                }
                else
                {
                    logmsg = QString("%1 INFO: %2").arg(time).arg(logmsg);
                }
                break;
            case NM_LOG_WARN:
                logmsg = QString("%1 WARNING: %2").arg(time).arg(logmsg);
                break;
            case NM_LOG_ERROR:
                logmsg = QString("%1 ERROR: %2").arg(time).arg(logmsg);
                break;
            case NM_LOG_DEBUG:
                if (!bForceNewLine)
                {
                    logmsg = QString("%1 ").arg(logmsg);
                }
                else
                {
                    logmsg = QString("%1 DEBUG: %2").arg(time).arg(logmsg);
                }
                break;
        }
    }
    emit sendLogMsg(logmsg);
}
