# ***********************************************
# FindLUMASSVTK.cmake
#
# created by Alexander Herzig
# Copyright 2014 Landcare Reseaerch New Zealand
#
# just looks for some possible places VTK might be installed in
# to populate VTK_DIR
# ***********************************************

FIND_PATH(VTK_DIR VTKConfig.cmake
PATH_SUFFIXES
  VTK-debug
  VTK-reldebinfo
  VTK-bin
  vtk-bin
  vtk
  VTK
  vtk-6.1
  vtk-6.2
  vtk-6.3
  vtk-6.4

PATHS
  /opt
  /usr/local
  /usr/local/lib
  /usr/local/src
  /usr/lib
  /usr/lib/cmake
  /usr/share
  c:/opt
  c:/build
  "c:/Programm Files (x86)"
  "c:/Programm Files"
)

message(STATUS "VTKConfig.cmake found in : ${VTK_DIR}")

if (NOT VTK_DIR)
    FIND_PATH(VTK_DIR vtk-config.cmake
    PATH_SUFFIXES
        VTK-debug
        VTK-9.0.1-dbg
        VTK-bin
        VTK-9.0.1-bin
        vtk-8.2
        VTK-9.0.1
        vtk-9.0.1

    PATHS
        /opt
        /usr/local
        /usr/local/lib
        /usr/lib/cmake
        /usr/lib/x86_64-linux/cmake
        c:/opt
        c:/build
    )
endif()

message(STATUS "vtk-config.cmake found in : ${VTK_DIR}")

