#******************************************************************************
# Created by Alexander Herzig
# Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
# 
# This file is part of 'LUMASS', which is free software: you can redistribute
# it and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************

PROJECT(bmi)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    SET(LUMASS_BMI_VTK_LIBRARIES
        vtkCommonCore
        vtkCommonDataModel
        vtkIOCore
        vtkIOSQL
        vtkIOLegacy
        vtkIOXML
    )
    include(${VTK_USE_FILE})
else()
   SET(LUMASS_BMI_VTK_LIBRARIES
       VTK::CommonCore
       VTK::CommonDataModel
       VTK::IOCore
       VTK::IOSQL
       VTK::IOLegacy
       VTK::IOXML
   )
endif()

INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
    ${NMVtk_SOURCE_DIR}
    ${gui_SOURCE_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${lumass_BINARY_DIR}
    ${bmi_BINARY_DIR}
    ${lumass_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${OTB_INCLUDE_DIRS}
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${bmi_SOURCE_DIR}
    ${bmi_SOURCE_DIR}/test
    ${Python3_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_DIRS}
    ${SQLite_SOURCE_DIR}
    ${YAML_CPP_INCLUDE_DIR}

    # for TESTING ONLY
    #${NCXX4_INCLUDE_DIR}
    #${NETCDF_INCLUDE_DIR}
    #${NETCDFIO_SOURCE_DIR}
    #${NETCDFIO_BINARY_DIR}

)

LINK_DIRECTORIES(
    ${OTB_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${opt_BINARY_DIR}
    ${bmi_BINARY_DIR}
    ${RASDAMAN_LIBRARIES_DIR}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIBLIB_DIR}
    ${NCXX4_LIB_DIR}
    ${NETCDF_LIB_DIR}
    ${Python3_LIBRARIES_DIR}
    ${HDF5_ROOT}
    ${shared_BINARY_DIR}
    ${YAML_CPP_LIB_DIR}
)

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

file(GLOB BMI_HEADER
    ${bmi_SOURCE_DIR}/*.h*
)

file(GLOB BMI_SRC
    ${bmi_SOURCE_DIR}/lumassbmi.cpp
    ${NMVtk_SOURCE_DIR}/NMvtkDelimitedTextWriter.cxx
)

file(GLOB PYTHON_BMI_SRC
    ${bmi_SOURCE_DIR}/python*.cpp
)

SET(LumassBMI_TEST_FILES
    ${bmi_SOURCE_DIR}/test/LumassBMIConfigTemplate.yaml
)

SET(BMI_LINKLIBS
    ${MPI_CXX_LIBRARIES}
    ${QT5_LIBRARIES}
    NMOTBSupplCoreRAT
    NMOTBSupplCoreIO
    NMMosra
    NMShared
    NMModFrameCore
    ${LUMASS_BMI_VTK_LIBRARIES}
    ${LPSOLVE_LIBRARY}
    ${YAML_CPP_LIBRARIES}
)

if(WIN32)
    list(APPEND BMI_LINK_LIBS 
    NMOTBSupplCoreReader
    NMOTBSupplCoreWriter
    NMOTBSupplTemplates
    libcurl_imp libxml2)
else()
    list(APPEND BMI_LINK_LIBS curl xml2)
endif()

SET(PYTHON_BMI_LINKLIBS
    ${Python3_LIBRARIES}
    ${QT5_LIBRARIES}
    pybind11::pybind11
    pybind11::embed
    ${YAML_CPP_LIBRARIES}
)

SET(PYTHON_BMI_DEPLIBS
    NMModFrameCore
)


SET(BMI_DEPLIBS
    NMMosra
    NMModFrameCore
)

set(CMAKE_AUTOMOC YES)
include(GenerateExportHeader)

# -----------------------------------------
# build the LUMASS BMI interface
ADD_LIBRARY(LumassBMI SHARED ${BMI_SRC})
generate_export_header(LumassBMI)
TARGET_LINK_LIBRARIES(LumassBMI ${BMI_LINKLIBS})
ADD_DEPENDENCIES(LumassBMI ${BMI_DEPLIBS})


if (PYTHONSUPPORT)
    ADD_DEFINITIONS(-DLUMASS_PYTHON)
    ADD_LIBRARY(LumassPythonBMI SHARED ${PYTHON_BMI_SRC})
    generate_export_header(LumassPythonBMI)
    TARGET_LINK_LIBRARIES(LumassPythonBMI ${PYTHON_BMI_LINKLIBS} ${MPI_CXX_LIBRARIES}
    )
endif()

if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
    vtk_module_autoinit(TARGETS LumassBMI LumassPythonBMI MODULES ${LUMASS_BMI_VTK_LIBRARIES})
endif()


# -----------------------------------------
# add the test application
#set(BMITEST_LINK_LIBS dl)
#if(PYTHONSUPPORT)
#    list(APPEND BMITEST_LINK_LIBS pybind11::embed)
#endif()


#ADD_EXECUTABLE(LumassBMITest ${bmi_SOURCE_DIR}/test/LumassBMITest.cpp)
#if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    TARGET_LINK_LIBRARIES(LumassBMITest dl)
#endif()

#SET(LUMASS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

#if (WIN32)
        #configure_file("${bmi_SOURCE_DIR}/test/LumassEnv.cmd.in"
    #           "${bmi_BINARY_DIR}/test/LumassEnv.cmd" @ONLY)
        #LIST(APPEND LumassBMI_TEST_FILES ${LumassBMITest_BINARY_DIR}/LumassEnv.cmd)
#endif()

#install(TARGETS LumassBMITest DESTINATION test)
#install(FILES ${LumassBMI_TEST_FILES} DESTINATION test)

# -------------------------------------------
# INSTALL LIBRARY
SET(BMI_INSTALL_TARGETS LumassBMI)
if (PYTHONSUPPORT)
    list(APPEND BMI_INSTALL_TARGETS LumassPythonBMI)
endif()

#install(FILES ${BMI_HEADER} DESTINATION include)
if (WIN32)
    install (TARGETS ${BMI_INSTALL_TARGETS}
             LIBRARY DESTINATION lib
             RUNTIME DESTINATION bin
     )
else()
    install(TARGETS ${BMI_INSTALL_TARGETS} LIBRARY DESTINATION lib)
endif()


ADD_SUBDIRECTORY(test ${bmi_BINARY_DIR}/test)
