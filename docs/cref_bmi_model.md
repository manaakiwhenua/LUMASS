---
title: "BMIModel"
permalink: "/docs/cref_bmi_model"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixels.
OutputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the output image pixels.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of image dimensions (axes) of input and output images.
YamlConfigFileName | compulsory,<br>user-defined | 0 | A configuration file detailing some important model information required for instantiation and execution of the model (s. YamlConfigFile below).

## Supported Image & Pipeline Features

Feature | Details | Comments
---------------|---------------
Image dimensions | 1D, 2D, 3D | 
Multi-band images | no |
Pipeline role | process, sink |s. [YamlConfigFile]( {{ "docs/cref_bmi_model#yamlconfigfile" | relative_url }})
Sequential processing | yes |
Parallel processing | no  | Python implementations
Number of inputs | 1..multiple | as returned by `<BmiSubclass>.get_input_item_count()`
Number of outputs | 1..multiple | as returned by `<BmiSubclass>.get_output_item_count()` 
Input image names | defined by inputs' [`UserID`]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }}) | input names shall match the names returned by `<BmiSubclass>.get_input_var_names()`
Output image names | defined by BMI model |  as returned by `<BmiSubclass>.get_output_var_names()`


**Info:** For BMI models written in Python, we recommend the use of [Numba](https://numba.pydata.org/) to parallelise pixel-based processing 'inside' the BMI model. 
{: .notice--info}

## Overview

The `BMIModel` component enables the integration of Python model components that implement the [Basic Model Interface](https://bmi.readthedocs.io/en/latest/) (BMI) into a LUMASS processing pipeline. However, to exploit the sequential processing capabilities of a processing pipeline, LUMASS needs to be in charge of reading and writing the respective input and output images, including memory management. Therefore, the BMI model needs to fulfil a few additional requirements. We provide a sample implementation as part of the LUMASS source tree that can be subclassed and adapted to the specific model requirements. Key aspects of a compatible BMI model implementation are outlined below:

- The model must not allocate any image memory as part of the model, i.e. it must not create any input or ouput arrays itself, except for intermediary data that is only stored temporarily
- The model should not read datasets byitself, but use the `<BmiSubclass>.set_value()` method to set pre-allocated named (numpy) arrays

**Important**: If different `InputPixelType` and `OutputPixelType` are specified, it is the BMI model's responsibility to appropriately cast between those types before populating the output image(s).
{: .notice--warning}

## YamlConfigFile

```
LumassBMIConfig:

    # bmi interface type <bmi-cxx | bmi-python>
    type: "bmi-python"

    # path to bmi library
    #       python: path to *.py module file
    #          e.g.: /home/python
    #                C:/src/python
    #
    #       note: 
    #       - multiple path entries may be concatenated with the
    #         operating system's path separator, for example ';'
    #         on windows and ':' on linux
    #       - for type 'bmi-python' the specified path(s) will be
    #         apped to python's search path for python modules
    #         (=PYTHONPATH)
    #
    # path: "C:/src/python/watyield/bmi;C:/src/python/bmi-python;C:/src/python/watyield"
    path: "/home/herziga/garage/python/watyield/bmi:/home/herziga/garage/python/bmi-python:/home/herziga/garage/python/watyield"
    
    # name of library contaning the bmi::Bmi subclass (s. class_name)
    # note: its path needs to be set with path (s. above)
    # 
    #   python: C:/src/python/watyieldbmi.py 
    #   C++: windows: C:/src/cpp/awesome_bmi_lib.dll 
    #           linux:   /home/alex/cpp/awesome_bmi_lib.so
    library_name: "watyieldbmi.py"
    
    # name of class implementing the BMI interface
    #    python: Bmi subclass
    #    C++: bmi::Bmi subclass            
    class_name: "WatYieldBMI"
    
    # whether or not the model component produces and output that can
    # be fetched by a downstream component, i.e. the component doesn't 
    # write its output 
    # if issink == true, the component becomes executable and will be 
    # called by the lumas model controller depending on the time level
    # of its host BMIModel component
    issink: false 
    
    # whether the component's implemented algorithm supports streaming, 
    # i.e. the sequential processing of parts of the input array; 
    streamable: true

    # whether or not the component is threadable, i.e. can be called
    # safely from multiple threads of the processing pipeline; 
    # note: for python-bmi we assume that any threading is done 
    # within the component, e.g. using numba, since the python interpreter cannot be called safely from multiple threads 
    threadable: false
```
