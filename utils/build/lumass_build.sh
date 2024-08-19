#!/bin/bash

# LUMASS build dependencies from source

# OS: Ubuntu 22.04.4 LTS
# Linux 6.5.0-41-generic #41~22.04.2-Ubuntu SMP PREEMPT_DYNAMIC Mon Jun  3 11:32:55 UTC 2 x86_64 x86_64 x86_64 GNU/Linux

# disk space requirement: ~13GB

# PATH definitions
# need to be adapted when updating dependencies

export CPP_DIR=$HOME/garage/cpp
export BIN_DIR=$HOME/garage/build

export KEA_VERSION=1.5.3
export LIBKEA_VERSION=1.5
export KEA_SRC=$HOME/garage/cpp/kealib
export KEA_BIN=$HOME/garage/build/kealib-$KEA_VERSION

export ITK_SRC=$HOME/garage/cpp/ITK-4.13.3
export ITK_BIN=$HOME/garage/build/ITK-4.13.3
export ITK_INSTALL=$ITK_BIN/install/lib/cmake/ITK-4.13

export OTB_SRC=$HOME/garage/cpp/OTB_9
export OTB_BIN=$HOME/garage/build/OTB-9.0.0
export OTB_INSTALL=$OTB_BIN/install/lib/cmake/OTB-9.0

export VTK_SRC=$CPP_DIR/VTK
export VTK_BIN=$HOME/garage/build/VTK-9.3

export NCVERSION=4.9.2

export LUMASS_SRC=$CPP_DIR/lumass
export LUMASS_UTILS_DIR=$LUMASS_SRC/utils
export LUMASS_BIN=$BIN_DIR/lumass-build

mkdir -p $CPP_DIR

# exit on first failed command
set -e

#building libKEA for *.kea support
cd $CPP_DIR
git clone https://github.com/ubarsc/kealib.git
cd $KEA_SRC
git checkout kealib-$KEA_VERSION
mkdir -p $KEA_BIN
cd $KEA_BIN
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DGDAL_CONFIG_EXECUTABLE=/usr/bin/gdal-config -DLIBKEA_WITH_GDAL=ON $KEA_SRC
make -j 14

### Insight Toolkit - ITK 4.13.3
cd $CPP_DIR
wget https://github.com/InsightSoftwareConsortium/ITK/archive/v4.13.3.tar.gz
tar zxf v4.13.3.tar.gz
cd $ITK_SRC
wget https://raw.githubusercontent.com/orfeotoolbox/OTB/develop/SuperBuild/patches/ITK/itk-3-remove-gcc-version-debian-medteam-all.diff
patch -ut -p1 < itk-3-remove-gcc-version-debian-medteam-all.diff
mkdir -p $ITK_BIN
cd $ITK_BIN
cmake -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="$ITK_BIN/install" -DITK_BUILD_DEFAULT_MODULES=ON $ITK_SRC
make -j 14 install


### Orfeo Toolbox - OTB 9.0
cd $CPP_DIR
git clone https://github.com/orfeotoolbox/OTB.git OTB_9
cd $OTB_SRC
git checkout 9.0.0
wget https://raw.githubusercontent.com/manaakiwhenua/LUMASS/develop/utils/patches/otb-ENH-3D-streaming.diff
patch -ut -p1 < otb-ENH-3D-streaming.diff
mkdir -p $OTB_BIN
cd $OTB_BIN
cmake -DCMAKE_INSTALL_PREFIX="$OTB_BIN/install"  -DBUILD_COOKBOOK=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DBoost_USE_MULTITHREADED=ON -DBoost_USE_STATIC_LIBS=OFF -DOTB_USE_SIFTFAST=OFF -DOTB_WRAP_PYTHON=OFF -DITK_DIR=$ITK_BIN -DOTBGroup_Core=OFF -DOTB_BUILD_DEFAULT_MODULES=ON $OTB_SRC
LD_LIBRARY_PATH=$ITK_INSTALL/../..:$LD_LIBRARY_PATH make -j 14

### VTK - 9.3
cd $CPP_DIR
git clone https://github.com/Kitware/VTK.git -b v9.3.0 VTK
mkdir -p $VTK_BIN
cd $VTK_BIN
cmake -DBUILD_SHARED_LIBS=ON -DVTK_BUILD_DOCUMENTATION=OFF -DVTK_BUILD_EXAMPLES=OFF -DVTK_BUILD_TESTING=OFF -DVTK_GROUP_ENABLE_Imaging=YES -DVTK_GROUP_ENABLE_Qt=YES -DVTK_GROUP_ENABLE_Rendering=YES -DVTK_GROUP_ENABLE_Qt=YES -DVTK_GROUP_ENABLE_Rendering=YES -DVTK_MODULE_ENABLE_VTK_ChartsCore=YES -DVTK_MODULE_ENABLE_VTK_FiltersExtraction=YES -DVTK_MODULE_ENABLE_VTK_FiltersFlowPaths=YES -DVTK_MODULE_ENABLE_VTK_FiltersGeneral=YES -DVTK_MODULE_ENABLE_VTK_FiltersGeneric=YES -DVTK_MODULE_ENABLE_VTK_GUISupportQt=YES -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick=YES -DVTK_MODULE_ENABLE_VTK_GUISupportQtSQL=YES -DVTK_MODULE_ENABLE_VTK_IOGDAL=YES -DVTK_MODULE_ENABLE_VTK_RenderingAnnotation=YES -DVTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2=YES -DVTK_MODULE_ENABLE_VTK_RenderingFFMPEGOpenGL2=YES -DVTK_MODULE_ENABLE_VTK_RenderingFreeType=YES -DVTK_MODULE_ENABLE_VTK_RenderingImage=YES -DVTK_MODULE_ENABLE_VTK_RenderingLOD=YES -DVTK_MODULE_ENABLE_VTK_RenderingLabel=YES -DVTK_MODULE_ENABLE_VTK_RenderingMatplotlib=NO -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2=YES -DVTK_MODULE_ENABLE_VTK_RenderingOpenVR=NO -DVTK_MODULE_ENABLE_VTK_RenderingOpenXR=NO -DVTK_MODULE_ENABLE_VTK_RenderingUI=YES -DVTK_MODULE_ENABLE_VTK_RenderingVtkJS=YES -DVTK_MODULE_ENABLE_VTK_ViewsCore=YES -DVTK_MODULE_ENABLE_VTK_WebGLExporter=YES -DVTK_USE_64BIT_IDS=ON $VTK_SRC
make -j 14


### netcdf-c 
cd $CPP_DIR
git clone https://github.com/heralex/netcdf-c.git -b b_4.9.0
mkdir -p $BIN_DIR/netcdf-c-$NCVERSION
cd $BIN_DIR/netcdf-c-$NCVERSION

cmake -DHDF5_ROOT=/usr/lib/x86_64-linux-gnu/hdf5/mpich -DCMAKE_INSTALL_PREFIX=$BIN_DIR/netcdf-c-$NCVERSION/install -DBUILD_SHARED_LIBS:BOOL=ON  -DHDF5_HL_LIBRARY:STRING=/usr/lib/x86_64-linux-gnu/hdf5/mpich/libhdf5_hl.so -DHDF5_C_LIBRARY:STRING=/usr/lib/x86_64-linux-gnu/hdf5/mpich/libhdf5.so -DHDF5_INCLUDE_DIR:STRING=/usr/lib/x86_64-linux-gnu/hdf5/mpich/include -DHDF5_VERSION:STRING="1.10.7" -DNETCDF_LIB_NAME:STRING=netcdf_par $CPP_DIR/netcdf-c
make -j 14 install


### netcdf-cxx
cd $CPP_DIR
git clone https://github.com/heralex/netcdf-cxx4.git -b heralex_v4.3.1
mkdir $BIN_DIR/netcdf-cxx-4.3.1
cd $BIN_DIR/netcdf-cxx-4.3.1
cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX=$BIN_DIR/netcdf-cxx-4.3.1/install -DHDF5_ROOT=/usr/lib/x86_64-linux-gnu/hdf5/mpich -DBUILD_SHARED_LIBS:BOOL=OFF -DnetCDF_DIR=$BIN_DIR/netcdf-c-$NCVERSION/install/lib/cmake/netCDF -DnetCDF_LIBRARIES:STRING=$BIN_DIR/netcdf-c-$NCVERSION/install/lib/libnetcdf_par.a -DnetCDF_INCLUDE_DIR:STRING=$BIN_DIR/netcdf-c-$NCVERSION/install/include -DCMAKE_CXX_FLAGS=-fPIC $CPP_DIR/netcdf-cxx4
make -j 14 install


## LUMASS 
cd $CPP_DIR
git clone https://github.com/manaakiwhenua/LUMASS.git lumass
mkdir -p $LUMASS_BIN
cd $LUMASS_BIN
cmake -DHDF5_ROOT:Path=/lib/x86_64-linux-gnu/hdf5/mpich -DCMAKE_BUILD_TYPE=Release -DOTB_DIR=$OTB_BIN -DVTK_DIR:Path=$VTK_BIN -DNC_CONFIG_PATH=$BIN_DIR/netcdf-c-$NCVERSION/install/bin -DNCXX4_CONFIG_PATH=$BIN_DIR/netcdf-cxx-4.3.1/install/bin -Dpybind11_DIR:PATH=/usr/lib/cmake/pybind11 -DYAML_CPP_CMAKE_DIR=/usr/lib/x86_64-linux-gnu/cmake/yaml-cpp -DYAML_CPP_INCLUDE_DIR=/usr/include/yaml-cpp $LUMASS_SRC
# !! NOTE !!  you need >= 16GB of RAM to use 2 cores 
make -j 2 install

## ========================================
## AppImage Creation

### download apps for creating an appimage
export APPIMGDIR=$HOME/garage/apps
mkdir -p $APPIMGDIR
cd $APPIMGDIR
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/1-alpha-20230713-1/linuxdeploy-plugin-appimage-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-checkrt/releases/download/continuous/linuxdeploy-plugin-checkrt-x86_64.sh
chmod u+x *.*

# generating lumass AppImage from install tree
cd $LUMASS_BIN
rm -f ./*.AppImage
export APPDIR=$(pwd)/$(ls -d lumass-*)
#export MWLRPLUG_DIR=$HOME/garage/build/lumass-mwlr-plugins

$APPIMGDIR/linuxdeploy-x86_64.AppImage --appdir $APPDIR --custom-apprun=$LUMASS_UTILS_DIR/RunLumassAppImage -l $KEA_BIN/gdal/gdal_KEA.so -l $KEA_BIN/src/libkea.so.$LIBKEA_VERSION --plugin qt --plugin checkrt --output appimage




