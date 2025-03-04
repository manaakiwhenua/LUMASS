#!/usr/bin/env bash

# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null


SYNOPSIS="$0 <test_name> <test_resultfilename> <alltests_resultfilename>"

if [ $# -lt 3 ]; then
    echo "Usage: $SYNOPSIS"
    exit
fi

name=$1
test_file=$2
all_file=$3

#init the result variable 
result=1

if [ ! -f "$test_file" ];then
    echo "File $test_file does not exist!"
    exit 1
fi

#if [ ! -f "$all_file" ];then
#    echo "File $all_file does not exist!"
#    exit 1
#fi

# if we find an test value > 0, we mark the error
while IFS="" read -r line || [ -n "$line" ]; do
    if [[ $line =~ =([0-9]+) ]]; then
        number=${BASH_REMATCH[1]}
        if [ $number -gt 0 ]; then
            result=0
        fi
    fi
done < $test_file



if [ $result -eq 1 ]; then
    RES="PASSED"
else
    RES="FAILED"
fi

echo $RES : $name
echo $RES : $name >> $all_file 
