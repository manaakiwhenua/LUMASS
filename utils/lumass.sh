#!/bin/bash



# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null


ADIR=$CURDIR/
BDIR=$CURDIR

# set the relevant paths
export LD_LIBRARY_PATH="$CURDIR$/lib:/usr/lib:/usr/local/lib:$LD_LIBRARY_PATH"
export GDAL_DRIVER_PATH="$CURDIR/lib:$GDAL_DRIVER_PATH"

# start lumass
$CURDIR/bin/lumass
