#!/bin/bash



# get the current directory
pushd `dirname $0` > /dev/null
CURDIR=`pwd`
popd > /dev/null

# synopsis
# del.bat <path> <file>
cd "$1"
shift

for file in $@; do
  if [ -d "$file" ]; then
    rmdir "$file"
  else
    rm "$file"
  fi
done
