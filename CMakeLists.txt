# This is the root CMakeLists.txt file for LUMASS
# author: Alexander Herzig
# copyright: Landcare Research New Zealand Ltd

PROJECT(lumass)
INCLUDE_REGULAR_EXPRESSION("^.*$")

cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
          
# set some paths for binary output and install
SET(EXECUTABLE_OUTPUT_PATH ${lumass_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${lumass_BINARY_DIR}/lib)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	SET(CMAKE_INSTALL_PREFIX "/opt/lumass" 
		CACHE PATH "LUMASS installation directory" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


#==================================================================
# THE DEFAULT BUILD TYPE - credits to http://vtk.org/gitweb?p=VTK.git;a=blob;f=CMakeLists.txt;h=a57fd66ef333fc26bf74fe658c88c3c634f54c3a;hb=HEAD#l12 
#==================================================================

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()   

# define the DEBUG symbol, if we're in debug mode
string(TOLOWER "${CMAKE_BUILD_TYPE}" testvar_in)
set(testvar_target "debug")         
if (testvar_in STREQUAL testvar_target)
	ADD_DEFINITIONS(-DDEBUG)
endif (testvar_in STREQUAL testvar_target)

#====================================================================
# QT 4 
#====================================================================

FIND_PACKAGE(Qt4 4.7 COMPONENTS QtCore QtGui QtXml REQUIRED)
INCLUDE(${QT_USE_FILE})

#====================================================================
# OpenGL 
#====================================================================

FIND_PACKAGE(OpenGL)
if(NOT OPENGL_GLU_FOUND)
  message(FATAL_ERROR "LUMASS requires OpenGL's GL/glu.h !")
endif(NOT OPENGL_GLU_FOUND)

message(STATUS "OpenGL's include dir: "${OPENGL_INCLUDE_DIR})
message(STATUS "OpenGL's GLU library: "${OPENGL_glu_LIBRARY})

#====================================================================
# Orfeo Toolbox 
#====================================================================

FIND_PACKAGE(LUMASSOTB)  
INCLUDE(${OTB_USE_FILE})
if (NOT OTB_FOUND)
  message(FATAL_ERROR "You have to install Orfeo Toolbox prior to compiling LUMASS!")
endif(NOT OTB_FOUND)

#====================================================================
# VTK 
#====================================================================

FIND_PACKAGE(LUMASSVTK)
if(NOT VTK_FOUND)
  message(FATAL_ERROR "You have to install VTK prior to compiling LUMASS!")
else()
  INCLUDE(${VTK_DIR}/VTKConfig.cmake)
endif()

message(STATUS "vtk include dirs: "${VTK_INCLUDE_DIRS})
message(STATUS "vtk dir: "${VTK_DIR})

#====================================================================
# GDAL SUPPORT - always required otherwise we pull out
#====================================================================

FIND_PACKAGE(GDAL)
IF (NOT GDAL_FOUND)
  message(FATAL_ERROR "You have to install GDAL prior to compiling LUMASS!")
ENDIF()

message(STATUS "gdal lib: "${GDAL_LIBRARY})
message(STATUS "gdal include dir: "${GDAL_INCLUDE_DIR})


#====================================================================
# FIND LPSOLVE 
#====================================================================

FIND_PACKAGE(Lpsolve)
IF (NOT LPSOLVE_FOUND)
  message(FATAL_ERROR "You have to install lp_solve prior to compiling LUMASS!")
ENDIF()

message(STATUS "lp_solve include dir: "${LPSOLVE_INCLUDE_DIR})
message(STATUS "lp_solve library: "${LPSOLVE_LIBRARY})

#====================================================================
# RASDAMAN SUPPORT - optional (requires PostgreSQL support)
#====================================================================

OPTION(RASSUPPORT "Build lumass with rasdaman support?" ON )
IF(RASSUPPORT)
  FIND_PACKAGE(Rasdaman)
  IF (RASDAMAN_FOUND)            
     message(STATUS "rasdaman source dir: "${RASDAMAN_SOURCE_DIR})
     message(STATUS "rasdaman libraries dir: "${RASDAMAN_LIBRARIES_DIR})
  ENDIF ()
  
  # when we use rasdaman, we need postgres as well!
  FIND_PACKAGE(LUMASSPostgreSQL)
  IF (PostgreSQL_FOUND)
     message(STATUS "postgres include dirs: "${PostgreSQL_INCLUDE_DIRS})
     message(STATUS "postgres libraries dirs: "${PostgreSQL_LIBRARY_DIRS})
  ENDIF()

  IF (PostgreSQL_FOUND AND RASDAMAN_FOUND)
    OPTION(RASDEBUG "Is rasdaman running in debug mode?" OFF)
    IF (RASDEBUG)
        ADD_DEFINITIONS(-DRMANDEBUG)
    ENDIF (RASDEBUG)
    ADD_DEFINITIONS(-DBUILD_RASSUPPORT)
    message(STATUS "building with rasdaman support and define BUILD_RASSUPPORT")
  ELSE(PostgreSQL_FOUND AND RASDAMAN_FOUND)          
    set(RASSUPPORT OFF)
    message(STATUS "building without rasdaman support because either rasdaman
         or PostgreSQL support is mising!")
  ENDIF(PostgreSQL_FOUND AND RASDAMAN_FOUND)
ENDIF()

message(STATUS "rasdaman support: "${RASSUPPORT})
message(STATUS "rasdaman source dir: "${RASDAMAN_SOURCE_DIR})
message(STATUS "rasdaman include dir: "${RASDAMAN_INCLUDE_DIR})
message(STATUS "rasdaman libraries dir: "${RASDAMAN_LIBRARIES_DIR})

#====================================================================
# SOME OTHER STUFF WE WANNA USE GLOBALLY
#====================================================================

add_definitions(-Wno-deprecated)

#====================================================================
# ADD SUBDIRECTORIES
#====================================================================

ADD_SUBDIRECTORY(shared ${lumass_BINARY_DIR}/shared)
ADD_SUBDIRECTORY(otbsuppl ${lumass_BINARY_DIR}/otbsuppl)
ADD_SUBDIRECTORY(QtPropertyBrowser ${lumass_BINARY_DIR}/QtPropertyBrowser)
ADD_SUBDIRECTORY(optimisation ${lumass_BINARY_DIR}/optimisation)
ADD_SUBDIRECTORY(modellingframework ${lumass_BINARY_DIR}/modellingframework)
ADD_SUBDIRECTORY(gui ${lumass_BINARY_DIR}/gui)



