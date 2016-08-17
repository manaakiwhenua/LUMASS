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
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
        ${OSGEO4W_ROOT}
    DOC "Path to Spatialite include directory"
)

if(WIN32)
        set(SPATIALITE_LIB spatialite4.dll)
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
set(SPATIALITE_LIBLIB spatialite4.lib)

FIND_PATH(FIND_SPATIALITE_LIBLIB_DIR
	NAMES ${SPATIALITE_LIBLIB}
    PATH_SUFFIXES
        lib
        bin
    PATHS
		C:/opt/spatialite-bin
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
ENDIF(WIN32)
