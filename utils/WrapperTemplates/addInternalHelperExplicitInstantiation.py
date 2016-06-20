#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string


if __name__ == '__main__':

    '''
    create a header file with explicit template class instantiations

    argv[0]: wrapper class filename
    argv[1]: helperclass name

    '''
    if len(sys.argv) < 3:
        print "    Usage: $ %s <wrapper class filename> <helperclass name>" % sys.argv[0]
        sys.exit()

    filename = sys.argv[1]
    classname = sys.argv[2]

    print "    >>> filename=%s" % filename
    print "    >>> helperclassname=%s" % classname


    # create list with data types
    dt1 = ['unsigned char', 'char', 'unsigned short', 'short', \
            'unsigned int', 'int', 'unsigned long', 'long', \
            'float', 'double']

    hStr = None
    with open(hPath, 'r') as hfile:
        hStr = hfile.read()

        instStr = ''
        for dim in range(1, int(ndim)+1):
            for t1 in dt1:
                inst = "template class %s<%s, %s>;\n"   \
                     % (className, t1, dim)
                instStr = instStr + inst

        hStr = hStr.replace("/*$<HelperClassInstantiation>$*/", instStr)


    with open(hPath, 'w') as hfile:
        hfile.write(hStr)

    print "done!"

