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
    if len(sys.argv) < 4:
        print "    Usage: $ %s <wrapper class filename> <helperclass name> <num templ. args>" % sys.argv[0]
        sys.exit()

    filename = sys.argv[1]
    classname = sys.argv[2]
    ntplargs = int(sys.argv[3])
    ndim = 3

    print "    >>> filename=%s" % filename
    print "    >>> helperclassname=%s" % classname
    print "    >>> num templ. args=%d" % ntplargs

    if ntplargs == 1:
        print " one arg "
    elif ntplargs == 2:
        print " two args "

    # create list with data types
    dt1 = ['unsigned char', 'char', 'unsigned short', 'short', \
            'unsigned int', 'int', 'unsigned long', 'long', \
            'float', 'double']

    dt2 = dt1

    hStr = None
    with open(filename, 'r') as hfile:
        hStr = hfile.read()

        instStr = ''
        if ntplargs == 1:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    inst = "template class %s<%s, %s>;\n"   \
                         % (classname, t1, dim)
                    instStr = instStr + inst

        elif ntplargs == 2:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    for t2 in dt2:
                        inst = "template class %s<%s, %s, %s>;\n"   \
                             % (classname, t1, t2, dim)
                        instStr = instStr + inst

        hStr = hStr.replace("/*$<HelperClassInstantiation>$*/", instStr)


    with open(filename, 'w') as hfile:
        hfile.write(hStr)

    print "done!"

