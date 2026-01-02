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

PROJECT(NETCDFIO)

INCLUDE_DIRECTORIES(
    ${NCXX4_INCLUDE_DIR}
    ${NETCDF_INCLUDE_DIR}
    ${GDALRATImageIO_SOURCE_DIR}
    ${GDALRATImageIO_BINARY_DIR}
    ${ITK_INCLUDE_DIRS}
    ${OTB_INCLUDE_DIRS}
    ${OTBSupplCore_SOURCE_DIR}
    ${OTBSupplCore_BINARY_DIR}
    ${OTB_DIR}/Code/Common
    ${OTB_DIR}/Code/IO
    ${OTB_DIR}/Utilities/ITK/Code/Common
    ${shared_SOURCE_DIR}
    ${NETCDFIO_SOURCE_DIR}
    ${NETCDFIO_BINARY_DIR}
    ${shared_BINARY_DIR}
    ${filters_BINARY_DIR}
    ${filters_SOURCE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

LINK_DIRECTORIES(NetCDFIO
    ${HDF5_ROOT}
    ${NCXX4_LIB_DIR}
    ${NETCDF_LIB_DIR}
    ${OSGEO4W_ROOT}/lib
)

#add_definitions(-DNETCDFIO_EXPORTS -DNM_PROC_LOG)
#include(GenerateExportHeader)
#ADD_LIBRARY(NMNetCDFIO SHARED
#    ${NETCDFIO_SOURCE_DIR}/nmNetCDFIO.cxx
#)

#set_property(TARGET NMNetCDFIO PROPERTY LINKER_LANGUAGE CXX)
#set_property(TARGET NMNetCDFIO PROPERTY POSITION_INDEPENDENT_CODE 1)
#generate_export_header(NMNetCDFIO)

#if(WIN32)
#    TARGET_LINK_LIBRARIES(NMNetCDFIO
#        ${NCXX4_LIBRARY}
#        ${NETCDF_LIBRARY}
#        libcurl_imp libxml2
#        ${MPI_CXX_LIBRARIES}
#        ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
#        OTBImageIO
#        OTBCurlAdapters
#    )
#else()
#    TARGET_LINK_LIBRARIES(NMNetCDFIO
#        ${NCXX4_LIBRARY}
#        ${NETCDF_LIBRARY}
#        curl xml2
#        ${MPI_CXX_LIBRARIES}
#        ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_hl_LIBRARY_RELEASE}
#        OTBImageIO
#        OTBCurlAdapters
#    )
#endif()

#LIST(APPEND NETCDF_HEADER ${NETCDFIO_SOURCE_DIR}/nmNetCDFIO.h )
#install(FILES ${NETCDF_HEADER} DESTINATION include )
#
#if(WIN32)
#    install(TARGETS NMNetCDFIO
#        LIBRARY DESTINATION lib
#        RUNTIME DESTINATION bin
#    )
#else()
#    install(TARGETS NMNetCDFIO LIBRARY DESTINATION lib )
#endif()
