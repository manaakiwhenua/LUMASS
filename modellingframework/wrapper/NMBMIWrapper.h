/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
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
 * NMBMIWrapper.h
 *
 *  Created on: 2020-04-30
 *      Author: Alex
 */

#ifndef NMBMIWRAPPER_H_
#define NMBMIWRAPPER_H_

#include <string>
#include <map>
#include <iostream>
#include <memory>

#include <QStringList>
#include <QList>

#include "bmi.hxx"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMProcess.h"
#include "NMItkDataObjectWrapper.h"

#include "nmbmiwrapper_export.h"

template<class TInputImage, class TOutputImage, unsigned int Dimension=2>

class NMBMIWrapper_Internal;

class
NMBMIWrapper : public NMProcess
{
    Q_OBJECT
    Q_PROPERTY(QString YamlConfigFileName READ getYamlConfigFileName WRITE setYamlConfigFileName)

public:

    enum NMBMIComponetType
    {
        NM_BMI_COMPONENT_TYPE_NATIVE,
        NM_BMI_COMPONENT_TYPE_PYTHON
    };

    NMBMIWrapper(QObject* parent=0);
    virtual ~NMBMIWrapper();

    template<class TInputImage, class TOutputImage, unsigned int Dimension>
    friend class NMBMIWrapper_Internal;

    QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    void instantiateObject(void);

    void setNthInput(unsigned int numInput,
              QSharedPointer<NMItkDataObjectWrapper> imgWrapper);

    QString getYamlConfigFileName(){return mYamlConfigFileName;}
    void setYamlConfigFileName(const QString& YamlConfigFileName);

    void bmilog(int ilevel, const char* msg);

signals:
    void needPythonInterpreter(const QString& compName);

protected:
    void linkParameters(unsigned int step,
            const QMap<QString, NMModelComponent*>& repo);

    void parseYamlConfig();
    void initialiseBMILibrary();


    // will have mbIsSink in superclass (i.e. NMProcess)

    bool mbIsStreamable;
    bool mbIsThreadable; // no for python
    NMBMIComponetType mBMIComponentType;

    std::shared_ptr<bmi::Bmi> mPtrBMILib;
    QString mComponentName;
    QString mComponentPath;
    QString mBMIClassName;

    QString mYamlConfigFileName;

};


#endif /* NMBMIWrapper_H_ */
