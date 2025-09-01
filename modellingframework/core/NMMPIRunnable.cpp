/******************************************************************************
* Created by Alexander Herzig
* Copyright 2024 Landcare Research New Zealand Ltd
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
 * NMMPIRunnable.cpp
 *
 *  Created on: 2024-09-25
 *      Author: heralex
 */
#ifndef NM_ENABLE_LOGGER
    #define NM_ENABLE_LOGGER
#endif
#include "nmlog.h"
#include "mpi.h"

#include "NMMPIRunnable.h"
#include "NMModelController.h"

const std::string NMMPIRunnable::ctx = "NMMPIRunnable";

NMMPIRunnable::NMMPIRunnable()
    : mLogger(nullptr),
      mMPICompProgWin(MPI_WIN_NULL),
      mbAbortionRequested(false)
{
	// TODO Auto-generated constructor stub

}

NMMPIRunnable::~NMMPIRunnable()
{
	// TODO Auto-generated destructor stub
}

void
NMMPIRunnable::setData(const int          _nprocs,
        const QStringList& _modelComps,
        const QString&     _lumassPath,
        const QString&     _yamlFN,
        const QString&     _logFN,
        NMLogger*& logger,
        MPI_Comm& mergedComm,
        MPI_Comm& parentComm,
        MPI_Win&  rmaWin,
        MPI_Win&  abortWin,
        int*&     compState,
        int*&     abort)
{
    nprocs     = _nprocs             ;
    modelComps = _modelComps;
    lumassPath = _lumassPath    ;
    yamlFN     = _yamlFN        ;
    logFN      = _logFN         ;

    mLogger         = logger;
    mMergedComm     = mergedComm;
    mParentMPIComm  = parentComm;
    mMPICompProgWin = rmaWin;
    mMPICompState   = compState;

    mMPIParentAbort = abortWin;
    mMPIAbort       = abort;
}

void
NMMPIRunnable::processAbortionRequest()
{
    mbAbortionRequested = true;
    NMDebugAI(<< ctx << ": Model abortion requested" << std::endl);
}

void
NMMPIRunnable::run()
{
    NMDebugCtx(ctx, << "...");

    bool execute = true;
    bool bHaveStarted = false;
    bool bChildAborted = false;

    QSet<QString> execStack;

    const int ncomps = modelComps.size();
    const int nvals  = 2; // {event==value, progress==value+1}

    QDateTime modelStarted = QDateTime::currentDateTime();
    QSet<int> abortedChildRanks;

    // give every process a chance to convey their message
    // we loop over each component and collect progress information
    // for a given component from each process
    bool bSignalledAbortion = false;
    while (execute)
    {
        QString compName;
        NMModelController::ModelEvent event;

        // forward abortion requests to child processes
        // if the user requested model abortion
        if (mbAbortionRequested && !bSignalledAbortion)
        {
            int abort = 1;

            NMDebugAI(<< ctx << ": Parent about to set the ABORT signal!" << std::endl);
            MPI_Win_lock(MPI_LOCK_SHARED, 0, 0,
                         mMPIParentAbort);
            *mMPIAbort = 1;
            //MPI_Put(static_cast<void*>(&abort), 1, MPI_INT, 0, 0, 1, MPI_INT, mMPIParentAbort);
            MPI_Win_unlock(0, mMPIParentAbort);

            bSignalledAbortion = true;

            NMDebugAI(<< ctx << ": Model abortion signalled!" << std::endl);
        }

        for (int mcomp=0; mcomp < modelComps.size(); ++mcomp)
        {
            // determine the (min.) state across child processes' model components

            // if at least one of the child comps is currently executing this is true!
            bool bStarted = false;
            bool bStopped = false;
            bool bProgress = false;
            bool bNumIterChgd = false;
            bool bCompleted = false;

            // least progress of a given component across all parallel processes
            int minProgress = 100;
            // sum of progress across all parallel processes
            int sumProgress = 0;
            // number of iterations
            int numIter = 1;

            for (int rs=0; rs < nprocs; ++rs)
            {

                // log event
                std::stringstream logstr;

                const int compId = mcomp;
                compName = modelComps.at(compId);

                const int target_pos = rs * ncomps * nvals + compId * nvals;

                // note that the RMA window is defined over the parent AND child
                // process together: #0=parent, #1=child0, #2=child1, etc.,
                // therefore, we're using rs+1 as rank for accessing the child
                // process windows ...
                MPI_Win_lock(MPI_LOCK_SHARED, rs+1, MPI_MODE_NOCHECK,
                             mMPICompProgWin);
                MPI_Get(static_cast<void*>(&mMPICompState[target_pos]), 2, MPI_INT,
                        rs+1, target_pos, 2, MPI_INT, mMPICompProgWin);
                MPI_Win_unlock(rs+1, mMPICompProgWin);

                const NMModelController::ModelEvent event =
                        static_cast<NMModelController::ModelEvent>(mMPICompState[target_pos]);
                const int prog = mMPICompState[target_pos+1];

                NMDebugAI(<< "server at target_pos=" << target_pos
                          << " child #" << rs << "'s " << compName.toStdString()
                          << "::" << event << "=" << prog << std::endl);

                logstr << "received: #" << rs << ": '" << compName.toStdString() << "' ";
                switch(event)
                {
                case NMModelController::NM_EVENT_EXEC_STOPPED: // 4
                    logstr << "stopped";
                    bStopped = true;
                    break;
                case NMModelController::NM_EVENT_EXEC_STARTED: // 3
                    logstr << "started";
                    bStarted = true;
                    break;
                case NMModelController::NM_EVENT_PROGRESS:     // 2
                    logstr << " is at " << prog << "%";
                    bProgress = true;
                    minProgress = std::min(prog, minProgress);
                    sumProgress += prog;
                    break;
                case NMModelController::NM_EVENT_EXEC_ABORTED: // 7
                    logstr << "aborted!";
                    mbAbortionRequested = true;
                    bChildAborted = true;
                    abortedChildRanks.insert(rs);
                    minProgress = 0;
                    sumProgress = 0;
                    break;
                case NMModelController::NM_EVENT_NUMITER_CHGD: //8
                    logstr << "numIterChgd";
                    bNumIterChgd = true;
                    numIter = std::max(prog, numIter);
                    break;
                case NMModelController::NM_EVENT_MODEL_COMPLETED: //9
                    logstr << "model completed!";
                    abortedChildRanks.insert(rs);
                    if (abortedChildRanks.size() == nprocs)
                    {
                        bCompleted = true;
                    }
                    //minProgress = 0;
                    //sumProgress = 100;
                    break;

                case NMModelController::NM_EVENT_UNKNOWN:
                default:
                    //logstr << " not started yet!";
                    break;
                }

                NMDebugAI( << logstr.str() << std::endl);
                //logstr.str("");

                if (bChildAborted)
                {
                    break;
                }
                else if (bNumIterChgd)
                {
                    emit signalMPIEvent(compName, NMModelController::NM_EVENT_NUMITER_CHGD, numIter);
                    bNumIterChgd = false;
                    NMDebugAI(<< "server: signal: " << compName.toStdString()
                              << " NumIter changed - value=" << numIter << std::endl);
                }

            } // end of child proc iteration

            // signal the components 'overall' status across processes
            // to the modelview widget
            if (bChildAborted)
            {
                NMDebugAI(<< "server: signal: " << compName.toStdString()
                          << " model aborted!" << std::endl);
                emit signalExecStopped(compName);
                emit signalMPIEvent(compName, NMModelController::NM_EVENT_EXEC_ABORTED, minProgress);
                //execStack.clear();
                // don't continue processing! Child processes may have stopped and terminated themselves already!
                break;
            }
            else if (bProgress && minProgress >= 0)
            {
                NMDebugAI(<< "server: signal: " << compName.toStdString()
                          << " is at " << minProgress << " %" << std::endl);
                emit signalExecStarted(compName);
                emit signalMPIEvent(compName, NMModelController::NM_EVENT_PROGRESS, minProgress);
                execStack << compName;
                bHaveStarted = true;
            }
            else if (bStarted)
            {
                NMDebugAI(<< "server: signal: " << compName.toStdString()
                          << " started" << std::endl);
                emit signalExecStarted(compName);
                emit signalMPIEvent(compName, NMModelController::NM_EVENT_EXEC_STARTED, 0);
                execStack << compName;
                bHaveStarted = true;
            }
            else if (bStopped && sumProgress == 0)
            {
                NMDebugAI(<< "server: signal: " << compName.toStdString()
                          << " stopped" << std::endl);
                emit signalExecStopped(compName);
                emit signalMPIEvent(compName, NMModelController::NM_EVENT_EXEC_STOPPED, 0);
                execStack.remove(compName);
            }
            else if (bCompleted)
            {
                NMDebugAI(<< "server: signal: model completed!" << std::endl);
                emit signalMPIEvent(compName, NMModelController::NM_EVENT_MODEL_COMPLETED, 100);
            }
            //else if (bNumIterChgd)
            //{
            //    NMDebugAI(<< "server: signal: " << compName.toStdString()
            //              << " NumIter changed!" << std::endl);
            //    emit signalMPIEvent(compName, NMModelController::NM_EVENT_NUMITER_CHGD, numIter);
            //    bNumIterChgd = false;
            //}
        } // end of comp iteration

        //DEBUG - new stack entries
        NMDebugAI(<< "execStack: ");

        foreach(const QString& comp, execStack)
        {
            NMDebug(<< comp.toStdString() << " ");
        }
        NMDebug(<< std::endl);

        if (    (bChildAborted && bSignalledAbortion)
             || (execStack.size() == 0 && bHaveStarted)
             || abortedChildRanks.size() == nprocs
           )
        {
            execute = false;
        }
    }

    QDateTime modelStopped = QDateTime::currentDateTime();
    int msec = modelStarted.msecsTo(modelStopped);
    int min = msec / 60000;
    double sec = (msec % 60000) / 1000.0;

    QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
    NMMsg(<< "Model run took (min:sec): " << elapsedTime.toStdString() << std::endl);
    NMLogInfo(<< "NMMPIRunnable: Model completed in (min:sec): " << elapsedTime.toStdString());

    NMDebugAI(<< "ParentProcess exited mpi event loop!" << std::endl);

    signalMPILoopFinished(this);
    NMDebugCtx(ctx, << "done!");
}
