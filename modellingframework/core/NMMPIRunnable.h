/******************************************************************************
* Created by Alexander Herzig
* Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
********************************************************************************/

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
 * NMMPIRunnable.h
 *
 *  Created on: 2024-09-25
 *      Author: heralex
 */

#ifndef NMMPIRunnable_H_
#define NMMPIRunnable_H_

#include <qrunnable.h>
#include <QString>

#include "NMLogger.h"
#include "NMModelController.h"

class NMMPIRunnable: public QObject, public QRunnable
{
    Q_OBJECT

public:
    NMMPIRunnable();
    virtual ~NMMPIRunnable();

    void setData(const int _nprocs,
            const QStringList& _modelComps,
            const QString& _lumassPath,
            const QString& _yamlFN,
            const QString& _logFN,
            NMLogger*& logger,
            MPI_Comm& mergedComm,
            MPI_Comm& parentComm,
            MPI_Win&  rmaWin,
            MPI_Win&  abortWin,
            int*&     compState,
            int*&     abort
            );
	void run();
	void setLogger(NMLogger* logger) { mLogger = logger; }

public slots:

    void processAbortionRequest();

signals:
    void signalMPIEvent(const QString compName, const NMModelController::ModelEvent event,
                        const float value);
    void signalMPILoopFinished(NMMPIRunnable* obj);
    void signalExecStarted(const QString compName);
    void signalExecStopped(const QString compName);


private:
    int         nprocs;
    QStringList modelComps;
    QString     lumassPath;
    QString     yamlFN;
    QString     logFN;

    NMLogger* mLogger;
    MPI_Comm mMergedComm;
    MPI_Comm mParentMPIComm;
    MPI_Win mMPICompProgWin;
    MPI_Win mMPIParentAbort;
    int* mMPICompState;
    int* mMPIAbort;

    bool mbAbortionRequested;

    static const std::string ctx;

};

#endif /* NMMPIRunnable_H_ */
