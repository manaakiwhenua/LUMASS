# This is the root CMakeLists.txt file for LUMASS
# author: Alexander Herzig
# copyright: Landcare Research New Zealand Ltd

cmake_minimum_required(VERSION 3.22)

# CMAKE
#SET(CMAKE_C_COMPILER "/usr/bin/clang-15")
#SET(CMAKE_CXX_COMPILER "/usr/bin/clang++-15")

PROJECT(lumass)

INCLUDE_REGULAR_EXPRESSION("^.*$")
SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_ROOT}/Modules
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

# ugly hack for now ... (2020-07-28)
if (WIN32)
        list(APPEND CMAKE_MODULE_PATH "C:/src/pybind11/tools")
endif()

######### DEBUG DEBUG DEBUG ########
#set(CMAKE_VERBOSE_MAKEFILE on)
####################################

INCLUDE(GetGitRevisionDescription)

# set some paths for binary output and install
SET(EXECUTABLE_OUTPUT_PATH ${lumass_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${lumass_BINARY_DIR}/lib)

# we're setting some more variables
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 14)

#====================================================================
# MPI
#====================================================================
OPTION(LUMASS_MPI_SUPPORT "Build LUMASS with MPI parallel processing support?" ON)

# this is still under development (like most of this file anyway)
# so won't bother with MPI on win for now ...

if (NOT WIN32 AND LUMASS_MPI_SUPPORT)
    find_package(MPI)
    if (MPI_CXX_FOUND)
        message(STATUS "MPI_CXX: found version: ${MPI_CXX_VERSION}")
        message(STATUS "MPI_CXX_INCLUDE_DIRS=${MPI_CXX_INCLUDE_DIRS}")
        message(STATUS "MPI_CXX_LIBRARIES=${MPI_CXX_LIBRARIES}")
        message(STATUS "MPI_CXX_COMPILER=${MPI_CXX_COMPILER}")
        message(STATUS "MPI_C_COMPILER=${MPI_C_COMPILER}")
    else ()
        set(MPI_CXX_INCLUDE_DIRS "MPI_CXX_INCLUDE_DIRS-NOTFOUND" CACHE STRING "Please specify the MPI-CXX include directories!")
        set(MPI_CXX_LIBRARIES "MPI_CXX_LIBRARIES-NOTFOUND" CACHE STRING "Please specify the MPI-CXX libraries!")
    endif()
else()
    set(MPI_CXX_INCLUDE_DIRS="")
    set(MPI_CXX_LIBRARIES="")
endif()

IF(NOT WIN32)
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC -fPIE")

        # this cmake variable doesn't seem to be honoured with this CMAKE version
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -fPIE")

        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g -fPIC -fno-aggressive-loop-optimizations")
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fPIC")
ELSE(NOT WIN32)
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT ")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oy /Oi /Ot ")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Ob2 /Oy /Oi /Ot ")
ENDIF(NOT WIN32)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        SET(CMAKE_INSTALL_PREFIX "/opt/lumass"
                CACHE PATH "LUMASS installation directory" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# check whether long is 64bit
include(CheckTypeSize)
CHECK_TYPE_SIZE(long SIZEOF_LONG LANGUAGE CXX)
CHECK_TYPE_SIZE("unsigned long" SIZEOF_ULONG LANGUAGE CXX)
message(STATUS "SIZEOF_LONG= ${SIZEOF_LONG}")
message(STATUS "SIZEOF_LONG_CODE= ${SIZEOF_LONG_CODE}")
CHECK_TYPE_SIZE("long long" SIZEOF_LONGLONG LANGUAGE CXX)
CHECK_TYPE_SIZE("unsigned long long" SIZEOF_ULONGLONG LANGUAGE CXX)
message(STATUS "SIZEOF_LONGLONG= ${SIZEOF_LONGLONG}")
message(STATUS "SIZEOF_LONGLONG_CODE= ${SIZEOF_LONGLONG_CODE}")

#==================================================================
# set a RPATH for the whole project
#==================================================================
# credit to: - http://www.cmake.org/Wiki/CMake_RPATH_handling
#            - http://cmake.3232098.n2.nabble.com/Shared-libraries-cannot-be-found-after-deploying-a-CPack-package-td3263935.html#a3264158

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

#SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# found in CMAKE users group/email list:
# sets the RPATH to be relative to the origin of the executable
# on MAC and Linux, rather than to a fixed directory specified
# at configure time!
set(CMAKE_INSTALL_NAME_DIR "@executable_prefix/../lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
#LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
#IF("${isSystemDir}" STREQUAL "-1")
#   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#ENDIF("${isSystemDir}" STREQUAL "-1")

#==================================================================
# THE DEFAULT BUILD TYPE
# credit to http://vtk.org/gitweb?p=VTK.git;a=blob;f=CMakeLists.txt;h=a57fd66ef333fc26bf74fe658c88c3c634f54c3a;hb=HEAD#l12
#==================================================================

if(NOT CMAKE_BUILD_TYPE)
        if (NOT WIN32)
                message(STATUS "Setting build type to 'Debug' as none was specified.")
                set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
        else ()
                message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
                set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
        endif()

        # Set the possible values of build type for cmake-gui
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
        "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "LUMASS build type: ${CMAKE_BUILD_TYPE}")

# define the DEBUG symbol, if we're in debug mode
string(TOLOWER "${CMAKE_BUILD_TYPE}" testvar_in)
set(testvar_target "debug")
if (testvar_in STREQUAL testvar_target)
        ADD_DEFINITIONS(-DLUMASS_DEBUG)
endif (testvar_in STREQUAL testvar_target)


#==================================================================
# LUMASS VERSION AND REVISION
#==================================================================

find_package(Git)
SET(LUMASS_VERSION_MAJOR 0)
SET(LUMASS_VERSION_MINOR 9)
SET(LUMASS_VERSION_REVISION 71)
SET(LUMASS_VERSION_SUFFIX "-b8")
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_date(LUMASS_COMMIT_DATE)


message(STATUS "building LUMASS ${LUMASS_VERSION_MAJOR}.${LUMASS_VERSION_MINOR}.${LUMASS_VERSION_REVISION} (${CMAKE_BUILD_TYPE})"
               " - commit ${GIT_SHA1} from ${LUMASS_COMMIT_DATE}")


SET(LUMASS_OPENGA_VERSION "1.0.5")

#====================================================================
# avoid 'missing Threads_FOUND' trouble ...
#====================================================================

#set(CMAKE_THREAD_LIBS_INIT "-lpthread")
#set(CMAKE_HAVE_THREADS_LIBRARY 1)
#set(CMAKE_USE_WIN32_THREADS_INIT 0)
#set(CMAKE_USE_PTHREADS_INIT 1)
#set(THREADS_PREFER_PTHREAD_FLAG ON)

#====================================================================
# QT 5
#====================================================================

FIND_PACKAGE(LUMASSQt5)
IF(NOT QT5_PREFIX_PATH)
        message(FATAL_ERROR "Need to specify Qt5 install DIR!")
ENDIF()
#if (${QT5_VERSION_STRING} VERSION_LESS "5.7")
#    message(FATAL_ERROR "LUMASS >= 0.9.60 requires Qt >= 5.7")
#endif ()

#====================================================================
# OpenGL
#====================================================================

if (${CMAKE_VERSION} VERSION_GREATER "3.11.0")
    cmake_policy(SET CMP0072 NEW)
    SET(OpenGL_GL_PREFERENCE "LEGACY")
endif()

FIND_PACKAGE(OpenGL)

if(NOT OPENGL_INCLUDE_DIR)
  if(WIN32)
        set(OPENGL_INCLUDE_DIR "C:/Program Files/Microsoft SDKs/Windows/v7.1/Include")
  else()
        message(FATAL_ERROR "LUMASS requires OpenGL's GL/glu.h !")
  endif()
endif()

message(STATUS "OpenGL's include dir: ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL's GL library:  ${OPENGL_gl_LIBRARY}")
message(STATUS "OpenGL's GLU library: ${OPENGL_glu_LIBRARY}")

#====================================================================
# Find boost
#====================================================================

FIND_PACKAGE(Boost COMPONENTS filesystem serialization)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Please install the Boost::filesystem library!")
else()
    message(STATUS "Boost version: " ${Boost_VERSION_STRING})
    message(STATUS "Boost LIB DIRS: " ${Boost_LIBRARY_DIRS})
endif()

#====================================================================
# Orfeo Toolbox
#====================================================================

# if on windows, ask for the OSGEO4W_ROOT PATH
if(WIN32)
    FIND_PACKAGE(LUMASSOSGeo4W)
    if(NOT OSGEO4W_ROOT)
            message(FATAL_ERROR "Specify the OSGeo4W installation directory!")
    endif()
endif()

IF(NOT OTB_DIR)
    FIND_PACKAGE(LUMASSOTB)
    if(NOT OTB_FOUND)
      message(FATAL_ERROR "You have to install Orfeo Toolbox prior to compiling LUMASS!")
    endif(NOT OTB_FOUND)
ELSE()
    include(${OTB_DIR}/OTBConfig.cmake)
ENDIF()

SET(LUMASS_ITK_VERSION "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH}")
SET(LUMASS_OTB_VERSION "${OTB_VERSION_MAJOR}.${OTB_VERSION_MINOR}.${OTB_VERSION_PATCH}")
SET(LUMASS_OTB_LIB_VERSION "${OTB_VERSION_MAJOR}.${OTB_VERSION_MINOR}")
if (LUMASS_OTB_VERSION VERSION_LESS "6.4")
    message(FATAL_ERROR "LUMASS >= 0.9.60 requires OTB >= 6.4")
else()
    add_definitions(-DOTB_VERSION_SIX)
endif ()

message(STATUS "ITK_VERSION: ${LUMASS_ITK_VERSION}")
#message(STATUS "ITK_LIBRARY_DIRS: ${ITK_LIBRARY_DIRS}")
#message(STATUS "ITK_INCLUDE_DIRS: ${ITK_INCLUDE_DIRS}")
#message(STATUS "ITK_LIBRARIES: ${ITK_LIBRARIES}")
#message(STATUS "ITK_LINK_DIR: ${ITK_LINK_DIR}")
message(STATUS "OTB_VERSION: ${LUMASS_OTB_VERSION}")
message(STATUS "OTB_LIBRARY_DIRS: ${OTB_LIBRARY_DIRS}")
message(STATUS "OTB_INCLUDE_DIRS: ${OTB_INCLUDE_DIRS}")
#message(STATUS "OTB_LIBRARIES: ${OTB_LIBRARIES}")
message(STATUS "OTB_LINK_DIR: ${OTB_LINK_DIR}")

#====================================================================
# VTK
#====================================================================

# here we just make sure we've got a proper VTK version
# available; set up specifics later with respect to the
# different executables

FIND_PACKAGE(LUMASSVTK)

if(NOT VTK_DIR)
    set(LUMASS_VTK_DIR "LUMASS_VTK_DIR NOT FOUND" CACHE STRING "VTK directory (to vtk-config.cmake)")
    message(FATAL_ERROR "Couldn't find VTK!")
else()
    set(LUMASS_VTK_DIR ${VTK_DIR} CACHE STRING "VTK directory (to vtk-config.cmake)")
    message(STATUS "LUMASS_VTK_DIR set to ${LUMASS_VTK_DIR}")
    find_package(VTK REQUIRED PATHS ${LUMASS_VTK_DIR} NO_DEFAULT_PATH)
    #include("${LUMASS_VTK_DIR}/vtk-config.cmake"})

#    if (EXISTS "${VTK_DIR}/vtk-config.cmake")
#        include("${VTK_DIR}/vtk-config.cmake")
        set(VTK_VERSION_STRING "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
        message(STATUS "found VTK ${VTK_VERSION_STRING} at ${VTK_DIR}")
        #message(STATUS "found VTK Components: ${VTK_AVAILABLE_COMPONENTS}")

#        if (${VTK_VERSION_STRING} VERSION_LESS "8.2")
#            message(FATAL_ERROR "LUMASS >= 0.9.60 requires VTK >= 8.2")
#        endif ()
#    endif()
endif()

#find_package(VTK NO_MODULE)

#====================================================================
# GDAL SUPPORT - always required otherwise we pull out
#====================================================================

FIND_PACKAGE(LUMASSGDAL)
IF(NOT GDAL_FOUND)
  message(FATAL_ERROR "You have to install GDAL prior to compiling LUMASS!")
ENDIF()

IF(NOT GDAL_VERSION)
    set(GDAL_VERSION "1.11.0" CACHE STRING "GDAL version")
    message(FATAL_ERROR "Please specify the GDAL version")
ENDIF()

IF(NOT GDAL_VERSION VERSION_LESS "1.11")
    ADD_DEFINITIONS(-DGDAL_NEWRATAPI)
ENDIF()
IF(NOT GDAL_VERSION VERSION_LESS "2.0.0")
    ADD_DEFINITIONS(-DGDAL_200)
ENDIF()


message(STATUS "GDAL version: ${GDAL_VERSION}")
message(STATUS "GDAL lib: ${GDAL_LIBRARY}")
message(STATUS "gdal include dir: ${GDAL_INCLUDE_DIR}")


#====================================================================
# FIND LPSOLVE
#====================================================================

FIND_PACKAGE(Lpsolve)
IF(NOT LPSOLVE_FOUND)
  message(FATAL_ERROR "You have to install lp_solve prior to compiling LUMASS!")
#ELSE()
  #add_definitions(-DSQLITE_THREADSAFE=1)
ENDIF()

message(STATUS "lp_solve include dir: ${LPSOLVE_INCLUDE_DIR}")
message(STATUS "lp_solve library: ${LPSOLVE_LIBRARY}")

#====================================================================
# FIND SQLite3 / Spatialite
#====================================================================

# just define the current version included in the source tree
set(NM_SQLite_VERSION "3.42.0")

#find_package(Sqlite3)
#if(NOT SQLITE3_FOUND)
#  message(FATAL_ERROR "You have to install SQLite3 prior to compiling LUMASS!")
#endif()

#message(STATUS "SQLite3 include dir: ${SQLITE3_INCLUDE_DIR}")
#message(STATUS "SQLite3 library: ${SQLITE3_LIBRARY}")

# loof for spatialite
find_package(LUMASSSpatialite)
if(NOT SPATIALITE_FOUND)
        message(FATAL_ERROR "Please install Spatialite prior to installing LUMASS!")
endif()

message(STATUS "Spatialite version: ${SPATIALITE_VERSION}")
message(STATUS "Spatialite include dir: ${SPATIALITE_INCLUDE_DIR}")
message(STATUS "Spatialite library dir: ${SPATIALITE_LIBLIB_DIR}")
message(STATUS "Spatialite library: ${SPATIALITE_LIB}")

#====================================================================
# FIND netCDF CXX
#====================================================================

if (WIN32)
        #if (NOT NETCDF_BIN_DIR)
                #	set(NETCDF_BIN_DIR "c:/opt/netcdf-c-4.7.3" CACHE PATH "Please set the netcdf binary dir!")
                #endif()
                #
                #include(${NETCDF_BIN_DIR}/netCDFConfig.cmake)
                #
                #message(STATUS "netCDF-c version: ${NetCDFVersion}")

        if (NOT NETCDF_INCLUDE_DIR OR NOT NETCDF_LIB_DIR)
                set(NETCDF_INCLUDE_DIR "C:/src/netcdf-c-4.7.3/include" CACHE PATH "Please check the netCDF include DIR!")
                set(NETCDF_LIB_DIR "C:/opt/netcdf-c-4.7.3/liblib/Release" CACHE PATH "Please check the netCDF lib DIR!")
                set(NETCDF_LIBRARY "C:/opt/netcdf-c-4.7.3/liblib/Release/netcdf.lib" CACHE PATH "Please check the path to the netCDF library!")
        endif()



        if (NOT NCXX4_INCLUDE_DIR OR NOT NCXX4_LIBRARY)
                set(NCXX4_INCLUDE_DIR "C:/src/netcdf-cxx4-4.3.1/cxx4" CACHE PATH "Please check the netCDF include DIR!")
                set(NCXX4_LIB_DIR "C:/opt/netcdf-cxx4-4.3.1/cxx4/Release" CACHE PATH "Please check the netCDF lib DIR!")
                SET(NCXX4_LIBRARY "C:/opt/netcdf-cxx4-4.3.1/cxx4/Release/netcdf-cxx4.lib" CACHE PATH "Please check the path to the ncxx library!")
        endif()

                SET(LUMASS_NETCDF_VERSION "4.7.3")
                SET(LUMASS_NCXX4_VERSION "4.3.1")

else()

    find_package(LUMASSNetCDF)

    if(NOT NC_CONFIG_PATH)
        set(NC_CONFIG_PATH "" CACHE STRING "Please specify the path to the netcdf nc-config file." )
        message(FATAL_ERROR "Couldn't find netcdf configuration info!")
    else()
        set(NC_CONFIG_PATH ${NC_CONFIG_PATH} CACHE STRING "Path to the netcdf nc-config file." )
    endif()

    if (NOT NCXX4_CONFIG_PATH)
        set(NCXX4_CONFIG_PATH "" CACHE STRING "Please specify the path to the netcdf-cxx4 ncxx4-config file.")
    else()
        set(NCXX4_CONFIG_PATH ${NCXX4_CONFIG_PATH} CACHE STRING "Path to the ncxx4-config file.")
    endif()

    SET(LUMASS_NETCDF_VERSION ${NETCDF_VERSION})
    SET(LUMASS_NCXX4_VERSION ${NCXX4_VERSION})

    find_package(HDF5 COMPONENTS C HL REQUIRED)

endif()

message(STATUS "NetCDF-C version: ${LUMASS_NETCDF_VERSION}")
message(STATUS "NETCDF_INCLUDE_DIR=${NETCDF_INCLUDE_DIR}")
message(STATUS "NETCDF_LIB_DIR=${NETCDF_LIB_DIR}")
message(STATUS "NETCDF_LIBRARY=${NETCDF_LIBRARY}")
message(STATUS "NetCDF-CXX4 version: ${LUMASS_NCXX4_VERSION}")
message(STATUS "NCXX4_INCLUDE_DIR=${NCXX4_INCLUDE_DIR}")
message(STATUS "NCXX4_LIB_DIR=${NCXX4_LIB_DIR}")
message(STATUS "NCXX4_LIBRARY=${NCXX4_LIBRARY}")

if(HDF5_FOUND)
  message(STATUS "HDF5_VERSION=${HDF5_VERSION}")
  message(STATUS "HDF5_INCLUDE_DIRS=${HDF5_INCLUDE_DIRS}")
  message(STATUS "HDF5_LIBRARIES=${HDF5_LIBRARIES}")
endif()

#====================================================================
# FIND PYBIND11
#====================================================================

OPTION(PYTHONSUPPORT "Build LUMASS with Python support?" ON )

if (PYTHONSUPPORT)

        find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)

        message(STATUS "Python3_FOUND: ${Python3_FOUND}")
        message(STATUS "Python3_Interpreter_FOUND: ${Python3_Interpreter_FOUND}")
        message(STATUS "Python3_Interpreter_ID: ${Python3_Interpreter_ID}")
        message(STATUS "Python3_Executable: ${Python3_EXECUTABLE}")
        message(STATUS "Python3_Compiler_FOUND: ${Python3_Compiler_FOUND}")
        message(STATUS "Python3_Development_FOUND: ${Python3_Development_FOUND}")
        message(STATUS "Python3_Development.Embed_FOUND: ${Python3_Development.Embed_FOUND}")
        message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
        message(STATUS "Python3_LIBRARY_DIRS: ${Python3_LIBRARY_DIRS}")
        message(STATUS "Python3_LIBRARIES: ${Python3_LIBRARIES}")
        message(STATUS "Python3 Version: ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}.${Python3_VERSION_PATCH}")
        message(STATUS "Python3 Numpy Version: ${Python3_NumPy_VERSION}")

    find_package(pybind11 CONFIG REQUIRED)

        ##find_path(pybind11_DIR pybind11Config.cmake
    ##    HINTS
    ##        /opt/pybind11-bin/install/share/cmake/pybind11
    ##    PATHS
    ##        /opt/pybind11-bin/install
    ##        /usr/lib/x86_64-linux/cmake
    ##        /usr/local/lib
    ##        /usr
    ##        /usr/local
    ##    PATH_SUFFIXES
    ##        share/cmake/pybind11
    ##        cmake/pybind11
    ##        pybind11
    ##)
    ##message(STATUS "pybind11_DIR=${pybind11_DIR}")
        ##
    ##if (pybind11_DIR-NOTFOUND)
    ##    set(pybind11_DIR "NOT_FOUND" CACHE PATH "Please specify the path to pybind11Config.cmake !")
    ##    message(FATAL_ERROR "Please specify the path to pybind11Config.cmake")
    ##endif() #else ()
        ##
    ##    include(${pybind11_DIR}/pybind11Config.cmake)
    ##    include(${pybind11_DIR}/pybind11ConfigVersion.cmake)
    ##    SET(LUMASS_pybind11_version "${PACKAGE_VERSION}")
    ##    message(STATUS "pybind11_VERSION=${PACKAGE_VERSION}")
    ##    message(STATUS "pybind11_INCLUDE_DIR=${pybind11_INCLUDE_DIR}")
    ##    message(STATUS "pybind11_INCLUDE_DIRS=${pybind11_INCLUDE_DIRS}")
    ##    message(STATUS "pybind11_LIBRARIES=${pybind11_LIBRARIES}")
    ##    message(STATUS "pybind11_DEFINITIONS=${pybind11_DEFINITIONS}")
endif(PYTHONSUPPORT)


#====================================================================
# FIND TOKYO CABINET
#====================================================================

#find_package(TokyoCabinet)
#if(NOT TCADB_FOUND)
#  message(FATAL_ERROR "You have to install Tokyo Cabient prior to compiling LUMASS!")
#endif()

#message(STATUS "Tokyo Cabinet include dir: ${TCADB_INCLUDE_DIR}")
#message(STATUS "Tokyo Cabinet library dir: ${TCADB_LIB_DIR}")

#====================================================================
# ask for YAML library and header dir
#====================================================================
# the yaml-cpp-config.cmake does not work properly, so we make do without it for now
#find_package(yaml-cpp
#    HINTS
#        /usr/lib/x86_64-linux-gnu/cmake/yaml-cpp/yaml-cpp-config.cmake
#                c:/opt/yaml-cpp/yaml-cpp-config.cmake
#    PATHS
#        /usr/lib/x86_64-linux-gnu/cmake
#        /usr/lib/cmake
#            c:/opt/yaml-cpp
#)

if (NOT YAML_CPP_CMAKE_DIR)
        set(YAML_CPP_CMAKE_DIR "YAML_CPP_CMAKE_DIR-NOTFOUND" CACHE STRING "Please specify the path of yaml-cpp-config.cmake" )
        message(FATAL_ERROR "Please specify the path of yaml-cpp-config.cmake!" )
else()
        include(${YAML_CPP_CMAKE_DIR}/yaml-cpp-config-version.cmake)
        SET(LUMASS_yaml_version "${PACKAGE_VERSION}")
        set(YAML_CPP_LIBRARIES "yaml-cpp")
        message(STATUS "yaml-cpp version: ${LUMASS_yaml_version}")
endif()

IF(NOT YAML_CPP_INCLUDE_DIR)
    set(YAML_CPP_INCLUDE_DIR "YAML_CPP_INCLUDE_DIR-NOTFOUND" CACHE STRING "Location of yaml-cpp include directory" )
    message(FATAL_ERROR "Please specify the YAML_CPP_INCLUDE_DIR!" )
ENDIF()

#IF(NOT YAML_CPP_LIB_DIR)
#    set(YAML_CPP_LIB_DIR "YAML_CPP_LIB_DIR" CACHE STRING "Location of yaml-cpp library directory" )
#    message(FATAL_ERROR "Please specify the YAML_CPP_LIB_DIR!" )
#else()
#    FIND_LIBRARY(YAML_CPP_LIBRARIES libyaml-cpp.so ${YAML_CPP_LIB_DIR} NO_DEFAULT_PATH)
#    if (NOT YAML_CPP_LIBRARIES)
#        FIND_LIBRARY(YAML_CPP_LIBRARIES libyaml-cppd.so ${YAML_CPP_LIB_DIR} NO_DEFAULT_PATH)
#    endif()
#ENDIF()


#find_package(yaml-cpp PATHS ${yaml-cpp_DIR} NO_DEFAULT_PATH)
#if (NOT ${yaml-cpp_DIR} )
#    message(FATAL_ERROR "Please specify the directory containing yaml-cpp-config.cmake" )
#endif()
#find_package(yaml-cpp)
#include(/opt/yaml-debug/install/share/cmake/yaml-cpp/yaml-cpp-config.cmake)
message(STATUS "YAML_CPP_VERSION=${LUMASS_yaml_version}")
message(STATUS "YAML_CPP_INCLUDE_DIR=${YAML_CPP_INCLUDE_DIR}")
message(STATUS "YAML_CPP_LIBRARIES=${YAML_CPP_LIBRARIES}")

#IF(NOT YAML_HEADERDIR)
#    set(YAML_HEADERDIR "/opt/yaml-cpp/install/include" CACHE STRING "Path to yaml header directory")
#    message(FATAL_ERROR "Please specify the yaml header dir!")
#ENDIF()

#====================================================================
# RASDAMAN SUPPORT - optional (requires PostgreSQL support)
#====================================================================

# rasdaman is only available on linux
IF(UNIX AND NOT APPLE)
    OPTION(RASSUPPORT "Build lumass with rasdaman support?" OFF )
    IF(RASSUPPORT)
      FIND_PACKAGE(Rasdaman)
      IF(RASDAMAN_FOUND)
         message(STATUS "rasdaman source dir: ${RASDAMAN_SOURCE_DIR}")
         message(STATUS "rasdaman libraries dir: ${RASDAMAN_LIBRARIES_DIR}")
      ENDIF()

      # when we use rasdaman, we need postgres as well!
      FIND_PACKAGE(LUMASSPostgreSQL)
      IF(PostgreSQL_FOUND)
         message(STATUS "postgres include dirs: ${PostgreSQL_INCLUDE_DIRS}")
         message(STATUS "postgres libraries dirs: ${PostgreSQL_LIBRARY_DIRS}")
      ENDIF()

      IF(PostgreSQL_FOUND AND RASDAMAN_FOUND)
        OPTION(RASDEBUG "Is rasdaman running in debug mode?" OFF)
        IF(RASDEBUG)
            ADD_DEFINITIONS(-DRMANDEBUG)
        ENDIF(RASDEBUG)

        ADD_DEFINITIONS(-DBUILD_RASSUPPORT)
        message(STATUS "building with rasdaman support and define BUILD_RASSUPPORT")
      ELSE(PostgreSQL_FOUND AND RASDAMAN_FOUND)
        set(RASSUPPORT OFF)
        message(STATUS "building without rasdaman support because either rasdaman
             or PostgreSQL support is mising!")
      ENDIF(PostgreSQL_FOUND AND RASDAMAN_FOUND)
    ENDIF()

    message(STATUS "rasdaman support: ${RASSUPPORT}")
    message(STATUS "rasdaman source dir: ${RASDAMAN_SOURCE_DIR}")
    message(STATUS "rasdaman include dir: ${RASDAMAN_INCLUDE_DIR}")
    message(STATUS "rasdaman libraries dir: ${RASDAMAN_LIBRARIES_DIR}")
ENDIF()

#====================================================================
# THE RIGHT TIME TO POPULATE THE LUMASSConfig.h FILE AND others
#====================================================================

configure_file("${CMAKE_SOURCE_DIR}/LUMASSConfig.h.in"
               "${lumass_BINARY_DIR}/LUMASSConfig.h" @ONLY)

configure_file("${CMAKE_SOURCE_DIR}/shared/nmtypeinfo.h.in"
               "${lumass_BINARY_DIR}/shared/nmtypeinfo.h" @ONLY)


configure_file("${CMAKE_SOURCE_DIR}/docs/lumassDocConfig.in"
              "${CMAKE_SOURCE_DIR}/docs/lumassDocConfig" @ONLY)

#====================================================================
# SOME OTHER STUFF WE WANNA USE GLOBALLY
#====================================================================

SET(MFW_WRAPPER_NAMES
    NMBMIWrapper
    NMMosraFilterWrapper
    NMDEMSlopeAspectFilterWrapper
    NMFlowAccumulationFilterWrapper
    NMItkCastImageFilterWrapper
    NMRandomImageSourceWrapper
    NMResampleImageFilterWrapper
    NMVectorImageToImageFilterWrapper
    NMSumZonesFilterWrapper
    NMStreamingImageFileWriterWrapper
    NMCostDistanceBufferImageWrapper
    NMRATBandMathImageFilterWrapper
    NMScriptableKernelFilter2Wrapper
    NMJSKernelFilterWrapper
    NMExternalExecWrapper
    NMSQLiteProcessorWrapper
    #NMUniqueCombinationFilterWrapper
    NMStreamingROIImageFilterWrapper
    NMImage2DToCubeSliceFilterWrapper
    NMCubeSliceToImage2DFilterWrapper
    NMImage2TableFilterWrapper
    NMTable2NetCDFFilterWrapper
)

SET(OTB_LINK_LIBS
    OTBCommon
    OTBExtendedFilename
    OTBImageBase
    OTBImageIO
    OTBIOGDAL
    OTBOSSIMAdapters
    otbossimplugins
    #OTBStreaming
)


#====================================================================
# ADD SUBDIRECTORIES
#====================================================================

ADD_SUBDIRECTORY(shared ${lumass_BINARY_DIR}/shared)
ADD_SUBDIRECTORY(utils ${lumass_BINARY_DIR}/utils)
ADD_SUBDIRECTORY(otbsuppl ${lumass_BINARY_DIR}/otbsuppl)
ADD_SUBDIRECTORY(modellingframework/core ${lumass_BINARY_DIR}/modellingframework/core)
ADD_SUBDIRECTORY(optimisation ${lumass_BINARY_DIR}/optimisation)
ADD_SUBDIRECTORY(modellingframework/wrapper ${lumass_BINARY_DIR}/modellingframework/wrapper)
ADD_SUBDIRECTORY(gui ${lumass_BINARY_DIR}/gui)
ADD_SUBDIRECTORY(terminalapp ${lumass_BINARY_DIR}/terminalapp)
ADD_SUBDIRECTORY(bmi ${lumass_BINARY_DIR}/bmi)


#====================================================================
# Packaging
#====================================================================

IF (UNIX AND NOT APPLE)

    set(CPACK_PACKAGE_NAME "lumass")
    set(CPACK_PACKAGE_VERSION "${LUMASS_VERSION_MAJOR}.${LUMASS_VERSION_MINOR}.${LUMASS_VERSION_REVISION}-0ubuntu1")
    set(CPACK_PACKAGE_VENDOR "Manaaki Whenua - Landcare Research")
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    #set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

    set(CPACK_RESOURCE_FILE_LICENSE "${lumass_SOURCE_DIR}/LICENCE")
    set(CPACK_RESOURCE_FILE_README "${lumass_SOURCE_DIR}/README.md")
    set(CPACK_DEBIAN_DESCRIPTION "LUMASS - spatial modelling & optimisation framework")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alex Herzig")
    set(CPACK_DEBIAN_PACKAGE_CONTACT "herziga@landcareresearch.co.nz")

    set(CPACK_GENERATOR "DEB")
    include(CPack)

ENDIF()
