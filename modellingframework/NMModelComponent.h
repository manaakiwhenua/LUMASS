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
/**
  * class NMModelComponent
  * 
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
	//NMPropertyGetSet(TimeLevel, short)
    NMPropertyGetSet(Description, QString)
    NMPropertyGetSet(NumIterations, unsigned int)

signals:
	void NMModelComponentChanged();
//	void HostComponentChanged(NMModelComponent*);
//	void TimeLevelChanged(short);
//	void DescriptionChanged(QString);
//	void NumIterationsChanged(unsigned int);

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

    /**
     * Get the value of mFilterChainStart
     * \return the value of mFilterChainStart
     */
    NMModelComponent* getInternalStartComponent(void);
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

//    void increaseTimeLevel(void)
//    	{++this->mTimeLevel;}
//
//    void decreaseTimeLevel(void)
//    	{this->mTimeLevel = (this->mTimeLevel-1) < 0 ? 0 : --this->mTimeLevel;}

    /**
     * - adds a filter at  the end of this component's pipeline
     * \param  filter
     */
    void addModelComponent(NMModelComponent* comp);


    /**
     * \param  filter
     * \param  previousFilter
     */
    void insertModelComponent(NMModelComponent* proc, QString previousComponent );


    /**
     * \return NMFilter*
     * \param  filterName
     */
    NMModelComponent* findModelComponent(QString compName );


    /**
     * \param  filterName
     */
    NMModelComponent* removeModelComponent(QString compName);

    int countComponents(void);


    /**
     * \param  inputComponent
     */
    void setInput(NMItkDataObjectWrapper* inputImg)
    	{this->setNthInput(0, inputImg);};
    virtual void setNthInput(unsigned int idx, NMItkDataObjectWrapper* inputImg);

    /**
     * \return
     */
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
