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
  vtk
  vtk-bin
  VTK
  VTK-bin

PATHS
  /opt
  /usr/local
  /usr/local/lib
  /usr/local/src
  /usr/lib
  /usr/share
)

