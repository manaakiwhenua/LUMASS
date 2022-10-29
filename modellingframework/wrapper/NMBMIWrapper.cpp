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
 *  NMBMIWrapper.cpp
 *
 *  Created on: 2020-01-13
 *      Author: Alex
 */

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif


#if defined _WIN32
    #define WINCALL __stdcall
    #include <windows.h>
    #include <strsafe.h>
    #include <libloaderapi.h>

    #pragma push_macro("GetCurrentTime")
        #undef GetCurrentTime

        #ifdef LUMASS_PYTHON
        #include "pythonbmi.h"
        #endif

        #include "NMBMIWrapper.h"
    #pragma pop_macro("GetCurrentTime")
#else
    #define WINCALL
    #include <dlfcn.h>

    #ifdef LUMASS_PYTHON
    #include "pythonbmi.h"
    #endif

    #include "NMBMIWrapper.h"
#endif

#include <string>
#include <stdlib.h>
#include <yaml-cpp/yaml.h>

#include <QFileInfo>
#include <QDir>

#ifdef LUMASS_PYTHON
#include "Python_wrapper.h"
#endif

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"
#include "NMModelComponent.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbBMIModelFilter.h"


#include "nmbmiwrapper_export.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMBMIWrapper_Internal

{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::BMIModelFilter<InImgType, OutImgType>  FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
                               unsigned int numBands)
    {
        FilterTypePointer f = FilterType::New();
        otbFilter = f;
    }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj, const QString& name)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        if (!name.isEmpty())
        {
            filter->SetInput(name.toStdString(), dataObj);
        }
        filter->SetNthInput(idx, dataObj);
    }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
                                      unsigned int numBands, unsigned int idx)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
    }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
                                      unsigned int numBands, const QString& name)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        return filter->GetOutputByName(name.toStdString());
    }


    /*$<InternalRATGetSupport>$*/

    /*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
                                       unsigned int numBands, NMProcess* proc,
                                       unsigned int step, const QMap<QString, NMModelComponent*>& repo)
    {
        NMDebugCtx("NMBMIWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMBMIWrapper* p =
                dynamic_cast<NMBMIWrapper*>(proc);
        // make sure we've got a valid filter object
        if (f == 0)
        {
            NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
            e.setDescription("We're trying to link, but the filter doesn't seem to be initialised properly!");
            throw e;
            return;
        }

        /* do something reasonable here */
        bool bok;
        int givenStep = step;

        QVariant curYamlConfigFileNameVar = p->getParameter("YamlConfigFileName");
        std::string curYamlConfigFileName;
        if (curYamlConfigFileNameVar.isValid())
        {
            curYamlConfigFileName = curYamlConfigFileNameVar.toString().toStdString();
            f->SetYamlConfigFileName(curYamlConfigFileName);
            p->mParsedYamlConfigFileName = curYamlConfigFileNameVar.toString();

            QString YamlConfigFileNameProvN = QString("nm:YamlConfigFileName=\"%1\"").arg(curYamlConfigFileName.c_str());
            p->addRunTimeParaProvN(YamlConfigFileNameProvN);
        }

        step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());
        std::vector<std::string> userIDs;
        QStringList currentInputs;
        QStringList inputNamesProvVal;
        if (step < p->mInputComponents.size())
        {
            currentInputs = p->mInputComponents.at(step);
            int cnt=0;
            foreach (const QString& input, currentInputs)
            {
                std::stringstream uid;
                uid << "L" << cnt;

                // double check whether name is provided as part inputspec
                std::string inputname;
                QStringList nameparts = input.split(":", QString::SkipEmptyParts);
                if (nameparts.size() == 2)
                {
                    QVariant npvar = QVariant::fromValue(nameparts.at(1));

                    bool bOK = true;
                    int npnum = npvar.toInt(&bOK);

                    if (bOK == false)
                    {
                        inputname = npvar.toString().toStdString();
                    }
                }

                // in case the name is not provided with the inputspec, use the UserID instead
                QString inputCompName = p->getModelController()->getComponentNameFromInputSpec(input);
                NMModelComponent* comp = p->getModelController()->getComponent(inputCompName);

                if (!inputname.empty())
                {
                    userIDs.push_back(inputname);
                }
                else if (comp != 0)
                {
                    if (comp->getUserID().isEmpty())
                    {
                        userIDs.push_back(uid.str());
                    }
                    else
                    {
                        userIDs.push_back(comp->getUserID().toStdString());
                    }
                }
                else
                {
                    userIDs.push_back(uid.str());
                }
                inputNamesProvVal << QString(userIDs.back().c_str());
                ++cnt;
            }
        }
        f->SetInputNames(userIDs);
        QString inputNamesProvN = QString("nm:InputNames=\"%1\"").arg(inputNamesProvVal.join(' '));
        p->addRunTimeParaProvN(inputNamesProvN);
        //p->initialiseBMILibrary();

        //QVariant curOutputNamesVar = p->getParameter(QStringLiteral("OutputNames"));
        //QStringList curOutputNames;
        //if (curOutputNamesVar.isValid())
        //{
        //    curOutputNames = curOutputNamesVar.toString();
        //    std::vector<std::string> voutnames;
        //    foreach (const QString& oname, curOutputNames)
        //    {
        //        voutnames.push_back(oname.toStdString());
        //    }
        //    f->SetOutputNames(voutnames);
        //
        //    QString outputNamesProvN = QString("nm:OutputNames=\"%1\"").arg(curOutputNames.join(''));
        //    p->addRunTimeParaProvN(outputNamesProvN);
        //
        //}

        // pass on the wrapper object name, so the filter can fetch
        // associated python modules from the global module map
        f->SetWrapperName(p->objectName().toStdString());

        // need to do this after initialisation, i.e.
        // after the yaml config for the BMI model has
        // been parsed
        f->SetIsStreamable(p->mbIsStreamable);
        f->SetIsThreadable(p->mbIsThreadable);

        //if (p->mPtrBMILib.get() == nullptr)
        //{
        //    NMErr("NMBMIWrapper", << "BMI library initialisation failed!");
        //    NMMfwException be(NMMfwException::NMProcess_UninitialisedProcessObject);
        //    be.setDescription("BMI library initialisation failed!");
        //    throw be;
        //}
        //f->SetBMIModule(p->mPtrBMILib);

        p->initialiseBMILibrary();

        // pass on the wrapper object name, so the filter can fetch
        // associated python modules from the global module map
        f->SetWrapperName(p->objectName().toStdString());

        // need to do this after initialisation, i.e.
        // after the yaml config for the BMI model has
        // been parsed
        f->SetIsStreamable(p->mbIsStreamable);
        f->SetIsThreadable(p->mbIsThreadable);

        if (p->mPtrBMILib.get() == nullptr)
        {
            NMErr("NMBMIWrapper", << "BMI library initialisation failed!");
            NMMfwException be(NMMfwException::NMProcess_UninitialisedProcessObject);
            be.setDescription("BMI library initialisation failed!");
            throw be;
        }
        f->SetBMIModule(p->mPtrBMILib);

        std::vector<std::string> voutnames = p->mPtrBMILib->GetOutputVarNames();
        QStringList curOutNames;
        for (int n=0; n < voutnames.size(); ++n)
        {
            curOutNames << voutnames[n].c_str();
        }
        //f->SetOutputNames(voutnames);

        QString outputNamesProvN = QString("nm:OutputNames=\"%1\"").arg(curOutNames.join(' '));
        p->addRunTimeParaProvN(outputNamesProvN);


        NMDebugCtx("NMBMIWrapper_Internal", << "done!");
    }
};

InstantiateObjectWrap( NMBMIWrapper, NMBMIWrapper_Internal )
SetNthInputWrap( NMBMIWrapper, NMBMIWrapper_Internal )
//stSetOutputNamesWrap( NMBMIWrapper, NMBMIWrapper_Internal )
GetOutputWrap( NMBMIWrapper, NMBMIWrapper_Internal )
GetOutputWrapByName( NMBMIWrapper, NMBMIWrapper_Internal )
LinkInternalParametersWrap( NMBMIWrapper, NMBMIWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/


NMBMIWrapper
::NMBMIWrapper(QObject* parent)
    : mPtrBMILib(nullptr)
{
    this->setParent(parent);
    this->setObjectName("NMBMIWrapper");
    this->mInputNumBands = 1;
    this->mParameterHandling = NMProcess::NM_USE_UP;

    mUserProperties.clear();
    mUserProperties.insert(QStringLiteral("NMInputComponentType"), QStringLiteral("InputPixelType"));
    mUserProperties.insert(QStringLiteral("NMOutputComponentType"), QStringLiteral("OutputPixelType"));
    mUserProperties.insert(QStringLiteral("OutputNumDimensions"), QStringLiteral("NumDimensions"));
    //mUserProperties.insert(QStringLiteral("OutputNames"), QStringLiteral("OutputNames"));
    mUserProperties.insert(QStringLiteral("YamlConfigFileName"), QStringLiteral("YamlConfigFileName"));
}

void NMBMIWrapper::bmilog(int ilevel, const char* msg)
{
    if (this->mLogger == nullptr)
    {
        QString name = this->parent() != nullptr ?
                       this->parent()->objectName() : this->objectName();
        std::cerr << name.toStdString() << " - BMI ERROR: " << msg << std::endl;
        return;
    }

    const int alevel = ilevel;
    switch (alevel)
    {
    case 0:
    case 1: NMLogDebug(<< msg); break;
    case 2: NMLogInfo(<< msg ); break;
    case 3: NMLogWarn(<< msg ); break;
    case 4:
    case 5: NMLogError(<< msg); break;
    default: ;
    }
}

void
NMBMIWrapper::initialiseBMILibrary()
{
    // parse YAML for wrapper relevant settings
    this->parseYamlConfig();

#ifdef LUMASS_PYTHON
    if (this->mBMIComponentType == NM_BMI_COMPONENT_TYPE_PYTHON)
    {
        if (this->parent() != nullptr)
        {
            this->getModelController()->registerPythonRequest(this->parent()->objectName());
        }
    }

    if (mBMIComponentType == NM_BMI_COMPONENT_TYPE_PYTHON)
    {
        std::string compName = this->parent() != nullptr ? this->parent()->objectName().toStdString()
                                                     : this->objectName().toStdString();
        std::vector<std::string> pypath;
        foreach(const QString & pp, mComponentPathList)
        {
            pypath.push_back(pp.toStdString());
        }

        // strip the '.py' suffix from the component name (if any was provided)
        QFileInfo modinfo(mComponentName);
        std::string modulename = modinfo.baseName().toStdString();
        mPtrBMILib = std::make_shared<bmi::PythonBMI>(bmi::PythonBMI(modulename,
                                   pypath,
                                   mBMIClassName.toStdString(),
                                   compName));

        if (this->mLogger != nullptr)
        {
            bmi::PythonBMI* pybmi = static_cast<bmi::PythonBMI*>(mPtrBMILib.get());
            if (pybmi != nullptr)
            {
                pybmi->setWrapLog(this, &NMBMIWrapper::bmilog);
            }
        }

        try
        {
            mPtrBMILib->Initialize(mYamlConfigFileName.toStdString());
            if (!lumass_python::ctrlPyObjects[compName].is_none())
            {
                lumass_python::ctrlPyObjectSinkMap[compName] = mIsSink;
                NMLogInfo(<< "Successfully initialised '" << mPtrBMILib->GetComponentName() << "'!");
            }
            else
            {
                NMLogError(<< "Failed initialising '" << mComponentName.toStdString() << "'!");
            }
        }
        catch(pybind11::cast_error& ce)
        {
            NMLogError(<< "Failed initialisation of '" << mComponentName.toStdString() << "': " << ce.what());
        }
        catch(std::exception& e)
        {
            NMLogError(<< "Failed initialisation of '" << mComponentName.toStdString() << "': " << e.what());
        }
    }
    else
#endif
    {
        if (mBMIComponentType == NM_BMI_COMPONENT_TYPE_NATIVE)
        {
            // init the native BMI library here
            NMLogInfo(<< "Oopsi - can't handle native libs yet :-(")
        }
    }
}

NMBMIWrapper
::~NMBMIWrapper()
{
}

void
NMBMIWrapper::parseYamlConfig()
{
    QFileInfo fifo(this->mParsedYamlConfigFileName);
    if (!fifo.isReadable())
    {
        NMLogError(<< "Failed reading BMI model configuration from '"
                   << mYamlConfigFileName.toStdString() << "'!");
        return;
    }


    try
    {
        YAML::Node configFile = YAML::LoadFile(mParsedYamlConfigFileName.toStdString());
        YAML::Node config;

        if (configFile.IsMap() && configFile["LumassBMIConfig"])
        {
            config = configFile["LumassBMIConfig"];
        }
        else
        {
            std::stringstream msg;
            msg << "LumassBMIConfig not found in yaml!";
            NMLogError(<< msg.str().c_str());
            return;
        }

        std::string bmitype;
        if (config.IsMap())
        {
              if (config["type"])
              {
                  bmitype = config["type"].as<std::string>();
              }
              NMLogDebug(<< "type = " << bmitype);

              this->mBMIClassName.clear();
              if (config["class_name"])
              {
                  mBMIClassName = config["class_name"].as<std::string>().c_str();
              }
              NMLogDebug(<< "class_name = " << mBMIClassName.toStdString());

              this->mComponentName.clear();
              if (config["library_name"])
              {
                  mComponentName = config["library_name"].as <std::string>().c_str();
              }
              NMLogDebug(<< "library_name = " << mComponentName.toStdString());

              this->mComponentPath.clear();
              if (config["path"])
              {
                  mComponentPath = config["path"].as<std::string>().c_str();
              }
              NMLogDebug(<< "path = " << mComponentPath.toStdString());

              mComponentPathList.clear();

              QStringList tmpList = mComponentPath.split(QDir::listSeparator(), QString::SkipEmptyParts);

              foreach(const QString & pathItem, tmpList)
              {
                  QDir aDir(pathItem);
                  if (aDir.isReadable())
                  {
                      mComponentPathList << pathItem;
                  }
                  else
                  {
                      NMLogError(<< "path: '" << pathItem.toStdString() << "' is not a readable path!");
                      return;
                  }
              }

              if (config["issink"])
              {
                  mIsSink = config["issink"].as<bool>();
              }
              NMLogDebug(<< "issink = " << mIsSink);

              if (config["streamable"])
              {
                  mbIsStreamable = config["streamable"].as<bool>();
              }
              NMLogDebug(<< "streamable = " << mbIsStreamable);

              if (config["threadable"])
              {
                  mbIsThreadable = config["threadable"].as<bool>();
              }
              NMLogDebug(<< "threadable = " << mbIsThreadable);
        }

        if (bmitype.compare("bmi-python") == 0)
        {
            this->mBMIComponentType = NM_BMI_COMPONENT_TYPE_PYTHON;
        }
        else
        {
            this->mBMIComponentType = NM_BMI_COMPONENT_TYPE_NATIVE;
        }
    }
    catch(YAML::ParserException& pe)
    {
        NMLogError(<< pe.what());
    }
    catch(YAML::RepresentationException& re)
    {
        NMLogError(<< re.what());
    }
}

void
NMBMIWrapper::setYamlConfigFileName(const QString &YamlConfigFileName)
{
    this->mYamlConfigFileName = YamlConfigFileName;

    emit nmChanged();
}
