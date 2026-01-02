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

PROJECT(filters)

INCLUDE_DIRECTORIES(
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${NCXX4_INCLUDE_DIRS}
    ${NETCDF_INCLUDE_DIRS}
    ${filters_SOURCE_DIR}
    ${filters_BINARY_DIR}
    ${shared_SOURCE_DIR}
    ${QT5_INCLUDE_DIRS}
    ${OTB_DIR}/Code/Common
    ${OTB_INCLUDE_DIRS}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${mfw_core_BINARY_DIR}
    ${utils_SOURCE_DIR}
    ${OTB_DIR}/Utilities/ITK/Utilities/vxl/core/vnl
    ${lumass_SOURCE_DIR}/utils/ITK
    ${lumass_SOURCE_DIR}
    ${PYBIND11_INCLUDE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
    ${opt_SOURCE_DIR}
    ${muparser_SOURCE_DIR}
    ${bmi_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${muparser_BINARY_DIR}
    ${muparser_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    ${OTB_BINARY_DIR}
    ${OTB_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/bin
    ${OTB_DIR}/bin/${CMAKE_BUILD_TYPE}
    ${OTB_DIR}/lib
    ${SPATIALITE_LIB_DIR}
    ${SPATIALITE_LIBLIB_DIR}
    ${NETCDF_LIB_DIR}
)

file(GLOB OTBSUPPL_FILTERS
    ${filters_SOURCE_DIR}/*.txx
    ${filters_SOURCE_DIR}/*.cxx
    #${OTBSupplCore_SOURCE_DIR}/*.cxx
    #${OTBSupplCore_SOURCE_DIR}/*.hxx
)

file(GLOB OTBSUPPL_HEADER
    ${filters_SOURCE_DIR}/*.h
    ${filters_BINARY_DIR}/*.h
)

include(GenerateExportHeader)

add_definitions(-DNM_PROC_LOG)

if(WIN32)
        add_definitions(-DNMOTBSUPPLFILTERS_STATIC_DEFINE)
        ADD_LIBRARY(NMOTBSupplFilters STATIC ${OTBSUPPL_FILTERS})
else(WIN32)
        ADD_LIBRARY(NMOTBSupplFilters SHARED ${OTBSUPPL_FILTERS})
endif(WIN32)

set_target_properties(NMOTBSupplFilters PROPERTIES
                        LINKER_LANGUAGE CXX)

generate_export_header(NMOTBSupplFilters)

if(WIN32)
    target_link_libraries(NMOTBSupplFilters
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
        Qt5::Core Qt5::Qml
    )
else()
    target_link_libraries(NMOTBSupplFilters
        ${MPI_CXX_LIBRARIES}
        ${NETCDF_LIBRARY}
        NMOTBSupplCoreRAT 
        NMOTBSupplCoreIO
        NMOTBSupplCoreReader
        NMOTBSupplCoreWriter
        Qt5::Core Qt5::Qml
    )
endif()

add_dependencies(NMOTBSupplFilters 
    NMOTBSupplCoreRAT 
    NMOTBSupplCoreIO
    NMOTBSupplCoreReader
    NMOTBSupplCoreWriter
    MuParser)

install(FILES ${OTBSUPPL_HEADER} DESTINATION include)
if(WIN32)
        install(TARGETS NMOTBSupplFilters
                #LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                RUNTIME DESTINATION bin
        )
else()
        install(TARGETS NMOTBSupplFilters LIBRARY DESTINATION lib)
endif()
