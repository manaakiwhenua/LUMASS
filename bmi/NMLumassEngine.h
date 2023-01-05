/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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

#ifndef NMLUMASSENGINE_H_
#define NMLUMASSENGINE_H_

#include <string>
#include <QString>
#include <QFile>
#include <yaml-cpp/yaml.h>

#include "NMLogger.h"
//#include "lumassbmi_export.h"


/*
 *  The LUMASS Engine 'NMLumassEngine' singleton class
 *  represents the one and only LUMASS engine instance
 *  interfacing with either the command line application
 *  'lumassengine' or the BMI-compliant liblumassbmi.(so|dll)
 *  library for running either
 *
 *  model: spatial system dynamics models or
 *  moso:  standalone spatial optimisation scenarios
 *
 */

class NMMosra;
class NMModelController;

//class LUMASSBMI_EXPORT NMLumassEngine : public QObject
class NMLumassEngine : public QObject
{
    Q_OBJECT

public:

    using LumassEngineMode = enum _LumassEngineMode {
        NM_ENGINE_MODE_MODEL = 1,
        NM_ENGINE_MODE_MOSO = 2,
        NM_ENGINE_MODE_UNKNOWN = 3
    };

    using BMILog = void(*)(int, const char*);

    NMLumassEngine(QObject* parent = nullptr);
    virtual ~NMLumassEngine();

    void setBMILogFunc(BMILog logger) { mBMILogger = logger; }
    NMLogger* getLogger() const {return mLogger;}
    void setLogFileName(const QString& fn);
    QString getLogFileName(void) {return mLogFileName;}
    void about(void);

    void shutdown(void);

public slots:
    /*! passes a message to the internal logger associcated with this engine*/
    void log(const QString& type, const QString& qmsg);

    /*! writes a message to the log file */
    void writeLogMsg(const QString& msg);

    /*! passes a !global! string parameter to the controller for processing
        of LUMASS expressions
    */
    std::string processStringParameter(const QString& param);

    /*! loads the LUMASS model (*.lmx) given by the pathname modelfile */
    int loadModel(const QString& modelfile);

    /*! allows for configuring the loaded(!) model
     *  passing a LumassBMI ModelConfig yaml node
     */
    int configureModel(const YAML::Node& modelConfig);


    /*! load optimisation settings file*/
    int loadOptimisationSettings(const QString& losFileName);

    /*! configure a LUMASS optimisation scenario using a
     *  YAML file
     */
    int configureOptimisation(const YAML::Node& modelConfig);

    /*! parse optimisation settings (*.los) specified in the
     *  file given by logFileName and run the optimisation
     */
    int runOptimisation(void);


    /*!
     *  run a model
    */
    int runModel(double fromTimeStep, double toTimeStep);


    /*! updates a NMModelController setting given by the key value pair */
    void setSetting(const QString& key, const QString& value);

    /*! set whether LUMASS should log data provenance during model execution;
     *  Please note that this setting only applies to system dynamics models.
     *  The standalone spatial optimsation framework does its own provenance
     *  tracking by default (i.e. cannot be turned off)
     */
    void setLogProvenance(bool logProv);

    LumassEngineMode getEngineMode(void) { return mMode; }

    void doMOSO(const QString& losFileName);
    void doModel(const QString& userFile, QString& workspace, QString& enginePath, bool bLogProv);


protected:
    QString getYamlNodeTypeAsString(const YAML::Node& node);

    QVariant parseYamlSetting(const YAML::const_iterator& nit, const QObject* obj);
    bool isYamlSequence(const YAML::Node& node);

    int doMOSOsingle();
    int doMOSObatch();

private:
    NMModelController* mController;
    NMLogger* mLogger;
    QString mLogFileName;
    QFile mLogFile;
    NMMosra* mMosra;
    BMILog mBMILogger;

    bool mbMPICleanUp;


    LumassEngineMode mMode;

    static const std::string ctx;
};


#endif /* NMLUMASSENGINE_H */
