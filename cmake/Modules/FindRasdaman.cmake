### FindRasdaman.cmake ############
#
### author: Alexander Herzig
### copyright: Landcare Research New Zealand Ltd
### purpose: this cmake module is specifically designed for
###          use in conjunction with LUMASS and 
###          has only been tested on Linux

# the module defines  
#         RASDAMAN_SOURCE_DIR      (by looking for applications/rasgeo/RasdamanHelper2.cxx)
#         RASDAMAN_LIBRARIES       (by looking for libraslib.a)
# if both files are found 
#         RASDAMAN_FOUND
# is defined


FIND_PATH(RASDAMAN_SOURCE_DIR applications/rasgeo/RasdamanHelper2.cxx
   HINTS   
     $ENV{RMANHOME}    
     $ENV{RMANSOURCE}
     $ENV{RASDAMAN}
     $ENV{RASSOURCE}
     $ENV{RASDAMAN_SOURCE}
   PATH_SUFFIXES
      rasdaman
      rasdaman/src
      rasdaman/source
      rassource
      ras_source
      rasdaman_source
   PATHS
      /usr/src
      /usr
      /usr/share
      /usr/local
      /opt
   DOC "path to rasdaman's source folder"
)  

FIND_PATH(RASDAMAN_INCLUDE_DIR rasdaman.hh
   HINTS   
     $ENV{RMANHOME}    
     $ENV{RMANSOURCE}
     $ENV{RASDAMAN}
     $ENV{RASSOURCE}
     $ENV{RASDAMAN_SOURCE}
   PATH_SUFFIXES
     include
   PATHS
      /usr/src
      /usr
      /usr/share
      /usr/local
      /opt   
      /opt/rasdaman  
   DOC "path to the rasdaman include directory"
)

FIND_PATH(RASDAMAN_LIBRARIES_DIR libraslib.a
   PATH_SUFFIXES
     lib
     bin
   PATHS
     /opt/rasdaman  
     /usr/local
     /usr/share
   DOC "path to the rasdaman libraries"
)

IF (RASDAMAN_LIBRARIES_DIR AND RASDAMAN_INCLUDE_DIR AND RASDAMAN_SOURCE_DIR)
  set(RASDAMAN_FOUND TRUE)
ENDIF (RASDAMAN_LIBRARIES_DIR AND RASDAMAN_INCLUDE_DIR AND RASDAMAN_SOURCE_DIR)

