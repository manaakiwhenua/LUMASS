---
title: "Dynamic Model Parameters"
permalink: "/docs/mod_dynamic_parameters"
--- 

To support the representation of dynamic systems as well as enabling the assessment of model sensitivities and uncertainties, LUMASS provides the concept of dynamic parameters by way of i) parameter lists for supplying different values to component properties during an iterative model execution and ii) parameter expressions for the dynamic generation of component property values depending on user-defined model configuration options or the state of other model components or input datasets. 

## Parameter Lists

[![Dynamic_Model_Parameters]({{ "/assets/images/docs/dynamic_parameters.png" | relative_url }}){:height="55%" width="55%"}]({{ "/assets/images/docs/dynamic_parameters.png" | relative_url }})<br>
**Parameter lists**

 A parameter list provides a set of values (`1..n`) to a particular component property. During an iterative execution (`1..m`) of the component’s host component (s. AggregateComponent in figure above), these parameter values are passed on to the process component’s property one after another, as long as there are more values on the list. In case the iteration continues beyond the last available parameter value, i.e. `m > n`, the last value is re-used for the remainder of the iterations. By default, an iteration starts with `IterationStep = 1` (cf. [General model and aggregate component properties]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }})). However, it may be re-configured by the user, e.g. to debug or test a particular modelling step or to disable a particular component (`IterationStep > NumIterations`, cf. [Branching and looping]({{ "/docs/mod_execution_control#branching-and-looping" | relative_url }})). Disabled aggregate componets are indicated by transparent background. 

## Parameter Expressions

Parameter expressions extend LUMASS’ capabilities to dynamically generate model parameters at runtime (s. ). They enable 

- access to `UserID`, pre-defined list values, and numeric property values of model components, 
- retrieval of table properties and values, 
- evaluation of mathematical expressions to calculate numerical parameter values,
- application of auxillary functions, e.g. for string processing, conditional generation of text-based parameter values, or comparing file modification dates, and 
- access to (global) model configuration parameters.

LUMASS distinguishes four different types of paramater expressions: i) value expressions, ii) mathematical expressions, iii) functional expressions, and iv) global value expressions. All four parameter expression types may be nested and may occur anywhere inside a string-typed component property specification. Nested parameter expressions are evaluated successively from the inside to the outside. Individual expressions at the same nesting level are evaluated at the same time. The return value of an expression 'replaces' the ex

**Info**: The return value of each expression is always of type `string`, regardless of whether it represents a number, a character sequence, or a boolean value. The resulting `string` value will be converted into the respective parameter type during the pipeline initialisation phase, immediately before the execution of the next iteration step. Please refer to the [Model Component Reference]({{ "/docs/mod_component_reference" | relative_url }}) for information on model parameter types. 
{: .notice--info}

**Important #1**: Parameter expressions are evaluated ***before*** a model (component) is executed and the generated parameter values are applied for the next iteration (execution) of the component. Note that the sequential execution of a processing pipeline still represents only a single iteration. Any code or expressions, such as `JSMapKernelScript` `KernelScript`s or [`MapAlgebra` `MapExpression`s]({{ "docs/cref_map_algebra#mapexpressions" | relative_url }}), e.g. `InputLayer__InputColumn > 100 ? 75 : 50`, is evaluated during the execution of a given model component, i.e. ***after*** all parameter expressions have been evaluated. Therefore, elements of such expressions, e.g. `InputLayer__InputColumn`, must not be used as argument of a parameter expression. However, parameter expressions may be nested inside, for example a `MapExpression`, as long as they can be sucessfully evaluated before the `MapAlgebra` component is executed. 
{: .notice--warning}

**Important #2**: Before an expression is evaluated, the expression (string) is simplified, i.e. all leading and trailing whitespaces (`'\t', '\n', '\v', '\f', '\r', and ' '`) are removed and all sequences of whitespaces inside the expression are replaced by a single whitespace (`' '`).
{: .notice--warning}

### Note

Expression examples provided in the following sections are provided in the following form: 
```
$[<expression>]$    : <return value>
```


### Value Expressions

Parameter expressions to access component property values or to retrieve table values take the following general form: 

```
$[<component>:<property | column>:<list index | table row (primary-key value)>]$
```

- `<component>`: Unique `ComponentName` or `UserID` of the component providing the parameter. To access table values, the referenced component must be either a `ParameterTable` or a [`DataBuffer`]({{ "docs/cref_databuffer#overview" | relative_url }}) (or `DataBufferReference`).  

    **Important**: Since a `UserID` is a non-unique identifier, it references the first upstream component matching the given `UserID` that LUMASS finds on the following search path: i) the component itself, ii) all upstream components belonging to the same processing piepline, iii) all components within the same [aggregate component]({{ "/docs/mod_aggregate_components" | relative_url}}) (host component) sitting on a higher `TimeLevel`, iv) all components within the host's host component up to the highest aggregation level, the `root` component. During this step the search is directed strictly upward, i.e. child components of any aggregate component found along the way, are ignored.
    {: .notice--warning}

    **Note**: Please also refer to the function expression [`hasAttribute`]({{ "docs/mod_dynamic_parameters#hasattribute" | relative_url }}) to test for the presence of specific table columns.
    {: .notice--info}

- `<property | column>`: Property or table column name<br>
- `<index | table row (primary-key value)>`: In case the given property name references a list of values, the index refers to the 1-based position within that list.

    **Note**: If the expression references a table column value, the index value actually refers to the table’s primary key. If the primary key is 0-based, LUMASS adjusts the user-specified 1-based index automatically to deliver the appropriate result. If the given index cannot be found in the table, model execution is aborted and LUMASS reports an error in the Notifications window.
    {: .notice--info}

#### Value Expression Examples

[![ValueExprTableDemo]({{ "/assets/images/docs/value_expr_table_demo.png" | relative_url }}){:height="80%" width="80%"}]({{ "/assets/images/docs/value_expr_table_demo.png" | relative_url }})<br>
**Value expressions**

```
$[CatIter:IterationStep]$              : 14
$[CatIter]$                            : 14
$[AggrComp1]$                          : 14
```
All expressions retrieve the value of the `IterationStep` property of aggregate component `AggrComp1` with `UserID=CatIter`. The second and third expression are a shorthand notation to read the `IterationStep` property value of a process or aggregate component. The second expression references the component by its `UserID` and the third by its unique `ComponentName`.

```
$[para:LuOpt:14]$                      : Oni
$[para:LuOpt:$[CatIter]$]$             : Oni
```
Both expressions read the value of `ParameterTable` with `TableName=SeqOpt_HL` in column `LuOpt` with `rowid=14`. The second expression uses `CatIter`'s `IterationStep` property in a nested expression to specify the table's primary-key value for looking-up the table's value in column `LuOpt`.

```
$[para:LuOpt:Oni]$                     : 14
$[para:MinAreaHa:$[para:LuOpt:Oni]$]$  : 1210
```
Value expressions can also be used to retrieve the primary-key value (`rowid`) of a specific value in a given column. Note that this query yields an ambiguous result if the specified value occurs more than once in that column. The second expression demonstrates how a `rowid` look-up can be used to acess specific values in other columns, e.g. the `MinAreaHa` value for `LuOpt=Oni`.

### Math Expressions

A mathematical parameter expression is initiated by the key word `math` and is followed by a mathematical expression as understood by the mathematical function parser [muparser](https://beltoforion.de/en/muparser/features.php#idStart). 

```
$[math: <mathematical expression>]$
```

- `<mathematical expression>`: muparser expression that supports the [functions](https://beltoforion.de/en/muparser/features.php#idDef1) and [operators](https://beltoforion.de/en/muparser/features.php#idDef2) listed below.

    **Functions**

    Name | Number of args. | Explanation
    -----|:---------------:|---------------
    sin 	| 1 	| sine function
    cos 	| 1 	| cosine function
    tan 	| 1 	| tangens function
    asin 	| 1 	| arcus sine function
    acos 	| 1 	| arcus cosine function
    atan 	| 1 	| arcus tangens function
    sinh 	| 1 	| hyperbolic sine function
    cosh 	| 1 	| hyperbolic cosine
    tanh 	| 1 	| hyperbolic tangens function
    asinh | 	1 | 	hyperbolic arcus sine function
    acosh | 	1 | 	hyperbolic arcus tangens function
    atanh | 	1 | 	hyperbolic arcur tangens function
    log2 	| 1 	| logarithm to the base 2
    log10 | 	1 | 	logarithm to the base 10
    log 	| 1 	| logarithm to base e (2.71828...)
    ln 	  | 1 	| logarithm to base e (2.71828...)
    exp 	| 1 	| e raised to the power of x
    sqrt 	| 1 	| square root of a value
    sign 	| 1 	| sign function -1 if x<0; 1 if x>0
    rint 	| 1 	| round to nearest integer
    abs 	| 1 	| absolute value
    min 	| var.|  	min of all arguments
    max 	| var.|  	max of all arguments
    sum 	| var.|  	sum of all arguments
    avg 	| var.|  	mean value of all arguments
    
    <br>

    **Operators** 

    Operator | 	Description |	Priority
    ---------|--------------|------------
    && 	| logical and 	    | 1
    \|\| 	| logical or 	      | 2
    <= 	| less or equal 	  | 4
    >= 	| greater or equal 	| 4
    != 	| not equal 	      | 4
    == 	| equal 	          | 4
    > 	| greater than 	    | 4
    < 	| less than 	      | 4
    + 	| addition 	        | 5
    - 	| subtraction 	    | 5
    * 	| multiplication 	  | 6
    / 	| division 	        | 6
    ^ 	| raise x to the power of y | 7
    ?:  | ternary if-then-else operator |  

#### Math Expression Examples

[![MathExprExamples]({{ "/assets/images/docs/mathexpr_numiter_paracalc_demo.png" | relative_url }})]({{ "/assets/images/docs/mathexpr_numiter_paracalc_demo.png" | relative_url }})<br>
**Math expressions - parameter calculation**


```
$[math: 
    $[para:Prep:$[Preprocessor]$]$ 
 && $[para:Rasterize:$[Preprocessor]$]$
]$  : 1
```

The above `NumIterationsExpression` of `AggrComp97` (`UserID=RasterizeLandCover`, cf. [Math expressions - parameter calculation]({{ "/assets/images/docs/mathexpr_numiter_paracalc_demo.png" | relative_url }})) uses a logical expression (`<num1> && <num2>`) to control whether the component is going to be executed in this iteration or not. For the component to be executed, both arguments of the logical 'and' operator (`&&`) need to evaluate to a non-zero numeric value. The nested value expressions

```
$[para:Prep:$[Preprocessor]$]$              : 1 (<num1>)
$[para:Rasterize:$[Preprocessor]$]$         : 1 (<num2>)
``` 

and fetch the values of columns `Prep` and `Rasterize` of table `para` in the row that corresponds to the `IterationStep` of the component with `UserID=Preprocessor`, respectively. Since both columns show a value of `1` in the first table row, both expressions evaluate to `1` and thus does the main expression (`$[math: <num1> && <num2>]$`).

```
$[math:
    $[func:strListItem($[para:Extent:1]$, " ", 0)]$ 
 +  $[func:strListItem($[para:CellSize:1]$, " ", 0)]$ * 0.5
]$  : 1080107.5
```

The second expression (above) depicted in [Math expressions - parameter calculation]({{ "/assets/images/docs/mathexpr_numiter_paracalc_demo.png" | relative_url }})) calculates the x-coordinate of the `OutputOrigin` parameter of `ResampleImage1` (`Description=cookie cutter`). It utilises two [function expressions]({{ "docs/mod_dynamic_parameters#function-expressions" | relative_url }}) to retrieve the mininum x-cooridnate and pixel size along the x-axis from the `Extent` and `CellSize` columns of table `para` to calculate output origin's x-coordinate.


### Function Expressions

Functional parameter expressions are indicated by the 'func' keyword and take the following general form: 

```
$[func:<functionSignature>]$
```

`<functionSignature>` represents a complete function signature, i.e. function name and function parameters.


#### File Functions

**Info**: We strongly recommend the use of the forward slash `/` as universal file path separator on Linux **and** Windows whenever a filename is specified manually, i.e. without using a file dialog. In conjunction with the use of other expressions, e.g. [global value expressions]({{ "docs/mod_dynamic_parameters#global-value-expressions" | relative_url }}), to refer to base directories, it enables the development of 'cross-platform' models. Additionally it prevents the need for escaping the backslash path separator (`\\`) on Windows platforms. 
{: .notice--info}

##### fileBaseName

```
$[func:fileBaseName(<filename>)]$                      : <string>
```
The function returns the filename without path and any filename extension. It does not test whether the specified file does actually exist on the given system (cf. [isFile]({{ "docs/mod_dynamic_parameters#isfile" | relative_url }})).

```
$[func:fileBaseName("/home/dir/anImage.kea")]$         : anImage
$[func:fileBaseName("D:/dir/lumass.tar.gz")]$          : lumass
$[func:fileBaseName("lumass.tar.gz")]$                 : lumass
```


##### fileCompleteBaseName

```
$[func:fileCompleteBaseName(<filename>)]$              : <string>
```
This function returns the filename without path and the *last* filename extension.  

```
$[func:fileCompleteBaseName("/home/dir/anImage.kea")]$ : anImage
$[func:fileCompleteBaseName("D:/dir/lumass.tar.gz")]$  : lumass.tar
$[func:fileCompleteBaseName("lumass.tar.gz")]$         : lumass.tar
```

##### fileCompleteSuffix

```
$[func:fileCompleteSuffix(<filename>)]$                : <string>
```
This function returns all filename extensions of filename.  

```
$[func:fileCompleteSuffix("/home/dir/anImage.kea")]$   : kea 
$[func:fileCompleteSuffix("D:/dir/lumass.tar.gz")]$    : tar.gz
$[func:fileCompleteSuffix("lumass.tar.gz")]$           : tar.gz
```

##### fileOneIsNewer

```
$[func:fileOneIsNewer(<filename_one>, <filename_two>)]$       : <boolean {0,1}>
```
This function tests whether the first specified file (filename_one) has been modified more recently than the second file specified (filename_two). 

[![ExprFileIsNewer]({{ "/assets/images/docs/expr_fileoneisnewer.png" | relative_url}}){:height="55%" width="55%"}]({{ "/assets/images/docs/expr_fileoneisnewer.png" | relative_url }})<br>

```
$[func:fileOneIsNewer("/tmp/dem.kea", "/tmp/dem_2root.kea")]$ : 0
$[func:fileOneIsNewer("/tmp/dem_2root.kea", "/tmp/dem.kea")]$ : 1
$[func:fileOneIsNewer("/tmp/dem.kea", "/tmp/dem.kea")]$       : 0
$[func:fileOneIsNewer("/tmp/dem_2rot.kea", "/tmp/dem.kea")]$  : 0
$[func:fileOneIsNewer("/tmp/dem_2root.kea", "/tmp/dm.kea")]$  : 1
$[func:fileOneIsNewer("/tmp/dem_2rot.kea", "/tmp/dm.kea")]$   : 0
```

Note that if one of specified files does not exist, it is considered to be less recently modified than the existent file. If both files specified are nonexistent, the function returns `0`.   


##### filePath

```
$[func:filePath(<filename>)]$                          : <string>
```
This function returns the absolute file path of a given filename. The function does not test the existence of the specified file or path. On Linux, the returned path always starts with the root path `/`; on Windows, it starts with a drive letter, e.g. `C:`, or a double forward slash `//` for network drives. If the specified filename does not include a path portion, on Linux the function returns the user's home directory and on Windows the path of the LUMASS folder. If `<filename>` is empty, the function returns an empty string.

*Linux*

```
$[func:filePath("/home/dir/anImage.kea")]$             : /home/dir
$[func:filePath("lumass.tar.gz")]$                     : /home/user
$[func:filePath(".")]$                                 : /home
$[func:filePath("")]$                                  :
$[func:filePath("/unreal/path/./to/../a/file.txt")]$   : /unreal/path/a
```

*Windows*

```
$[func:filePath("D:/dir/lumass.tar.gz")]$              : D:/dir
$[func:filePath("//network/drive/lumass.tar.gz")]$     : //network/drive
$[func:filePath("lumass.tar.gz")]$                     : C:/opt/lumass-0.9.66
$[func:filePath(".")]$                                 : C:/opt
$[func:filePath("")]$                                  :
$[func:filePath("F:/unreal/path/./to/../a/file.txt")]$ : F:/unreal/path/a
```


##### fileSuffix

```
$[func:fileCompleteSuffix(<filename>)]$              : <string>
```
This function returns the (last) filename extension of `<filename>`.  

```
$[func:fileCompleteSuffix("/home/dir/anImage.kea")]$ : kea 
$[func:fileCompleteSuffix("D:/dir/lumass.tar.gz")]$  : gz
$[func:fileCompleteSuffix("lumass.tar.gz")]$         : gz
```

##### isDir

```
$[func:isDir(<directory>)]$                          : <boolean {0,1}>
```
The function tests whether (`1`) or not (`0`) `<directory>` points to a directory or symbolic link to a directory on the current system.

```
$[func:isDir("/home/dir/anImage.kea")]$              : 0 
$[func:isDir("D:/dir")]$                             : 1
$[func:isDir("lumass.tar.gz")]$                      : 0
```

##### isFile

```
$[func:isFile(<filename>)]$                          : <boolean {0,1}>
```

The function tests whether (`1`) or not (`0`) `<filename>` points to a file or symbolic link to a file on the current system.

```
$[func:isFile("/home/dir/anImage.kea")]$             : 1 
$[func:isFile("D:/dir")]$                            : 0
$[func:isFile("lumass.tar.gz")]$                     : 0
```

On Linux systems, the last example returned `1`, if `lumass.tar.gz` was in the user's home directory. On Windows, the last example returned `1` if the `lumass.tar.gz` was inside the main lumass folder, e.g. `C:/opt/lumass-0.9.66`.


#### String Functions

**Important**: Keep in mind that each parameter expression is [simplified]({{ "docs/mod_dynamic_parameters#parameter-expressions" | relative_url }}) before it is being evaluated! This may affect the expected result of the function!
{: .notice--warning}

##### strCompare

```
$[func:strCompare("<string_1>", "<string_2>", <CaseSensitive (boolean {0,1}, default: 0)>)]$ : <boolean {0,1}>
```

The function lexicographically compares `<string_1>` and `<string_2>` disregarding their case (default). It returns a number smaller, equal to, or greater than zero, if `<string_1>` is less, equal to, or greater than `<string_2>`. If the third argument is `1`, the comparison is case sensitive. 

```
$[func:strCompare("hello", "Hello")]$                : 0
$[func:strCompare("hello", "Hello", 0)]$             : 0
$[func:strCompare("hello", "Hello", 1)]$             : 32
$[func:strCompare("Hello", "hello", 1)]$             : -32
$[func:strCompare("hello", "hello", 1)]$             : 0
```

##### strContains

```
$[func:strContains("<string_1>", "<string_2>", <CaseSensitive (boolean {0,1}, default: 0)>) : <boolean {0,1}>
```

The function tests whether `<string_2>` is contained in `<string_1>` disregarding the case of `<string_2>` (default). It returns `0` if `<string_2>` is not contained in `<string_1>` and `1` if it is. If the third argument to this function is `1` than the case of `<string_2>` is considered while testing its occurence in `<string_1>`. 

```
$[func:strContains("Spatial modelling rocks!", "rocks")]$    :1
$[func:strContains("Spatial modelling rocks!", "rocks", 0)]$ :1
$[func:strContains("Spatial modelling rocks!", "Rocks", 1)]$ :0
$[func:strContains("Spatial modelling rocks!", "spatial")]$  :1
$[func:strContains("Spatial modelling rocks!", "ial mod")]$  :1
```

##### strIsEmpty

```
$[func:strIsEmpty("<string>")]$                      : <boolean {0,1}>
```

The function tests whether (`1`) or not (`0`) `<string>` is empty, i.e. contains any characters.

```
$[func:strIsEmpty("")]$                              : 1
$[func:strIsEmpty(" ")]$                             : 0
$[func:strIsEmpty("1")]$                             : 0
$[func:strIsEmpty("Hi!")]$                           : 0
```

##### strLength

```
$[func:strLength("<string>")]$                       : <integer>
```

The function counts the number of characters in `<string>` and returns the result. 


```
$[func:strLength("")]$                               : 0
$[func:strLength(" ")]$                              : 1
$[func:strLength("123 ")]$                           : 4
$[func:strLength("Pi  !")]$                          : 4
```

##### strListItem

```
$[func:strListItem("<string>", "<sep>", <idx>)]$     : <string>
```

The function returns the item at position `<idx>` from the list defined in `<string>`. Thereby, `<idx>` is a 0-based index and the list items in `<string>` are separated by `<sep>`. If `<string>` does not contain the specified separator `<sep>`, `<string>` represents a single element list and regardless of the specified position (`<idx>`), the single element in the list is returned. If `<sep>` is an empty string, the list is comprised of the individual characters of `<string>` and the charcter referenced by `<idx>` is returned.

```
$[func:strListItem("255 108 16", " ", 0)]$           : 255
$[func:strListItem("left,right,centre", ",", 2)]$    : centre
$[func:strListItem("OneSingleSelement", "S", 2)]$    : element
$[func:strListItem("OneSingleSelement", " ", 2)]$    : OneSingleSelement
$[func:strListItem("OneSingleSelement", "", 2)]$     : e
```

##### strListLength

```
$[func:strListLength("<string>", "<sep>")]$          : <integer>
```

The function counts the number of items in `<string>` separated by `<sep>`.

```
$[func:strListItem("255 108 16", " ", 0)]$           : 3
$[func:strListItem("left,right,centre", ",", 2)]$    : 3
$[func:strListItem("OneSingleSelement", "S", 2)]$    : 3
$[func:strListItem("OneSingleSelement", " ", 2)]$    : 1
$[func:strListItem("OneSingleSelement", "", 2)]$     : 17
$[func:strListLength("", "")]$                       : 0
```

##### strReplace

```
$[func:strReplace("<string>", "<find string>", "<replace string>")]$ : <string>
```

The function replaces `<find string>` inside `<string>` with `<replace string>`. The search for `<find string>` is case sensitive. If `<find string>` cannot be found inside `<string>`, `<string>` is returned unaltered.

```
$[func:strReplace("TimePeriod=100", "100", "500")]$  : TimePeriod=500
$[func:strReplace("/home/scenB", "/H", "D:\h")]$     : /home/scenB
$[func:strReplace("/home/scenB", "/h", "D:\h")]$     : D:\home/scenB
$[func:strReplace(
    "$[func:strReplace("/home/project/scenarioB", 
    "/h", "D:\h")]$", 
    "/", "\")]$                                      : D:\home\scenB
```

##### strSubstring

```
$[func:strSubstring("<string>", <start pos>, <num chars>)]$ : <string>
```

The function creates a new string from `<string>` by copying the `<num chars>` long character sequence starting at 0-based index `<start pos>` from `<string>`. If `<num chars>` is a negative number, all remaining characters towards the end of the `<string>` (left to right) are extracted. If both `<start pos>` and `<num chars>` are negative numbers or are not specified at all, `<string>` is returned. If `<start pos>` is equal to or greater than the length of `<string>` or `<num chars>` is zero (`0`), an empty string is returned.

```
$[func:strSubstring("/home/scenB/out.tiff", 1, 4)]$   : home
$[func:strSubstring("/home/scenB/out.tiff", 15, 5)]$  : .tiff
$[func:strSubstring("/home/scenB/out.tiff", 12, -1)]$ : out.tiff
$[func:strSubstring("/home/scenB/out.tiff", 12, 0)]$  :
$[func:strSubstring("/home/scenB/out.tiff", -1,-1)]$  : /home/scenB/out.tiff
$[func:strSubstring("/home/scenB/out.tiff", 100,-1)]$ :
```

#### Other Functions

##### cond

```
$[func:cond(<boolean expression>, "<true return value>", "<false return value>")]$
```
If `<boolean expression>` evaluates to a non-zero value, `true`, or `yes` the function returns the `<true return value>`. If `<boolean expression>` evaluates to `0`, `false`, or `no`, it returns the `<false return value>`. `<boolean expression>` may also be provided in the form of (direct) [MuParser](https://beltoforion.de/en/muparser/features.php#idStart) expressions.

```
$[func:cond(3 > 4, "/home/user/green.img", "/home/user/red.img")]$    : /home/user/red.img
$[func:cond(true, "/home/user/green.img", "/home/user/red.img")]$     : /home/user/green.img
$[func:cond(5 > 4, "/home/user/green.img", "/home/user/red.img")]$    : /home/user/green.img
$[func:cond($[func:hasAttribute("woody", "natero")]$, "woody__natero", "woody_relero")]$ : woody__natero (if woddy has 'natero' column) 
```  

##### hasAttribute

```
$[func:hasAttribute("<component name | user id", "<column name>")]$   : <string>
```
[![HasAttribute]({{ "/assets/images/docs/hasattribute_test.png" | relative_url}}){:width="80%", height="80%"}]({{ "/assets/images/docs/hasattribute_test.png" | relative_url}})<br>


```
$[func:hasAttribute("img", "scalar")]$
$[func:hasAttribute("tab", "AreaHa")]$
$[func:hasAttribute("buf", "scalar")]$
$[func:hasAttribute("para", "Value")]$
```

### Global Value Expressions

Global value expressions provide access to LUMASS settings or parameter definitions specified in [user tool tables]({{ "docs/mod_model_configuration#user-tools" | relative_url}}) or in [YAML-based model configuration files]({{ "docs/mod_model_configuration#yaml-based-configuration-files" | relative_url}}). The defined values are globally accessible inside a LUMASS model and do not change during a model run. They include LUMASS settings such as `UserModels` or the directory of the LUMASS executable `LUMASSPath`. Global value expressions take the following general form:

```
$[LUMASS:<LUMASS setting | configuration parameter>]$
```

**Important**: If the specified setting or parameter cannot be found or is not defined, LUMASS does not return an error message but an empty string!
{: .notice--warning}

#### Global Value Expression Examples

```
$[LUMASS:UserPath]$      : /home/opt/lumass/bin
$[LUMASS:Workspace]$     : /home/modeller/crunch/lumass_workspace
$[LUMASS:ConfigPath]$    : /home/modeller/crunch/myproject/scenarioA/ScenAConfig.yaml
$[LUMASS:MyProjectDir]$  : /home/modeller/crunch/myproject
$[LUMASS:NotDefined]$    : 
```

### Troubleshooting

Nested parameter expressions are evaluated iteratively. In each iteration LUMASS evaluates all non-nested expressions from left to right and replaces the expressions with their respecitve results. If any of the expressions returns an error, the evaluation is stopped and the thus far evaluated expression, including the error message, is returned. For example, the expression below 

```
$[LUMASS:OptPrepDir]$/$[func:fileBaseName($[para:RasterName:$[Preprocessor]$]$)]$norat.kea
```

might evaluate to 

```
/opt/prep/ni_nzlrinorat.kea
```

The evaluation is done in three iterations.

```
#1: /opt/prep/$[func:fileBaseName($[para:RasterName:4]$)]$norat.kea
#2: /opt/prep/$[func:fileBaseName(nz_nzlri.kea)]$norat.kea
#3: /opt/prep/nz_nzlrinorat.kea
```

For example if the `<component>` in the first expression is misspelled

```
expr:   $[LUMAS:OptPrepDir]$/$[func:fileBaseName($[para:RasterName:$[Preprocessor]$]$)]$norat.kea
return: ERROR: '$[LUMAS:OptPrepDir]$' - component 'LUMAS' not found!/$[func:fileBaseName($[para:RasterName:$[Preprocessor]$]$)]$norat.kea
```

the evaluation stops immediately and none of the following expressions is resolved. The `component '...' not found!` error is always returned, if the first part of an expression, i.e. `<component>`, `math`, or `func`, is not recognised. However, if the `<index>` expression of the nested value expression in our example contains an error, 

```
expr:   $[LUMASS:OptPrepDir]$/$[func:fileBaseName($[para:RasterName:$[Peprocessor]$]$)]$norat.kea
return: /opt/prep/$[func:fileBaseName($[para:RasterName:ERROR: '$[Peprocessor]$' - component 'Peprocessor' not found!]$)]$norat.kea
```

the first non-nested expression in iteration #1 can still be evaluated correctly before the error occurs. If a function name in a function expression is not recognised

```
expr:   $[LUMASS:OptPrepDir]$/$[func:filBaseName($[para:RasterName:$[Preprocessor]$]$)]$norat.kea
return: /opt/prep/ Unknown function 'filBaseName'norat.kea
```

`Unknown function '...'` is returned and any function arguments are omitted.