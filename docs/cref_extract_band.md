---
title: "ExtractBand"
permalink: "/docs/cref_extract_band"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `ushort` | 0 | The data type of the input image bands (pixel components).
OutputPixelType | compulsory,<br>user-defined, default: `ushort` | 0 | The data type of the output image pixels.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of image dimensions (axes) of input and output image.
Band | compulsory,<br>user-defined | 1 | The 1-based band number to be written into the output image. 

## Supported Image & Pipeline Features

Characteristic | Details | Notes
---------------|---------|------------
Image dimensions | 1D, 2D, 3D  | The output image will have the same number of dimensions as the input image
Multi-band images | yes | The output image will be a single-band (scalar) image.
Pipeline role | process |
Sequential processing | yes |
Parallel processing | yes |


## Overview

The `ExtractBand` component writes the specified input band (`Band`) of `InputPixelType` of the input image to a single-band output image of `OutputPixelType`. Data loss may occur if the `OutputPixelType` precision and/or scale is smaller than that of the `InputPixelType`.
