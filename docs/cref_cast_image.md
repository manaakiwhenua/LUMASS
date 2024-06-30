---
title: "CastImage"
permalink: "/docs/cref_cast_image"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixels (components).
OutputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the output image pixels (comoponents).
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of image dimensions (axes) of the input and output image.

## Supported Image & Pipeline Features

Feature | Details | Notes
---------------|---------------------
Image dimensions | 1D, 2D, 3D   |
Multi-band images | yes | All bands must have the same data type.
Pipeline role | process |
Sequential processing | yes |
Parallel processing | yes |
Number of inputs | 1
Number of outputs | 1


## Overview

The `CastImage` component converts an input image of `InputPixelType` into an output image of `OutputPixelType`. For multi-band images the specified type conversion applies to the individual image bands (pixel components).