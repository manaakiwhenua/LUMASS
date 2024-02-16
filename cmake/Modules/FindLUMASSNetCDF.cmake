# =============
# FindNetCDF.cmake
# ==============
# this file is part of LUMASS and provided as is
# - no warranty whatsoever for this being even remotely useful in whichever shape or form!

# find parallel netcdf nc-config
find_path(NC_CONFIG_PATH NAMES nc-config
    HINTS
        $ENV{HOME}/garage/build/netcdf-c-4.9.2/install/bin
        /opt/netcdf-4.7.3/install/bin
        /opt/netcdf-4.8.1/install/bin
        /opt/netcdf-c-4.8.1/install/bin
        /opt/netcdf-bin/install/bin
    PATHS
        $ENV{HOME}/garage/build/netcdf-c-4.9.2
        /opt/netcdf-4.7.3
        /opt/netcdf-bin
        /opt/netcdf-4.8.1
        /opt/netcdf-c-4.8.1
        /usr/local
        /usr
    PATH_SUFFIXES
        install/bin
        bin
    DOC "Path to the netcdf-c config script 'nc-config'"
)

#set(NC_CONFIG_PATH ${NC_CONFIG_PATH} CACHE STRING "Please specify the path to the netcdf nc-config file." )

execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --version OUTPUT_VARIABLE NETCDF_VERSION_TEXT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --includedir OUTPUT_VARIABLE NETCDF_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --libdir OUTPUT_VARIABLE NETCDF_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${NC_CONFIG_PATH}/nc-config --libs OUTPUT_VARIABLE NETCDF_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE " " ";" NETCDF_VERSION_LIST "${NETCDF_VERSION_TEXT}")
list(LENGTH NETCDF_VERSION_LIST NC_LEN)
IF(NC_LEN GREATER_EQUAL 1)
    list(GET NETCDF_VERSION_LIST 1 NETCDF_VERSION)
ENDIF()

if(WIN32)
        SET(NETCDF_LIBRARY "C:/opt/netcdf-c-4.7.3/liblib/Release/netcdf.lib")
else()
    #if (LUMASS_MPI_SUPPORT)
        #SET(NETCDF_LIBRARY ${NETCDF_LIB_DIR}/libnetcdf_mpi.so)
        #SET(NETCDF_INCLUDE_DIR ${NETCDF_LIB_DIR}/netcdf/mpi/include)
        set(NETCDF_LIBRARY "netcdf_par")
    #else()
    #    SET(NETCDF_LIBRARY ${NETCDF_LIB_DIR}/libnetcdf.so)
    #endif()
endif()

#message(STATUS "NetCDF config path: ${NC_CONFIG_PATH}")
#message(STATUS "NetCDF version: ${NETCDF_VERSION}")
#message(STATUS "NETCDF_INCLUDE_DIR: ${NETCDF_INCLUDE_DIR}")
#message(STATUS "NetCDF library dir: ${NETCDF_LIB_DIR}")
#message(STATUS "NetCDF libraries: ${NETCDF_LIBRARIES}")
#message(STATUS "NetCDF library: ${NETCDF_LIBRARY}")

# find also netcdf-cxx4
find_path(NCXX4_CONFIG_PATH ncxx4-config
    HINTS
        $ENV{HOME}/garage/build/netcdf-cxx-4.3.1/install/bin
        /opt/ncxx4-dbg/install/bin
        /opt/netcdf-cxx4/install/bin
    PATHS
        $ENV{HOME}/garage/build/netcdf-cxx-4.3.1
        /opt/ncxx4-dbg
        /opt/netcdf-cxx4
        /usr/local
        /usr
    PATH_SUFFIXES
        bin
        install/bin
    DOC "Path to the netcdf-cxx4 config script 'ncxx4-config'"
)

#message(STATUS "NCXX4_CONFIG_PATH: ${NCXX4_CONFIG_PATH}")

#if (NOT NCXX4_CONFIG_PATH)
#    message(FATAL_ERROR "Please specify the path to the netcdf-cxx4 config script 'ncxx4-config'")
#else()
    execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --version OUTPUT_VARIABLE NCXX4_VERSION_TEXT OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --includedir OUTPUT_VARIABLE NCXX4_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --libdir OUTPUT_VARIABLE NCXX4_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${NCXX4_CONFIG_PATH}/ncxx4-config --libs OUTPUT_VARIABLE NCXX4_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
#endif()

string(REPLACE " " ";" NCXX4_VERSION_LIST ${NCXX4_VERSION_TEXT})
list(LENGTH NCXX4_VERSION_LIST NX_LEN)
IF(NX_LEN GREATER_EQUAL 1)
    list(GET NCXX4_VERSION_LIST 1 NCXX4_VERSION)
ENDIF()

if (WIN32)

else()
    #SET(NCXX4_LIBRARY ${NCXX4_LIB_DIR}/libnetcdf-cxx4.so)
    SET(NCXX4_LIBRARY "netcdf-cxx4")
endif()

#message(STATUS "NetCDF-cxx4 version: ${NCXX4_VERSION}")
#message(STATUS "NetCDF-cxx4 include dir: ${NCXX4_INCLUDE_DIR}")
#message(STATUS "NetCDF-cxx4 library dir: ${NCXX4_LIB_DIR}")
#message(STATUS "NetCDF-cxx4 libraries: ${NCXX4_LIBRARIES}")
#message(STATUS "NetCDF-cxx4 library: ${NCXX4_LIBRARY}")


