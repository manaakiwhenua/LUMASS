#!/usr/bin/env python

import sys
import shutil
import inspect, os
import string
import re
import errno

# ========================================================================================
def buildDict(profileFile):
    ''' parse the lumass wrapper profile and build a
        dictionary with of keywords and properties
    '''
    fp = profileFile
    
    # define keywords to look for
    keys = ["Year", "WrapperClassName", "FileDate", "Author", 
            "FilterClassFileName", "FilterTypeDef",
            "RATGetSupport", "RATSetSupport"]

    pdict = {}
    # initialise list elements
    pdict['Property'] = []
    pdict['InputTypeFunc'] = []
    
    # now crawl through the file line by line
    # and build a dictionary with string objects
    with fp as f:
        for line in f:
            cleanline = line.strip()
            if len(cleanline) == 0 or cleanline[0] == '#':
                continue

            if (line.find('Property') >= 0 and line.find('Property') < line.find('=')) \
               or (line.find('InputTypeFunc') >= 0 and line.find('InputTypeFunc') < line.find('=')):
                # handle properties
                tl = line.split('=')
                tkey = tl[0]
                if len(tl[1]) > 0:
                    tl2 = tl[1].split(':')
                    if len(tl2) == 3:
                        tl3 = []
                        for y in tl2:
                            tl3.append(y.strip())
                        if tkey.find('Property') >= 0:
                            pdict['Property'].append(tl3)
                        elif tkey.find('InputTypeFunc') >= 0:
                            pdict['InputTypeFunc'].append(tl3)
            else:
                # handle the rest
                for k in keys:
                    if line.find(k) >= 0 and line.find(k) < line.find('='):
                        tl = line.split('=')
                        if len(tl[1]) > 0:
                            pdict[k] = tl[1].strip()
                            
    return pdict                    

# ===============================================================================
def getPropertyType(propertyElementList):
    '''
    returns the wrapper type given the list of properties
    elements
    '''
    if len(propertyElementList) < 3:
        print "ERROR - Invalid property element list length!"
        return None
    propDim = int(propertyElementList[1])
    
    if propDim == 1:
        type = 'QStringList'
    elif propDim == 2:
        type = 'QList<QStringList>'
    elif propDim == 3:
        type = 'QList< QList<QStringList> >'
    else:
        print "ERROR - cannot handle property beyond 3 dimensions!"
        type = None
    
    return type

    
# ===============================================================================
def formatPropertyDefinition(propertyList):
    '''
    formats the property definition section of a header file
    given the property elements (name, dimension, type), e.g.
    Q_PROPERTY(QList<QList<QStringList> > Weights READ getWeights WRITE setWeights)
    '''     
    
    defSection = ''  
    for prop in propertyList:
        propType = getPropertyType(prop)
        tmp = "    Q_PROPERTY(%s %s READ get%s WRITE set%s);" % (propType, prop[0], prop[0], prop[0])
        defSection = defSection + '\n' + tmp
        
    return defSection

# ===============================================================================
def formatPropertyGetSet(propertyList):
    '''
    formats properties' getter and setter methods using an NMMacro, e.g. 
    NMPropertyGetSet ( Weights, QList<QList<QStringList> >)    
    '''     

    getset = ''  
    for prop in propertyList:
        propType = getPropertyType(prop)
        tmp = "    NMPropertyGetSet( %s, %s );" % (prop[0], propType)
        getset = getset + '\n' + tmp
        
    return getset


# ===============================================================================
def formatPropertyVariable(propertyList):
    '''
    formats the properties' variables 
    QList<QList<QStringList> >  mWeights;
    '''    
    
    vardef = ''
    tmp = ''  
    for prop in propList:
        propType = getPropertyType(prop)
        propVarType = prop[2]
        propDim = int(prop[1])
        
        if propDim == 0:
            tmp = "    %s m%s;" % (propVarType, prop[0])
        elif propDim > 0:
            tmp = "    %s m%s;" % (propType, prop[0])
        
        vardef = vardef + '\n' + tmp
        tmp = ''
    return vardef

# ===============================================================================
def formatTypeConversion(type):
    
    typeConv = ''
    if type == "double":
        typeConv = ".toDouble(&bok)"
    elif type == "long":
        typeConv = ".toLong(&bok)"
    elif type == "bool":
        typeConv = ".toInt(&bok)"

    return typeConv

# ===============================================================================
def formatInternalParamSetting(propertyList, className):
    '''
    formats parameter setting of the internal templated
    wrapper helper class
    '''

    #tmp = "    %s m%s" % (propType, prop[0])
    # -> setMTime 
    # -> NMProcess::mapHostIndexToPolicyIndex
    
    #step = this->mapHostIndexToPolicyIndex(givenStep, this->mMapExpressions.size());
    #QString currentExpression;
    #if (step < this->mMapExpressions.size())
    #{
    #    currentExpression = this->mMapExpressions.at(step);//.toLower();
    #    this->setInternalExpression(currentExpression);
    #}
    
    # in case something goes wrong while parsing / setting parameters, we debug
    # and throw an exception, roughly like this ...
    #NMErr("NMFocalNeighbourhoodDistanceWeighting_Internal",
    #        << "Invalid weights matrix detected!");
    #
    #NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
    #e.setMsg("Invalid weights matrix detected!");
    #throw e;
    
    paramSetting = ''
    tmp = ''
    for prop in propertyList:
        propType = getPropertyType(prop)
        propName = prop[0]
        propDim = int(prop[1])
        propVarType = prop[2]
        typeConv = formatTypeConversion(propVarType)

        if propDim == 0:
            tmp = \
            "        f->Set%s(m%s);\n" % (propName, propName)
        elif propDim == 1:
            tmp = \
            "        step = p->mapHostIndexToPolicyIndex(givenStep, p->m%s.size());\n" \
            "        %s cur%s;\n"                                                            \
            "        if (step < p->m%s.size())\n"                                         \
            "        {\n"                                                                    \
            "            cur%s = p->m%s.at(step)%s;\n"                                   \
            % (propName, propVarType, propName, propName, propName, propName, typeConv)

            test = ''
            if propVarType != "string":
                test = \
                "            if (bok)\n"                                                         \
                "            {\n"                                                                \
                "                f->Set%s(cur%s);\n"                                             \
                "            }\n"                                                                \
                "            else\n"                                                             \
                "            {\n"                                                                \
                "                NMErr(\"%s_Internal\", << \"Invalid value for '%s'!\");\n"      \
                "                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);\n"\
                "                e.setMsg(\"Invalid value for '%s'!\");\n"                       \
                "                throw e;\n"                                                     \
                "            }\n"                                                                \
                % (propName, propName, className, propName, propName)
                tmp = tmp + test
            else:
                test = \
                "                f->Set%s(cur%s);\n"                                             \
                % (propName, propName)
                tmp = tmp + test
                
            tmp = tmp + \
            "        }\n"
        else: 
            print "WARNING - cannot format multi-dimensional parameter settings yet!!"
            return None
        
        if tmp != '':
            paramSetting = paramSetting + '\n' + tmp        
        tmp = ''
        
    return paramSetting

# ===============================================================================
def formatInternalRATGetSupport():

    s = \
    "    static otb::AttributeTable::Pointer getRAT(\n"            \
    "        itk::ProcessObject::Pointer& procObj, \n"  \
    "        unsigned int numBands, unsigned int idx)\n"\
    "    {\n"                                                      \
    "        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());\n"\
    "        return f->getRAT(idx);\n"                             \
    "    }\n"

    return s

# ===============================================================================
def formatInternalRATSetSupport():

    s = \
    "    static void setRAT(\n"            \
    "        itk::ProcessObject::Pointer& procObj, \n"  \
    "        unsigned int numBands, unsigned int idx,\n"\
    "        otb::AttributeTable::Pointer& rat)\n"      \
    "    {\n"                                                      \
    "        FilterType *f = dynamic_cast<FilterType*>(procObj.GetPointer());\n"\
    "        return f->setRAT(idx, rat);\n"                        \
    "    }\n"

    return s

# ===============================================================================
def formatInternalStdSetNthInput():

    s = \
    "    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,\n"                     \
    "                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)\n"  \
    "    {\n"                                                                                   \
    "        InImgType* img = dynamic_cast<InImgType*>(dataObj);\n"                             \
    "        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());\n"         \
    "        filter->SetInput(idx, img);\n"                                                     \
    "    }\n"                                                                                   \

    return s

# ===============================================================================
def formatInternalSetNthInput(propList):

    start = \
    "    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,\n"                     \
    "                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)\n"  \
    "    {\n"                                                                                   \
    "        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());\n"

    middle = ''
    counter = 0
    for tf in propList:
        idx = tf[0]
        type = tf[1]
        func = tf[2]

        opening = ''
        if counter == 0:
            opening = "        if (idx == %s)\n" % idx
        else:
            opening = "        else if (idx == %s)\n" % idx

        body = \
        "        {\n"                                               \
        "            %s* img = dynamic_cast<%s*>(dataObj);\n"       \
        "            filter->%s(img);\n"                            \
        "        }\n"                                               \
        % (type, type, func)

        middle = middle + opening + body
        counter = counter + 1

    elsepart = ''
    if len(middle) > 0:
        elsepart = \
        "        else\n"                                                      \
        "        {\n"                                                         \
        "            InImgType* img = dynamic_cast<InImgType*>(dataObj);\n"   \
        "            filter->SetInput(idx, img);\n"                           \
        "        }\n"                                                         \

    end = "    }\n"

    return start + middle + elsepart + end

# ===============================================================================
def formatRATSetSupportWrap(className):

    return "SetRATWrap( %s, %s_Internal )\n" % (className, className)

# ===============================================================================
def formatRATGetSupportWrap(className):

    return "GetRATWrap( %s, %s_Internal )\n" % (className, className)

# ===============================================================================
def formatRATGetSupportDecl():

    return "NMItkDataObjectWrapper* getRAT(unsigned int idx);\n"

# ===============================================================================
def formatRATSetSupportDecl():

    s = "void setRAT(unsigned int idx, \n" \
        "        NMItkDataObjectWrapper* imgWrapper);\n"
    return s

# ===============================================================================
if __name__ == '__main__':
    '''
    open the files, to the copy, and do the main 
    workflow here, then we'll break out to do some things
    more special
    '''

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
    print ''
    print pDict
    print ''

    # ---------------------------------------------------------------------  
    # copying the wrapper template to new files
    print "    >>> creating wrapper class files ..."
        
    className = pDict['WrapperClassName']

    # copy file
    filepath = inspect.getfile(inspect.currentframe())
    path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    pos = string.rfind(path, '/', 0)
    pos = string.rfind(path, '/', 0, pos)
    homepath = path[0:pos]
    targetpath = homepath + "/modellingframework/wrapper"
    print "    >>> lumass HOME=%s" % homepath
    print "    >>> wrapper file path: %s" % targetpath
    
    inCppPath = "%s/WrapperTemplate.cpp" % (path)
    inHPath   = "%s/WrapperTemplate.h" % (path)
    
    cppPath = "%s/%s.cpp" % (targetpath, className)
    hPath = "%s/%s.h" % (targetpath, className)
    
    shutil.copyfile(inHPath, hPath)
    shutil.copyfile(inCppPath, cppPath)

    # -----------------------------------------------------------------------
    # process copied files
    print "    >>> processing wrapper header file ..."    

    #/*$<InternalRATGetSupport>$*/
    #/*$<InternalRATSetSupport>$*/
    #/*$<RATGetSupportWrap>$*/
    #/*$<RATSetSupportWrap>$*/
    #/*$<RATGetSupportDecl>$*/
    #/*$<RATSetSupportDecl>$*/

    # HEADER FILE
    hStr = None
    with open(hPath, 'r') as hWrapper:
        hStr = hWrapper.read()

        for key in pDict:
            if key == 'Property':
                propList = pDict[key]
                
                # property definition, e.g. Q_PROPERTY(QList<QList<QStringList> > Weights READ getWeights WRITE setWeights)
                propDef    = formatPropertyDefinition(propList)
                hStr = hStr.replace("/*$<WrapperPropertyList>$*/", propDef)
                
                propGetSet = formatPropertyGetSet(propList)
                hStr = hStr.replace("/*$<WrapperPropertyGetSetter>$*/", propGetSet)
                
                propVarDef = formatPropertyVariable(propList)
                hStr = hStr.replace("/*$<PropertyVarDef>$*/", propVarDef)
            
            elif key == 'RATGetSupport':
                getsupp = int(pDict[key])
                if getsupp == 1:
                    hStr = hStr.replace("/*$<RATGetSupportDecl>$*/", formatRATGetSupportDecl())

            elif key == 'RATSetSupport':
                setsupp = int(pDict[key])
                if setsupp == 1:
                    hStr = hStr.replace("/*$<RATSetSupportDecl>$*/", formatRATSetSupportDecl())

            elif key != 'InputTypeFunc':
                keyword = "/*$<%s>$*/" % str(key)
                hStr = hStr.replace(keyword, pDict[key])
                
        # check whether there is any keyword left unreplaced ...
        if hStr.find("/*$<") != -1:
            print "WARNING: There's likely one or more unreplaced wrapper keywords left in %s!" % (hPath)
    
    with open(hPath, 'w') as hWrapper:
        hWrapper.write(hStr)
    


    print "    >>> processing wrapper c++ implementation file ..."
    # CPP FILE
    if 'RATGetSupport' in pDict:
        RATGetSupp = int(pDict['RATGetSupport'])
    else:
        RATGetSupp = 0

    putStdNthInput = False
    if len(pDict['InputTypeFunc']) == 0:
        putStdNthInput = True

    cppStr = None
    with open(cppPath, 'r') as cppWrapper:
        cppStr = cppWrapper.read()

        if RATGetSupp == 0:
            cppStr = cppStr.replace("/*$<GetOutPutWrap>$*/", "GetOutputWrap")
        else:
            cppStr = cppStr.replace("/*$<GetOutPutWrap>$*/", "GetOutputRATWrap")

        if putStdNthInput:
            # internal setNthInput formatting
            stdsetinput = formatInternalStdSetNthInput()
            cppStr = cppStr.replace("/*$<InternalSetNthInput>$*/", stdsetinput)


        for key in pDict:
            if key == 'Property':
                propList = pDict[key]
                
                # internal property setting 
                paramSetting = formatInternalParamSetting(propList, className)
                cppStr = cppStr.replace("/*$<InternalFilterParamSetter>$*/", paramSetting)
            
            elif key == 'RATGetSupport':
                getsupp = int(pDict[key])
                if getsupp == 1:
                    cppStr = cppStr.replace("/*$<InternalRATGetSupport>$*/", \
                                        formatInternalRATGetSupport())
                    cppStr = cppStr.replace("/*$<RATGetSupportWrap>$*/", \
                                        formatRATGetSupportWrap(className))
            elif key == 'RATSetSupport':
                setsupp = int(pDict[key])
                if setsupp == 1:
                    cppStr = cppStr.replace("/*$<InternalRATSetSupport>$*/", \
                                        formatInternalRATSetSupport())
                    cppStr = cppStr.replace("/*$<RATSetSupportWrap>$*/", \
                                        formatRATSetSupportWrap(className))

            elif key == 'InputTypeFunc':
                typefuncs = pDict[key]

                # internal setNthInput formatting
                setinput = formatInternalSetNthInput(typefuncs)
                cppStr = cppStr.replace("/*$<InternalSetNthInput>$*/", setinput)

            else:
                keyword = "/*$<%s>$*/" % str(key)
                cppStr = cppStr.replace(keyword, pDict[key])
                
        # check whether there is any keyword left unreplaced ...
        if hStr.find("/*$<") != -1:
            print "WARNING: There's likely one or more unreplaced wrapper keywords left in %s!" % (cppPath)
    
    with open(cppPath, 'w') as cppWrapper:
        cppWrapper.write(cppStr)

    print "    >>> I'm done!"