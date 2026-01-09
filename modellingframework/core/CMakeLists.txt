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

PROJECT(mfw_core)

if (${VTK_VERSION_STRING} VERSION_LESS "8.90")
    SET(MFW_CORE_VTK_LIBRARIES
        vtkCommonDataModel
        vtkCommonCore
        vtkIOCore
    )
        include(${VTK_USE_FILE})
else()
   SET(MFW_CORE_VTK_LIBRARIES
       VTK::CommonDataModel
       VTK::CommonCore
       VTK::IOCore
   )
endif()

INCLUDE_DIRECTORIES(
    ${SQLite_SOURCE_DIR}
    ${SPATIALITE_INCLUDE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${mfw_core_BINARY_DIR}
    ${mfw_core_SOURCE_DIR}
    ${mfw_wrapper_SOURCE_DIR}
    ${mfw_wrapper_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${NCXX4_INCLUDE_DIR}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${gui_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${lumass_SOURCE_DIR}/modellingframework/wrapper
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${OTB_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
    ${shared_BINARY_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${bmi_SOURCE_DIR}
    ${bmi_BINARY_DIR}
    ${opt_SOURCE_DIR}
    ${opt_BINARY_DIR}
    ${LPSOLVE_INCLUDE_DIR}
    ${YAML_CPP_INCLUDE_DIR}
)

LINK_DIRECTORIES(
    ${OTB_DIR}/lib
    ${OTB_DIR}/bin
    ${OTB_LINK_DIR}
    ${ITK_LIBRARY_DIRS}
    ${ITK_LIBRARY_DIRS}/../
    ${ITK_LINK_DIRS}
    ${LIBRARY_OUTPUT_PATH}
    ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${OSGEO4W_ROOT}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${NETCDF_LIB_DIR}
    ${NCXX4_LIB_DIR}
    ${YAML_CPP_LIB_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${mfw_core_BINARY_DIR}
    ${lumass_BINARY_DIR}/lib/Release
    ${lumass_BINARY_DIR}/lib/Debug
    ${shared_BINARY_DIR}
)

file(GLOB MFW_CORE_CPP
    ${mfw_core_SOURCE_DIR}/*.cpp
    ${mfw_core_SOURCE_DIR}/*.cxx
)

LIST(REMOVE_ITEM MFW_CORE_CPP ${mfw_core_SOURCE_DIR}/NMConditionalIterComponent.cpp)

file(GLOB MFW_CORE_H
    ${mfw_core_SOURCE_DIR}/*.h
    ${mfw_core_BINARY_DIR}/*.h
)

LIST(REMOVE_ITEM MFW_CORE_H ${mfw_core_SOURCE_DIR}/NMConditionalIterComponent.h)

#if (NOT RASSUPPORT)
#  list(REMOVE_ITEM MFW_H
#  	"${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.h"
#  )

#  list(REMOVE_ITEM MFW_CPP
#        "${mfw_wrapper_SOURCE_DIR}/NMRasdamanConnectorWrapper.cpp"
#  )
#endif()


SET(MFW_CORE_MOC_H ${MFW_CORE_H})
LIST(REMOVE_ITEM MFW_CORE_MOC_H ${mfw_core_SOURCE_DIR}/NMMfwException.h)

message(STATUS "NMModFrameCore: OTB_LINK_LIBS=${OTB_LINK_LIBS}")

if (WIN32)
    set(MFW_CORE_UTIL_LIBS
            SQLite
            spatialite
            geos_c
            freexl_i
            libxml2
            zlib
            iconv
            librttopo
            proj
    )

    set(MFW_CORE_LINK_LIBS
        ${QT5_LIBRARIES}
        ${MFW_CORE_VTK_LIBRARIES}
        MuParser
        NMOTBSupplTemplates
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
        NMShared
        NMOTBSupplFilters
        NMMosra
        NMItk
        ${OTB_LINK_LIBS}
        ${YAML_CPP_LIBRARIES}
    )

else()
    set(MFW_CORE_UTIL_LIBS
            SQLite
            spatialite
            geos_c
            freexl
            xml2
            proj
    )

    set(MFW_CORE_LINK_LIBS
        ${QT5_LIBRARIES}
        ${MFW_CORE_VTK_LIBRARIES}
        MuParser
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMShared
        NMOTBSupplFilters
        NMMosra
        NMItk
        ${YAML_CPP_LIBRARIES}
        ${OTB_LIBRARIES}
        ${ITK_LIBRARIES}
    )
endif()


if(PYTHONSUPPORT)
    list(APPEND MFW_CORE_LINK_LIBS
            ${Python3_LIBRARIES}
            pybind11::pybind11
            pybind11::embed
        )

endif()

set(MFW_CORE_DEP_LIBS
    MuParser
    NMShared
    NMOTBSupplCoreRAT 
    NMOTBSupplCoreIO
    NMOTBSupplFilters
    NMItk
        #RasImageIO
)

if (WIN32)
    list(APPEND MFW_CORE_DEP_LIBS NMOTBSupplTemplates NMOTBSupplCoreReader NMOTBSupplCoreWriter)
endif()

SET(CMAKE_AUTOMOC YES)

#IF(RASSUPPORT)
#    add_definitions(-DBUILD_RASSUPPORT)
#    LIST(APPEND MFW_LINK_LIBS RasImageIO)
#    LIST(APPEND MFW_DEP_LIBS RasImageIO)
#ENDIF(RASSUPPORT)

if (PYTHONSUPPORT)
    add_definitions(-DLUMASS_PYTHON)
endif()

add_definitions(-DNMModFrameCore_EXPORTS -DNM_PROC_LOG)

if (WIN32)
    add_compile_options(/bigobj)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND MFW_CORE_UTIL_LIBS msvcrtd)
    endif()
endif()

include(GenerateExportHeader)
ADD_LIBRARY(NMModFrameCore SHARED ${MFW_CORE_CPP}) 
generate_export_header(NMModFrameCore)

target_link_libraries(NMModFrameCore ${MFW_CORE_LINK_LIBS} ${MFW_CORE_UTIL_LIBS})
ADD_DEPENDENCIES(NMModFrameCore ${MFW_CORE_DEP_LIBS})

install(FILES ${MFW_CORE_H} DESTINATION include)
if(WIN32)
        install(TARGETS NMModFrameCore
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )
else()
        install(TARGETS NMModFrameCore LIBRARY DESTINATION lib)
endif()
