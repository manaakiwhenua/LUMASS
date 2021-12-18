---
title: "CubeSliceToImage2D"
permalink: "/docs/cref_cube2img2d"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixels.
OutputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the output image pixels.
DimMapping | compulsory,<br>user-defined, default: `empty` | 2 | The input image dimension indices (with `x=1, y=2, z=3`) that map onto the 2D output dimensions x and y. Currently, only {% raw %}`DimMapping={{{1}{2}}}`{% endraw %} is supported.
InputOrigin | compulsory,<br>user-defined, default: `empty` | 2 | The origin in map coordinates of the input image region to be copied to the output image.
InputSize | compulsory,<br>user-defined, default: `empty` | 2 | The size in image pixels  along each image direction of the image region to be extracted from the cube. 
InputIndex | compulsory,<br>user-defined, default: `empty` | 2 | The image (pixel) coordinates of the start point of the 2D image region to be extracted from the image cube.

**Note** In LUMASS, the origin of a 2D image is located in the centre of the top left pixel. The origin of an image cube in LUMASS is identical with the origin of the 2D image slice at the bottom of the cube.  
{: .notice--info} 
  

## Supported Image & Pipeline Features

Feature | Details 
---------------|---------------
Image dimensions | input: 3D, output: 2D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | yes
Number of inputs | 1
Number of outputs | 1

## Overview

The `CubeSliceToImage2D` component copies a 2D xy-slice from an 3D input image into the 2D output image. The image region to be copied is defined by the `InputIndex` and the `InputSize`. The spatial reference in map coordinates is provided by the `InputOrigin`. Input and output image may have different pixel types. 
