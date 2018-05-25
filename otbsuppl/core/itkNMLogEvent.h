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
/*
 * NOTE: The implementation draws on the itkEventObject.h (s. copyright below)
 *
         *=========================================================================
         *
         *  Copyright Insight Software Consortium
         *
         *  Licensed under the Apache License, Version 2.0 (the "License");
         *  you may not use this file except in compliance with the License.
         *  You may obtain a copy of the License at
         *
         *         http://www.apache.org/licenses/LICENSE-2.0.txt
         *
         *  Unless required by applicable law or agreed to in writing, software
         *  distributed under the License is distributed on an "AS IS" BASIS,
         *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
         *  See the License for the specific language governing permissions and
         *  limitations under the License.
         *
         *=========================================================================
 *
 * Couldn't use the macro here since we wanted to make
 * the NMLogEvent class a bit richer than the common itk::EventObject
 *
 */


#ifndef NMLOGEVENT_H
#define NMLOGEVENT_H

#include "itkEventObject.h"

#include <ctime>
#include <cstdio>
#include <string>
#include <vector>


namespace itk
{

class NMLogEvent : public itk::EventObject
{
public:

    typedef enum {
        NM_LOG_DEBUG = 0,
        NM_LOG_INFO = 1,
        NM_LOG_WARN = 2,
        NM_LOG_ERROR = 3,
        NM_LOG_PROVN = 4
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
    } LogProvNType;

    NMLogEvent()
        : mMsg(""), mType(NM_LOG_INFO),
          mTime("")
        {readTime();}

    NMLogEvent(const std::string& msg,
               LogEventType type)
        : mType(type), mMsg(msg),
          mTime("")
        {readTime();}

    NMLogEvent(const NMLogEvent& logEvent)
    {
        NMLogEvent& le = const_cast<NMLogEvent&>(logEvent);
        mMsg = le.getLogMsg();
        mType = le.getLogType();
        mTime = le.getLogTime();
    }

    NMLogEvent(const LogEventType& logType,
               const LogProvNType& provType,
               const std::vector<std::string>& args,
               const std::vector<std::string>& attrs)
        : mType(logType), mProvNType(provType),
          mArgs(args), mAttrs(attrs)
    {
    }

    std::string getLogMsg()
        {return mMsg;}

    std::string getLogTime()
        {return mTime;}

    LogEventType getLogType()
        {return mType;}

    LogProvNType getProvNType()
        {return mProvNType;}

    std::vector<std::string> getProvNArgs()
        {return mArgs;}

    std::vector<std::string> getProvNAttrs()
        {return mAttrs;}


    virtual const char* GetEventName() const
        {return "NMLogEvent";}
    virtual bool CheckEvent(const ::itk::EventObject* e) const
        {return (dynamic_cast< const NMLogEvent* >(e) != ITK_NULLPTR);}
    ::itk::EventObject* MakeObject() const {return new NMLogEvent();}

protected:
    void readTime(void)
    {
        time_t timestamp;
        struct tm* timeinfo;
        time(&timestamp);
        timeinfo = localtime(&timestamp);
        static char res[16];
        sprintf(res, "%.2d:%.2d:%.2d",
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec);
        mTime = res;
    }


private:
    std::string mMsg;
    std::string mTime;
    LogEventType mType;
    std::vector<std::string> mArgs;
    std::vector<std::string> mAttrs;
    LogProvNType mProvNType;
};

} // end namespace itk

#endif // NMLOGEVENT_H
