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

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif
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

#include "NMModelObject.h"
#include "NMModelComponent.h"
#include "NMItkDataObjectWrapper.h"
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "otbImageIOBase.h"
#include "itkCommand.h"
#include "itkEventObject.h"
#include "otbAttributeTable.h"

#include "nmmodframecore_export.h"

class NMProcessFactory;

/*!
 * \brief The NMProcess class - the working horse of modelling framework
 *
 * NMProcess subclasses
 * - encapsulate/wraps either an itk::ProcessObject doing some
 *   number crunching on otb::Image or otb::AttributeTable data,
 * - provide their own little processing capabilities
 * - or wrap some other processing capabilities coming from elsewhere
 *
 * Since NMProcess is an abstract class, the following pure virtual functions need
 * to be implemented in order to create an instance of a processing object
 *
 * setNthInput
 * getOutput
 * instantiateObject
 *
 * A number of other classes can be overridden, to account for process-specific
 * behaviour, especially interms of linking inputs and parameters
 *
 * public methods
 *
 * update
 *      This methods needs to be overridden to implement any non itk::ProcessObject-
 *      based processing capabilities
 *
 * linkInPipeline
 *      Standard implementation caters for linking requirements of itk::ProcessObject-
 *      based classes; however it also encapsulates a carefully crafted mechanism of
 *      how to behave within the different time levels of the modelling framework;
 *      I recommend looking at the next two protected members for overriding any
 *      of the linking logic
 *
 * protected methods
 *
 * linkInputs
 *      links all user defined inputs to this process component
 *
 * linkParameters
 *      sets all necessary parameters of the itk::ProcessObject underneath
 *
 * abortExecution
 *       deals with user requested abortion of the processing
 *
 *
 */


class NMMODFRAMECORE_EXPORT NMProcess : public QObject, public NMModelObject
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
    typedef itk::MemberCommand<NMProcess> ObserverType;
    typedef ObserverType::Pointer ObserverTypePointer;

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
    void setReleaseData(bool release){mbReleaseData = release;}
    bool getReleaseData(void){return mbReleaseData;}


    /*! Convenience method to set the input/output/(in- & output)
     *  image type specification from a NMItkDataObjectWrapper */
    void setInputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);
    void setOutputImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);
    void setImgTypeSpec(QSharedPointer<NMItkDataObjectWrapper> dw);

    //void setLogger(NMLogger* logger){mLogger = logger;}


	QDateTime getModifiedTime(void)
		{return mMTime;}

    ObserverTypePointer getObserver(void){return mObserver;}

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

    /**
     * @brief getParameter - Fetches the property value from this (NMProcess) object
     *        for the current IterationStep.
     *        Note: If the property type is QString, QStringList, or QList<QStringList>,
     *        the parameter(s) are parsed by \ref NMProcess::processStringParameter
     *        before it (they) is (are) passed on to the underlying itk::Process object
     *        or processed in by \ref NMProcess::update()
     * @param property - the name of the property
     * @return QVariant wrapped property value
     */
    QVariant getParameter(const QString& property);

    QStringList getRunTimeParaProvN(void){return mRuntimeParaProv;}
    void addRunTimeParaProvN(const QString& provNAttr){mRuntimeParaProv << provNAttr;}

    int getAuxDataIdx(void)
        {return this->mAuxDataIdx;}


public slots:
	void removeInputComponent(const QString& input);
	virtual void abortExecution(void);
	void setInputComponents(QList<QStringList> inputComponents);
	QList<QStringList> getInputComponents(void)
			{return this->mInputComponents;}

    virtual void setRAT(unsigned idx,
                        QSharedPointer<NMItkDataObjectWrapper> imgWrapper) {}
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
    bool mIsSink;

    otb::AttributeTable::Pointer mAuxTab;
    int mAuxDataIdx;

    ObserverType::Pointer mObserver;

    QStringList mRuntimeParaProv;


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
    bool mbReleaseData;

};

Q_DECLARE_METATYPE(QList<QStringList>)
Q_DECLARE_METATYPE(QList<QList<QStringList> >)
Q_DECLARE_METATYPE(NMProcess::AdvanceParameter)


#endif /* NMProcess_H_ */
