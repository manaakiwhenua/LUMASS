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
            "RATGetSupport", "RATSetSupport",
            "ForwardInputUserIDs", "NumTemplateArgs",
            "ComponentName", "ComponentIsSink"]

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
                    if len(tl2) >= 3:
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
    elements, which could look like this:
        OutputSpacing:2:double:OutputSpacingValueType
    '''
    if len(propertyElementList) < 3:
        print "ERROR - Invalid property element list length!"
        return None
    propDim = int(propertyElementList[1])
    
    if propDim == 0:
        type = propertyElementList[2]
    elif propDim == 1:
        type = 'QStringList'
    elif propDim == 2:
        type = 'QList<QStringList>'
    elif propDim == 3:
        type = 'QList< QList<QStringList> >'
    else:
        print "ERROR - cannot handle property with 3+ dimensions!"
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
        tmp = "    Q_PROPERTY(%s %s READ get%s WRITE set%s)" % (propType, prop[0], prop[0], prop[0])
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
        tmp = "    NMPropertyGetSet( %s, %s )" % (prop[0], propType)
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
        #propVarType = prop[2]
        #propDim = int(prop[1])
        
        tmp = "    %s m%s;" % (propType, prop[0])

        #if propDim == 0:
        #    tmp = "    %s m%s;" % (propVarType, prop[0])
        #elif propDim > 0:
        #    tmp = "    %s m%s;" % (propType, prop[0])

        
        vardef = vardef + '\n' + tmp
        tmp = ''
    return vardef

# ===============================================================================
def formatTypeConversion(type):
    
    typeConv = ''
    if type == "int":
        typeConv = ".toInt(&bok)"
    elif type == "unsigned int":
        typeConv = ".toUInt(&bok)";
    elif type == "double":
        typeConv = ".toDouble(&bok)"
    elif type == "long":
        typeConv = ".toLong(&bok)"
    elif type == "long long":
        typeConv = ".toLongLong(&bok)"
    elif type == "bool":
        typeConv = ".toInt(&bok)"
    elif type == "std::string" or type == "string":
        typeConv = ".toString().toStdString()"
        #typeConv = ".toStdString().c_str()"

    return typeConv

# ===============================================================================
def formatInternalParamSetting(propertyList, className):
    '''
    formats parameter setting of the internal templated
    wrapper helper class; note propertyList could look like this:
            OutputSpacing:2:double:OutputSpacingValueType
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
    #e.setDescription("Invalid weights matrix detected!");
    #throw e;
    
    paramSetting = ''
    tmp = ''
    for prop in propertyList:
        propType = getPropertyType(prop)
        propName = prop[0]
        propDim = int(prop[1])
        propVarType = prop[2]
        if propVarType == 'string':
            propVarType = 'std::string'

        # no longer exclusively a string to number conversion,
        # we forgot that we have to convert QString to std::string
        # as well
        strToNumConv = formatTypeConversion(propVarType)

        varTargetType = propVarType
        varTargetCast = ''
        varTargetPointerCast = ''
        propTypeVector = False

        if propDim == 2:
            if len(prop) >= 4:
                varTargetType = prop[3]
                varTargetCast = "static_cast<%s>" % varTargetType
                varTargetPointerCast = "static_cast<%s*>" % varTargetType

            if len(prop) >= 5:
                if prop[4] == 'vector':
                    propTypeVector = True


        print "parsed property 'attributes' for %s: ..." % propName
        elstr = ''
        for elm in prop:
            elstr = elstr + elm + ' '
        print "raw list = %s" % elstr
        print "propVarType=%s\nvarTargetType=%s\nvarTargetCast=%s\nvarTargetPointerCast=%s\npropTypeVector=%s\n" \
                % (propVarType, varTargetType, varTargetCast, varTargetPointerCast, propTypeVector)

        if propDim == 0:
            tmp = \
            "        f->Set%s(%s(m%s));\n" % (propName, varTargetCast, propName)

        # ------------------------------------- QStringList -------------------------------------
        elif propDim == 1:
            tmp = \
            "        QVariant cur%sVar = p->getParameter(\"%s\");\n" \
            "        %s cur%s;\n"                                                            \
            "        if (cur%sVar.isValid())\n" \
            "        {\n" \
            "            cur%s = cur%sVar%s;\n" \
            % (propName, propName, propVarType, propName, propName, propName, propName, strToNumConv)

            test = ''
            if propVarType != "std::string":
                test = \
                "            if (bok)\n"                                                         \
                "            {\n"                                                                \
                "                f->Set%s(%s(cur%s));\n"                                         \
                "            }\n"                                                                \
                "            else\n"                                                             \
                "            {\n"                                                                \
                "                NMErr(\"%s_Internal\", << \"Invalid value for '%s'!\");\n"      \
                "                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);\n"\
                "                e.setDescription(\"Invalid value for '%s'!\");\n"                       \
                "                throw e;\n"                                                     \
                "            }\n"                                                                \
                % (propName, varTargetCast, propName, className, propName, propName)
                tmp = tmp + test
            else:
                test = \
                "            f->Set%s(cur%s);\n"                                             \
                % (propName, propName)
                tmp = tmp + test
                
            tmp = tmp + \
            "        }\n"

        # ---------------------------------- QList< QStringList > -------------------------------
        elif propDim == 2:

            tmp = \
            "        QVariant cur%sVar = p->getParameter(\"%s\");\n" \
            "        if (cur%sVar.isValid())\n"                      \
            "        {\n"                                            \
            "           std::vector<%s> vec%s;\n"                                         \
            "           QStringList curValVarList = cur%sVar.toStringList();\n"            \
            "           foreach(const QString& vStr, curValVarList) \n"                   \
            "           {\n"                                        \
            "                %s cur%s = vStr%s;\n"                                          \
            % (propName, propName, propName, varTargetType, propName, propName, propVarType, propName, strToNumConv)

            test = ''
            if propVarType != "std::string":
                test = \
                "                if (bok)\n"                                                         \
                "                {\n"                                                                \
                "                    vec%s.push_back(%s(cur%s));\n"                                  \
                "                }\n"                                                                \
                "                else\n"                                                             \
                "                {\n"                                                                \
                "                    NMErr(\"%s_Internal\", << \"Invalid value for '%s'!\");\n"      \
                "                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);\n"\
                "                    e.setDescription(\"Invalid value for '%s'!\");\n"                       \
                "                    throw e;\n"                                                     \
                "                }\n"                                                                \
                % (propName, varTargetCast, propName, className, propName, propName)
                tmp = tmp + test
            else:
                test = \
                "            vec%s.push_back(cur%s);\n"                                             \
                % (propName, propName)
                tmp = tmp + test

            if propTypeVector:
                tmp = tmp + \
                "            }\n"                              \
                "            f->Set%s(vec%s);\n"               \
                "        }\n"                                  \
                % (propName, propName)
            else:
                tmp = tmp + \
                "            }\n"                              \
                "            if (vec%s.size() > 0)\n"          \
                "            {\n"                              \
                "                f->Set%s(%s(&vec%s[0]));\n"   \
                "            }\n"                              \
                "            else\n"                           \
                "            {\n"                              \
                "                f->Set%s(0);\n"               \
                "            }\n"                              \
                "        }\n"                                  \
                % (propName, propName, varTargetPointerCast, propName, propName)

        else:
            print "WARNING - cannot format multi-dimensional parameter settings yet!!"
            return None
        
        if tmp != '':
            paramSetting = paramSetting + '\n' + tmp        
        tmp = ''
        
    return paramSetting

# ===============================================================================
def formatForwardInputUserIDs(funcName):

    s = "\n"\
    "	    step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());				\n"  \
    "	    std::vector<std::string> userIDs;                                                                       \n"  \
    "	    QStringList currentInputs;                                                                              \n"  \
    "	    if (step < p->mInputComponents.size())                                                                  \n"  \
    "	    {                                                                                                       \n"  \
    "		    currentInputs = p->mInputComponents.at(step);                                                   \n"  \
    "		    int cnt=0;                                                                                      \n"  \
    "		    foreach (const QString& input, currentInputs)                                                   \n"  \
    "		    {                                                                                               \n"  \
    "		        std::stringstream uid;                                                                      \n"  \
    "		        uid << \"L\" << cnt;                                                                          \n"  \
    "		        QString inputCompName = NMModelController::getComponentNameFromInputSpec(input);            \n"  \
    "		        NMModelComponent* comp = NMModelController::getInstance()->getComponent(inputCompName);     \n"  \
    "		        if (comp != 0)                                                                              \n"  \
    "		        {                                                                                           \n"  \
    "			        if (comp->getUserID().isEmpty())                                                        \n"  \
    "			        {                                                                                       \n"  \
    "				        userIDs.push_back(uid.str());                                                   \n"  \
    "			        }                                                                                       \n"  \
    "			        else                                                                                    \n"  \
    "			        {                                                                                       \n"  \
    "				        userIDs.push_back(comp->getUserID().toStdString());                             \n"  \
    "			        }                                                                                       \n"  \
    "		        }                                                                                           \n"  \
    "		        else                                                                                        \n"  \
    "		        {                                                                                           \n"  \
    "			        userIDs.push_back(uid.str());                                                           \n"  \
    "		        }                                                                                           \n"  \
    "		        ++cnt;                                                                                      \n"  \
    "		    }                                                                                               \n"  \
    "	    }                                                                                                       \n"  \
    "	    f->%s(userIDs);" % (funcName)
    return s



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

    return "QSharedPointer<NMItkDataObjectWrapper> getRAT(unsigned int idx);\n"

# ===============================================================================
def formatRATSetSupportDecl():

    s = "void setRAT(unsigned int idx, \n" \
        "        QSharedPointer<NMItkDataObjectWrapper> imgWrapper);\n"
    return s


# ===============================================================================
def formatInternalHelperInst(clname, nargs):
    '''
    explicit instantiation of the templated internal
    helper class to set the filters parameters
    during pipeline execution

    '''

    dt1 = ['unsigned char', 'char', 'unsigned short', 'short', \
           'unsigned int', 'int', 'unsigned long', 'long', \
           'float', 'double']
    dt2 = dt1

    classname = clname + "_Internal"
    ndim = 3
    instStr = ''
    if nargs == 1:
        for dim in range(1, int(ndim)+1):
            for t1 in dt1:
                inst = "template class %s<%s, %s>;\n"   \
                     % (classname, t1, dim)
                instStr = instStr + inst

    elif nargs == 2:
        for dim in range(1, int(ndim)+1):
            for t1 in dt1:
                for t2 in dt2:
                    inst = "template class %s<%s, %s, %s>;\n"   \
                         % (classname, t1, t2, dim)
                    instStr = instStr + inst

    return instStr


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
    if os.sep == '/':
        pos = string.rfind(path, '/', 0)
        pos = string.rfind(path, '/', 0, pos)
    else:
        pos = string.rfind(path, '\\', 0)
        pos = string.rfind(path, '\\', 0, pos)

    homepath = path[0:pos]

    frameworkpath = os.path.join(homepath, 'modellingframework')
    targetpath = os.path.join(frameworkpath, 'wrapper')
    print "    >>> lumass HOME=%s" % homepath
    print "    >>> wrapper file path: %s" % targetpath

    inCppPath = os.path.join(path, 'WrapperTemplate.cpp')
    inHPath   = os.path.join(path, 'WrapperTemplate.h')

    print "   >>> inCppPath=%s" % inCppPath
    print "   >>> inHPath=%s" % inHPath

    classNameCPP = "%s.cpp" % (className)
    classNameH = "%s.h" % (className)
    cppPath = os.path.join(targetpath, classNameCPP)
    hPath = os.path.join(targetpath, classNameH)

    print "   >>> cppPath=%s" % inCppPath
    print "   >>> hPath=%s" % hPath


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

            elif key == 'ForwardInputUserIDs':
                if len(pDict['ForwardInputUserIDs']) > 0:
                    cppStr = cppStr.replace("/*$<ForwardInputUserIDs_Include>$*/", '#include \"NMModelController.h\"')
                    forwardUserIDsStr = formatForwardInputUserIDs(pDict['ForwardInputUserIDs'])
                    cppStr = cppStr.replace("/*$<ForwardInputUserIDs_Body>$*/", forwardUserIDsStr)

            elif key == 'NumTemplateArgs':
                nargs = int(pDict[key])
                clname = pDict['WrapperClassName']

                if nargs > 0 and clname <> '':
                    helpinst = formatInternalHelperInst(clname, nargs)
                    cppStr = cppStr.replace("/*$<HelperClassInstantiation>$*/", helpinst)

            else:
                keyword = "/*$<%s>$*/" % str(key)
                cppStr = cppStr.replace(keyword, pDict[key])
                
        # check whether there is any keyword left unreplaced ...
        if hStr.find("/*$<") != -1:
            print "WARNING: There's likely one or more unreplaced wrapper keywords left in %s!" % (cppPath)
    
    with open(cppPath, 'w') as cppWrapper:
        cppWrapper.write(cppStr)

    print "    >>> I'm done with the wrapper class!"


    # -----------------------------------------------------------------------
    # integrate wrapper class into ProcessFactory
    # -----------------------------------------------------------------------

    print "    >>> framework integration ..."

    #/*$<IncludeWrapperHeader>$*/   = #include "/*$<WrapperClassName>$*/.h"
    #ComponentName   /*$<RegisterComponentName>$*/    = mProcRegister << QString::fromLatin1($<ComponentName>$);
    #ComponentIsSink    /*$<RegisterComponentAsSink>$*/  = mSinks << QString::fromLatin1($<ComponentName>$);

    procFactoryPath = os.path.join(frameworkpath, 'NMProcessFactory.cpp')

    procFactoryStr = None
    with open(procFactoryPath, 'r') as procFactory:
        procFactoryStr = procFactory.read()

    compNameStr = pDict['ComponentName']

    wrapperInclude = "#include \"%s.h\"\n/*$<IncludeWrapperHeader>$*/" % (pDict['WrapperClassName'])
    procFactoryStr = procFactoryStr.replace('/*$<IncludeWrapperHeader>$*/', wrapperInclude)

    regCompNameStr = "    mProcRegister << QString::fromLatin1(\"%s\");\n/*$<RegisterComponentName>$*/" % compNameStr
    procFactoryStr = procFactoryStr.replace('/*$<RegisterComponentName>$*/', regCompNameStr)

    if not pDict['ComponentIsSink'] is None:
        if int(pDict['ComponentIsSink']) == 1:
            regSingCompStr = "    mSinks << QString::fromLatin1(\"%s\");\n/*$<RegisterComponentAsSink>$*/)" % compNameStr


    nameFromAliasStr = \
        "    else if (alias.compare(\"%s\") == 0)\n"            \
        "    {\n"                                               \
        "        return \"%s\";\n"                              \
        "    }\n"                                               \
        "/*$<WrapperClassNameFromComponentName>$*/"       \
        % (compNameStr, pDict['WrapperClassName'])
    procFactoryStr = procFactoryStr.replace('/*$<WrapperClassNameFromComponentName>$*/', nameFromAliasStr)


    createProcStr = \
        "    else if (procClass.compare(\"%s\") == 0)\n"                    \
        "    {\n"                                                           \
        "        proc = new %s(this);\n"                                    \
        "    }\n"                                                           \
        "/*$<CreateProcessObjFromWrapperClassName>$*/"                \
        % (pDict['WrapperClassName'], pDict['WrapperClassName'])

    procFactoryStr = procFactoryStr.replace('/*$<CreateProcessObjFromWrapperClassName>$*/', createProcStr)


    with open(procFactoryPath, 'w') as procFactory:
        procFactory.write(procFactoryStr)


    print "    >>> I'm done with the ProcessFactory integration!"


    # -----------------------------------------------------------------------
    # add process to process component list (GUI)
    # -----------------------------------------------------------------------

    print "    >>> user interface integration ..."

    guipath = os.path.join(homepath, 'gui')
    procListPath = os.path.join(guipath, 'NMProcCompList.cpp')

    procListStr = None
    with open(procListPath, 'r') as procList:
        procListStr = procList.read()


    guiliststr = \
        "    this->addItem(QString::fromLatin1(\"%s\"));\n"     \
        "/*$<AddComponentToGUICompList>$*/"                     \
        % compNameStr
    procListStr = procListStr.replace('/*$<AddComponentToGUICompList>$*/', guiliststr)


    with open(procListPath, 'w') as procList:
        procList.write(procListStr)

    print "    >>> I'm done!"









