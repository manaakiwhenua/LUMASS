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
#include <QFile>

#include "NMObject.h"
#include "otbAttributeTable.h"

#include "nmmodframe_export.h"

class NMItkDataObjectWrapper;
class NMModelComponent;
class NMIterableComponent;
class NMProcess;
class NMLogger;

/*! \brief NMModelController is responsible for managing and
 *   running a single LUMASS model.
 *
 *   One NMModelController is responsible for the model being
 *   edited/run within the modelling environment; each user tool
 *   has its own NMModelController associated with it for running
 *   the particular model and so has the lumassengine.
 *   Each NMModelController manages its own list of model settings,
 *   e.g. for writing provenace information or not.
 *
 *
 *
 *   \see NMModelComponent, NMProcess, NMAggregateComponentItem,
 *   NMProcessComponentItem
 *
 */

class NMMODFRAME_EXPORT NMModelController: public QObject, public NMObject
{
	Q_OBJECT

public:

    enum DataComponentPropertyType
    {
        NM_DATAPROP_COLUMNS = 0
    };

    NMModelController(QObject* parent=0);
    virtual ~NMModelController();

    virtual void setLogger(NMLogger* logger);

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

//    NMLogger* getLogger(void){return mLogger;}

//	static NMModelController* getInstance(void);

    QString getComponentNameFromInputSpec(const QString& inputSpec);

    QStringList getPropertyList(const QObject* obj);
    QStringList getProvNAttributes(const QObject *comp);

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
     *******************************
     *** GENERAL PARAMETER EXPRESSION
     *******************************
     *  Parses and evaluates the parameter expression str. Parameter expressions have
     *  the general form
     *
     *      $[<component identifier>:<component property>:<property index>]$,
     *
     *      Note: component identifiers may not contain any '-' or '+' sign!
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
     *** ********************************
     *** SHORT HAND EXPRESSION
     *** ********************************
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
     *
     *****************************
     *** LUMASS AND MODEL SETTINGS
     * *****************************
     * LUMASS or model settings may be accessed by parameter expressions, if 'LUMASS'
     * is used as identifier and the particular setting keyword is used as property, e.g.
     *
     *      $[LUMASS:UserModels]$
     *
     * the expressions returns the user models directory as string. Other global settings are 'Workspace'
     * and 'LUMASSPath'. The latter is set automatically when lumass starts and provides a path
     * to the storage location of the currently running lumass executable, e.g. lumass.exe.
     *
     *
     **************************
     *** FUNCTIONAL EXPRESSIONS
     ***************************
     *
     *      $[math: <mathematical equation parsable by MuParser>]$
     *
     *      String and filename processing functions based on
     *      Qt equivalents, which returns string values.
     *
     *      $[func:isFile(<filename>)]$
     *          /home/user/anImage.kea  -> 1
     *          My grandma has a cold   -> 0
     *          /home/user              -> 0
     *         ""                       -> 0
     *
     *      $[func:isDir(<filename>)]$
     *          /home/user/anImage.kea  -> 0
     *          My grandma has a cold   -> 0
     *          /home/user              -> 1
     *         ""                       -> 0
     *
     *      $[func:fileBaseName(<filename>)]$
     *          /home/user/archive.tar.gz   -> archive
     *
     *      $[func:fileCompleteBaseName(<filename>)]$
     *          /home/user/archive.tar.gz   -> archive.tar
     *
     *      $[func:filePath(<filename>)]$
     *          /home/user/archive.tar.gz   -> /home/user
     *          ../user                     -> /home/user
     *
     *      $[func:fileSuffix(<filename>)]$
     *          /home/user/archive.tar.gz   -> gz
     *
     *      $[func:fileCompleteSuffix(<filename>)]$
     *          /home/user/archive.tar.gz   -> tar.gz
     *
     *      $[func:strIsEmpty(<string>)]$
     *          /home/user/archive.tar.gz   -> 0
     *          ""                          -> 1
     *
     *      $[func:strLength(<string>)]$
     *          "Hello LUMASS!"             -> 13
     *          ""                          -> 0
     *
     *      // string_1, string_1: strings to be compared lexicographically
     *      // CaseSensitive: 0: case insensitive; 1: case sensitive
     *      // return: returns an integer number smaller, equal to, or greater
     *      //         than zero, if string_1 is less, equal to, or greater
     *      //         than string_1
     *      $[func:strCompare("<string_1>", "<string_2>", <int: CaseSensitive=0>)
     *          ("hello", "Hello", 0)       -> 0
     *          ("hello", "Hello", 1)       -> 32
     *          ("Hello", "hello", 1)       -> -32
     *          ("Hello", "Hello", 1)       -> 0
     *
     *      $[func:strReplace("<string>", "<find string>", "<replace string>")]$
     *          ("/home/user/archive.tar.gz", "/", "_") -> _home_user_archive.tar.gz
     *
     *      $[func:strSubstring("<string>", <start pos>, <num chars>)]$
     *          ("/home/user/archive.tar.gz", 1, 4)     -> home
     *
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

    QString evalFunc(const QString& funcName, const QStringList& args);
    QStringList parseQuotedArguments(const QString& args, const QChar& sep= ',');

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

    void updateSettings(const QString& key, QVariant value);

    QStringList getModelSettingsList(void);
    void clearModelSettings(void);

    QVariant getSetting(const QString& key) const
        {return mSettings[key];}

    bool isLogProvOn(){return mbLogProv;}
    void setLogProvOn() {mbLogProv = true;}
    void setLogProvOff() {mbLogProv = false;}
    void startProv(const QString& fn, const QString& compName);
    void endProv();
    void writeProv(const QString& provLog);

signals:
	/*! Signals whether any of the process components controlled
	 *  by this controller is currently running or not */
	void signalIsControllerBusy(bool);
    void signalExecutionStopped(const QString&);

    void signalModelStarted();
    void signalModelStopped();

    /*! Notify listeners that a component was deleted from the controller */
    void componentRemoved(const QString&);

    void settingsUpdated(const QString& key, const QVariant& value);

protected:
	void resetExecutionStack(void);
    void logProvNComponent(NMModelComponent* comp);
    void trackIdConceptRev(const QString& id, const QString&, const int& rev);

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

    QStringList mToBeDeleted;

    QMap<QString, QVariant> mSettings;

    bool mbLogProv;
    QFile mProvFile;
    QString mProvFileName;
    QMap<QString, QMap<QString, int> > mMapProvIdConRev;


private:
	static const std::string ctx;

};

#endif /* NMMODELCONTROLLER_H_ */
