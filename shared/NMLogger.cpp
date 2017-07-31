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
