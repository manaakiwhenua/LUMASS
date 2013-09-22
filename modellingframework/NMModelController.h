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
#include "NMModelSerialiser.h"
 
#include <qobject.h>
#include <QThread>
#include <QMap>
#include <QString>
#include <QDateTime>

#include <string>
#include <iostream>

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

class NMItkDataObjectWrapper;
class NMProcess;

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

	NMItkDataObjectWrapper* getOutputFromSource(const QString& inputSrc);
	NMModelComponent* getComponent(const QString& name);
	QString addComponent(NMModelComponent* comp,
			NMModelComponent* host=0);
	bool removeComponent(const QString& name);
	bool contains(const QString& compName);
	NMIterableComponent* identifyRootComponent(void);

	const QMap<QString, NMModelComponent*>& getRepository(void)
			{return this->mComponentMap;}

	static NMModelController* getInstance(void);

public slots:

	/*! Requests the execution of the named component. */
	void executeModel(const QString& compName);

	/*! Resets the named model component and (recursively!)
	 *  all of its sub components, i.e. reset is called on
	 *  each process component hosted by this component.
	 *  This causes all process components to be re-initialised
	 *  upon next execution (i.e. either explicit update call or
	 *  implicit execution as part of a pipeline).
	 */
	void resetComponent(const QString& compName);

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

	/*! Sets NMProcess::mAbortExecution to true for the most recently
	 *  executed and still running process component at the time this
	 *  function is invoked (note: due to delay in signal
	 *  slot processing across threads, this may differ from the
	 *  highlighted process item in the GUI at the time the
	 *  user requested model abortion); If the process objects
	 *  supports abortion it stops at a process specific point and
	 *  signals back to the model controller that it stopped
	 *  (cf. NMModelController::reportExecutionStopped()).
	 *  This method also sets NMModelController::mbAbortionRequested
	 *  to true, which is periodically checked within a model
	 *  component's update iteration loop to prevent (re-) execution
	 *  of 'aborted' process objects and process objects, which doesn't
	 *  support abortion.
	 */
	void abortModel(void);

	const QDateTime& getModelStartTime(void)
		{return this->mModelStarted;}

	/*! Indicates whether a model's update iteration loop should be
	 *  terminated.*/
	bool isModelAbortionRequested(void)
		{return this->mbAbortionRequested;}

	signals:
	/*! Signals whether any of the process components controlled
	 *  by this controller is currently running or not */
	void signalIsControllerBusy(bool);
	void signalExecutionStopped(const QString&);

protected:
	NMModelController(QObject* parent=0);
	virtual ~NMModelController();

	void resetExecutionStack(void);

	QMap<QString, NMModelComponent*> mComponentMap;
	QStack<QString> mExecutionStack;

	NMIterableComponent* mRootComponent;

	//NMModelComponent* mRunningModelComponent;
	bool mbModelIsRunning;
	bool mbAbortionRequested;

	QDateTime mModelStarted;
	QDateTime mModelStopped;

	NMModelController* mModelController;

private:
	static const std::string ctx;

};

#endif /* NMMODELCONTROLLER_H_ */
