# This is the root CMakeLists.txt file for LUMASS
# author: Alexander Herzig
# copyright: Landcare Research New Zealand Ltd

cmake_minimum_required(VERSION 2.8.9)
cmake_policy(SET CMP0003 NEW)
# from version 2.8.11 onwards
if(NOT ${CMAKE_VERSION} VERSION_LESS "2.8.11")
	cmake_policy(SET CMP0020 NEW)
endif()
# from version 2.8.12 onwards
if(NOT ${CMAKE_VERSION} VERSION_LESS "2.8.13")
	cmake_policy(SET CMP0045 OLD)
endif()

PROJECT(lumass)

INCLUDE_REGULAR_EXPRESSION("^.*$")
SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_ROOT}/Modules
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

######### DEBUG DEBUG DEBUG ########
#set(CMAKE_VERBOSE_MAKEFILE on)
####################################

INCLUDE(GetGitRevisionDescription)

# set some paths for binary output and install
SET(EXECUTABLE_OUTPUT_PATH ${lumass_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${lumass_BINARY_DIR}/lib)

# we're setting some more variables
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(NOT WIN32)
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC -fPIE")

	# this cmake variable doesn't seem to be honoured with this CMAKE version
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -fPIE")

        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g -fPIC")
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fPIC")
ENDIF(NOT WIN32)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	SET(CMAKE_INSTALL_PREFIX "/opt/lumass" 
		CACHE PATH "LUMASS installation directory" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

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

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "LUMASS build type: ${CMAKE_BUILD_TYPE}")

# define the DEBUG symbol, if we're in debug mode
string(TOLOWER "${CMAKE_BUILD_TYPE}" testvar_in)
set(testvar_target "debug")
if (testvar_in STREQUAL testvar_target)
        ADD_DEFINITIONS(-DDEBUG)
endif (testvar_in STREQUAL testvar_target)


#==================================================================
# LUMASS VERSION AND REVISION 
#==================================================================

find_package(Git)
SET(LUMASS_VERSION_MAJOR 0)
SET(LUMASS_VERSION_MINOR 9)
SET(LUMASS_VERSION_REVISION 41)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_date(LUMASS_COMMIT_DATE)


message(STATUS "building LUMASS ${LUMASS_VERSION_MAJOR}.${LUMASS_VERSION_MINOR}.${LUMASS_VERSION_REVISION}"
               " - commit ${GIT_SHA1} from ${LUMASS_COMMIT_DATE}")               

#====================================================================
# QT 5
#====================================================================

FIND_PACKAGE(LUMASSQt5)
IF(NOT CMAKE_PREFIX_PATH)
	message(FATAL_ERROR "Need to specify Qt5 install DIR!")
ENDIF()

#====================================================================
# OpenGL 
#====================================================================

#FIND_PACKAGE(LUMASSOpenGL)
FIND_PACKAGE(OpenGL)

if(NOT OPENGL_INCLUDE_DIR)
  if(WIN32)
	set(OPENGL_INCLUDE_DIR "C:/Program Files/Microsoft SDKs/Windows/v7.1/Include")
  else()
	message(FATAL_ERROR "LUMASS requires OpenGL's GL/glu.h !")
  endif()
endif()

message(STATUS "OpenGL's include dir: ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL's GLU library: ${OPENGL_glu_LIBRARY}")

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

FIND_PACKAGE(LUMASSOTB)  
INCLUDE(${OTB_USE_FILE})
if(NOT OTB_FOUND)
  message(FATAL_ERROR "You have to install Orfeo Toolbox prior to compiling LUMASS!")
endif(NOT OTB_FOUND)

SET(LUMASS_OTB_VERSION "${OTB_VERSION_MAJOR}.${OTB_VERSION_MINOR}.${OTB_VERSION_PATCH}")
message(STATUS "found OTB ${LUMASS_OTB_VERSION}")

#====================================================================
# VTK 
#====================================================================

# here we just make sure we've got a proper VTK version
# available; set up specifics later with respect to the
# different executables

FIND_PACKAGE(LUMASSVTK)
#FIND_PACKAGE(VTK)

if(NOT VTK_DIR)
    message(FATAL_ERROR "Couldn't find VTK!")
endif()

find_package(VTK NO_MODULE)
if(VTK_MAJOR_VERSION LESS 6)
  message(FATAL_ERROR "LUMASS requires VTK 6 or greater!")
endif()

set(VTK_VERSION_STRING "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

message(STATUS "found VTK ${VTK_VERSION_STRING} at ${VTK_DIR}")

#====================================================================
# GDAL SUPPORT - always required otherwise we pull out
#====================================================================

FIND_PACKAGE(GDAL)
IF(NOT GDAL_FOUND)
  message(FATAL_ERROR "You have to install GDAL prior to compiling LUMASS!")
ENDIF()

IF(GDAL_CONFIG)
    EXEC_PROGRAM(${GDAL_CONFIG} ARGS --version OUTPUT_VARIABLE GDAL_VERSION)
    IF(NOT GDAL_VERSION VERSION_LESS "1.11")
        ADD_DEFINITIONS(-DGDAL_NEWRATAPI)
    ENDIF()
ELSE()
    OPTION(NEW_GDAL_API "GDAL version is greater or equal to 1.11?" ON)
    IF(NEW_GDAL_API)
        ADD_DEFINITIONS(-DGDAL_NEWRATAPI)
    ENDIF()
ENDIF()


message(STATUS "gdal version: ${GDAL_VERSION}")
message(STATUS "gdal lib: ${GDAL_LIBRARY}")
message(STATUS "gdal include dir: ${GDAL_INCLUDE_DIR}")


#====================================================================
# FIND LPSOLVE 
#====================================================================

FIND_PACKAGE(Lpsolve)
IF(NOT LPSOLVE_FOUND)
  message(FATAL_ERROR "You have to install lp_solve prior to compiling LUMASS!")
ELSE()
  add_definitions(-DSQLITE_THREADSAFE=1)
ENDIF()

message(STATUS "lp_solve include dir: ${LPSOLVE_INCLUDE_DIR}")
message(STATUS "lp_solve library: ${LPSOLVE_LIBRARY}")

#====================================================================
# FIND SQLite3 / Spatialite
#====================================================================

# just define the current version included in the source tree
set(NM_SQLite_VERSION "3.8.11.1")

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

message(STATUS "Spatialite include dir: ${SPATIALITE_INCLUDE_DIR}")
message(STATUS "Spatialite library: ${SPATIALITE_LIB}")

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
# RASDAMAN SUPPORT - optional (requires PostgreSQL support)
#====================================================================

# rasdaman is only available on linux
IF(UNIX)
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
ENDIF(UNIX)

#====================================================================
# THE RIGHT TIME TO POPULATE THE LUMASSConfig.h FILE
#====================================================================

configure_file("${CMAKE_SOURCE_DIR}/LUMASSConfig.h.in"
               "${lumass_BINARY_DIR}/LUMASSConfig.h" @ONLY)

#====================================================================
# SOME OTHER STUFF WE WANNA USE GLOBALLY
#====================================================================

add_definitions(-DWno-deprecated)

#====================================================================
# ADD SUBDIRECTORIES
#====================================================================

ADD_SUBDIRECTORY(shared ${lumass_BINARY_DIR}/shared)
ADD_SUBDIRECTORY(otbsuppl ${lumass_BINARY_DIR}/otbsuppl)
ADD_SUBDIRECTORY(utils ${lumass_BINARY_DIR}/utils)
ADD_SUBDIRECTORY(optimisation ${lumass_BINARY_DIR}/optimisation)
ADD_SUBDIRECTORY(modellingframework ${lumass_BINARY_DIR}/modellingframework)
ADD_SUBDIRECTORY(gui ${lumass_BINARY_DIR}/gui)
ADD_SUBDIRECTORY(terminalapp ${lumass_BINARY_DIR}/terminalapp)


