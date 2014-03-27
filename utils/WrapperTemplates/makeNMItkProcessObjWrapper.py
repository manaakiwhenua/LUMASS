#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string
import re

# get the profile file path from the command line
if len(sys.argv) < 2:
    print "    Usage: $ %s <LUMASS wrapper profile file name>.lwp" % sys.argv[0]
    sys.exit()
profile = sys.argv[1]

# check whether we've got actually access to the file
try:
    fp = open(profile)
except IOError as e:
    if e.errno == errno.EACCES:
        print "Failed opening %s" % profile
        sys.exit()
    raise

# copy file
filepath = inspect.getfile(inspect.currentframe())
path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))



# define keywords to look for
keywords = ["Year", "WrapperClassName", "FileDate", "Author", 
            "FilterClassName", "WrapperTemplateTypePamphlet", 
            "InternalInImgTypedef", "InternalOutImgTypedef",
            "InternalFilterTypedef", "GetOutputImgTypename",
            "InternalFilterParamSetter", "WrapperPropertyList",
            "WrapperPropertyGetSetter", "PropertyVarDef"]

# now crawl through the file line by line
# and build a dictionary with string objects
with fp as f:
    for line in f:
        
        
    