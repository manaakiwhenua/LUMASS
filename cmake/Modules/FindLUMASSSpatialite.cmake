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
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
        ${OSGEO4W_ROOT}
        "c:/opt"
        "c:/build"
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
        /opt
        /opt/local
        /usr
        /usr/local
        /usr/share
        ${OSGEO4W_ROOT}
        "c:/opt"
        "c:/build"
    DOC "Path to the spatialite library (e.g. /usr/lib/libspatilite.so)"
)

# for windows, we also need the *.lib library for linking
if (WIN32)
FIND_PATH(FIND_SPATIALITE_LIBLIB_DIR
	NAMES spatialite_i.lib
    PATH_SUFFIXES
        lib
        bin
    PATHS
        ${OSGEO4W_ROOT}
        "c:/opt"
        "c:/build"
    DOC "Path to the spatialite library (e.g. C:/OSGEO4W/lib/spatialite.lib)"
	)

	if (FIND_SPATIALITE_LIBLIB_DIR)
		SET(SPATIALITE_LIBLIB_DIR ${FIND_SPATIALITE_LIBLIB_DIR} 
			CACHE FILEPATH "Spatialite import library dir" FORCE)
	endif()
endif()

SET(SPATIALITE_LIB_DIR ${FIND_SPATIALITE_LIB_DIR} 
	CACHE FILEPATH "Spatialite link directories" FORCE)		

IF (SPATIALITE_LIB_DIR AND SPATIALITE_INCLUDE_DIR)
  set(SPATIALITE_FOUND TRUE)
ENDIF()
