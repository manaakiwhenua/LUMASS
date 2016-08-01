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
    if len(sys.argv) < 3:
        print "    Usage: $ %s <namespace> <filter classname> [<num templ. args>] [<num dim.>]" % sys.argv[0]
        sys.exit()
    elif len(sys.argv) < 4:
        nargs = 2
        ndim = 3
    elif len(sys.argv) < 5:
        nargs = int(sys.argv[3])
        ndim = 3
    else:
        nargs = int(sys.argv[3])
        ndim = sys.argv[4]

    namespace = sys.argv[1]
    className = sys.argv[2]

    for num in range(0, len(sys.argv)):
        print str(sys.argv[num])


    filepath = inspect.getfile(inspect.currentframe())
    path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    pos = string.rfind(path, '/', 0)
    pos = string.rfind(path, '/', 0, pos)
    homepath = path[0:pos]
    targetpath = homepath + "/otbsuppl/filters"
    print "    >>> lumass HOME=%s" % homepath
    print "    >>> wrapper file path: %s" % targetpath

    inHPath   = "%s/ExplicitInstTemplate.h" % (path)

    hPath = "%s/%s%s_ExplicitInst.h" % (targetpath, namespace, className)

    shutil.copyfile(inHPath, hPath)


    # create list with data types
    dt1 = ['unsigned char', 'char', 'unsigned short', 'short', \
            'unsigned int', 'int', 'unsigned long', 'long', \
            'float', 'double']
    dt2 = dt1

    hStr = None
    with open(hPath, 'r') as hfile:
        hStr = hfile.read()

        hStr = hStr.replace("/*$<NameSpace>$*/", namespace)
        hStr = hStr.replace("/*$<FilterClassName>$*/", className)

        instStr = ''
        if nargs == 2:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    for t2 in dt2:
                        inst = "template class OTBSUPPLFILTERS_EXPORT %s::%s< otb::Image<%s, %s>, otb::Image<%s, %s> >;\n"   \
                             % (namespace, className, t1, dim, t2, dim)
                        instStr = instStr + inst
        else:
            for dim in range(1, int(ndim)+1):
                for t1 in dt1:
                    inst = "template class OTBSUPPLFILTERS_EXPORT %s::%s< otb::Image<%s, %s> >;\n"   \
                         % (namespace, className, t1, dim)
                    instStr = instStr + inst

        hStr = hStr.replace("/*$<ExplicitInstantiation>$*/", instStr)


    with open(hPath, 'w') as hfile:
        hfile.write(hStr)

    # now we add an include line for this header to the
    # GUI_template_inst.h file (/*$<NextExplictInstHeader>$*/)
    guipath = homepath + "/gui/GUI_template_inst.h"
    with open(guipath, 'r') as gfile:

        gStr = gfile.read()

        ginsert = "#include \"%s%s_ExplicitInst.h\"\n/*$<NextExplictInstHeader>$*/" % (namespace, className)
        gStr = gStr.replace("/*$<NextExplictInstHeader>$*/", ginsert)

    with open(guipath, 'w') as gfile:
        gfile.write(gStr)


    print "done!"

