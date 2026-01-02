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

# =============
# FindNetCDF.cmake
# ==============
# this file is part of LUMASS and provided as is
# - no warranty whatsoever for this being even remotely useful in whichever shape or form!

# find parallel netcdf nc-config
if (NOT NC_CONFIG_PATH)
    find_path(NC_CONFIG_PATH NAMES nc-config
        HINTS
            C:/Install/netCDF/bin
            C:/Install/netCDF_dbg/bin
            $ENV{HOME}/garage/build/netcdf-c-4.9.2/install/bin
            /opt/netcdf-4.7.3/install/bin
            /opt/netcdf-4.8.1/install/bin
            /opt/netcdf-c-4.8.1/install/bin
            /opt/netcdf-bin/install/bin
        PATHS
            C:/Install/netCDF/bin
            C:/Install
            $ENV{HOME}/garage/build/netcdf-c-4.9.2
            /opt/netcdf-4.7.3
            /opt/netcdf-bin
            /opt/netcdf-4.8.1
            /opt/netcdf-c-4.8.1
            /usr/local
            /usr
        PATH_SUFFIXES
            netCDF/bin
            install/bin
            bin
            netcdf-c-4.9.3
        DOC "Path to the netcdf-c config script 'nc-config'"
    )
endif()
    
if (EXISTS ${NC_CONFIG_PATH}/nc-config)
    set(NC_FOUND 1)
else()
    set(NC_FOUND 0)
    set(NC_CONFIG_PATH "" CACHE STRING "Please specify the path to 'nc-config' in the NetCDF install directory!" FORCE)    
    message(STATUS "NetCDF not found!")
endif()

if (NC_FOUND)
    if (NOT WIN32)
        execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --version OUTPUT_VARIABLE NETCDF_VERSION_TEXT OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --includedir OUTPUT_VARIABLE NETCDF_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --libdir OUTPUT_VARIABLE NETCDF_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --libs OUTPUT_VARIABLE NETCDF_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)

        string(REPLACE " " ";" NETCDF_VERSION_LIST "${NETCDF_VERSION_TEXT}")
        list(LENGTH NETCDF_VERSION_LIST NC_LEN)
        IF(NC_LEN GREATER_EQUAL 1)
            list(GET NETCDF_VERSION_LIST 1 NETCDF_VERSION)
        ENDIF()
    endif()


    if(WIN32)
            SET(NETCDF_LIBRARY "${NC_CONFIG_PATH}/../lib/netcdf_par.lib")
            SET(NETCDF_INCLUDE_DIR "${NC_CONFIG_PATH}/../include")
            SET(NETCDF_VERSION "4.9.3")
    else()
        #if (LUMASS_MPI_SUPPORT)
            #SET(NETCDF_LIBRARY ${NETCDF_LIB_DIR}/libnetcdf_mpi.so)
            #SET(NETCDF_INCLUDE_DIR ${NETCDF_LIB_DIR}/netcdf/mpi/include)
            set(NETCDF_LIBRARY "netcdf_par")
        #else()
        #    SET(NETCDF_LIBRARY ${NETCDF_LIB_DIR}/libnetcdf.so)
        #endif()
    endif()
endif()

#message(STATUS "NetCDF config path: ${NC_CONFIG_PATH}")
#message(STATUS "NetCDF version: ${NETCDF_VERSION}")
#message(STATUS "NETCDF_INCLUDE_DIR: ${NETCDF_INCLUDE_DIR}")
#message(STATUS "NetCDF library dir: ${NETCDF_LIB_DIR}")
#message(STATUS "NetCDF libraries: ${NETCDF_LIBRARIES}")
#message(STATUS "NetCDF library: ${NETCDF_LIBRARY}")

# find also netcdf-cxx4
if (NOT NCXX4_CONFIG_PATH)
    find_path(NCXX4_CONFIG_PATH ncxx4-config
        HINTS
            C:/Install/netcdf-cxx-4.3.1
            $ENV{HOME}/garage/build/netcdf-cxx-4.3.1/install/bin
            /opt/ncxx4-dbg/install/bin
            /opt/netcdf-cxx4/install/bin
        PATHS
            $ENV{HOME}/garage/build/netcdf-cxx-4.3.1
            /opt/ncxx4-dbg
            /opt/netcdf-cxx4
            /usr/local
            /usr
            C:/Install
        PATH_SUFFIXES
            bin
            install/bin
            netcdf-cxx-4.3.1/bin
        DOC "Path to the netcdf-cxx4 config script 'ncxx4-config'"
    )
endif()

if (EXISTS ${NCXX4_CONFIG_PATH}/ncxx4-config)
    set(NCXX4_FOUND 1)
else()
    set(NCXX4_FOUND 0)
    set(NCXX4_CONFIG_PATH "" CACHE STRING "Please specify the path to 'ncxx4-config' in the NetCDF-cxx install directory!" FORCE)    
    message(STATUS "NetCDF not found!")
endif()


if (NCXX4_FOUND)
    if (NOT WIN32)
        execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --version OUTPUT_VARIABLE NCXX4_VERSION_TEXT OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --includedir OUTPUT_VARIABLE NCXX4_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --libdir OUTPUT_VARIABLE NCXX4_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --libs OUTPUT_VARIABLE NCXX4_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REPLACE " " ";" NCXX4_VERSION_LIST ${NCXX4_VERSION_TEXT})
        list(LENGTH NCXX4_VERSION_LIST NX_LEN)
        IF(NX_LEN GREATER_EQUAL 1)
            list(GET NCXX4_VERSION_LIST 1 NCXX4_VERSION)
        ENDIF()
    endif()


    if (WIN32)
        SET(NCXX4_LIBRARY "${NCXX4_CONFIG_PATH}/../lib/netcdf-cxx4.lib")
        SET(NCXX4_INCLUDE_DIR "${NCXX4_CONFIG_PATH}/../include")
        SET(NCXX4_VERSION "4.3.1")
    else()
        #SET(NCXX4_LIBRARY ${NCXX4_LIB_DIR}/libnetcdf-cxx4.so)
        SET(NCXX4_LIBRARY "netcdf-cxx4")
    endif()
endif()

#message(STATUS "NetCDF-cxx4 version: ${NCXX4_VERSION}")
#message(STATUS "NetCDF-cxx4 include dir: ${NCXX4_INCLUDE_DIR}")
#message(STATUS "NetCDF-cxx4 library dir: ${NCXX4_LIB_DIR}")
#message(STATUS "NetCDF-cxx4 libraries: ${NCXX4_LIBRARIES}")
#message(STATUS "NetCDF-cxx4 library: ${NCXX4_LIBRARY}")


