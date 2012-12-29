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
 * NMModelController.h
 *
 *  Created on: 11/06/2012
 *      Author: alex
 */

#ifndef NMMODELCONTROLLER_H_
#define NMMODELCONTROLLER_H_

#include "nmlog.h"
#include "NMModelComponent.h"
#include "NMModelSerialiser.h"
 
#include <qobject.h>
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QString>

#include <string>
#include <iostream>

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

/*! \brief NMModelController is responsible for managing
 *         all NMModelComponents.
 *
 *   (Currently) NMModelController is implemented as Singleton
 *   to keep track of and manage a model's components
 *   (i.e. process components and aggregate components).
 *   The model controller lives in a separate thread and moves
 *   any new components being added to its component repository
 *   to the thread its running in. Also, it becomes the parent
 *   of each component it looks after (i.e. which is referenced in its
 *   repository).
 *
 *   \see NMModelComponent, NMProcess, NMAggregateComponentItem,
 *   NMProcessComponentItem
 *
 */

class NMModelController: public QObject
{
	Q_OBJECT

public:
	NMModelComponent* getComponent(const QString& name);
	QString addComponent(NMModelComponent* comp,
			NMModelComponent* host=0);
	bool removeComponent(const QString& name);
	bool contains(const QString& compName);
	NMModelComponent* identifyRootComponent(void);

	const QMap<QString, NMModelComponent*>& getRepository(void)
			{return this->mComponentMap;}

	static NMModelController* getInstance(void);

public slots:

	/*! Requests the execution of the named component. */
	void executeModel(const QString& compName);

	/*! Indicates whether any of the process components
	 *         controlled by this controller is currently
	 *         being executed or not.
	 */
	bool isModelRunning(void);

	/*! These *Stopped and *Started slots are usually connected
	 *         to a NMProcess's signalExecutionStarted/Stopped signals
	 *         to inform the controller about which model is currently
	 *         being executed or not.
	 */
	void reportExecutionStopped(const QString & compNamde);

	/*! These *Stopped and *Started slots are usually connected
	 *         to a NMProcess's signalExecutionStarted/Stopped signals
	 *         to inform the controller about which model is currently
	 *         being executed or not.
	 */
	void reportExecutionStarted(const QString & compName);

	/*! Sets NMProcess::mAbortExecution and NMModelController::mbAbortionRequested
	 *  to true; the process object then either aborts the process execution
	 *  or not and if so, it signals it back to the model controller
	 *  (cf. NMModelController::reportExecutionStopped()). The member
	 *  mbAbortionRequested is periodically checked within the update
	 *  iteration loop of a model component and the component breaks out of
	 *  the loop if an abortion is requested.
	 */
	void abortModel(void);

	/*! Indicates whether a model's update iteration loop should be
	 *  terminated.*/
	bool isModelAbortionRequested(void)
		{return this->mbAbortionRequested;}

	signals:
	/*! Signals whether any of the process components controlled
	 *  by this controller is currently running or not */
	void signalIsControllerBusy(bool);

protected:
	NMModelController(QObject* parent=0);
	virtual ~NMModelController();

	QMap<QString, NMModelComponent*> mComponentMap;
	NMModelComponent* mRootComponent;

	NMModelComponent* mRunningModelComponent;
	bool mbModelIsRunning;
	bool mbAbortionRequested;

	NMModelController* mModelController;

private:
	std::string ctx;

};

#endif /* NMMODELCONTROLLER_H_ */
