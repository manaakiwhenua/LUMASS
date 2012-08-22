### FindLpsolve.cmake ############
#
### author: Alexander Herzig
### copyright: Landcare Research New Zealand Ltd
### purpose: this cmake module is specifically designed for
###          use in conjunction with LUMASS and 
###          has only been tested on Linux

# the module defines  
#         LPSOLVE_INCLUDE_DIR
#         LPSOLVE_LIBRARY
# if both files are found 
#         LPSOLVE_FOUND
# is defined


FIND_PATH(LPSOLVE_INCLUDE_DIR lp_lib.h
    PATH_SUFFIXES
        include
        include/lpsolve
        include/lp_solve
    PATHS
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
    DOC "path ot lp_solve's include directory"
)

FIND_LIBRARY(LPSOLVE_LIBRARY 
    NAMES liblpsolve55.so liblpsolve50.so liblpsolve40.so
    PATH_SUFFIXES
        lib
        lpsolve/lib
        lp_solve/lib
        bin
        lib/lpsolve
        lib/lp_solve
    PATHS
        /opt
        /opt/local
        /usr
        /usr/local
        /usr/share
    DOC "path to the lp_solve library (e.g. /usr/lib/liblpsolve55.so)"
)

IF (LPSOLVE_LIBRARY AND LPSOLVE_INCLUDE_DIR)
  set(LPSOLVE_FOUND TRUE)
ENDIF()
