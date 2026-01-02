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

### FindLUMASSSpatialite.cmake ############
### created: 16/11/2015
### author: Alexander Herzig
### copyright: Landcare Research New Zealand Ltd

# the module defines  
#         SPATIALITE_INCLUDE_DIR
#         SPATIALITE_LIB_DIR
# if both files are found 
#         SPATIALITE_FOUND
# is defined


FIND_PATH(SPATIALITE_INCLUDE_DIR spatialite.h
    PATH_SUFFIXES
        include
    PATHS
		C:/opt/spatialite-bin
        c:/OSGeo4W/bin
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
        ${OSGEO4W_ROOT}
    DOC "Path to Spatialite include directory"
)

if(WIN32)
        set(SPATIALITE_LIB spatialite.dll)
else()
        set(SPATIALITE_LIB libspatialite.so)
endif()

FIND_PATH(FIND_SPATIALITE_LIB_DIR
    NAMES ${SPATIALITE_LIB}
    PATH_SUFFIXES
        lib
        lib/x86_64-linux-gnu
        bin
    PATHS
        C:/opt/spatialite-bin
        C:/OSGeo4W/bin
        C:/OSGeo4W/lib
        /opt
        /opt/local
        /usr
        /usr/local
        /usr/share
        ${OSGEO4W_ROOT}
    DOC "Path to the spatialite library (e.g. /usr/lib/libspatilite.so)"
)


# for windows, we also need the *.lib library for linking

if(WIN32)
    set(SPATIALITE_LIBLIB spatialite_i.lib)

    FIND_PATH(FIND_SPATIALITE_LIBLIB_DIR
            NAMES ${SPATIALITE_LIBLIB}
        PATH_SUFFIXES
            lib
            bin
        PATHS
            C:/opt/spatialite-bin
            c:/OSGeo4W/lib
            ${OSGEO4W_ROOT}
        DOC "Path to the spatialite library (e.g. C:/OSGEO4W/lib/spatialite.lib)"
    )


    if (FIND_SPATIALITE_LIBLIB_DIR)
            message(STATUS "found liblib dir: ${FIND_SPATIALITE_LIBLIB_DIR}")
            SET(SPATIALITE_LIBLIB_DIR ${FIND_SPATIALITE_LIBLIB_DIR}
                    CACHE FILEPATH "Spatialite import library dir" FORCE)
    endif()
endif()

SET(SPATIALITE_LIB_DIR ${FIND_SPATIALITE_LIB_DIR} 
	CACHE FILEPATH "Spatialite link directories" FORCE)		

IF(WIN32)
    IF (SPATIALITE_LIB_DIR AND SPATIALITE_LIBLIB_DIR AND SPATIALITE_INCLUDE_DIR)
      set(SPATIALITE_FOUND TRUE)
    ENDIF(SPATIALITE_LIB_DIR AND SPATIALITE_LIBLIB_DIR AND SPATIALITE_INCLUDE_DIR)
ELSE(WIN32)
    IF (SPATIALITE_LIB_DIR AND SPATIALITE_INCLUDE_DIR)
      set(SPATIALITE_FOUND TRUE)
    ENDIF(SPATIALITE_LIB_DIR AND SPATIALITE_INCLUDE_DIR)

    # determine version
    execute_process(COMMAND pkg-config --modversion spatialite OUTPUT_VARIABLE SPATIALITE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

ENDIF(WIN32)
