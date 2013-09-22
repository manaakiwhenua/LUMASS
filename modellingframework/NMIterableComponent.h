 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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

#ifndef NMITERABLECOMPONENT_H
#define NMITERABLECOMPONENT_H

#include <qobject.h>
#include <string>
#include <vector>

#include <QList>
#include <QStringList>
#include <QMap>
#include <QMetaType>
#include <QSharedPointer>

#include "nmlog.h"
#include "NMMacros.h"
#include "NMModelComponent.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"


/*! \brief NMIterableComponent
 *
 *	\see NMModelComponent, NMDataComponent, NMProcess, NMModelController
 */

class NMIterableComponent : public NMModelComponent
{
	Q_OBJECT

public:
    virtual ~NMIterableComponent(void);

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

    void reset(void);


    //void setInput(NMItkDataObjectWrapper* inputImg)
    //	{this->setNthInput(0, inputImg);};
    virtual void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);

    virtual NMItkDataObjectWrapper* getOutput(unsigned int idx);

    virtual void changeTimeLevel(int diff);
    virtual void setProcess(NMProcess* proc);
    virtual NMProcess* getProcess(void)
    	{return this->mProcess;};

    virtual void setInputs(const QList<QStringList>& inputs);
    virtual const QList<QStringList> getInputs(void);

    void initialiseComponents(unsigned int timeLevel);
    virtual void update(const QMap<QString, NMModelComponent*>& repo);
    virtual void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    void destroySubComponents(QMap<QString, NMModelComponent*>& repo);

    void mapTimeLevels(unsigned int startLevel,
    		QMap<unsigned int,
    		QMap<QString, NMModelComponent*> >& timeLevelMap);


    // DEPRECATED: part of ancient execution models, no longer valid
    NMModelComponent* getEndOfTimeLevel(void);
    void getEndOfPipelineProcess(NMProcess*& endProc);


    bool isSubComponent(NMModelComponent* comp);

    /* This method identifies executable sub components on the specified
     * time level. Note executable components are components which don't
     * serve as input to any other component on the specified time level.
     */
    const QStringList findExecutableComponents(unsigned int timeLevel,
    		int step);


    /*! creates the execution sequence for the given
     *  time level and the current iteration;
     *  the particular list position of input components is
     *  specified by the iteration counter; the sequence is stored in:
     *  execList
     *
     *  - the outer list (of StringList) contains the order of
     *    execution, beginning with index = 0;
     *  - the inner list denotes either a single model component
     *    (i.e. process component, data component) or
     *    processing pipeline;
     *
     */
    void createExecSequence(QList<QStringList>& execList,
    		unsigned int timeLevel, int step);


protected:
    QMap<unsigned int, QMap<QString, NMModelComponent*> > mMapTimeLevelComp;

    NMProcess* mProcess;
    NMModelComponent* mProcessChainStart;
    NMModelComponent* mProcessChainPointer;
    unsigned int mMaxInternalTimeLevel;

    //QList<QStringList> mExecSequence;


    NMIterableComponent(QObject* parent=0);
    NMIterableComponent(const NMIterableComponent& modelComp){};

    void initAttributes(void);


    /*! returns the index of the inner list within the outer list
     *  within which comp is an executable component, i.e. has the
     *  largest index. returns -1, if comp is not exec comp.
     */
    int isCompExecComp(const QList<QStringList>& execList,
    		const QString& compName);

    std::vector<int> findSourceComp(const QList<QStringList>& execList,
    		const QString& compName);

    std::vector<int> findTargetComp(const QList<QStringList>& execList,
    		const QString& compName, int step);

    int isInExecList(const QList<QStringList>& execList,
    		const QString& compName);


    virtual void iterativeComponentUpdate(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel)=0;//{};
    virtual void componentUpdateLogic(const QMap<QString, NMModelComponent*>& repo,
    		unsigned int minLevel, unsigned int maxLevel,
    		unsigned int step);

private:
    static const std::string ctx;
};

//Q_DECLARE_METATYPE(NMIterableComponent)
//Q_DECLARE_METATYPE(NMIterableComponent*)

#endif // NMITERABLECOMPONENT_H
