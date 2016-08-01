#!/bin/bash
#========== some functions
bailout ()
{
	echo "ERROR: $MSG"
	exit
}
#==========

# check the number of arguments
SYNOPSIS="Synopsis: $0 <lumass install dir> <package target dir> <lumass version>"
if [ $# -ne 3 ]; then
	MSG=$SYNOPSIS
	bailout
fi

# get the directory for this script
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null

# check the lumass install directory
if [ ! -d $1/lib -o ! -w $1/lib ]; then
	MSG="Invalid or non-writable install directory specified!"
	bailout	
fi

INDIR_=$1
cd $INDIR_
INDIR=`pwd`
cd $CURDIR

# get the 'root' dir of the tar ball
BASENAME=$( echo "$INDIR" | awk -F '/' '{print $NF}' )


# check the package target directory
if [ ! -d $2 -o ! -w $2 ]; then
	MSG="Invalid or non-writable package target directory specified!"
	bailout	
fi

OUTDIR_=$2
cd $OUTDIR_
OUTDIR=`pwd`
cd $CURDIR

# lumass version
LUMASS_VERSION=$3 #0.9.32

# ==========================================
# package LUMASS
# ==========================================

# locate and copy lumass dependencies into install tree
# for distribution; we accept that some libs are tried
# to be copied more that once which will give a warning
# that the lib already exist and won't be copied again,
# so no harm done - just ignore admittledly ugly warnings ...

# copy the lumass start script to INDIr
cp $CURDIR/lumass.sh $INDIR/

# copy lumass depencies
cd $INDIR
cp $( ldd bin/lumass | cut -d " " -f 3) lib/

# add platform specific libqxcb.so to file tree
mkdir bin/platforms
cp $( locate libqxcb.so ) bin/platforms/
# also add all dependencies of libqxcb.so to lib dir in distr. tree
cp $( ldd bin/platforms/libqxcb.so | cut -d " " -f 3) lib/

# need this one as well, so just copy it
cp $( locate libQt5DBus.so ) lib/

# also need to copy gdal kea plugin
LIBKEA=$( locate -n 1 libkea.so. ) 
cp $( ldd $( echo "$LIBKEA" ) | cut -d " " -f 3) lib/
cp $( echo "$LIBKEA" ) lib/

# create symlink to full version libkea
LIBKEA2=$( echo "$LIBKEA" | awk -F '/' '{print $NF}' )
if [ -a "lib/libkea.so" -o -L "lib/libkea.so" ]; then
	rm lib/libkea.so
fi

#echo "---> versioned libkea = $LIBKEA2"

#ln -s $( echo "$INDIR" )/lib/$( echo "$LIBKEA2" ) lib/libkea.so
cd lib
ln -s './$LIBKEA2' ./libkea.so
cd ..

#echo "---> created symlink lib/libkea.so ok"

GDAL_KEA=$( locate -n 1 gdal_KEA.so)
cp $( echo "$GDAL_KEA" ) lib/

cd lib
for LIB in $( ls ./ ); do
	if grep -q "PATH" <<< $( readelf --dynamic $LIB ); then
		chrpath -d $LIB
	fi
done

# package ...
cd ../..

OS=$( uname -s )
REL=$( uname -r )
PLATFORM=$( uname -i )

tar czvf lumass-$( echo $LUMASS_VERSION )_$OS-$( echo $REL )_$PLATFORM.tar.gz "$BASENAME"

mv lumass-$( echo $LUMASS_VERSION )_$OS-$( echo $REL )_$PLATFORM.tar.gz "$OUTDIR"/
