#!/bin/bash

# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null


SYNOPSIS="$0 <test_name> <original_file> <test_file> <filename_results>"

if [ $# -lt 4 ]; then
    echo "Usage: $SYNOPSIS"
    exit
fi

name=$1
org_file=$2
test_file=$3
res_file=$4

md5_org=$( md5sum "$org_file" | cut -d " " -f1 )
md5_test=$( md5sum "$test_file" | cut -d " " -f1 )

if [ "$md5_org" == "$md5_test" ]; then
    RES="PASSED"
else
    RES="FAILED"
fi

echo $RES : $name original == test : $md5_org == $md5_test
echo $RES : $name >> $res_file 
