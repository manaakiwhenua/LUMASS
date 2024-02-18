# - Find an OTB installation or build tree.

# When OTB is found, the OTBConfig.cmake file is sourced to setup the
# location and configuration of OTB.  Please read this file, or
# OTBConfig.cmake.in from the OTB source tree for the full list of
# definitions.  Of particular interest is OTB_USE_FILE, a CMake source file
# that can be included to set the include directories, library directories,
# and preprocessor macros.  In addition to the variables read from
# OTBConfig.cmake, this find module also defines
#  OTB_DIR  - The directory containing OTBConfig.cmake.
#             This is either the root of the build tree,
#             or the lib/otb directory.
#             This is the only cache entry.
#
#  OTB_FOUND - Whether OTB was found.  If this is true,
#              OTB_DIR is okay.
#
#  USE_OTB_FILE - The full path to the UseOTB.cmake file.
#                 This is provided for backward
#                 compatability.  Use OTB_USE_FILE
#                 instead.

# This is adapted from the FindITK.cmake distributed with cmake
# WARNING: the adaptation is not fully tested yet and needs some rework

# This file is adapted in Aug 2012 by Alexander Herzig 
# from FindOTB.cmake distributed with the Orfeo Toolbox
# and adds some further search paths to the original version

SET(OTB_DIR_STRING "directory containing OTBConfig.cmake.  This is either the root of the build tree, or PREFIX/lib/otb for an installation.")

# Search only if the location is not already known.
IF(NOT OTB_DIR)
  # Get the system search path as a list.
  IF(UNIX)
    STRING(REGEX MATCHALL "[^:]+" OTB_DIR_SEARCH1 "$ENV{PATH}")
  ELSE(UNIX)
    STRING(REGEX REPLACE "\\\\" "/" OTB_DIR_SEARCH1 "$ENV{PATH}")
  ENDIF(UNIX)
  STRING(REGEX REPLACE "/;" ";" OTB_DIR_SEARCH2 ${OTB_DIR_SEARCH1})

  # Construct a set of paths relative to the system search path.
  SET(OTB_DIR_SEARCH "")
  FOREACH(dir ${OTB_DIR_SEARCH2})
    SET(OTB_DIR_SEARCH ${OTB_DIR_SEARCH} "${dir}/../lib/otb")
  ENDFOREACH(dir)
  
  # add some paths, which might seem unusual, but I'm actually using them ...
  SET (OTB_DIR_SEARCH 
      ${OTB_DIR_SEARCH}
      /opt/OTB-bin
      /opt/OTB
      /opt/otb
      /opt/otb-bin
      /opt/*
      /opt/OTB-8.1-bin/install/lib/cmake
      /usr/lib/cmake
      /usr/lib/x86_64-linux-gnu
      /home/$ENV{USER}/garage/build/OTB-8.1.2
      /home/users/$ENV{USER}/garage/build/OTB-8.1.2/install/lib/cmake
      C:/build/OTB-debug
      C:/opt/OTB-debug
      C:/build/OTB-reldebinfo
      C:/opt/OTB-reldebinfo	  
      C:/build/OTB-bin
      C:/opt/OTB-bin
  )

  #
  # Look for an installation or build tree.
  #
  FIND_PATH(OTB_DIR OTBConfig.cmake
    PATH_SUFFIXES
        OTB-5.0
        OTB-4.4
        OTB-5.1
        OTB-5.2
        OTB-5.3
        OTB-5.4
        OTB-5.5
        OTB-5.6
        OTB-6.0
        OTB-6.2
        OTB-6.4
        OTB-6.6
        OTB-7.0
        OTB-7.1
        OTB-8.1
        cmake/OTB-5.0
        cmake/OTB-4.4
        cmake/OTB-5.1
        cmake/OTB-5.2
        cmake/OTB-5.3
        cmake/OTB-5.4
        cmake/OTB-5.5
        cmake/OTB-5.6
        cmake/OTB-6.0
        cmake/OTB-6.2
        cmake/OTB-6.4
        cmake/OTB-6.6
        cmake/OTB-7.0
        cmake/OTB-7.1
        cmake/OTB-8.1
        OTB/build
    PATHS
    # Look for an environment variable OTB_DIR.
    $ENV{OTB_DIR}

    # Look in places relative to the system executable search path.
    ${OTB_DIR_SEARCH}

    # Look in standard UNIX install locations.
    /usr/local/lib/otb
    /usr/lib/otb

    # Read from the CMakeSetup registry entries.  It is likely that
    # OTB will have been recently built.
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild10]

    # Help the user find it if we cannot.
    DOC "The ${OTB_DIR_STRING}"
  )
ENDIF(NOT OTB_DIR)

# If OTB was found, load the configuration file to get the rest of the
# settings.
IF(OTB_DIR)
  SET(OTB_FOUND 1)
  INCLUDE(${OTB_DIR}/OTBConfig.cmake)
  
  # Set USE_OTB_FILE for backward-compatability.
  SET(USE_OTB_FILE ${OTB_USE_FILE})
ELSE(OTB_DIR)
  SET(OTB_FOUND 0)
  IF(OTB_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Please set OTB_DIR to the ${OTB_DIR_STRING}")
  ENDIF(OTB_FIND_REQUIRED)
ENDIF(OTB_DIR)

## some more stuff .. 
if(ITK_FOUND)
    SET(LUMASS_ITK_VERSION "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH}")
    if (UNIX AND NOT APPLE)
        find_path(ITK_LINK_DIR libITKCommon-${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.so
            PATHS
                /usr/lib
                /usr/lib/x86_64-linux-gnu
        )
    endif()
endif(ITK_FOUND)
if ("${LUMASS_ITK_VERSION}" VERSION_LESS "4.11")
    message(FATAL_ERROR "LUMASS >= 0.9.60 requires ITK >= 4.11")
endif ()

INCLUDE(${USE_OTB_FILE})

# define OTB_LINK_DIR
if (UNIX AND NOT APPLE)
    find_path(OTB_LINK_DIR libOTBCommon-${OTB_VERSION_MAJOR}.${OTB_VERSION_MINOR}.so
        PATHS
            /usr
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /opt/otb-bin
            /opt/OTB-bin
            /opt/OTB-7.0
            /opt/OTB-7.1
            /opt/OTB-7.2
        PATH_SUFFIXES
            lib
            local/lib
            install/lib
    )
endif()

#message(STATUS "ITK_VERSION: ${LUMASS_ITK_VERSION}")
#message(STATUS "ITK_LIBRARY_DIRS: ${ITK_LIBRARY_DIRS}")
#message(STATUS "ITK_INCLUDE_DIRS: ${ITK_INCLUDE_DIRS}")
#message(STATUS "ITK_LIBRARIES: ${ITK_LIBRARIES}")
#message(STATUS "ITK_LINK_DIR: ${ITK_LINK_DIR}")
#message(STATUS "OTB_LINK_DIR: ${OTB_LINK_DIR}")
