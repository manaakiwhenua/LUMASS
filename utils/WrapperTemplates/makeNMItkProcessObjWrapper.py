#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string
import re


def buildDict(profileFile):
    ''' parse the lumass wrapper profile and build a
        dictionary with of keywords and properties
    '''
    fp = profileFile
    
    # define keywords to look for
    keys = ["Year", "WrapperClassName", "FileDate", "Author", 
                "FilterClassName"]
    pdict = {}
    # initialise an empty property list 
    pdict['Property'] = []
    
    # now crawl through the file line by line
    # and build a dictionary with string objects
    with fp as f:
        for line in f:
            if line.find('Property') >= 0 and line.find('Property') < line.find('='):
                # handle properties
                tl = line.split('=')
                if len(tl[1]) > 0:
                    tl2 = tl[1].split(':')
                    if len(tl2) == 3:
                        tl3 = []
                        for y in tl2:
                            tl3.append(y.strip())
                        pdict['Property'].append(tl3)
            else:
                # handle the rest
                for k in keys:
                    if line.find(k) >= 0 and line.find(k) < line.find('='):
                        tl = line.split('=')
                        if len(tl[1]) > 0:
                            pdict[k] = tl[1].strip()
                            
    return pdict                    
       


# open the files, to the copy, and do the main 
# workflow here, then we'll break out to do somethings
# more special
if __name__ == '__main__':

    # -----------------------------------------------------------------------
    # check input profile file
    
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
    
    # -----------------------------------------------------------------------
    # parsing the profile file
    
    print "    >>> using '%s' as lumass wrapper profile ..." % str(profile)
    pDict = buildDict(fp)
    print pDict
  
    
    # ---------------------------------------------------------------------  
    # copying the wrapper template to new files
    print "    >>> creating wrapper class files ..."
        
    className = pDict['WrapperClassName']

    # copy file
    filepath = inspect.getfile(inspect.currentframe())
    path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    targetpath="/home/alex/tmp"
    
    for tf in ('h', 'cpp'):
        inFN  = "%s/WrapperTemplate.%s" % (path, tf)
        outFN = "%s/%sWrapper.%s" % (targetpath, className, tf)
        shutil.copyfile(inFN, outFN)
        
    
        
        
        
    

    
    # something for replacement rather than scanning
    #"InternalFilterParamSetter", 
    #            "WrapperPropertyList", "WrapperPropertyGetSetter", 
    #            "PropertyVarDef"]
    