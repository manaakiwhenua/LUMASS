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
/*
 * NMProcess.h
 *
 *  Created on: 16/04/2012
 *      Author: alex
 */

#ifndef NMProcess_H_
#define NMProcess_H_
#define ctxNMProcess "NMProcess"

#include <exception>

#include "nmlog.h"
#include "NMMacros.h"

#include <qobject.h>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QSharedPointer>
#include <QMetaObject>
#include <QMetaType>
#include <QDateTime>
#include "NMModelComponent.h"
#include "NMItkDataObjectWrapper.h"
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImageIOBase.h"
#include "itkCommand.h"
#include "itkEventObject.h"

#include "nmmodframe_export.h"

class NMProcessFactory;

class NMMODFRAME_EXPORT NMProcess : public QObject
{
    friend class NMProcessFactory;

    Q_OBJECT
	Q_ENUMS(AdvanceParameter)
	Q_PROPERTY(QList<QStringList> InputComponents READ getInputComponents WRITE setInputComponents)
	Q_PROPERTY(NMProcess::AdvanceParameter ParameterHandling READ getParameterHandling WRITE setParameterHandling NOTIFY NMProcessChanged)
//	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMComponentType READ getNMComponentType WRITE setNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMInputComponentType READ getInputNMComponentType WRITE setInputNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMOutputComponentType READ getOutputNMComponentType WRITE setOutputNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int InputNumDimensions READ getInputNumDimensions WRITE setInputNumDimensions NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int OutputNumDimensions READ getOutputNumDimensions WRITE setOutputNumDimensions NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int InputNumBands READ getInputNumBands WRITE setInputNumBands NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int OutputNumBands READ getOutputNumBands WRITE setOutputNumBands NOTIFY NMProcessChanged)

public:
	/*! Defines the supported ways of (input) parameter supply to a process component upon
	 *  repetitive execution:\newline
	 *  NM_USE_UP
	 *  takes a new parameter from the list each time the process
	 *  is executed; the process re-uses the last parameter if the process is
	 *  executed again after all parameters have been used
	 *	NM_CYCLE
	 *	takes a new parameter from the list each time the process
	 *	is executed; after the last parameter was used, it starts at the beginning
	 *	of the list again
	 *	NM_SYNC_WITH_HOST:
	 *	the parameter to be taken from the list is synchronised with
	 *	with the iteration steps of the host components, i.e. the iteration steps
	 *	determines the index of the parameter list from which to take the next parameter
	 *
	 *	The default setting is NM_SYNC_WITH_HOST
	 */
	enum AdvanceParameter {NM_USE_UP=0, NM_CYCLE, NM_SYNC_WITH_HOST};

	//NMPropertyGetSet(InputComponents      , QList<QStringList>                     )
	//	NMPropertyGetSet(NMComponentType      , NMItkDataObjectWrapper::NMComponentType )
    NMPropertyGetSet(InputNumDimensions   , unsigned int                           )
    NMPropertyGetSet(OutputNumDimensions  , unsigned int                           )
    NMPropertyGetSet(InputNumBands        , unsigned int                           )
    NMPropertyGetSet(OutputNumBands	      , unsigned int                           )
    NMPropertyGetSet(ParameterHandling	  , NMProcess::AdvanceParameter            )

	NMItkDataObjectWrapper::NMComponentType getInputNMComponentType();
    NMItkDataObjectWrapper::NMComponentType getOutputNMComponentType();
	void setInputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype);
    void setOutputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype);


public:
	virtual ~NMProcess();
    otb::ImageIOBase::IOComponentType getOutputComponentType(void);
    otb::ImageIOBase::IOComponentType getInputComponentType(void);

	virtual void setNthInput(unsigned int numInput,
			QSharedPointer<NMItkDataObjectWrapper> img) = 0;
    void setInput (QSharedPointer<NMItkDataObjectWrapper> img)
    		{this->setNthInput(0, img);}
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx) = 0;
    virtual void update(void);
    void reset(void);

	virtual void instantiateObject(void) = 0;
	bool isInitialised(void)
        {return this->mbIsInitialised;}

    bool isSink(void) {return mIsSink;}

    /*! Convenience method to set the input/output/(in- & output)
     *  image type specification from a NMItkDataObjectWrapper */
    void setInputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);
    void setOutputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);
    void setImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);


	QDateTime getModifiedTime(void)
		{return mMTime;}

	itk::ProcessObject* getInternalProc(void)
		{return this->mOtbProcess;}

	/*! \brief Supplies properties' values to instantiated internal process object.
	 *
	 *  The default implementation calls linkInputs() and linkParameters(). Processes
	 *  which use different input mechanisms than the default process should overwrite
	 *  linkInputs() (e.g. NMImageReader, which needs to be re-instantiated with each
	 *  new filename parameter).
     *  The 'step' and 'repo' parameters are deprecated and no longer used in the default
     *  implementation: For accessing other model components use the singelton NMModelController
     *  object and for the step parameter use either the host's or host's host's
     *  mIterationStep parameter (e.g. step = host->getIterationStep()).
	 */
	virtual void linkInPipeline(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

    /*! \brief Translates the iteration index of the host component
     *         into an internal parameter position index subject
     *         to the user-specified parameter handling policy
     *         (i.e. use_up, cycle, sync).
     */
    unsigned short mapHostIndexToPolicyIndex(unsigned short step,
            unsigned short size);


    QVariant getParameter(const QString& property);


public slots:
	void removeInputComponent(const QString& input);
	virtual void abortExecution(void);
	void setInputComponents(QList<QStringList> inputComponents);
	QList<QStringList> getInputComponents(void)
			{return this->mInputComponents;}

signals:
		void NMProcessChanged();
        void nmChanged();
		void signalInputChanged(QList<QStringList> inputs);
		void signalProgress(float);
		void signalExecutionStarted(const QString &);
		void signalExecutionStopped(const QString &);


protected:
	NMProcess(QObject *parent=0);

	bool mbIsInitialised;
	bool mbAbortExecution;
	unsigned int mParamPos;
	float mProgress;
	QDateTime mMTime;
    QList<QStringList> mInputComponents;
	itk::ProcessObject::Pointer mOtbProcess;
	itk::DataObject::Pointer mOutputImg;
	itk::DataObject::Pointer mInputImg;
	otb::ImageIOBase::IOComponentType mInputComponentType;
	otb::ImageIOBase::IOComponentType mOutputComponentType;
	NMItkDataObjectWrapper::NMComponentType mNMComponentType;
	unsigned int mInputNumBands;
	unsigned int mOutputNumBands;
	unsigned int mInputNumDimensions;
	unsigned int mOutputNumDimensions;
	NMProcess::AdvanceParameter mParameterHandling;

	bool mbLinked;

	typedef itk::MemberCommand<NMProcess> ObserverType;
	ObserverType::Pointer mObserver;


	/*! \brief Call-back method for itk::Process-based NMProcess classes to
	 *         signal (emit) the process' state to listeners
	 *         (e.g. NMProcessComponentItem).
	 */
	virtual void UpdateProgressInfo(itk::Object*, const itk::EventObject&);

	/*! \brief Stores the parameter index. Needs to be set to 0 in constructor */
	//	unsigned int mParameterPos;

	/*! \brief Links input images with internal process object.
	 *
	 * 	NMProcess provides a default implementation, which suitable for most
	 * 	process objects.
	 */
	virtual void linkInputs(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

	/*! \brief Prepares the process for the next execution step.
	 *
	 *	Subclasses must implement this method to assign suitable parameter values for each
	 *	processing step (i.e. iteration of host component) to the underlying process object.
	 */
	virtual void linkParameters(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

private:
    unsigned int mStepIndex;
    bool mIsSink;

};

Q_DECLARE_METATYPE(QList<QStringList>)
Q_DECLARE_METATYPE(QList<QList<QStringList> >)
Q_DECLARE_METATYPE(NMProcess::AdvanceParameter)


#endif /* NMProcess_H_ */
