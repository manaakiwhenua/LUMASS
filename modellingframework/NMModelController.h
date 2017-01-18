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

#include <string>
#include <iostream>

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QThread>
#include <QMap>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include "otbAttributeTable.h"

#include "nmmodframe_export.h"

class NMItkDataObjectWrapper;
class NMModelComponent;
class NMIterableComponent;
class NMProcess;
class NMLogger;

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

class NMMODFRAME_EXPORT NMModelController: public QObject
{
	Q_OBJECT

public:

    enum DataComponentPropertyType
    {
        NM_DATAPROP_COLUMNS = 0
    };

    QSharedPointer<NMItkDataObjectWrapper> getOutputFromSource(const QString& inputSrc);
	NMModelComponent* getComponent(const QString& name);
    QList<NMModelComponent*> getComponents(const QString& userId);
	QString addComponent(NMModelComponent* comp,
			NMModelComponent* host=0);
	bool removeComponent(const QString& name);
	bool contains(const QString& compName);
	NMIterableComponent* identifyRootComponent(void);

	const QMap<QString, NMModelComponent*>& getRepository(void)
			{return this->mComponentMap;}

    QStringList getUserIDs(void);

    NMLogger* getLogger(void){return mLogger;}

	static NMModelController* getInstance(void);

    static QString getComponentNameFromInputSpec(const QString& inputSpec);

    static QStringList getPropertyList(const QObject* obj);

    otb::AttributeTable::Pointer getComponentTable(const NMModelComponent* comp);


    /*!
     * \brief Fetches a list of data component properties. Currently this only refers
     * to columns of an otb::AttributeTable object but might be extended in the future.
     * \param Model component object to extract the property list from; overloaded function
     *        takes the model component name instead
     * \param The property type. Currently only columns of otb::AttributeTable objects are
     *        supported
     * \return List of property names
     */
    QStringList getDataComponentProperties(const NMModelComponent* comp,
                                           DataComponentPropertyType type=NM_DATAPROP_COLUMNS);
    QStringList getDataComponentProperties(const QString& compName,
                                           DataComponentPropertyType type=NM_DATAPROP_COLUMNS);

    /*!
     * \brief processStringParameter
     * \param str
     * \return
     *
     * Parses and evaluates the parameter expression str. Parameter expressions have
     * the general form
     *
     *      $[<component identifier>:<component property>:<property index>]$,
     *
     * where the 2nd and 3rd parameter are optional and the 3rd parameter may only be
     * specified together with the second parameter. Parameter expressions may be nested
     * arbitrarily deep.
     * The component identifier can either be the (model) unique ComponentName property or
     * alternatively the UserID. Since the latter may not be unique throughout the model,
     * LUMASS searches for the first model component, which matches the given UserID. Thereby
     * it searches the execution stack 'upwards', starting within its own host aggregate
     * component (without looking into its sibling aggregate components(!)) until it finally
     * reaches the root model component.
     *
     * Parameter expressions may be used to access tabular data, e.g. held in a DataBuffer
     * object. In that case the 2nd parameter specifies the column of the table and
     * the thrid parameter the 1-based row number of the table, e.g.
     *
     *      $[MyParameterTable:FileNames:4]$
     *
     * fetches the value of table object 'MyParameterTable'
     *
     *
     * If the second and third parameter
     * are omitted, the expression is implicitly evaluated for the IterationStep of the
     * given model component. I.e.
     *
     *      $[component1]$ = $[component1:IterationStep]$
     *
     * If only the first parameter is specified, it may be followed by a '+' or a '-' operator
     * and an integer(!) operand to allow for simple arithmetic operations, i.e.
     *
     *      $[component1+4]$
     *
     * For example, if the IterationStep of component1 equals 3 the above epxression yields 7.
     *
     * Note: component identifiers may not contain any '-' or '+' sign!
     */
    QString processStringParameter(const QObject *obj, const QString& str);

    /*!
     * \brief getNextParamExpr Helper function to \ref processStringParameter
     * \param expr (Nested) parameter expression
     * \return List of (inner most) non-nested parameter expression
     *
     * Extracts non-nested parameter expressions from expr
     */
    QStringList getNextParamExpr(const QString& expr);


public slots:

	/*! Requests the execution of the named component. */
	void executeModel(const QString& compName);

    /*! Component destruction at the next suitble opportunity
     *  (i.e. either directly, or once the current model run has
     *  finished) */
    void deleteLater(QStringList compNames);


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

    void setUserId(const QString& oldId, const QString& newId);

signals:
	/*! Signals whether any of the process components controlled
	 *  by this controller is currently running or not */
	void signalIsControllerBusy(bool);
    void signalExecutionStopped(const QString&);

    /*! Notify listeners that a component was deleted from the controller */
    void componentRemoved(const QString&);

protected:
	NMModelController(QObject* parent=0);
	virtual ~NMModelController();

	void resetExecutionStack(void);

    /*! maps ComponentName to model component object */
	QMap<QString, NMModelComponent*> mComponentMap;
    /*! maps userId to ComponentName */
    QMultiMap<QString, QString> mUserIdMap;

	QStack<QString> mExecutionStack;
	NMIterableComponent* mRootComponent;

	bool mbModelIsRunning;
	bool mbAbortionRequested;

	QDateTime mModelStarted;
	QDateTime mModelStopped;

	NMModelController* mModelController;
    NMLogger* mLogger;

    QStringList mToBeDeleted;

private:
	static const std::string ctx;

};

#endif /* NMMODELCONTROLLER_H_ */
