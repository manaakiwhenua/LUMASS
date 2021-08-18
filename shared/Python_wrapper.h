// avoiding conflicts with Qt's 'slots' keyword and
// Python.h's : PyType_Slot *slots; // in object.h

// thanks to https://stackoverflow.com/users/1279291/andreasdr
// source: https://stackoverflow.com/questions/23068700/embedding-python3-in-qt-5

#ifndef LUMASS_PYTHON_WRAPPER_H_
#define LUMASS_PYTHON_WRAPPER_H_

#pragma push_macro("slots")
#undef slots
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#pragma pop_macro("slots")

#include <string>
#include <map>

namespace py = pybind11;


// defines the process-wide (global) python
// object store for class-based access
namespace lumass_python {
    static std::map<std::string, py::module_> ctrlPyModules;
    static std::map<std::string, py::object> ctrlPyObjects;
    static std::map<std::string, bool> ctrlPyObjectSinkMap;
}


#endif //LUMASS_PYTHON_WRAPPER_H_
