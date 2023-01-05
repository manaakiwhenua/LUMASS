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
    #include <mpi.h>
#endif

//#ifndef _WIN32
//namespace nmlog
//{
//extern int nmindent;
//}
//#endif

// ======================================================================
// DEBUG MACROS
// ======================================================================
#ifdef LUMASS_DEBUG
#define NMDebug(arg)  \
		{ \
			std::ostringstream str; \
			str arg; \
			std::cout << str.str(); \
		}

//#ifndef _WIN32
//#define NMDebugAI(arg) \
//		{ \
//			std::ostringstream str; \
//			for (int q=1; q <= nmlog::nmindent+1; q++) \
//			{\
//				str << "  "; \
//			}\
//			str arg; \
//			std::cout << str.str(); \
//		}
//#else
#ifndef _WIN32
#define NMDebugAI(arg) \
{\
    int rank=0; \
    int init=0; \
    MPI_Initialized(&init); \
    if (init)\
    {\
        MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
        std::ostringstream str; \
        str << "  r" << rank << ": " arg; \
        std::cout << str.str(); \
   } \
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

//#ifndef _WIN32
//#define NMDebugCtx(context, arg)  \
//		{ \
//			std::string tmp;\
//			std::ostringstream str; \
//			str arg;\
//			tmp = str.str();\
//			str.str(""); \
//			if (tmp == "...") \
//				nmlog::nmindent++; \
//			for (int q=1; q <= nmlog::nmindent; q++) \
//			{\
//				str << "--"; \
//			}\
//			str << context << "::" << \
//			       __FUNCTION__ << ": " arg; \
//			std::cout << str.str() << std::endl; \
//			if (tmp == "done!") \
//				nmlog::nmindent--; \
//		}
//#else

#ifndef _WIN32
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
            int rank=0; \
            int init=0; \
            MPI_Initialized(&init); \
            if (init)\
            {\
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);\
                str << "r" << rank << ":" << context << "::" << \
                __FUNCTION__ << ": " arg; \
            }\
            else\
            {\
                str << context << "::" << \
                __FUNCTION__ << ": " arg; \
            }\
            std::cout << str.str() << std::endl; \
        }
//#endif

#define NMDebugCtxNoMPI(context, arg)  \
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

//#endif

#define NMDebugTimeCtx(context, arg)  \
		{ \
			std::ostringstream str; \
			str << __TIME__ << ": " << context << "::" << \
			       __FUNCTION__ << ": " arg; \
			std::cout << str.str() << std::endl; \
		}


// ------------------------
// MPI DEBUG HELPER MACROS

#define lulog( rank, msg ) \
{                                                           \
    if (rank >= 0 && commRank == rank) std::cout << "r" << rank << ": " << msg << endl;   \
    else if (rank < 0) std::cout << "r" << commRank << ": " << msg << endl;   \
}

#define wulog( rank, msg ) \
{                                                           \
    if (rank >= 0 && worldRank == rank) std::cout << "r" << rank << ": " << msg << endl;   \
    else if (rank < 0) std::cout << "r" << worldRank << ": " << msg << endl;   \
}


#define vstr( vector, joinchar, outstr ) \
{ \
    outstr += "("; \
    for (int i=0; i < vector.size(); ++i) \
    { \
      outstr += std::to_string(vector[i]); \
      if (i < vector.size()-1) \
      { \
        outstr += joinchar; \
      } \
    } \
    outstr+= ")"; \
}

#define slstr( stringlist, joinchar, outstr ) \
{ \
    outstr += "("; \
    for (int i=0; i < stringlist.size(); ++i) \
    { \
      outstr += stringlist[i].toStdString(); \
      if (i < stringlist.size()-1) \
      { \
        outstr += joinchar; \
      } \
    } \
    outstr+= ")"; \
}
// ------------------------

#else
#define NMDebug(arg)            // just debug
#define NMDebugAI(arg)          // just AI debug
#define NMDebugInd(level, arg)  // just debug
#define NMDebugTime(arg)        // >>>
#define NMDebugTimeInd(level, arg)  // ...
#define NMDebugCtx(ctx, arg)    // ...
#define NMDebugTimeCtx(ctx, arg)    // ...
// MPI DEBUG HELPER
#define lulog( rank, msg )
#define wulog( rank, msg )
#define vstr( vector, joinchar, outstr )
#define slstr( stringlist, joinchar, outstr )
#endif



// ======================================================================
// ERROR & WARNING MACROS
// ======================================================================
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
#include <QDateTime>
#include "NMLogger.h"

#define NMLogInfo(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_INFO, \
                           str.str().c_str()); \
}

#define NMLogInfoNoNL(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_INFO, \
                           str.str().c_str(), false); \
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

#define NMLogDebugNoNL(arg) \
{ \
    std::stringstream str; \
    str arg; \
    mLogger->processLogMsg(QDateTime::currentDateTime().time().toString(), \
                           NMLogger::NM_LOG_DEBUG, \
                           str.str().c_str(), false); \
}

#define NMLogProv(concept, args, attr) mLogger->logProvN(concept, args, attr);


//** MPI ERROR LOG MACRO ********************************************
#define MPILogErr( MPICALL )
//\
//{                                                                                   \
//    const int mpierr = MPICALL ;                                                    \
//    if ( mpierr != MPI_SUCCESS )                                                    \
//    {                                                                               \
//        std::string msg;                                                            \
//        switch(mpierr)                                                              \
//        {                                                                           \
//        case MPI_ERR_COMM: msg = "Invalid communicator"; break;                     \
//        case MPI_ERR_RANK: msg = "Invalid rank"; break;                             \
//        case MPI_ERR_GROUP: msg = "Invalid group"; break;                           \
//        case MPI_ERR_OP: msg = "Invalid operation"; break;                          \
//        case MPI_ERR_ARG: msg = "Invalid argument of some other kind"; break;       \
//        case MPI_ERR_KEYVAL: msg = "Invalid key has been passed"; break;            \
//        case MPI_ERR_INTERN: msg = "Internal MPI (implementation) error"; break;    \
//        case MPI_ERR_INFO: msg = "Invalid info argument"; break;                    \
//        case MPI_ERR_SIZE: msg = "Invalid size argument"; break;                    \
//        case MPI_ERR_OTHER: msg = "Known error not in this list"; break;            \
//                                                                                    \
//        case MPI_ERR_UNKNOWN:                                                       \
//        default: msg = "Unknown error"; break;                                      \
//        }                                                                           \
//                                                                                    \
//        int rank=0;                                                                 \
//        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                       \
//        MPI_Finalize();                                                             \
//                                                                                    \
//        NMLogError(<< "r" << rank << " - MPI ERROR: " << msg);                      \
//    }                                                                               \
//}


# else
# define NMLogInfo(arg)
# define NMLogWarn(arg)
# define NMLogError(arg)
# define NMLogDebug(arg)
# define NMLogProv(concept, args, attr)
# define MPILogErr(arg)
#endif // NM_ENABLE_LOGGER


// =====================================================
// macros to facilitate invoking itk::NMLogEvent() in
//      itk::Object derived classes (itk::ProcessObject)
// =====================================================
#ifdef NM_PROC_LOG
#include "itkNMLogEvent.h"

#define NMProcProvN(provType, args, attrs) \
        { \
            this->InvokeEvent(itk::NMLogEvent(        \
                               itk::NMLogEvent::NM_LOG_PROVN, \
                               provType, args, attrs)); \
        }

#define NMProcErr(arg) \
        { \
            std::stringstream sstr; \
            sstr arg; \
            this->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_ERROR)); \
        }

#define NMProcWarn(arg)  \
        { \
            std::stringstream sstr; \
            sstr arg; \
            this->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                    itk::NMLogEvent::NM_LOG_WARN)); \
        }

#define NMProcInfo(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            this->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_INFO)); \
       }

#define NMProcDebug(arg) \
       { \
            std::stringstream sstr; \
            sstr arg; \
            this->InvokeEvent(itk::NMLogEvent(sstr.str(), \
                   itk::NMLogEvent::NM_LOG_DEBUG)); \
       }

#define MPIProcErr( MPICALL )
//\
//{                                                                                   \
//    const int mpierr = MPICALL ;                                                    \
//    if ( mpierr != MPI_SUCCESS )                                                    \
//    {                                                                               \
//        std::string msg;                                                            \
//        switch(mpierr)                                                              \
//        {                                                                           \
//        case MPI_ERR_COMM: msg =   "MPI ERROR: Invalid communicator"; break;        \
//        case MPI_ERR_RANK: msg =   "MPI ERROR: Invalid rank"; break;                \
//        case MPI_ERR_GROUP: msg =  "MPI ERROR: Invalid group"; break;              \
//        case MPI_ERR_OP: msg =     "MPI ERROR: Invalid operation"; break;             \
//        case MPI_ERR_ARG: msg =    "MPI ERROR: Invalid argument of some other kind"; break; \
//        case MPI_ERR_KEYVAL: msg = "MPI ERROR: Invalid key has been passed"; break;   \
//        case MPI_ERR_INTERN: msg = "MPI ERROR: Internal MPI (implementation) error"; break; \
//        case MPI_ERR_INFO: msg =   "MPI ERROR: Invalid info argument"; break;        \
//        case MPI_ERR_SIZE: msg =   "MPI ERROR: Invalid size argument"; break;       \
//        case MPI_ERR_OTHER: msg =  "MPI ERROR: Known error not in this list"; break;  \
//                                                                                    \
//        case MPI_ERR_UNKNOWN:                                                       \
//        default: msg =             "MPI ERROR: Unknown error"; break;               \
//        }                                                                           \
//                                                                                    \
//        int rank=0;                                                                 \
//        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                       \
//        msg = "r" + std::to_string(rank) + " - " + msg;                             \
//        MPI_Finalize();                                                             \
//                                                                                    \
//        this->InvokeEvent(itk::NMLogEvent(msg, itk::NMLogEvent::NM_LOG_ERROR));      \
//     }                                                                               \
//}

#else
#define NMProcProvN(provType, args, attrs)
#define NMProcErr(arg)
#define NMProcWarn(arg)
#define NMProcInfo(arg)
#define NMProcDebug(arg)
#define MPIProcErr(arg)
#endif // NM_PROC_LOG

#endif /* NMLOG_H_ */
