#!/bin/bash

# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null

# we assume that gdal is installed
# and working

# call gal_rasterize with all arguments
exec gdal_rasterize "$@"
