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

#include "pythonbmi.h"

#include <pybind11/numpy.h>
#include <pybind11/cast.h>
#include <pybind11/embed.h>

#include <iostream>

#include "lumasspythonbmi_export.h"

#define PyBMIFuncGuard( msg_str ) \
    std::stringstream _stream; \
    _stream << "PythonBMI::" msg_str; \
    if (mPyObject.is_none() || mPyObject.ptr() == nullptr) \
    { \
        bmilog(LEVEL_ERROR, _stream.str().c_str()); \
        PythonBMIException pe(_stream.str().c_str()); \
        throw pe; \
    }

#define PyBMICatchAll() \
    catch (py::error_already_set& eas) \
    { \
        bmilog(LEVEL_ERROR, eas.what()); \
        PythonBMIException pe(eas.what()); \
        throw pe; \
    } \
    catch (std::exception& se) \
    { \
        bmilog(LEVEL_ERROR, se.what()); \
        PythonBMIException pe(se.what()); \
        throw pe; \
    }


namespace bmi
{

    PythonBMI::PythonBMI(std::string pymodulename,
        std::vector<std::string> pythonpath,
        std::string bmiclass,
        std::string wrappername)
        : mPyModuleName(pymodulename),
        mPythonPath(pythonpath),
        mBMIClass(bmiclass),
        mBMIWrapperName(wrappername),
        mBMIWrap(nullptr),
        mWrapLogFunc(nullptr),
        mIsSink(false),
        mbReloadModule(false)
    {
    }

    void PythonBMI::logPyOutput(std::string msg)
    {
        if (msg.size() > 0)
        {
            int start = 0;
            for (int s=0; s < msg.size(); ++s)
            {
                if (    msg[s] == '\n'
                     //|| msg[s] == '\r\n'
                     || s == msg.size()-1
                   )
                {
                    // \nHier\nKommt
                    //  01234 56789
                    std::string logStr = "Python Interpreter: ";
                    logStr += msg.substr(start, s-start);
                    bmilog(2, logStr.c_str());
                    start = s+1;
                }
            }
        }

    }

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
        if (ilevel == 4 || ilevel == 5)
        {
            PythonBMIException pe(msg);
            throw pe;
        }
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
            LogPyOutputStart();
            std::stringstream msg;

            // =================================================================
            // always add the python path to avoid closing and re-openeing LUMASS
            // to reflect code-changes including the use of additional modules.
            // it would be a pain to always have to close and re-open LUMASS

            // add the module(s)'(s) path(s) to the system's python path
            //std::string path = "/home/alex/garage/python/watyield/bmi";
#if defined _WIN32
            std::string sep = ";";
#else
            std::string sep = ":";
#endif

            std::string ppath = mPythonPath.size() > 0 ? mPythonPath.at(0) : "";
            py::module_ pysys = py::module_::import("sys");
            for (int p = 0; p < mPythonPath.size(); ++p)
            {
                pysys.attr("path").attr("append")(py::cast(mPythonPath.at(p)));

                if (p > 0)
                {
                    ppath = ppath + sep + mPythonPath.at(p);
                }
            }

            msg << "PythonBMI::initialize - added '" << ppath << "' to system path!";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");


            // ================================================================
            // double check, whether we just have to 'reload' the module and class ...

            if (mbReloadModule)
            {
                msg << "Re-loading module '" << mPyModuleName << "' ...";
                bmilog(LEVEL_INFO, msg.str().c_str());
                msg.str("");
                mPyModule.reload();


                msg << "Instantiate model class '" << mBMIClass << "' ...";
                bmilog(LEVEL_INFO, msg.str().c_str());
                msg.str("");
                mPyObject = mPyModule.attr(mBMIClass.c_str())();
                mPyObject.attr("initialize")(config_file);

                msg << "'" << mBMIClass << "' successfully re-initialised!";
                bmilog(LEVEL_INFO, msg.str().c_str());
                msg.str("");
                LogPyOutputEnd();
                return;
            }

            // ================================================================
            // ... nope, nothing there. We'll do a first time init ...

            mIsSink = false;
            // import the module
            mPyModule = py::module_::import(this->mPyModuleName.c_str());
            if (mPyModule.is_none() || mPyModule.ptr() == nullptr)
            {
                LogPyOutputEnd();
                msg << "Module '" << this->mPyModuleName << "' import failed!";
                bmilog(LEVEL_ERROR, msg.str().c_str());
                return;
            }

            msg << "Module '" << this->mPyModuleName << "' imported";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            // load the PythonBMI class
            mPyObject = mPyModule.attr(this->mBMIClass.c_str())();
            if (mPyObject.is_none())
            {
                LogPyOutputEnd();
                msg << "PythonBMI model '" << this->mBMIClass << "' instantiation failed!";
                bmilog(LEVEL_ERROR, msg.str().c_str());
                return;
            }

            msg << "PythonBMI model '" << this->mBMIClass << "' instantiated";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            // call the init method on the bmi model
            mPyObject.attr("initialize")(config_file);

            msg << "PythonBMI model '" << this->mBMIClass << "' initialised";
            bmilog(LEVEL_INFO, msg.str().c_str());
            msg.str("");

            this->mIsSink = true;

            bmilog(LEVEL_INFO, "PythonBMI: intitialisation complete!");

            LogPyOutputEnd();
        }
        catch (py::cast_error& ce)
        {
            bmilog(LEVEL_ERROR, ce.what());
            PythonBMIException pe(ce.what());
            throw pe;
        }
        catch (py::error_already_set& eas)
        {
            bmilog(LEVEL_ERROR, eas.what());
            PythonBMIException pe(eas.what());
            throw pe;
        }
        catch (std::exception& se)
        {
            bmilog(LEVEL_ERROR, se.what());
            PythonBMIException pe(se.what());
            throw pe;
        }
        catch(...)
        {
            bmilog(LEVEL_ERROR, "Unknown error in PythonBMI!");
            PythonBMIException pe("Unknown error in PythonBMI!");
            throw pe;
        }
    }

    void PythonBMI::SetSetting(string key, string value)
    {
        PyBMIFuncGuard(<< "SetSetting() - Python module object invalid!");
        try
        {
            mPyObject.attr("setSetting")(key, value);
        }
        PyBMICatchAll();
    }

    void PythonBMI::
        Update()
    {
        PyBMIFuncGuard(<< "Update() - Python module object invalid!");
        try
        {
            LogPyOutputStart();
            mPyObject.attr("update")();
            LogPyOutputEnd();
        }
        PyBMICatchAll();
    }


    void PythonBMI::
        UpdateUntil(double t)
    {
        PyBMIFuncGuard(<< "UpdateUntil() - Python module object invalid!");

        try
        {
            mPyObject.attr("update_unitl")(t);
        }
        PyBMICatchAll();
    }


    void PythonBMI::
        Finalize()
    {
        LogPyOutputStart();
        // amazing code goes here ...
        LogPyOutputEnd();
    }


    int PythonBMI::
        GetVarGrid(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarGrid() - Python module object invalid!");

        try
        {
            LogPyOutputStart();
            py::object res = mPyObject.attr("get_var_grid")(name);
            LogPyOutputEnd();

            return res.cast<int>();
        }
        PyBMICatchAll();

        return -1;
    }


    std::string PythonBMI::
        GetVarType(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarType() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_var_type")(name);
            return res.cast<std::string>();
        }
        PyBMICatchAll();
        return "";
    }


    int PythonBMI::
        GetVarItemsize(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarItemSize() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_var_itemsize")(name);
            return res.cast<int>();
        }
        PyBMICatchAll();
        return -1;
    }


    std::string PythonBMI::
        GetVarUnits(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarUnits() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_var_units")(name);
            return res.cast<std::string>();
        }
        PyBMICatchAll();
        return "";
    }


    int PythonBMI::GetVarNbytes(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarNbytes() - Python module object invalid!");
        int itemsize;
        int gridsize;

        try
        {
            itemsize = this->GetVarItemsize(name);
            gridsize = this->GetGridSize(this->GetVarGrid(name));
            return itemsize * gridsize;
        }
        PyBMICatchAll();
        return -1;
    }


    std::string
        PythonBMI::GetVarLocation(std::string name)
    {
        PyBMIFuncGuard(<< "GetVarLocation() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_var_location")(name);
            return res.cast<std::string>();
        }
        PyBMICatchAll();
        return "";
    }


    void PythonBMI::
        GetGridShape(const int grid, int* shape)
    {
        PyBMIFuncGuard(<< "GetGridShape() - Python module object invalid!");
        try
        {
            int rank = this->GetGridRank(grid);
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
            mPyObject.attr("get_grid_shape")(py::cast(grid), res);
        }
        PyBMICatchAll();
    }


    void PythonBMI::
        GetGridSpacing(const int grid, double* spacing)
    {
        PyBMIFuncGuard(<< "GetGridSpacing() - Python module object invalid!");
        try
        {
            int rank = this->GetGridRank(grid);
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
            mPyObject.attr("get_grid_spacing")(py::cast(grid), res);
        }
        PyBMICatchAll();
    }


    void PythonBMI::
        GetGridOrigin(const int grid, double* origin)
    {
    }


    int PythonBMI::
        GetGridRank(const int grid)
    {
        PyBMIFuncGuard(<< "GetGridRank() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_grid_rank")(py::cast(grid));
            return res.cast<int>();
        }
        PyBMICatchAll();
        return 0;
    }


    int PythonBMI::
        GetGridSize(const int grid)
    {
        PyBMIFuncGuard(<< "GetGridSize() - Python module object invalid!");
        try
        {
            LogPyOutputStart();
            py::object res = mPyObject.attr("get_grid_size")(py::cast(grid));
            LogPyOutputEnd();
            return res.cast<int>();
        }
        PyBMICatchAll();
        return 0;
    }


    std::string PythonBMI::
        GetGridType(const int grid)
    {
        PyBMIFuncGuard(<< "GetGridType() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_grid_type")(py::cast(grid));
            return res.cast<std::string>();
        }
        PyBMICatchAll();
        return "";
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
        PyBMIFuncGuard(<< "GetValue() - Python module object invalid!");
        try
        {
            dest = this->GetValuePtr(name);
        }
        PyBMICatchAll();
    }


    void* PythonBMI::
        GetValuePtr(std::string name)
    {
        PyBMIFuncGuard(<< "GetValuePtr() - Python module object invalid!");
        try
        {
            LogPyOutputStart();
            py::array res = mPyObject.attr("get_value_ptr")(py::cast(name));
            if (!res.is_none() && res.ptr() != nullptr)
            {
                py::buffer_info resinfo = res.request(true);
                return resinfo.ptr;
            }
            LogPyOutputEnd();
        }
        PyBMICatchAll();
        return nullptr;
    }


    void PythonBMI::
        GetValueAtIndices(std::string name, void* dest, int* inds, int len)
    {
    }


    void PythonBMI::
        SetValue(std::string name, void* src)
    {
        PyBMIFuncGuard(<< "SetValue() - Python module object invalid!");

        try
        {
            std::string namepart = name;
            std::string typepart = "";
            size_t spos = name.find(' ');
            if (spos != std::string::npos)
            {
                namepart = name.substr(0, spos);
                typepart = name.substr(spos + 1);
            }

            // data type and dimension (either 'proper' region dimension or image value buffer dimension)
            // depend on the admin info requested
            const std::vector<std::string> integral_values = {"gridsize", "gridshape", "gridindex"};
            const std::vector<std::string> integral_scalar_values = {"itemsize","gridrank", "gridsize"};
            const std::vector<std::string> real_values = {"gridorigin", "gridspacing"};
            const std::vector<std::string> region_dim_values = {"gridorigin", "gridspacing", "gridindex"};

            LogPyOutputStart();
            int ndim = this->GetGridRank(GetVarGrid(namepart));
            if (std::find(region_dim_values.begin(), region_dim_values.end(), typepart) != region_dim_values.end())
            {
                ndim = this->GetGridRank(GetVarGrid("LPR"));
            }
            if (!typepart.empty() && typepart.compare("gridrank") != 0 && ndim < 0)
            {
                std::stringstream estr;
                estr << "Got invalid rank for '" << namepart << "'!";
                bmilog(LEVEL_ERROR, estr.str().c_str());
                LogPyOutputEnd();
                return;
            }

            if (typepart.empty())
            {
                const std::string vtype = this->GetVarType(namepart);
                const int vsize = this->GetVarItemsize(namepart);
                const int vgsize = this->GetGridSize(GetVarGrid(namepart));
                if (    vtype.find("float") != std::string::npos
                     || vtype.find("double") != std::string::npos
                   )
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
                        mPyObject.attr("set_value")(namepart, decar);
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
                        mPyObject.attr("set_value")(namepart, dar);
                    }
                }
                else if (    vtype.find("int") != std::string::npos
                          || vtype.find("long") != std::string::npos
                          || vtype.find("long long") != std::string::npos
                        )
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
                        mPyObject.attr("set_value")(namepart, iar);
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
                        mPyObject.attr("set_value")(namepart, llar);
                    }
                }
            }
            else
            {
                if (typepart.compare("type") == 0)
                {
                    const std::string tname = static_cast<char*>(src);
                    mPyObject.attr("set_value")(name, py::cast(tname));
                }
                else if (std::find(integral_scalar_values.begin(), integral_scalar_values.end(), typepart) != integral_scalar_values.end())
                {
                    const unsigned long lsize = static_cast<unsigned long>(*static_cast<size_t*>(src));
                    mPyObject.attr("set_value")(name, py::cast(lsize));
                }

                else if (std::find(integral_values.begin(), integral_values.end(), typepart) != integral_values.end())
                {
                    size_t* size = static_cast<size_t*>(src);
                    py::array_t<size_t> srcar(
                        py::buffer_info(
                            size,
                            sizeof(size_t),
                            py::format_descriptor<size_t>::format(),
                            1,
                            { ndim },
                            { sizeof(size_t) }
                        )
                    );
                    mPyObject.attr("set_value")(name, srcar);
                }
                else if (std::find(real_values.begin(), real_values.end(), typepart) != real_values.end())
                {
                    double* spacing = static_cast<double*>(src);
                    py::array_t<size_t> srcar(
                        py::buffer_info(
                            spacing,
                            sizeof(double),
                            py::format_descriptor<double>::format(),
                            1,
                            { ndim },
                            { sizeof(double) }
                        )
                    );
                    mPyObject.attr("set_value")(name, srcar);
                }

            }
            LogPyOutputEnd();
        }
        PyBMICatchAll();
    }


    void PythonBMI::
        SetValueAtIndices(std::string name, int* inds, int len, void* src)
    {
    }


    std::string PythonBMI::
        GetComponentName()
    {
        PyBMIFuncGuard(<< "GetComponentName() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_component_name")();
            return res.cast<std::string>();
        }
        PyBMICatchAll();
    }


    int PythonBMI::
        GetInputItemCount()
    {
        PyBMIFuncGuard(<< "GetInputItemCount() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_input_item_count")();
            return res.cast<int>();
        }
        PyBMICatchAll();
        return 0;
    }


    int PythonBMI::
        GetOutputItemCount()
    {
        PyBMIFuncGuard(<< "GetOutputItemCount() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_output_item_count")();
            return res.cast<int>();
        }
        PyBMICatchAll();
        return 0;
    }


    std::vector<std::string> PythonBMI::
        GetInputVarNames()
    {
        PyBMIFuncGuard(<< "GetInputVarNames() - Python module object invalid!");
        std::vector<std::string> names;
        try
        {
            py::tuple res = mPyObject.attr("get_input_var_names")();

            py::detail::tuple_iterator it = res.begin();
            while (it != res.end())
            {
                py::handle pyname = *it;
                names.push_back(pyname.cast<std::string>());
                ++it;
            }
        }
        PyBMICatchAll();
        return names;
    }


    std::vector<std::string> PythonBMI::
        GetOutputVarNames()
    {
        PyBMIFuncGuard(<< "GetOutputVarNames() - Python module object invalid!");
        std::vector<std::string> names;
        try
        {
            py::tuple res = mPyObject.attr("get_output_var_names")();

            py::detail::tuple_iterator it = res.begin();
            while (it != res.end())
            {
                py::handle pyname = *it;
                names.push_back(pyname.cast<std::string>());
                ++it;
            }
        }
        PyBMICatchAll();
        return names;
    }


    double
        PythonBMI::GetStartTime()
    {
        PyBMIFuncGuard(<< "GetStartTime() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_start_time")();
            return res.cast<double>();
        }
        PyBMICatchAll();
        return 0.0;
    }


    double
        PythonBMI::GetEndTime()
    {
        PyBMIFuncGuard(<< "GetEndTime() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_end_time")();
            return res.cast<double>();
        }
        PyBMICatchAll();
        return 0.0;
    }


    double
        PythonBMI::GetCurrentTime()
    {
        PyBMIFuncGuard(<< "GetGridShape() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_current_time")();
            return res.cast<double>();
        }
        PyBMICatchAll();
        return 0.0;
    }


    std::string
        PythonBMI::GetTimeUnits()
    {
        PyBMIFuncGuard(<< "GetTimeUnits() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_time_units")();
            return res.cast<std::string>();
        }
        PyBMICatchAll();
        return "";
    }


    double
        PythonBMI::GetTimeStep()
    {
        PyBMIFuncGuard(<< "GetTimeStep() - Python module object invalid!");
        try
        {
            py::object res = mPyObject.attr("get_time_step")();
            return res.cast<double>();
        }
        PyBMICatchAll();
        return 0.0;
    }


#ifdef _WIN32
    void main() {}
#endif

} // end of namespace bmi
