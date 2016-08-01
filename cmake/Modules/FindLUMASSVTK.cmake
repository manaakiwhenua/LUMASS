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

