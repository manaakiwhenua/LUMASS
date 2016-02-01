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

#ifndef NMMODELCOMPONENT_H
#define NMMODELCOMPONENT_H

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
#include "NMItkDataObjectWrapper.h"
#include "nmmodframe_export.h"

class NMIterableComponent;

/*! \brief DEPRECATED: NMModelComponent is one of the core building blocks of the LUMASS modelling
 *         framework. It represents either a single process (i.e. algorithm,
 *         called 'process component') or a chain of processes
 *         (i.e. a processing pipeline, called 'aggregate component').
 *
 *		   DEPRECATED:
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

class NMMODFRAME_EXPORT NMModelComponent : public QObject
{
	Q_OBJECT
    Q_PROPERTY(QString UserID READ getUserID WRITE setUserID)
	Q_PROPERTY(QString Description READ getDescription WRITE setDescription)
	Q_PROPERTY(short TimeLevel READ getTimeLevel WRITE setTimeLevel NOTIFY NMModelComponentChanged)
	Q_PROPERTY(QList<QStringList> Inputs READ getInputs WRITE setInputs NOTIFY NMModelComponentChanged)

public:
	NMPropertyGetSet(HostComponent, NMIterableComponent*);
    //NMPropertyGetSet(Description, QString);

signals:
	void NMModelComponentChanged();
    void nmChanged();
    void TimeLevelChanged(short level);
	void ComponentDescriptionChanged(const QString& descr);
    void ComponentUserIDChanged(const QString& userID);

public:
    virtual ~NMModelComponent(void);

	// common public interface with common behaviour for
    // all model component classes

    /*! Follows and lists the process chain upstream
     *  until it finds a component, which doesn't
     *  host an itk::Process object and hence cannot
     *  be 'piped' but has to be executed explicitly.  */
    void getUpstreamPipe(QList<QStringList>& hydra,
    		QStringList& upstreamPipe, int step);

    short getTimeLevel(void)
    	{return this->mTimeLevel;}
    void setTimeLevel(short level);
    virtual void changeTimeLevel(int diff);

    void setUserID(const QString& userID);
    QString getUserID()
        {return this->mUserID;}

    /*!
     * \brief getModelParameter fetches model component property values;
     *        in contrast to NMProcess::getParameter() this function also looks
     *        for paramSpec in external components; the search order starts
     *        at the component itself, and then successively comprises its siblings
     *        and progresses upward the hierarchy until it has found the first
     *        component matching the specified component name (i.e. UserID)
     *        NOTE: NMParameterComponent
     * \param paramSpec
     * \return
     */
    virtual QVariant getModelParameter(const QString& paramSpec);

    void setDescription(QString descr);
    QString getDescription()
    	{return this->mDescription;}

    QStringList getPropertyList(void);

    /*! Allows for recursive identification of
     *  components, which belong to the same
     *  host component, i.e. which are part of the
     *  very same doubly linked list as the
     *  component at hand */
    NMModelComponent* getDownstreamModelComponent(void)
    	{return this->mDownComponent;}
    NMModelComponent* getUpstreamModelComponent(void)
    	{return this->mUpComponent;}

    void setDownstreamModelComponent(NMModelComponent* comp)
    	{this->mDownComponent = comp;}
    void setUpstreamModelComponent(NMModelComponent* comp)
    	{this->mUpComponent = comp;}


    void setInput(QSharedPointer<NMItkDataObjectWrapper> inputImg)
        {this->setNthInput(0, inputImg);}

    // virtual functions, defining sub-class specific behaviour
    virtual void setInputs(const QList<QStringList>& inputs)
        {mInputs = inputs;}
    virtual const QList<QStringList> getInputs(void)
        {return mInputs;}
    virtual void setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg)=0;//{};
    virtual void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo)=0;//{};
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx)=0;//{return 0;};
    virtual void update(const QMap<QString, NMModelComponent*>& repo)=0;//{};
    virtual void reset(void){}

protected:
    NMModelComponent(QObject* parent=0);
    NMModelComponent(const NMModelComponent& modelComp){};

    QString mDescription;
    QString mUserID;

    NMIterableComponent* mHostComponent;

    NMModelComponent* mUpComponent;
    NMModelComponent* mDownComponent;

    short mTimeLevel;

    QList<QStringList> mInputs;

    virtual void initAttributes(void);

private:


    static const std::string ctx;
};

//Q_DECLARE_METATYPE(NMModelComponent)
//Q_DECLARE_METATYPE(NMModelComponent*)

#endif // NMMODELCOMPONENT_H
