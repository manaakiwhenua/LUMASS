#!/usr/bin/env python

import sys



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
        nargs = sys.argv[2]
        dim = 3
    else:
        nargs = sys.argv[2]
        dim = sys.argv[3]

    classname = sys.argv[1]

    for num in range(0, len(sys.argv)):
        print str(sys.argv[num])


