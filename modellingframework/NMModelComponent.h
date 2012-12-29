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

#ifndef NMMODELCOMPONENT_H
#define NMMODELCOMPONENT_H
#define ctxNMModelComponent "NMModelComponent"

#include <qobject.h>
#include <string>
#include <vector>

#include <QMap>
#include <QMetaType>
#include <QSharedPointer>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

class NMProcess;

/*! \brief NMModelComponent is one of the core building blocks of the LUMASS' modelling
 *         framework. It represents either a single process (i.e. algorithm,
 *         called 'process component') or a chain of processes
 *         (i.e. a processing pipeline, called 'aggregate component').
 *
 *		   NMModelComponent can be thought of as an intelligent container which
 *		   contains either
 *		   a single process object (NMProcess) or which references a doubly linked list
 *		   of model components (NMModelComponent) hosting a process object. Each
 *		   model component referenced in the doubly linked list managed by an individual
 *		   model component is referred to as a 'sub component' of this model
 *		   component, which is in turn referred to as the 'host component'.
 *		   Process objects are the real working horses of a model and contain
 *		   the algorithmic model logic, i.e. they are actually working on the data
 *		   to change it or create new data. NMModelComponent
 *		   objects on the other hand are used to organise individual processes and
 *		   to build complex models. NMModelCompoent implements the model execution logic
 *		   (s. NMModelComponent::update()). For example it provides properties to
 *		   control the temporal behaviour of model components (i.e. time level
 *		   and number of iterations) and thereby allows the creation of
 *		   dynamic models whose components run on different time scales (levels).
 *
 *	\see NMProcess, NMModelController
 */

class NMModelComponent : public QObject
{
	Q_OBJECT
	Q_PROPERTY(NMModelComponent* HostComponent READ getHostComponent WRITE setHostComponent NOTIFY NMModelComponentChanged)
	Q_PROPERTY(QString Description READ getDescription WRITE setDescription NOTIFY NMModelComponentChanged)
	Q_PROPERTY(short TimeLevel READ getTimeLevel WRITE setTimeLevel NOTIFY NMModelComponentChanged)
	Q_PROPERTY(unsigned int NumIterations READ getNumIterations WRITE setNumIterations NOTIFY NMModelComponentChanged)

public:
	NMPropertyGetSet(HostComponent, NMModelComponent*)
    NMPropertyGetSet(Description, QString)
    NMPropertyGetSet(NumIterations, unsigned int)

signals:
	void NMModelComponentChanged();

public:

    NMModelComponent(QObject* parent=0);
    NMModelComponent(const NMModelComponent& modelComp);
    virtual ~NMModelComponent(void);

    short getTimeLevel(void)
    	{return this->mTimeLevel;}
    void setTimeLevel(short level);
    void changeTimeLevel(int diff);

    void setInternalStartComponent (NMModelComponent* comp )
    	{this->mProcessChainStart = comp;};

    /*! Returns the first sub component referenced by the doubly linked
     *  list hosted by this component. Note that the order of referenced
     *  sub components is independent of the individual sub components
     *  time level. After this function has been called, the host component's
     *  internal pointer points at the second sub component of the
     *  doubly linked list (which is identical with the start component's
     *  'downstream component').*/
    NMModelComponent* getInternalStartComponent(void);

    /*! Returns the next sub component referenced in the doubly linked
     *  list of sub components. Note this is identical with the previously
     *  returned component's 'downstream component'
     *  (or the start component, if this method is called for the first time
     *  after NMModelComponent::getInternalStartComponent() has been called.
     */
    NMModelComponent* getNextInternalComponent(void);
    NMModelComponent* getLastInternalComponent(void);

    NMModelComponent* getDownstreamModelComponent(void)
    	{return this->mDownComponent;}
    NMModelComponent* getUpstreamModelComponent(void)
    	{return this->mUpComponent;}

    void setDownstreamModelComponent(NMModelComponent* comp)
    	{this->mDownComponent = comp;}
    void setUpstreamModelComponent(NMModelComponent* comp)
    	{this->mUpComponent = comp;}

    /*! Adds a sub component to the model component. This method does
     *  nothing, if the component hosts already a process object (NMProcess).
     *  If you want to turn a process component into an aggregate component,
     *  you have to first set the component's process pointer to NULL (s.
     *  NMModelComponent::setProcess()) and then call this method.
     *  \note The insertion order of sub components determines their
     *  execution order (s. NMModelComponent::update()) when the
     *  component's update method is called. However, all sub components on
     *  a higher time level than the host component are executed first depending
     *  on the order of their time level (i.e. highest level gets executed first).
     */
    void addModelComponent(NMModelComponent* comp);

    /*! Inserts a new sub component into the chain of model components
     *  after the named sub component (see also NMModelComponent::addModelComponent()).*/
    void insertModelComponent(NMModelComponent* proc, const QString& previousComponent );

    /*! Looks for the named sub component and either returns a valid pointer or NULL.*/
    NMModelComponent* findModelComponent(const QString& compName );

    /*! Removes the named sub component from this host.*/
    NMModelComponent* removeModelComponent(const QString& compName);

    /*! Counts the number of sub components. */
    int countComponents(void);


    void setInput(NMItkDataObjectWrapper* inputImg)
    	{this->setNthInput(0, inputImg);};
    virtual void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);

    virtual NMItkDataObjectWrapper* getOutput(void);

    virtual void setProcess(NMProcess* proc);
    virtual NMProcess* getProcess(void)
    	{return this->mProcess;};

    void initialiseComponents(void);
    void update(const QMap<QString, NMModelComponent*>& repo);
    void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    void destroySubComponents(QMap<QString, NMModelComponent*>& repo);

    void mapTimeLevels(unsigned int startLevel,
    		QMap<unsigned int,
    		QMap<QString, NMModelComponent*> >& timeLevelMap);
    NMProcess* getEndOfTimeLevel(void);
    void getEndOfPipelineProcess(NMProcess*& endProc);
    bool isSubComponent(NMModelComponent* comp);


protected:
    std::string ctx;
    QMap<unsigned int, QMap<QString, NMModelComponent*> > mMapTimeLevelComp;
//    QMap<unsigned int, QMap<QString, NMModelComponent*> > mMapPostProcComp;

    QString mDescription;
    NMProcess* mProcess;

    NMModelComponent* mHostComponent;

    NMModelComponent* mUpComponent;
    NMModelComponent* mDownComponent;

    NMModelComponent* mProcessChainStart;
    NMModelComponent* mProcessChainPointer;

    short mTimeLevel;
    unsigned int mNumIterations;
    unsigned int mMaxInternalTimeLevel;


    void initAttributes(void);
//    void registerComponent(NMModelComponent* comp);
//    void unregisterComponent(NMModelComponent* comp);

};

Q_DECLARE_METATYPE(NMModelComponent)
Q_DECLARE_METATYPE(NMModelComponent*)

#endif // NMMODELCOMPONENT_H
