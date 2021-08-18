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

#ifndef PYTHONBMI_H_
#define PYTHONBMI_H_

//#include "bmi.h"
//#include "NMLogger.h"
#include <string>
#include <vector>
//#include "bmi.hxx"

#include "Python_wrapper.h"

namespace py = pybind11;

#if defined _WIN32
#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
    #include "NMBMIWrapper.h"
#pragma pop_macro("GetCurrentTime")
#else
#include "NMBMIWrapper.h"
#endif

#include "lumasspythonbmi_export.h"

namespace bmi
{

    class PythonBMI : public Bmi
    {
    public:
        typedef enum {
            LEVEL_ALL,
            LEVEL_DEBUG,
            LEVEL_INFO,
            LEVEL_WARNING,
            LEVEL_ERROR,
            LEVEL_FATAL,
            LEVEL_NONE
        } Level;

        virtual ~PythonBMI() = default;
        PythonBMI(std::string pymodulename,
            std::vector<std::string> pythonpath,
            std::string bmiclass,
            std::string wrappername);

        using WrapLogFunc = void (NMBMIWrapper::*)(int, const char*);

        void Initialize(std::string config_file);
        void Update();
        void UpdateUntil(double time);
        void Finalize();

        std::string GetComponentName();
        int GetInputItemCount();
        int GetOutputItemCount();
        std::vector<std::string> GetInputVarNames();
        std::vector<std::string> GetOutputVarNames();

        int GetVarGrid(std::string name);
        std::string GetVarType(std::string name);
        int GetVarItemsize(std::string name);
        std::string GetVarUnits(std::string name);
        int GetVarNbytes(std::string name);
        std::string GetVarLocation(std::string name);

        double GetCurrentTime();
        double GetStartTime();
        double GetEndTime();
        std::string GetTimeUnits();
        double GetTimeStep();

        void GetValue(std::string name, void* dest);
        void* GetValuePtr(std::string name);
        void GetValueAtIndices(std::string name, void* dest, int* inds, int count);

        void SetValue(std::string name, void* src);
        void SetValueAtIndices(std::string name, int* inds, int len, void* src);

        int GetGridRank(const int grid);
        int GetGridSize(const int grid);
        std::string GetGridType(const int grid);

        void GetGridShape(const int grid, int* shape);
        void GetGridSpacing(const int grid, double* spacing);
        void GetGridOrigin(const int grid, double* origin);

        void GetGridX(const int grid, double* x);
        void GetGridY(const int grid, double* y);
        void GetGridZ(const int grid, double* z);

        int GetGridNodeCount(const int grid);
        int GetGridEdgeCount(const int grid);
        int GetGridFaceCount(const int grid);

        void GetGridEdgeNodes(const int grid, int* edge_nodes);
        void GetGridFaceEdges(const int grid, int* face_edges);
        void GetGridFaceNodes(const int grid, int* face_nodes);
        void GetGridNodesPerFace(const int grid, int* nodes_per_face);

        void bmilog(int, const char*);
        void setWrapLog(NMBMIWrapper* wrap, WrapLogFunc func);

        void setPyObjectName(std::string pymodulename)
        {
            mPyModuleName = pymodulename;
        }
        std::string getPyModuleName(void) { return mPyModuleName; }

        std::string getBMIClassName(void) { return mBMIClass; }

        std::string getBMIWrapperName(void) { return mBMIWrapperName; }

        //void setPyObjects(std::map<std::string, pybind11::object> *pyobjects);
        //std::map<std::string, py::object>* getPyObjects();
        //void setPyObjectSinkMap(std::map<std::string, bool>* sinkmap);
        //bool isPyObjectSink(std::string objname);

    private:
        std::string mPyModuleName;
        std::vector<std::string> mPythonPath;
        std::string mBMIClass;
        std::string mBMIWrapperName;
        std::string mLastMsg;
        NMBMIWrapper* mBMIWrap;
        WrapLogFunc mWrapLogFunc;

    };

}   // end of namespace bmi

#endif /* PYTHONBMI_H */

