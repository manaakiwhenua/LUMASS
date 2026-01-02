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
		    lpsolve55
		    lp_solve55
		    lp_solve
		    lpsolve
        lp_solve-5.5.2.14
    PATHS
        /opt
        /opt/local
        /usr/local
        /usr
        /usr/share
    		"c:/opt"
    		"c:/build"
        "C:/Install"
    DOC "path ot lp_solve's include directory"
)

if(WIN32)
	set(LPLIBNAMES lpsolve.lib)
else()
	set(LPLIBNAMES liblpsolve55.so liblpsolve50.so liblpsolve40.so)
endif()

FIND_LIBRARY(LPSOLVE_LIBRARY 
    NAMES ${LPLIBNAMES}
    PATH_SUFFIXES
      lib
      lpsolve/lib
      lp_solve/lib
      bin
      lib/lpsolve
      lib/lp_solve
      lp_solve-5.5.2.14
		  lpsolve55
		  lp_solve55
		  lp_solve
		  lpsolve
    PATHS
        /opt
        /opt/local
        /usr
        /usr/local
        /usr/share
		    "c:/opt"
		    "c:/build"
        "C:/Install"
    
    DOC "path to the lp_solve library (e.g. /usr/lib/liblpsolve55.so)"
)

if (LPSOLVE_LIBRARY)
  get_filename_component(LPSOLVE_LIB_DIR ${LPSOLVE_LIBRARY} DIRECTORY)
  get_property(LPSOLVE_LIB_NAME SOURCE ${LPSOLVE_LIBRARY} PROPERTY NAME)
  message(STATUS "LPSOLVE_LIBRARY: ${LPSOLVE_LIBRARY}")
  message(STATUS "LPSOLVE_LIB_DIR: ${LPSOLVE_LIB_DIR}")
endif()

IF (LPSOLVE_LIBRARY AND LPSOLVE_INCLUDE_DIR)
  set(LPSOLVE_FOUND TRUE)
ENDIF()
