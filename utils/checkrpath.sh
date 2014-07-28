#!/bin/bash

DIR=$1
cd $DIR

for LIB in $( ls $DIR ); do
	if grep -q "PATH" <<< $( readelf --dynamic $LIB ); then
		chrpath -d $LIB
	fi
done


