#!/bin/bash

DIR=$1
cd $DIR

DEPLIBS=$( readelf --dynamic $DIR/bin/lumass | grep NEEDED )
TMP=$( echo "$DEPLIBS" | grep "so" | awk -F '[\[\]]' '{print $(NF-1)}' )

for LIB in $( echo "$TMP" ); do
	
	LIBPATH=$( locate -n 1 $LIB )
	echo $LIBPATH

done




