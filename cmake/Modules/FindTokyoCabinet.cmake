### FindTokyoCabinet.cmake ############
#
### author: Alexander Herzig
### copyright: Landcare Research New Zealand Ltd
### purpose: this cmake module is specifically designed for
###          use in conjunction with LUMASS and 
###          has only been tested on Linux

# the module defines  
#         TCADB_INCLUDE_DIR
#         TCADB_LIB_DIR
# if both files are found 
#         TCADB_FOUND
# is defined


FIND_PATH(TCADB_INCLUDE_DIR tcadb.h
    PATH_SUFFIXES
        include
    PATHS
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
        "c:/opt"
        "c:/build"
    DOC "Path to Tokyo Cabient include directory"
)

if(WIN32)
        set(TCADB_LIB libtokyocabinet.lib)
else()
        set(TCADB_LIB libtokyocabinet.so)
endif()

FIND_PATH(TCADB_LIB_DIR
    NAMES ${TCADB_LIB}
    PATH_SUFFIXES
        lib
        lib/x86_64-linux-gnu
    PATHS
        /opt
        /opt/local
        /usr
        /usr/local
        /usr/share
        "c:/opt"
        "c:/build"
    DOC "path to the lp_solve library (e.g. /usr/lib/libTCADB55.so)"
)

IF (TCADB_LIB_DIR AND TCADB_INCLUDE_DIR)
  set(TCADB_FOUND TRUE)
ENDIF()
