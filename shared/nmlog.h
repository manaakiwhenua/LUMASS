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
/*
 * name: nmlog.h
 * author: Alexander Herzig
 *
 * last updated: 08/01/11
 *
 * These macros are inspired by OTB/ITK and rasdaman debug macros
 *
 */

#ifndef NMLOG_H_
#define NMLOG_H_

#include <string>
#include <sstream>
#include <iostream>

#ifndef _WIN32
namespace nmlog
{
extern int nmindent;
}
#endif

// DEBUG MACROs
#ifdef DEBUG
#define NMDebug(arg)  \
		{ \
			std::ostringstream str; \
			str arg; \
			std::cout << str.str(); \
		}

#ifndef _WIN32
#define NMDebugAI(arg) \
		{ \
			std::ostringstream str; \
			for (int q=1; q <= nmlog::nmindent+1; q++) \
			{\
				str << "  "; \
			}\
			str arg; \
			std::cout << str.str(); \
		}
#else
#define NMDebugAI(arg) \
		{ \
			std::ostringstream str; \
			str << "  " arg; \
			std::cout << str.str(); \
		}
#endif

#define NMDebugInd(level, arg) \
		{ \
			std::ostringstream str; \
			for (int q=1; q <= level; q++) \
			{\
				str << "  "; \
			}\
			str arg; \
			std::cout << str.str(); \
		}

#define NMDebugTimeInd(level, arg) \
		{ \
			std::ostringstream str; \
			for (int q=1; q <= level; q++) \
			{\
				str << "  "; \
			}\
			str << __TIME__ << " - " arg; \
			std::cout << str.str(); \
		}

#define NMDebugTime(arg) \
		{ \
			std::ostringstream str; \
			str << __TIME__ << " - " arg; \
			std::cout << str.str(); \
		}

#ifndef _WIN32
#define NMDebugCtx(context, arg)  \
		{ \
			std::string tmp;\
			std::ostringstream str; \
			str arg;\
			tmp = str.str();\
			str.str(""); \
			if (tmp == "...") \
				nmlog::nmindent++; \
			for (int q=1; q <= nmlog::nmindent; q++) \
			{\
				str << "--"; \
			}\
			str << context << "::" << \
			       __FUNCTION__ << ": " arg; \
			std::cout << str.str() << std::endl; \
			if (tmp == "done!") \
				nmlog::nmindent--; \
		}
#else
#define NMDebugCtx(context, arg)  \
		{ \
			std::string tmp;\
			std::ostringstream str; \
			str arg;\
			tmp = str.str();\
			str.str(""); \
			for (int q=1; q <= 2; q++) \
			{\
				str << "--"; \
			}\
			str << context << "::" << \
			       __FUNCTION__ << ": " arg; \
			std::cout << str.str() << std::endl; \
		}
#endif

#define NMDebugTimeCtx(context, arg)  \
		{ \
			std::ostringstream str; \
			str << __TIME__ << ": " << context << "::" << \
			       __FUNCTION__ << ": " arg; \
			std::cout << str.str() << std::endl; \
		}
#else
#define NMDebug(arg)            // just debug
#define NMDebugAI(arg)          // just AI debug
#define NMDebugInd(level, arg)  // just debug
#define NMDebugTime(arg)        // >>>
#define NMDebugTimeInd(level, arg)  // ...
#define NMDebugCtx(ctx, arg)    // ...
#define NMDebugTimeCtx(ctx, arg)    // ...
#endif

// ERROR MACRO
#define NMErr(context, arg)  \
		{ \
			std::ostringstream str; \
			str << "ERROR - " << context << "::" << \
			       __FUNCTION__ << ", l. " << \
			       __LINE__ << ": " arg; \
			std::cerr << str.str() << std::endl; \
		}

#define NMWarn(context, arg)  \
		{ \
			std::ostringstream str; \
			str << "WARNING - " << context << "::" << \
			       __FUNCTION__ << ", l. " << \
			       __LINE__ << ": " arg; \
			std::cerr << str.str() << std::endl; \
		}


// MESSAGE MACRO
#define NMMsg(arg) \
		{ \
			std::ostringstream str; \
			str arg; \
			std::cout << str.str() << std::endl; \
		}

// =====================================================
// macros to facilitate invoking NMLogger::processLogMsg()
// in the NMModellingFramework and GUI classes
// =====================================================
#ifdef NM_ENABLE_LOGGER
#include "NMLogger.h"

#define NMLogInfo(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_INFO, \
                           str.str().c_str()); \
}

#define NMLogWarn(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_WARN, \
                           str.str().c_str()); \
}

#define NMLogError(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_ERROR, \
                           str.str().c_str()); \
}

#define NMLogDebug(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_DEBUG, \
                           str.str().c_str()); \
}
# else
# define NMLogInfo(arg)
# define NMLogWarn(arg)
# define NMLogError(arg)
# define NMLogDebug(arg)
#endif


// =====================================================
// macros to facilitate invoking itk::NMLogEvent() in
//      itk::Object derived classes (itk::ProcessObject)
// =====================================================
#ifdef NM_PROC_LOG
#include "itkNMLogEvent.h"

#define NMProcErr(arg) \
        { \
            this->InvokeEvent(itk::NMLogEvent(arg, \
                    itk::NMLogEvent::NM_LOG_ERROR)); \
        }

#define NMProcWarn(arg)  \
        { \
            this->InvokeEvent(itk::NMLogEvent(arg, \
                    itk::NMLogEvent::NM_LOG_WARN)); \
        }

#define NMProcInfo(arg) \
       { \
           this->InvokeEvent(itk::NMLogEvent(arg, \
                   itk::NMLogEvent::NM_LOG_INFO)); \
       }

#define NMProcDebug(arg) \
       { \
           this->InvokeEvent(itk::NMLogEvent(arg, \
                   itk::NMLogEvent::NM_LOG_DEBUG)); \
       }
#endif // NM_PROC_LOG

#endif /* NMLOG_H_ */
