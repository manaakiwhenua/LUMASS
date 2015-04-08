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
  
PATHS
  /opt
  /usr/local
  /usr/local/lib
  /usr/local/src
  /usr/lib
  /usr/share
  c:/opt
  c:/build
  "c:/Programm Files (x86)"
  "c:/Programm Files"
)

