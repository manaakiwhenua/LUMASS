---
title: "MapAlgebra"
permalink: "/docs/cref_map_algebra"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
PixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the input and output image pixels.  
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of dimensions of the input and output images. 
InputTables | optional,<br>user-defined | 2 | absolute path to the image/RAT to be read; uses forward slash `/` as path separator
MapExpressions | compulsory,<br>user-defined | 1 | List of map algebra expressions per iteration. Multiple expressions per iteration are separated by a comma (`,`). Each expression creates a separate output image.
OutputNames | optional,<br>user-defined | 2 | List of short names for individual output images per ieration. The specified names **must not contain any colons (`:`)**! The order of the names corresponds with the order of the comma separated `MapExpressions`. These short names can be used instead of an index number as part of an input image specification for another process component.  
UseTableColumnCache | optional,<br>user-defined | 0 | Checkbox to indicate wether to cache table columns prior to processing. Note, this option is only effective when SQLite-based attribute tables are processed.  

## Supported Image & Pipeline Features

Feature | Details 
---------------|---------------
Image dimensions | 1D, 2D, 3D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | yes
Number of inputs | 1..multiple
Number of outputs | 1..multiple

## Overview

The `MapAlgebra` process component performs pixel-wise mathematical and logical operations (s. [`MapExpressions`]({{ "docs/cref_map_algebra#mapexpressions" | relative_url }})) on multi-dimensional single-band (scalar) input images. Operations may be performed on the actual image pixel values or on raster attribute table values associated with a given pixel value. The component accepts multiple input images of the same `PixelType`. `MapExpressions` refer to different inputs by the input components' [`UserID`]({{ "docs/mod_structure#general-model-and-aggregate-component-properties" | relative_url }}).

**Important**: Irrespective of the specified `PixelType`, the `MapAlgebra` component casts all input pixel values into `double` before applying the specified algebra expression. The result is then cast into the specified `PixelType` and written into the output image. Therefore, information loss may occur if the precision and/or scale of `PixelType` is larger than the precision and/or scale of `double`.
{: .notice--warning}

## MapExpressions

LUMASS uses the [muparser](https://beltoforion.de/en/muparser) library for parsing and evaluating mathematical and logical expressions. It supports a large set of mathematical [functions](https://beltoforion.de/en/muparser/features.php#idDef1) and [operators](https://beltoforion.de/en/muparser/features.php#idDef2) (cf. [Math Expressions]({{ "docs/mod_dynamic_parameters#math-expressions" | relative_url }})). Image pixel values or associated values in a raster attribute table (RAT) are referred to by the input image's `UserID` (e.g. `img1`) and, if applicable, the RAT's column name (e.g. `biomass`) appended to the `UserID` using a double underscore `__`, i.e. `img1__biomass`. Please refer to the [Examples]({{ "docs/cref_map_algebra#examples" | relative_url }}) section for more details. 

**Important**: The look-up of RAM-based RAT values is based on the assumption that image pixel values represent 0-based row numbers of the RAT. If this assumption is not valid, wrong table values may be returned as a result.
{: .notice--warning}

### Examples

Input images' `UserIDs` | MapExpression(s) | Explanation 
------------------------|------------------|-------------
`dem`                   | `dem - 0.5`      | subtraction of `0.5` from `dem`'s pixel values
`rainfall`              | `rainfall^2`     | raising `rainfall`'s pixel values to the power of `2`
`dz_dx`, `dz_dy`        | `atan(sqrt(dz_dx^2 + dz_dy^2)) * 180 / pi` | calculation of slope angle in degree from slope in x and y direction
`land_use`              | `land_use__ProductionHa * land_use__AreaHa` | calculation of total production per land use (category)
`dem`                    | `dem < 0 ? 0 : dem` | reclassification: assign all values of `dem` that are smaller than `0` value `0` and assign all other values their original (i.e. input) value
`slope_deg`              | `slope_deg >= 0 && slope_deg < 5 ? 1 : slope_deg >= 5 && slope_deg < 15 ? 2 : slope_deg >= 15 ? 3 : 0` | reclassify the slope (in degree) image into 3 classes 1: 0-5, 2: 5-15, 3: >= 15, all other pixel are assigned the value `0`
`dem`                    | `dem < 0 ? dem : 0`, `dem >= 0 ? dem : 0` | splitting the value range of `dem` into two different output images; output image #1 represents all areas in `dem` that have below zero elevation and output image #2 represents all areas in `dem` that are at or above sea level

**Note #1**: LUMASS does not support the assignment operator. The result of the i<sup>th</sup> `MapExpression` is assigned to the current pixel of the i<sup>th</sup> ouput image depending on the values of the input image pixels.
{: .notice--info}

**Note #2**: To save both output images of the last example in the table above, you need to connect an [`ImageWriter`]({{ "docs/cref_image_writer" | relative_url }}) component (double link!) and specify two output names for the `ImageWriter` component. Please refer to the section '[Multiple Inputs and Outputs]({{ "docs/mod_multiple_inoutputs" | relative_url}})'.
{: .notice--info}

## UseTableColumnCache

This option is only effective when SQLite-based attribute tables are processed. SQLite tables are either provided by `ImageReader` components whose `RATType` property is set to `ATTABLE_TYPE_SQLITE`, or by TableReader components who always output SQLite tables. The option is ignored when a RAM-based table is provided, for example by an `ImageReader` whose `RATType` is set to `ATTABLE_TYPE_RAM`. 
