#! /usr/bin/env bash

# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null

# we assume that gdal/ogr2ogr is installed
# and working, however, need to define the 
# right environment first ... 

$CURDIR/env.sh

# call gal_rasterize with all arguments
exec ogr2ogr "$@"
