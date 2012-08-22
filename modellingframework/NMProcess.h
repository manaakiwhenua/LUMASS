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
 * NMProcess.h
 *
 *  Created on: 16/04/2012
 *      Author: alex
 */

#ifndef NMProcess_H_
#define NMProcess_H_
#define ctxNMProcess "NMProcess"

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

#include "NMItkDataObjectWrapper.h"
#include "NMModelComponent.h"
#include "itkProcessObject.h"
#include "itkDataObject.h"
#include "itkImageIOBase.h"

class NMModelComponent;

class NMProcess : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QList<QStringList> InputComponents READ getInputComponents WRITE setInputComponents NOTIFY NMProcessChanged)

//	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMComponentType READ getNMComponentType WRITE setNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMInputComponentType READ getInputNMComponentType WRITE setInputNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(NMItkDataObjectWrapper::NMComponentType NMOutputComponentType READ getOutputNMComponentType WRITE setOutputNMComponentType NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int InputNumDimensions READ getInputNumDimensions WRITE setInputNumDimensions NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int OutputNumDimensions READ getOutputNumDimensions WRITE setOutputNumDimensions NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int InputNumBands READ getInputNumBands WRITE setInputNumBands NOTIFY NMProcessChanged)
	Q_PROPERTY(unsigned int OutputNumBands READ getOutputNumBands WRITE setOutputNumBands NOTIFY NMProcessChanged)

public:

	NMPropertyGetSet(InputComponents      , QList<QStringList>                     )
//	NMPropertyGetSet(NMComponentType      , NMItkDataObjectWrapper::NMComponentType )
	NMPropertyGetSet(InputNumDimensions   , unsigned int                           )
	NMPropertyGetSet(OutputNumDimensions  , unsigned int                           )
	NMPropertyGetSet(InputNumBands        , unsigned int                           )
	NMPropertyGetSet(OutputNumBands	      , unsigned int                           )

	NMItkDataObjectWrapper::NMComponentType getInputNMComponentType();
    NMItkDataObjectWrapper::NMComponentType getOutputNMComponentType();
	void setInputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype);
    void setOutputNMComponentType(NMItkDataObjectWrapper::NMComponentType nmtype);


signals:
	void NMProcessChanged();
//	void InputComponentsChanged      (QList<QStringList>                     );
//	void InputNMComponentTypeChanged (NMItkDataObjectWrapper::NMComponentType);
//	void OutputNMComponentTypeChanged (NMItkDataObjectWrapper::NMComponentType);
////	void NMComponentTypeChanged      (NMItkDataObjectWrapper::NMComponentType);
//	void InputNumDimensionsChanged   (unsigned int                           );
//	void OutputNumDimensionsChanged  (unsigned int                           );
//	void InputNumBandsChanged        (unsigned int                           );
//	void OutputNumBandsChanged       (unsigned int                           );

public:
	virtual ~NMProcess();
    itk::ImageIOBase::IOComponentType getOutputComponentType(void);
    itk::ImageIOBase::IOComponentType getInputComponentType(void);

	virtual void setNthInput(unsigned int numInput,
			NMItkDataObjectWrapper* img) = 0;
    void setInput (NMItkDataObjectWrapper* img)
    		{this->setNthInput(0, img);}
    virtual NMItkDataObjectWrapper* getOutput(void) = 0;
    void update(void)
    	{this->mOtbProcess->Update();}

	virtual void instantiateObject(void) = 0;
	bool isInitialised(void)
		{return this->mbIsInitialised;};

	itk::ProcessObject* getInternalProc(void)
		{return this->mOtbProcess;}

	/*! \brief Supplies properties' values to instantiated internal process object.
	 *
	 *  The default implementation calls linkInputs() and linkParameters(). Processes
	 *  which use different input mechanisms than the default process should overwrite
	 *  linkInputs() (e.g. NMImageReader, which needs to be re-instantiated with each
	 *  new filename parameter).
	 */
	virtual void linkInPipeline(unsigned int step, const QMap<QString, NMModelComponent*>& repo);

public slots:
	void removeInputComponent(const QString& input);

protected:
	NMProcess(QObject *parent=0);

	bool mbIsInitialised;
    QList<QStringList> mInputComponents;
	itk::ProcessObject::Pointer mOtbProcess;
	itk::DataObject::Pointer mOutputImg;
	itk::DataObject::Pointer mInputImg;
	itk::ImageIOBase::IOComponentType mInputComponentType;
	itk::ImageIOBase::IOComponentType mOutputComponentType;
	NMItkDataObjectWrapper::NMComponentType mNMComponentType;
	unsigned int mInputNumBands;
	unsigned int mOutputNumBands;
	unsigned int mInputNumDimensions;
	unsigned int mOutputNumDimensions;

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

};

Q_DECLARE_METATYPE(QList<QStringList>)
Q_DECLARE_METATYPE(QList<QList<QStringList> >)


#endif /* NMProcess_H_ */
