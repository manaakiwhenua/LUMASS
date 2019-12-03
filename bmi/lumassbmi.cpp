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

/*
	BMI wrapper (OpenEarth flavour) for the LumassEngine. It enables the integration of 
	LUMASS models as components into composite / integrated models.

	This implementation is based on the OpenEarth BMI implementation: 
	https://github.com/openearth/bmi/blob/master/models/cpp/model.cpp
*/

#include "lumassbmi.h"

#include <yaml-cpp/yaml.h>
#include <iostream>

#include "gdal.h"
#include "gdal_priv.h"

#include "sqlite3.h"

Logger _bmilog = nullptr;

void bmilog(int ilevel, const char* msg)
{
	const int alevel = ilevel;
	Level level;
	switch (alevel)
	{
	case 0: level = LEVEL_ALL; break;
	case 1: level = LEVEL_DEBUG; break;
	case 2: level = LEVEL_INFO; break;
	case 3: level = LEVEL_WARNING; break;
	case 4: level = LEVEL_ERROR; break;
	case 5: level = LEVEL_FATAL; break;
	default: level = LEVEL_NONE;
	}

	if (_bmilog != nullptr)
	{
		_bmilog(level, msg);
	}
}

void cleanup()
{
    delete nmengine;
}

#ifdef _WIN32
void main() {}
#endif

	/* control functions. These return an error code. */
	BMI_API int initialize(const char *config_file)
	{
        // need a NMLumassEngine
        if (nmengine == nullptr)
        {
            nmengine = new NMLumassEngine();
        }


		bmilog(LEVEL_INFO, "LumassBMI: initialising...");
		//std::cout << "LumassBMI: init ... " << std::endl;

		YAML::Node configFile;
		YAML::Node config;

		// read yaml configuration file
		try
		{
			configFile = YAML::LoadFile(config_file);
			config = configFile["EngineConfig"];
		}
		catch (std::exception& e)
		{
			std::stringstream msg;
			msg << "ERROR: " << e.what();
			bmilog(LEVEL_ERROR, msg.str().c_str());
			//std::cout << "cout: " << msg.str() << std::endl;
			return 1;
		}

		if (config.IsNull() || !config.IsDefined())
		{
			std::stringstream msg;
			msg << "ERROR: EngineConfig not found in yaml!";
			bmilog(LEVEL_ERROR, msg.str().c_str());
			//std::cout << "cout: " << msg.str() << std::endl;
			return 1;
		}

		std::string mode = config["mode"].as<std::string>();
		std::string modelfile = config["modelfile"].as<std::string>();
		std::string enginepath = config["enginepath"].as<std::string>();
		std::string workspace = config["workspace"].as<std::string>();
		std::string logfile = config["logfile"].as<std::string>();
		bool bLogProv = config["logprovenance"].as<bool>();

		//std::cout << "mode=" << mode << std::endl
			//<< "modelfile=" << modelfile << std::endl
			//<< "enginepath=" << enginepath << std::endl;


		if (!logfile.empty())
		{
            nmengine->setLogFileName(logfile.c_str());
		}

		if (mode.empty())
		{
            nmengine->log("ERROR", "Please specify the LUMASS Engine mode "
                "('mode: <model | moso>') in the confiugration file!");
            cleanup();
            return 1;
		}

		if (modelfile.empty())
		{
            nmengine->log("ERROR", "Please specify the LUMASS "
                "model file path ('modelfile') in the configuration file!");
            cleanup();
            return 1;
		}

		// ==============================================================
		// some 3rd party library init we depend on

		// init GDAL for bread and butter stuff ....
		//std::cout << "init GDAL and SQLite ... " << std::endl;
		
		GDALAllRegister();
		GetGDALDriverManager()->AutoLoadDrivers();

		sqlite3_temp_directory = const_cast<char*>(workspace.c_str());

		// get the model / settings configuration node
		YAML::Node modelConfig = configFile["ModelConfig"];

		// ======================================================
		// SYSTEM DYNAMICS MODELLING
		if (mode.compare("model") == 0)
		{
			// ------------------------------------
			// set lumass engine configuration parameter

			if (enginepath.empty())
			{
                nmengine->log("ERROR", "Please specify the path "
                    "to the LUMASS Engine (*.so | *.dll)"
                    "('enginepath') in the configuration file!");
                cleanup();
                return 1;
			}

			if (workspace.empty())
			{
                nmengine->log("ERROR", "Please specify the workspace path "
                    "('workspace') in the conriguration file, so LUMASS where"
                    "to write intermediary files!");
                cleanup();
                return 1;
			}

            nmengine->setSetting("LUMASSPath", enginepath.c_str());
            nmengine->setSetting("Workspace", workspace.c_str());
            nmengine->setSetting("TimeFormat", "yyyy-MM-ddThh:mm:ss.zzz");
            nmengine->setLogProvenance(bLogProv);
            nmengine->loadModel(modelfile.c_str());


			if (!modelConfig.IsNull() && modelConfig.IsDefined())
			{
				//std::cout << "parsing Model configuration .. " << std::endl;
                nmengine->configureModel(modelConfig);
			}
			else
			{
                nmengine->log("WARN", "We couldn't find a model"
					" configuration, so we just run it as is!");
				//std::cout << "failed reading config, trying without ... " << std::endl;
			}
		}
		// ========================================================
		// (STANDALONE) SPATIAL OPTIMISATION
		else if (mode.compare("moso") == 0)
		{
            nmengine->loadOptimisationSettings(modelfile.c_str());

            // need to call this in any case for parsing the settings file
            // loaded in the previous statement; validity check of
            // 'modelConfig' is performed as part of the configuration
            nmengine->configureOptimisation(modelConfig);
		}
		else
		{
            nmengine->log("ERROR", "Didn't catch what you want! "
				"Please review your yaml!");
            cleanup();
			return  1;
		}

		//std::cout <<"init was great success! " << std::endl;
		bmilog(LEVEL_INFO, "LumassBMI: intitialisation complete!");
		return 0;
	}

	BMI_API int update(double dt)
	{
		//std::cout << "start modelling ... " << std::endl;

        NMLumassEngine::LumassEngineMode mode = nmengine->getEngineMode();
		std::stringstream msg;
		msg << "running model (mode=" << mode << ") for time step " << dt;
		bmilog(LEVEL_INFO, msg.str().c_str());

		switch (mode)
		{
		case NMLumassEngine::NM_ENGINE_MODE_MODEL:
		{
            // =====================================
			// MODEL
            nmengine->runModel(1, dt);
		}
		break;

		case NMLumassEngine::NM_ENGINE_MODE_MOSO:
		{
            // =====================================
			// OPTIMISATION

            nmengine->runOptimisation();
		}
		break;

		default:
			;
		}

		//std::cout << "model -- all good!" << std::endl;
		return 0;
	}

	BMI_API int finalize()
	{

		//std::cout << "finalising ... " << std::endl;
		GDALDestroyDriverManager();

		// delete mModelController
		// delete mMosra
		// reset mMosoSettings
		// 

        cleanup();
		bmilog(LEVEL_INFO, "LumassBMI successfully finalised!");

        return 0;
	}

	/* time control functions */
	BMI_API void get_start_time(double *t)
	{

	}

	BMI_API void get_end_time(double *t)
	{

	}

	BMI_API void get_current_time(double *t)
	{

	}

	BMI_API void get_time_step(double *dt)
	{

	}

	/* variable info */
	BMI_API void get_var_shape(const char *name, int shape[MAXDIMS])
	{

	}

	BMI_API void get_var_rank(const char *name, int *rank)
	{

	}

	BMI_API void get_var_type(const char *name, char *type)
	{

	}

	BMI_API void get_var_count(int *count)
	{

	}

	BMI_API void get_var_name(int index, char *name)
	{

	}

	/* get a pointer pointer - a reference to a multidimensional array */
	BMI_API void get_var(const char *name, void **ptr)
	{

	}

	/* Set the variable from contiguous memory referenced to by ptr */
	BMI_API void set_var(const char *name, const void *ptr)
	{

	}

	/* Set a slice of the variable from contiguous memory using start / count multi-dimensional indices */
	BMI_API void set_var_slice(const char *name, const int *start, const int *count, const void *ptr)
	{

	}

	/// this Logger is not supported by LUMASS
	BMI_API void set_logger(Logger logger)
	{
		_bmilog = logger;

        // need a NMLumassEngine
        if (nmengine == nullptr)
        {
            nmengine = new NMLumassEngine();
        }
        nmengine->setBMILogFunc(bmilog);
		bmilog(LEVEL_INFO, "LumassBMI is now connected to BMI log function!");
	}
