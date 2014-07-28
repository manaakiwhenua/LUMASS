#!/bin/bash

# directory of lumass executable
INDIR=$1
OUTDIR=$2
LUMASS_VERSION=$3 #0.9.32

# locate and copy lumass dependencies into install tree
# for distribution; we accept that some libs are tried
# to be copied more that once which will give a warning
# that the lib already exist and won't be copied again,
# so no harm done - just ignore admittledly ugly warnings ...

#
cd $INDIR

# copy lumass depencies
cp $( ldd bin/lumass | cut -d " " -f 3) lib/

# add platform specific libqxcb.so to file tree
mkdir bin/platforms
cp $( locate libqxcb.so ) bin/platforms/
# also add all dependencies of libqxcb.so to lib dir in distr. tree
cp $( ldd bin/platforms/libqxcb.so | cut -d " " -f 3) lib/

# need this one as well, so just copy it
cp $( locate libQt5DBus.so ) lib/

# also need to copy gdal kea plugin
LIBKEA=$( locate -n 1 libkea.so ) 
cp $( ldd $( echo $LIBKEA ) | cut -d " " -f 3) lib/
cp $( echo "$LIBKEA" ) lib/

GDAL_KEA=$( locate -n 1 gdal_KEA.so)
cp $( echo "$GDAL_KEA") lib/

# package ...

OS=$( uname -s )
REL=$( uname -r )
PLATFORM=$( uname -i )

cd ..

tar czvf lumass-$( echo $LUMASS_VERSION )_$OS-$( echo $REL )_$PLATFORM.tar.gz lumass

mv lumass-$( echo $LUMASS_VERSION )_$OS-$( echo $REL )_$PLATFORM.tar.gz "$OUTDIR"/
