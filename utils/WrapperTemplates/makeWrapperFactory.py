#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string
import re
import errno


# ===============================================================================
if __name__ == "__main__":
    """
    open the files, do the copy, and do the main
    workflow here, then we'll break out to do some things
    more special
    """
    if len(sys.argv) < 11:
        print(
            "Usage: $ %s makeWrapperFactory --cl_name <WrapperClassName> --alias <compAlias> --isSink <true | false> --author <author> --year <year> --date <date>"
        )
        sys.exit()

    pdict = {}

    for i in range(1, len(sys.argv)):
        print("processing '%s' ..." % sys.argv[i])

        if sys.argv[i] == "--cl_name":
            pdict["cl_name"] = sys.argv[i + 1]
            pdict["cl_name_lower"] = pdict["cl_name"].lower()
            pdict["cl_name_upper"] = pdict["cl_name"].upper()

            pdict["fac_name"] = sys.argv[i + 1] + "Factory"
        elif sys.argv[i] == "--isSink":
            if sys.argv[i + 1] == "true" or sys.argv[i + 1] == "false":
                pdict["isSink"] = sys.argv[i + 1]
            else:
                pdict["isSink"] = "false"
        elif sys.argv[i] == "--author":
            pdict["author"] = sys.argv[i + 1]
        elif sys.argv[i] == "--year":
            pdict["year"] = sys.argv[i + 1]
        elif sys.argv[i] == "--date":
            pdict["date"] = sys.argv[i + 1]
        elif sys.argv[i] == "--alias":
            pdict["alias"] == sys.argv[i + 1]

    # test - debug
    # for key in pdict:
    #    print "%s = %s" % (key, pdict[key])

    searchDict = {}
    searchDict["fac_name"] = "/*$<WrapperFactoryName>$*/"
    searchDict["cl_name"] = "/*$<WrapperClassName>$*/"
    searchDict["cl_name_lower"] = "/*$<WrapperClassNameLower>$*/"
    searchDict["cl_name_upper"] = "/*$<WrapperClassNameUpper>$*/"
    searchDict["fac_name"] = "/*$<WrapperFactoryName>$*/"
    searchDict["alias"] = "/*$<ComponentAlias>$*/"
    searchDict["author"] = "/*$<Author>$*/"
    searchDict["isSink"] = "/*$<ProcessIsSink>$*/"
    searchDict["year"] = "/*$<Year>$*/"
    searchDict["date"] = "/*$<FileDate>$*/"

    # =======================================================
    # create new wrapper factory class files from template files

    factoryClassName = pdict["fac_name"]

    filepath = inspect.getfile(inspect.currentframe())
    path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    if os.sep == "/":
        pos = path.rfind("/", 0)
        pos = path.rfind("/", 0, pos)
    else:
        pos = path.rfind("\\", 0)
        pos = path.rfind("\\", 0, pos)

    homepath = path[0:pos]

    print("    >>> Component Alias=%s" % pdict["alias"])

    frameworkpath = os.path.join(homepath, "modellingframework")
    targetpath = os.path.join(frameworkpath, "wrapper")
    print("    >>> lumass HOME=%s" % homepath)
    print("    >>> wrapper file path: %s" % targetpath)

    inCppPath = os.path.join(path, "WrapperFactoryTemplate.cpp")
    inHPath = os.path.join(path, "WrapperFactoryTemplate.h")

    print("   >>> inCppPath=%s" % inCppPath)
    print("   >>> inHPath=%s" % inHPath)

    classNameCPP = "%s.cpp" % (factoryClassName)
    classNameH = "%s.h" % (factoryClassName)
    cppPath = os.path.join(targetpath, classNameCPP)
    hPath = os.path.join(targetpath, classNameH)

    print("   >>> cppPath=%s" % cppPath)
    print("   >>> hPath=%s" % hPath)

    shutil.copyfile(inHPath, hPath)
    shutil.copyfile(inCppPath, cppPath)

    # =======================================================
    # process header file

    # HEADER FILE
    hStr = None
    with open(hPath, "r") as hWrapper:
        hStr = hWrapper.read()

        for key in pdict:
            hStr = hStr.replace(searchDict[key], pdict[key])

        if hStr.find("/*$<") != -1:
            print(
                "WARNING: There's likely one or more unreplaced wrapper keywords left in %s!"
                % (hPath)
            )

    with open(hPath, "w") as hWrapper:
        hWrapper.write(hStr)

    # CPP file
    cppStr = None
    with open(cppPath, "r") as cppWrapper:
        cppStr = cppWrapper.read()

        for key in pdict:
            cppStr = cppStr.replace(searchDict[key], pdict[key])

        if cppStr.find("/*$<") != -1:
            print(
                "WARNING: There's likely one or more unreplaced wrapper keywords left in %s!"
                % (cppPath)
            )

    with open(cppPath, "w") as cppWrapper:
        cppWrapper.write(cppStr)
