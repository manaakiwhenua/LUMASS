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
#ifndef NMLOGGER_H
#define NMLOGGER_H

#include <QObject>

class NMLogger : public QObject
{
    Q_OBJECT
public:

    typedef enum {
        NM_LOG_DEBUG = 0,
        NM_LOG_INFO = 1,
        NM_LOG_WARN = 2,
        NM_LOG_ERROR = 3

    } LogEventType;


    explicit NMLogger(QObject *parent = 0);
    void setHtmlMode(bool bhtml){mbHtml=bhtml;}
    void setLogLevel(LogEventType level){mLogLevel = level;}
    LogEventType getLogLevel(void){return mLogLevel;}

signals:
    void sendLogMsg(const QString& msg);

public slots:
    void processLogMsg(const QString& time,
                       LogEventType type,
                       const QString& msg);

protected:

    bool mbHtml;
    LogEventType mLogLevel;
};

#endif // NMLOGGER_H
