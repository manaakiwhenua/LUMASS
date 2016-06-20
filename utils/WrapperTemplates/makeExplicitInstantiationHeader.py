#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string


if __name__ == '__main__':

    '''
    create a header file with explicit template class instantiations

    argv[0]: script name
    argv[1]: filter class name
    argv[2]: num template args
    argv[3]: num dims

    '''
    if len(sys.argv) < 2:
        print "    Usage: $ %s <filter class name> [<num templ. args>] [<num dim.>]" % sys.argv[0]
        sys.exit()
    elif len(sys.argv) < 3:
        nargs = 2
        ndim = 3
    elif len(sys.argv) < 4:
        nargs = int(sys.argv[2])
        ndim = 3
    else:
        nargs = int(sys.argv[2])
        ndim = sys.argv[3]

    className = sys.argv[1]

    for num in range(0, len(sys.argv)):
        print str(sys.argv[num])


    #sys.exit()

    filepath = inspect.getfile(inspect.currentframe())
    path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    pos = string.rfind(path, '/', 0)
    pos = string.rfind(path, '/', 0, pos)
    homepath = path[0:pos]
    targetpath = homepath + "/otbsuppl/filters"
    print "    >>> lumass HOME=%s" % homepath
    print "    >>> wrapper file path: %s" % targetpath

    inHPath   = "%s/ExplicitInstTemplate.h" % (path)

    hPath = "%s/%s_ExplicitInst.h" % (targetpath, className)

    shutil.copyfile(inHPath, hPath)


    # create list with data types
    dt1 = ['unsigned char', 'char', 'unsigned short', 'short', \
            'unsigned int', 'int', 'unsigned long', 'long', \
            'float', 'double']
    dt2 = dt1

    hStr = None
    with open(hPath, 'r') as hfile:
        hStr = hfile.read()

        hStr = hStr.replace("/*$<FilterClassName>$*/", className)

        instStr = ''
        if nargs == 2:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    for t2 in dt2:
                        inst = "template class ITK_EXPORT otb::%s< otb::Image<%s, %s>, otb::Image<%s, %s> >;\n"   \
                             % (className, t1, dim, t2, dim)
                        instStr = instStr + inst
        else:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    inst = "template class ITK_EXPORT otb::%s< otb::Image<%s, %s> >;\n"   \
                         % (className, t1, dim)
                    instStr = instStr + inst

        hStr = hStr.replace("/*$<ExplicitInstantiation>$*/", instStr)


    with open(hPath, 'w') as hfile:
        hfile.write(hStr)

    print "done!"

