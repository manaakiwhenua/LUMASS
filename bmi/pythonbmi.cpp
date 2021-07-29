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
        BMI wrapper for CSDMS BMI v2.0 Python modules.
        It enables the integration of python code with C++ code.
        This wrapper is used within LUMASS to enable BMI-based Python 'plug-ins'
        to extend functionality of the LUMASS modelling environment.

    This implementation is based on the OpenEarth BMI implementation:
    https://github.com/openearth/bmi/blob/master/models/cpp/model.cpp
*/

//#ifndef NM_ENABLE_LOGGER
//#   define NM_ENABLE_LOGGER
//#   include "nmlog.h"
//#   undef NM_ENABLE_LOGGER
//#else
//#   include "nmlog.h"
//#endif

#include "pythonbmi.h"

//#include "Python_wrapper.h"
#include <pybind11/numpy.h>
#include <pybind11/cast.h>

//#include <yaml-cpp/yaml.h>
#include <iostream>

namespace py = pybind11;
namespace lupy = lumass_python;

#include "lumasspythonbmi_export.h"

namespace bmi
{

    PythonBMI::PythonBMI(std::string pymodulename,
        std::string pymodulepath,
        std::string bmiclass,
        std::string wrappername)
        : mPyModuleName(pymodulename),
        mPyModulePath(pymodulepath),
        mBMIClass(bmiclass),
        mBMIWrapperName(wrappername)
    {}

    void
        PythonBMI::setWrapLog(NMBMIWrapper* wrap, WrapLogFunc func)
    {
        this->mBMIWrap = wrap;
        this->mWrapLogFunc = func;
    }

    void PythonBMI::bmilog(int ilevel, const char* msg)
    {
#ifdef LUMASS_DEBUG
        std::cout << msg << std::endl;
#endif
        ((*mBMIWrap).*(mWrapLogFunc))(ilevel, msg);
    }


    void PythonBMI::
        Initialize(std::string config_file)
    {
        // check for python interpreter
        if (!Py_IsInitialized())
        {
            bmilog(LEVEL_ERROR, "No python interpreter available!"
                " PythonBMI initialisation failed!");
            return;
        }

        try
        {
            std::stringstream msg;

            // add the module's path to the system's python path
            //std::string path = "/home/alex/garage/python/watyield/bmi";
            py::module::import("sys").attr("path").attr("append")(mPyModulePath.c_str());

            msg << "PythonBMI::initialize - added '" << this->mPyModulePath << "' to system path!";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            // import the module
            py::object mod = py::module::import(this->mPyModuleName.c_str());
            if (mod.is_none())
            {
                msg << "Module '" << this->mPyModuleName << "' import failed!";
                bmilog(LEVEL_ERROR, msg.str().c_str());
                return;
            }
            py::print(mod);
            msg << "Module '" << this->mPyModuleName << "' imported";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            // load the python bmi class
            py::object model = mod.attr(this->mBMIClass.c_str())();
            if (model.is_none())
            {
                msg << "Python BMI model '" << this->mBMIClass << "' instantiation failed!";
                bmilog(LEVEL_ERROR, msg.str().c_str());
                return;
            }
            py::print(model);
            msg << "Python BMI model '" << this->mBMIClass << "' instantiated";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            // call the init method on the bmi model
            model.attr("initialize")(config_file);

            msg << "Python BMI model '" << this->mBMIClass << "' initialised";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            bmilog(LEVEL_INFO, "PythonBMI: intitialisation complete!");

            model.inc_ref();
            lupy::pyObjects->insert(std::pair<std::string, py::object>(mBMIWrapperName, model));
        }
        catch (py::cast_error& ce)
        {
            bmilog(LEVEL_ERROR, ce.what());
        }
        catch (py::error_already_set& eas)
        {
            bmilog(LEVEL_ERROR, eas.what());
        }
        catch (std::exception& se)
        {
            bmilog(LEVEL_ERROR, se.what());
        }
    }

    void PythonBMI::
        Update()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        if (pymod.is_none())
        {
            bmilog(LEVEL_ERROR, "PythonBMI::Update() - Python module object invalid!");
            return;
        }

        try
        {
            pymod.attr("update")();
        }
        catch (py::error_already_set& eas)
        {
            bmilog(LEVEL_ERROR, eas.what());
        }
        catch (std::exception& se)
        {
            bmilog(LEVEL_ERROR, se.what());
        }
    }


    void PythonBMI::
        UpdateUntil(double t)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        pymod.attr("update_until")(t);
    }


    void PythonBMI::
        Finalize()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        pymod.attr("finalize")();
    }


    int PythonBMI::
        GetVarGrid(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_var_grid")(name);
        return res.cast<int>();
    }


    std::string PythonBMI::
        GetVarType(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_var_type")(name);
        return res.cast<std::string>();
    }


    int PythonBMI::
        GetVarItemsize(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_var_itemsize")(name);
        return res.cast<int>();
    }


    std::string PythonBMI::
        GetVarUnits(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_var_units")(name);
        return res.cast<std::string>();
    }


    int
        PythonBMI::GetVarNbytes(std::string name)
    {
        int itemsize;
        int gridsize;

        itemsize = this->GetVarItemsize(name);
        gridsize = this->GetGridSize(this->GetVarGrid(name));

        return itemsize * gridsize;
    }


    std::string
        PythonBMI::GetVarLocation(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_var_location")(name);
        return res.cast<std::string>();
    }


    void PythonBMI::
        GetGridShape(const int grid, int* shape)
    {
        int rank = this->GetGridRank(grid);
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::array_t<int, py::array::c_style> res(
            py::buffer_info(
                shape,
                sizeof(int),
                py::format_descriptor<int>::format(),
                1,
                { rank },
                { sizeof(int) }
            )
        );
        pymod.attr("get_grid_shape")(py::cast(grid), res);
    }


    void PythonBMI::
        GetGridSpacing(const int grid, double* spacing)
    {
        int rank = this->GetGridRank(grid);
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::array_t<double, py::array::c_style> res(
            py::buffer_info(
                spacing,
                sizeof(double),
                py::format_descriptor<double>::format(),
                1,
                { rank },
                { sizeof(double) }
            )
        );
        pymod.attr("get_grid_spacing")(py::cast(grid), res);
    }


    void PythonBMI::
        GetGridOrigin(const int grid, double* origin)
    {
    }


    int PythonBMI::
        GetGridRank(const int grid)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_grid_rank")(py::cast(grid));
        return res.cast<int>();
    }


    int PythonBMI::
        GetGridSize(const int grid)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_grid_size")(py::cast(grid));
        return res.cast<int>();
    }


    std::string PythonBMI::
        GetGridType(const int grid)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_grid_type")(py::cast(grid));
        return res.cast<std::string>();
    }


    void PythonBMI::
        GetGridX(const int grid, double* x)
    {
    }


    void PythonBMI::
        GetGridY(const int grid, double* y)
    {
    }


    void PythonBMI::
        GetGridZ(const int grid, double* z)
    {
    }


    int PythonBMI::
        GetGridNodeCount(const int grid)
    {
        return 0;
    }


    int PythonBMI::
        GetGridEdgeCount(const int grid)
    {
        return 0;
    }


    int PythonBMI::
        GetGridFaceCount(const int grid)
    {
        return 0;
    }


    void PythonBMI::
        GetGridEdgeNodes(const int grid, int* edge_nodes)
    {
    }


    void PythonBMI::
        GetGridFaceEdges(const int grid, int* face_edges)
    {
    }


    void PythonBMI::
        GetGridFaceNodes(const int grid, int* face_nodes)
    {
    }


    void PythonBMI::
        GetGridNodesPerFace(const int grid, int* nodes_per_face)
    {
    }


    void PythonBMI::
        GetValue(std::string name, void* dest)
    {
    }


    void* PythonBMI::
        GetValuePtr(std::string name)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        if (pymod.is_none())
        {
            bmilog(LEVEL_ERROR, "PythonBMI::GetValuePtr(): Python object not initialised!");
            return nullptr;
        }

        try
        {
            py::array res = pymod.attr("get_value_ptr")(py::cast(name));
            py::buffer_info resinfo = res.request(false);
            return resinfo.ptr;
        }
        catch (py::cast_error& ce)
        {
            bmilog(LEVEL_ERROR, ce.what());
        }
        catch (py::error_already_set& eas)
        {
            bmilog(LEVEL_ERROR, eas.what());
        }
        catch (std::exception& se)
        {
            bmilog(LEVEL_ERROR, se.what());
        }

        return nullptr;

    }


    void PythonBMI::
        GetValueAtIndices(std::string name, void* dest, int* inds, int len)
    {
    }


    void PythonBMI::
        SetValue(std::string name, void* src)
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        if (pymod.is_none())
        {
            bmilog(LEVEL_ERROR, "PythonBMI::SetValue(): Python object not initialised!");
            return;
        }

        std::string namepart = name;
        std::string typepart = "";
        size_t spos = name.find(' ');
        if (spos != std::string::npos)
        {
            namepart = name.substr(0, spos);
            typepart = name.substr(spos + 1);
        }

        std::vector<std::string> gridattr = { "gridsize",
                                             "gridrank",
                                             "itemsize" };
        try
        {
            if (typepart.empty())
            {
                std::string vtype = this->GetVarType(namepart);
                int vsize = this->GetVarItemsize(namepart);
                int vgsize = this->GetGridSize(GetVarGrid(namepart));
                if (vtype.find("float") != std::string::npos)
                {
                    if (vsize == 4)
                    {
                        py::array_t<float> decar(
                            py::buffer_info(
                                src,
                                sizeof(float),
                                py::format_descriptor<float>::format(),
                                1,
                                { vgsize },
                                { sizeof(float) }
                            )
                        );
                        pymod.attr("set_value")(namepart, decar);
                    }
                    else if (vsize == 8)
                    {
                        py::array_t<double> dar(
                            py::buffer_info(
                                src,
                                sizeof(double),
                                py::format_descriptor<double>::format(),
                                1,
                                { vgsize },
                                { sizeof(double) }
                            )
                        );
                        pymod.attr("set_value")(namepart, dar);
                    }
                }
                else if (vtype.find("int") != std::string::npos)
                {
                    if (vsize == 4)
                    {
                        py::array_t<int> iar(
                            py::buffer_info(
                                src,
                                sizeof(int),
                                py::format_descriptor<int>::format(),
                                1,
                                { vgsize },
                                { sizeof(int) }
                            )
                        );
                        pymod.attr("set_value")(namepart, iar);
                    }
                    else if (vsize == 8)
                    {
                        py::array_t<long long> llar(
                            py::buffer_info(
                                src,
                                sizeof(long long),
                                py::format_descriptor<long long>::format(),
                                1,
                                { vgsize },
                                { sizeof(long long) }
                            )
                        );
                        pymod.attr("set_value")(namepart, llar);
                    }
                }
            }
            else
            {
                if (typepart.compare("type") == 0)
                {
                    const std::string tname = static_cast<char*>(src);
                    pymod.attr("set_value")(name, py::cast(tname));
                }
                else if (std::find(gridattr.begin(), gridattr.end(), typepart) != gridattr.end())
                {
                    const unsigned long lsize = static_cast<unsigned long>(*static_cast<size_t*>(src));
                    pymod.attr("set_value")(name, py::cast(lsize));
                }
                else if (typepart.compare("gridshape") == 0)
                {
                    size_t* size = static_cast<size_t*>(src);
                    py::array_t<size_t> srcar(
                        py::buffer_info(
                            size,
                            sizeof(size_t),
                            py::format_descriptor<size_t>::format(),
                            1,
                            { 1 },
                            { sizeof(size_t) }
                        )
                    );
                    pymod.attr("set_value")(name, srcar);
                }
            }
        }
        catch (py::cast_error& ce)
        {
            bmilog(LEVEL_ERROR, ce.what());
        }
        catch (py::error_already_set& eas)
        {
            bmilog(LEVEL_ERROR, eas.what());
        }
        catch (std::exception& se)
        {
            bmilog(LEVEL_ERROR, se.what());
        }
    }


    void PythonBMI::
        SetValueAtIndices(std::string name, int* inds, int len, void* src)
    {
    }


    std::string PythonBMI::
        GetComponentName()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_component_name")();
        return res.cast<std::string>();
    }


    int PythonBMI::
        GetInputItemCount()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_input_item_count")();
        return res.cast<int>();
    }


    int PythonBMI::
        GetOutputItemCount()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_output_item_count")();
        return res.cast<int>();
    }


    std::vector<std::string> PythonBMI::
        GetInputVarNames()
    {
        std::vector<std::string> names;
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::tuple res = pymod.attr("get_input_var_names")();

        py::detail::tuple_iterator it = res.begin();
        while (it != res.end())
        {
            py::handle pyname = *it;
            names.push_back(pyname.cast<std::string>());
            ++it;
        }

        return names;
    }


    std::vector<std::string> PythonBMI::
        GetOutputVarNames()
    {
        std::vector<std::string> names;

        try
        {
            py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
            py::tuple res = pymod.attr("get_output_var_names")();

            py::detail::tuple_iterator it = res.begin();
            while (it != res.end())
            {
                py::handle pyname = *it;
                names.push_back(pyname.cast<std::string>());
                ++it;
            }
        }
        catch (py::cast_error& ce)
        {
            std::stringstream es;
            es << "PythonBMI::GetOutputVarNames() - " << ce.what();
            bmilog(LEVEL_ERROR, es.str().c_str());
        }
        catch (py::error_already_set& eas)
        {
            std::stringstream es;
            es << "PythonBMI::GetOutputVarNames() - " << eas.what();
            bmilog(LEVEL_ERROR, es.str().c_str());
        }
        catch (std::exception& se)
        {
            std::stringstream es;
            es << "PythonBMI::GetOutputVarNames() - " << se.what();
            bmilog(LEVEL_ERROR, es.str().c_str());
        }

        return names;
    }


    double
        PythonBMI::GetStartTime()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_start_time")();
        return res.cast<double>();
    }


    double
        PythonBMI::GetEndTime()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_end_time")();
        return res.cast<double>();
    }


    double
        PythonBMI::GetCurrentTime()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_current_time")();
        return res.cast<double>();
    }


    std::string
        PythonBMI::GetTimeUnits()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_time_units")();
        return res.cast<std::string>();
    }


    double
        PythonBMI::GetTimeStep()
    {
        py::object pymod = lupy::pyObjects->at(mBMIWrapperName);
        py::object res = pymod.attr("get_time_step")();
        return res.cast<double>();
    }


#ifdef _WIN32
    void main() {}
#endif


    //Logger _bmilog = nullptr;

    //py::module* currentPyModule;
    //std::map<std::string, py::module>* pyModules = nullptr;
    //std::map<std::string, bool>* pyModuleSinkMap = nullptr;

    //void bmilog(int ilevel, const char* msg)
    //{
    //    const int alevel = ilevel;
    //    Level level;
    //    switch (alevel)
    //    {
    //    case 0: level = LEVEL_ALL; break;
    //    case 1: level = LEVEL_DEBUG; break;
    //    case 2: level = LEVEL_INFO; break;
    //    case 3: level = LEVEL_WARNING; break;
    //    case 4: level = LEVEL_ERROR; break;
    //    case 5: level = LEVEL_FATAL; break;
    //    default: level = LEVEL_NONE;
    //    }

    //    if (_bmilog != nullptr)
    //    {
    //        _bmilog(level, msg);
    //    }
    //}


    //void PythonBMI::setPyObjects(std::map<std::string, py::object>* pyobjects)
    //{
    //    if (pyobjects != nullptr && pyobjects->size())
    //    {
    //        lupy::pyObjects = pyobjects;
    //    }
    //}

    //std::map<std::string, py::object>*
    //PythonBMI::getPyObjects()
    //{
    //    return lupy::pyObjects;
    //}


    //void PythonBMI::setPyObjectSinkMap(std::map<std::string, bool>* sinkmap)
    //{
    //    if (sinkmap != nullptr && sinkmap->size())
    //    {
    //        lupy::pyObjectSinkMap = sinkmap;
    //    }
    //    else
    //    {
    //        lupy::pyObjectSinkMap = nullptr;
    //    }
    //}

    //bool PythonBMI::isPyObjectSink(std::string objname)
    //{
    //    if (lupy::pyObjectSinkMap != nullptr && lupy::pyObjectSinkMap->count(objname) > 0)
    //    {
    //        return lupy::pyObjectSinkMap->at(objname);
    //    }

    //    return false;
    //}


    /*
    // control functions. These return an error code.
    BMI_API int initialize(const char *config_file)
    {
        // check for python interpreter
        if (!Py_IsInitialized())
        {
            bmilog(LEVEL_ERROR, "No python interpreter available!"
                                " PythonBMI initialisation failed!");
            return 1;
        }

        bmilog(LEVEL_INFO, "PythonBMI: initialising...");

        YAML::Node configFile;
        YAML::Node config;

        // read yaml configuration file
        try
        {
            configFile = YAML::LoadFile(config_file);
            config = configFile["LumassBMIConfig"];
        }
        catch (std::exception& e)
        {
            std::stringstream msg;
            msg << "ERROR: " << e.what();
            bmilog(LEVEL_ERROR, msg.str().c_str());
            return 1;
        }

        if (config.IsNull() || !config.IsDefined())
        {
            std::stringstream msg;
            msg << "ERROR: LUMASSConfig not found in yaml!";
            bmilog(LEVEL_ERROR, msg.str().c_str());
            //std::cout << "cout: " << msg.str() << std::endl;
            return 1;
        }

        std::string pymodule = config["pythonmodule"].IsDefined() ? config["pythonmodule"].as<std::string>() : "";
        if (!pymodule.empty())
        {
            py::module amod = py::module::import(pymodule.c_str());
            pyModules->insert(std::pair<std::string, py::module>(pymodule, amod));
            currentPyModule = &amod;
        }
        else
        {
            bmilog(LEVEL_ERROR, "No python module specified!");
            return 1;
        }

        bool issink = config["issink"].IsDefined() ? config["issink"].as<bool>() : false;
        if (issink)
        {
            pyModuleSinkMap->insert(std::pair<std::string, bool>(pymodule, issink));
        }

        bmilog(LEVEL_INFO, "LumassBMI: intitialisation complete!");
        return 0;
    }

    BMI_API int update(double dt)
    {
        bmilog(LEVEL_INFO, "LumassMBI: update(" << dt << ") called!");

        return 0;
    }

    BMI_API int finalize()
    {

        bmilog(LEVEL_INFO, "LumassBMI successfully finalised!");

        return 0;
    }

    //time control functions
    void PythonBMI:: get_start_time(double *t)
    {

    }

    void PythonBMI:: get_end_time(double *t)
    {

    }

    void PythonBMI:: get_current_time(double *t)
    {

    }

    void PythonBMI:: get_time_step(double *dt)
    {

    }

    // variable info
    void PythonBMI:: get_var_shape(const char *name, int shape[MAXDIMS])
    {

    }

    void PythonBMI:: get_var_rank(const char *name, int *rank)
    {

    }

    void PythonBMI:: get_var_type(const char *name, char *type)
    {

    }

    void PythonBMI:: get_var_count(int *count)
    {

    }

    void PythonBMI:: get_var_name(int index, char *name)
    {

    }

    // get a pointer pointer - a reference to a multidimensional array
    void PythonBMI:: get_var(const char *name, void **ptr)
    {

    }

    // Set the variable from contiguous memory referenced to by ptr
    void PythonBMI:: set_var(const char *name, const void *ptr)
    {

    }

    // Set a slice of the variable from contiguous memory using start / count multi-dimensional indices
    void PythonBMI:: set_var_slice(const char *name, const int *start, const int *count, const void *ptr)
    {

    }

    /// this Logger is not supported by LUMASS
    void PythonBMI:: set_logger(Logger logger)
    {
        _bmilog = logger;

        bmilog(LEVEL_INFO, "LumassBMI is now connected to BMI log function!");
    }
    */

} // end of namespace bmi
