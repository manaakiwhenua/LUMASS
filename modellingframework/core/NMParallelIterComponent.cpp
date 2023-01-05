/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMParallelIterComponent.cpp
 *
 *  Created on: 2022-05-13
 *      Author: alex
 */
#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

#include "NMParallelIterComponent.h"
#include "NMDataComponent.h"
#include "NMModelController.h"
#include "NMMfwException.h"

const std::string NMParallelIterComponent::ctx = "NMParallelIterComponent";

NMParallelIterComponent::NMParallelIterComponent(QObject* parent)
{
    this->setParent(parent);
    NMIterableComponent::initAttributes();
}

NMParallelIterComponent::~NMParallelIterComponent()
{
}

void
NMParallelIterComponent::iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel)
{
    // get comm
    MPI_Comm comm = mController->getNextUpstrMPIComm(this->objectName());

    // get procs
    // get rank
    int worldRank = 0;
    int rank = 0;
    int procs = 1;
    if (comm != MPI_COMM_NULL)
    {
        MPI_Comm_size(comm, &procs);
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    }

    // let's catch-up with all the other ranks assigned to this task
    MPI_Barrier(comm);

    // de-register comm
    mController->deregisterParallelGroup(this->objectName());

    // re-allocate ranks (IterComm) to tasks; where
    // a task is one iteration of this component
    int numIterations = this->mNumIterations;
    if (this->mNumIterationsExpression.size() != 0)
    {
        numIterations = this->evalNumIterationsExpression(mIterationStep);
    }

    int ntasks = numIterations - (mIterationStep - 1);
    int nsplits = std::min(ntasks, procs);
    QMap<int, QPair<int, QVector<int>>> mapTaskSplitRanks;

    // remove debug below
    NMIterableComponent* host = this->getHostComponent();
    unsigned int hostIterStep = 9999;
    std::string hostName = "NoName";
    if (host != nullptr)
    {
        hostName = host->objectName().toStdString();
        hostIterStep = host->getIterationStep();
    }
    std::stringstream sstr1;
    sstr1 << this->objectName().toStdString()
         << "::iterativeComponentUpdate(): lr" << rank
         << " procs=" << procs
         << " numIterations=" << numIterations
         << " iterationStep=" << mIterationStep
         << " hostName=" << hostName
         << " hostIterStep=" << hostIterStep << std::endl;
    NMDebugAI(<< sstr1.str());
    // remove debug above


    int rankId = 0;
    int splitId = 0;
    int taskId = mIterationStep - 1;
    while (taskId < ntasks && rankId < procs)
    {
        if (mapTaskSplitRanks.find(taskId) == mapTaskSplitRanks.end())
        {
            QPair<int, QVector<int>> splitRanks;
            splitRanks.first = splitId;
            splitRanks.second.push_back(rankId);
            mapTaskSplitRanks.insert(taskId, splitRanks);
        }
        else
        {
            mapTaskSplitRanks[taskId].second.push_back(rankId);
        }

        if (procs < ntasks)
        {
            rankId = rankId < procs-1 ? rankId+1 : 0;
            ++taskId;
        }
        else
        {
            ++rankId;
            taskId = taskId < ntasks-1 ? taskId+1 : 0;
        }
        splitId = splitId < nsplits-1 ? splitId+1 : 0;
    }

    // DEBUG
    wulog(-1, "<<" << this->objectName().toStdString() << ">> Allocation RESULTS ... ")
    for (auto it=mapTaskSplitRanks.cbegin(); it != mapTaskSplitRanks.cend(); ++it)
    {
        std::string ranklist;
        vstr(it.value().second, " ", ranklist);
        wulog(-1, " lr" << rank << " - t" << it.key() << ": (" << it.value().first << "), " << ranklist)
    }
    // DEBUG

    QVector<int> allocatedRanks;
    MPI_Comm iterComm = MPI_COMM_NULL;
    if (procs > 1 && comm != MPI_COMM_NULL)
    {
        for (auto titer = mapTaskSplitRanks.cbegin(); titer != mapTaskSplitRanks.cend(); ++titer)
        {
            if (titer.value().second.contains(rank) && !allocatedRanks.contains(rank))
            {
                MPI_Comm_split(comm, titer.value().first, rank, &iterComm);
                wulog(-1, " lr" << rank << ": splitId=" << titer.value().first
                      << ": taskId=" << titer.key() << " iterComm=" << iterComm << endl)
                mController->registerParallelGroup(this->objectName(), iterComm);
                allocatedRanks.push_back(rank);
            }
            //else
            //{
            //    MPI_Comm_split(comm, -1, rank, &iterComm);
            //    wulog(-1, " lr" << rank << ": splitId=-1 : taskId=" << titer.key() << " iterComm=" << iterComm << endl)
            //}
        }
    }

    // iterate over rank list
    //    register IterComm / ranks for this component
    //    call componentUpdateLogic with iteration parameters
    //    BARRIER - IterComm

    QVector<int> busyRanks;
    unsigned int niter = numIterations;
    mIterationStepRun = mIterationStep;
    for (unsigned int i = mIterationStepRun-1; i < niter && !mController->isModelAbortionRequested(); ++i)
    {
        auto mtrIter = mapTaskSplitRanks.find(i);
        if (mtrIter != mapTaskSplitRanks.end())
        {

            if (mtrIter.value().second.contains(rank))
            {
                std::stringstream pmsg;
                pmsg << " lr" << rank << ": " << this->objectName().toStdString() <<
                        " step #" << i << ": splitId=" << mtrIter.value().first
                     << " : taskId=" << mtrIter.key();
                wulog(-1, pmsg.str());

                busyRanks.push_back(rank);
                emit signalProgress(mIterationStepRun);
                this->componentUpdateLogic(repo, minLevel, maxLevel, i);
            }
        }

        // once we've engaged all available ranks in allocated tasks,
        // we need to wait untill they're finished before we're
        // re-engaging a rank for the second time
        if (busyRanks.size() == procs)
        {
            busyRanks.clear();
            if (iterComm != MPI_COMM_NULL)
            {
                MPI_Barrier(iterComm);
            }
        }

        niter = evalNumIterationsExpression(mIterationStepRun+1);
        //NMDebugAI(<< this->objectName().toStdString() << ": in-loop: IterStep=" << getIterationStep()
        //                                        << " i=" << i << " niter=" << niter << std::endl);
        this->setNumIterations(niter);
        ++mIterationStepRun;
    }
    mIterationStepRun = mIterationStep;
    emit signalProgress(mIterationStep);


    // de-register IterComm
    // free IterComm
    if (iterComm != MPI_COMM_NULL)
    {
        MPI_Barrier(iterComm);
        mController->deregisterParallelGroup(this->objectName());
        MPI_Comm_free(&iterComm);
    }

    if (comm != MPI_COMM_NULL)
    {
        mController->registerParallelGroup(this->objectName(), comm);
    }
}

