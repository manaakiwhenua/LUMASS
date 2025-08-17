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

    // establish MPI context
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

    // de-register comm
    mController->deregisterParallelGroup(this->objectName());

    // prepare mpi child->parent state change signalling
    NMModelController::MPICompProg compProg;
    compProg.compName = this->objectName();

    // ----------------------------------------------------------
    // determine workload (ie number of tasks)
    int numIterations = this->mNumIterations;
    if (this->mNumIterationsExpression.size() != 0)
    {
        numIterations = this->evalNumIterationsExpression(mIterationStep);
        compProg.event = NMModelController::NM_EVENT_NUMITER_CHGD;
        compProg.progress = numIterations;
        mController->mpiSignalProgress(compProg);
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


    // --------------------  ALLOCATE ITERATION TASKS TO RANKS  -------------------------
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
                const int splitID = titer.value().first;
                std::string cname = this->objectName().toStdString() + "-" + std::to_string(splitID);
                NMDebugAI(<< "*** MPI_Comm_split(comm, "<< splitID << ", " << rank
                               << ", " << cname << ")" << std::endl);
                MPI_Comm_split(comm, splitID, rank, &iterComm);
                MPI_Comm_set_name(iterComm, cname.c_str());
                mController->registerParallelGroup(this->objectName(), iterComm);
                allocatedRanks.push_back(rank);
            }
        }
    }

    // DEBUG
    //if (procs > 1 && !allocatedRanks.contains(rank))
    //{
    //    NMDebugAI(<< "*** MISSING A SPLIT HERE! Rank #" << rank
    //              << " should have been assigned splitID=-1!!" << std::endl);
    //}

    // --------------------  EXECUTE ITERATIONS IN PARALLEL  ---------------------------------------------
    unsigned int niter = numIterations;
    mIterationStepRun = mIterationStep;
    for (unsigned int i = mIterationStepRun-1; i < niter && !mController->isModelAbortionRequested(); ++i)
    {
        auto mtrIter = mapTaskSplitRanks.find(i);
        if (mtrIter != mapTaskSplitRanks.end())
        {
            if (mtrIter.value().second.contains(rank))
            {
                NMDebugAI(<< "*** r" << rank << ":  is running: " << this->objectName().toStdString()
                          << "'s iteration #" << mIterationStepRun << std::endl);

                emit signalProgress(mIterationStepRun);
                compProg.event = NMModelController::NM_EVENT_PROGRESS;
                compProg.progress = mIterationStepRun;
                mController->mpiSignalProgress(compProg);
                this->componentUpdateLogic(repo, minLevel, maxLevel, i);
            }
        }

        niter = evalNumIterationsExpression(mIterationStepRun+1);
        if (niter != mIterationStepRun+1)
        {
            compProg.event = NMModelController::NM_EVENT_NUMITER_CHGD;
            compProg.progress = niter;
            mController->mpiSignalProgress(compProg);
        }

        this->setNumIterations(niter);
        ++mIterationStepRun;
    }
    mIterationStepRun = mIterationStep;
    emit signalProgress(mIterationStep);

    compProg.event = NMModelController::NM_EVENT_PROGRESS;
    compProg.progress = mIterationStep;
    mController->mpiSignalProgress(compProg);


    // de-register IterComm
    // free IterComm
    if (iterComm != MPI_COMM_NULL)
    {
        mController->deregisterParallelGroup(this->objectName());
        MPI_Comm_free(&iterComm);
    }

    // re-register 'original' comm
    if (comm != MPI_COMM_NULL)
    {
        mController->registerParallelGroup(this->objectName(), comm);
    }
}

