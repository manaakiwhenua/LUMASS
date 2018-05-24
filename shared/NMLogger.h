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

    typedef enum {
        NM_PROV_ENTITY = 0,
        NM_PROV_ACTIVITY,
        NM_PROV_GENERATION,
        NM_PROV_USAGE,
        NM_PROV_COMMUNICATION,
        NM_PROV_START,
        NM_PROV_END,
        NM_PROV_INVALIDATION,
        NM_PROV_DERIVATION,
        NM_PROV_AGENT,
        NM_PROV_ATTRIBUTION,
        NM_PROV_ASSOCIATION,
        NM_PROV_DELEGATION,
        NM_PROV_COLLECTION,
        NM_PROV_MEMBERSHIP
    } NMProvConcept;


    explicit NMLogger(QObject *parent = 0);
    void setHtmlMode(bool bhtml){mbHtml=bhtml;}
    void setLogLevel(LogEventType level){mLogLevel = level;}
    LogEventType getLogLevel(void){return mLogLevel;}

signals:
    void sendLogMsg(const QString& msg);
    void sendProvN(const QString& provLog);

public slots:
    void processLogMsg(const QString& time,
                       LogEventType type,
                       const QString& msg,
                       bool bForceNewLine=true);

    /*! CONCEPT
     *  entity, activity, generation, usage, communication,
     *  start, end, derivation, aggent, attribution, association,
     *  delegation, collection, membership
     *
     *
     */

    void logProvN(const NMProvConcept& concept,
                  const QStringList &args,
                  QStringList &attr);

protected:

    bool mbHtml;
    LogEventType mLogLevel;
};

#endif // NMLOGGER_H
