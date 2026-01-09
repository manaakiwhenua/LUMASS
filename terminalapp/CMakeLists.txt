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

PROJECT(lumassengine)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    SET(LUMASS_ENGINE_VTK_LIBRARIES
        vtkCommonCore
        vtkCommonDataModel
        vtkIOCore
        vtkIOSQL
        vtkIOLegacy
        vtkIOXML
    )
    include (${VTK_USE_FILE})
else()
    SET(LUMASS_ENGINE_VTK_LIBRARIES
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::IOCore
        VTK::IOSQL
        VTK::IOLegacy
        VTK::IOXML
    )
endif ()

INCLUDE_DIRECTORIES(
    ${QT5_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
    ${NMVtk_SOURCE_DIR}
    ${gui_SOURCE_DIR}
    ${bmi_SOURCE_DIR}
    ${lumass_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/bmi
    ${shared_SOURCE_DIR}
    ${lumass_BINARY_DIR}
    ${lumass_BINARY_DIR}/bmi
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${shared_BINARY_DIR}
    ${rimageio_SOURCE_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_core_BINARY_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${LPSOLVE_INCLUDE_DIR}
    ${OTB_INCLUDE_DIRS}
    ${RASDAMAN_INCLUDE_DIR}
    ${RASDAMAN_INCLUDE_DIR}/raslib
    ${RASDAMAN_SOURCE_DIR}/applications/rasgeo
    ${PostgreSQL_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${SQLite_SOURCE_DIR}
# for testing
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
)

LINK_DIRECTORIES(
    ${OTB_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${RASDAMAN_LIBRARIES_DIR}
    ${PostgreSQL_LIBRARY_DIRS}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIBLIB_DIR}
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
    ${bmi_BINARY_DIR}
    ${shared_BINARY_DIR}
    ${LIBRARY_OUTPUT_PATH}
    ${YAML_CPP_LIB_DIR}
)

# Qt5 requirement
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC")

SET(CMAKE_AUTOMOC YES)

file(GLOB ENGINE_SRC
    ${lumassengine_SOURCE_DIR}/*.cxx
)

set(ENGINE_LINKLIBS 
        Qt5::Core Qt5::Sql Qt5::Xml
        ${MPI_CXX_LIBRARIES}
        LumassBMI
        NMOTBSupplFilters
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMModFrameCore
        NMShared
        ${MFW_WRAPPER_NAMES}
        ${LUMASS_ENGINE_VTK_LIBRARIES} ${PostgreSQL_LIBRARIES}
        ${LPSOLVE_LIBRARY}
        ${YAML_CPP_LIBRARIES}
        ${OTB_LIBRARIES}
        ${ITK_LIBRARIES}
)


if(PYTHONSUPPORT)
    add_definitions(-DLUMASS_PYTHON)
    list(APPEND ENGINE_LINKLIBS ${Python3_LIBRARIES}
        pybind11::embed
        pybind11::pybind11)
endif()


if(WIN32)
	LIST(APPEND ENGINE_LINKLIBS NMOTBSupplCoreReader NMOTBSupplCoreWriter)
endif()

set(ENGINE_DEPLIBS
    LumassBMI
    NMModFrameCore ${MFW_WRAPPER_NAMES}
)

IF(RASSUPPORT)
    add_definitions(-DBUILD_RASSUPPORT)
    LIST(APPEND ENGINE_LINKLIBS raslib RasImageIO)
ENDIF()


# VTK6 requirement
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

ADD_EXECUTABLE(lumassengine ${ENGINE_SRC})# ${ENGINE_MOC_CPP})
#set_property(TARGET lumassengine PROPERTY POSITION_INDEPENDENT_CODE 1)
TARGET_LINK_LIBRARIES(lumassengine ${ENGINE_LINKLIBS})
add_dependencies(lumassengine ${ENGINE_DEPLIBS})

if (${VTK_VERSION_STRING} VERSION_GREATER_EQUAL "8.90")
    vtk_module_autoinit(TARGETS lumassengine MODULES ${LUMASS_ENGINE_VTK_LIBRARIES})
endif()

install(TARGETS lumassengine DESTINATION bin)
