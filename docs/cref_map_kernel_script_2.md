---
title: "MapKernelScript2"
permalink: "/docs/cref_map_kernel_script_2"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

... to be completed ...

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the input image pixel.
OutputPixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the output image pixel.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of image dimensions (axes) of input and output image.
KernelRadius | compulsory,<br>user-defined, default: `0` | 1 | The radius of the kernel window around the (central) pixel being calculated. 
KernelShape | compulsory,<br>user-defined, default: `RECTANGULAR` | 0 | The shape of the kernel window {`RECTANGULAR`, `CIRCULAR`}.



## Supported Image Characteristics

Characteristic | Details
---------------|---------
Image dimensions | 2D
Multi-band images | no

## Overview

The `MapKernelScript2` component enables the calculation of output pixel values based on its configurable neighbourhood, i.e. `KernelRadius` and `KernelShape`. The processing instructions are provided as a small script based on MuParser expressions extended by looping capabilties (s. `KernelScript` for details). 

converts an input image of `InputPixelType` into an output image of `OutputPixelType`. For multi-band images the specified type conversion applies to the individual image bands (pixel components).

- 
- all input image pixels are converted into double then processed and then converted into the output pixel type
- Data loss may occur if the `OutputPixelType` precision and/or scale is smaller than that of the `InputPixelType` or if the precision and/or scale of `double` is smaller than that of the `InputPixelType`, e.g. integral values of type `longlong`.


## KernelScript
