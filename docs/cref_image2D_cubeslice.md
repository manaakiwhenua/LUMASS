---
title: "Image2DToCubeSlice"
permalink: "/docs/cref_image2D_cubeslice"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixels.
OutputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the output image pixels.
DimMapping | compulsory,<br>user-defined, default: `empty` | 2 | The input image dimension indices (with `x=1, y=2`) that map onto the 3D output image dimensions x and y. Currently, only {% raw %}`DimMapping={{{1}{2}}}`{% endraw %} is supported.
OutputOrigin | compulsory,<br>user-defined, default: `empty` | 2 | The origin in map coordinates of the ouput image cube.
OutputSize | compulsory,<br>user-defined, default: `empty` | 2 | The size in image pixels along each image direction of the output image cube. The z-axis `OutputSize` must be `1`.
OutputIndex | compulsory,<br>user-defined, default: `empty` | 2 | The x- and y-coordinates specifiy the image (pixel) coordinates of the start point of the 2D input image region to be copied into the output cube. The z-coordinate specifies the 'target slice' of the 2D input image region.

**Note** In LUMASS, the origin of a 2D image is located in the centre of the top left pixel. The origin of an image cube in LUMASS is identical with the origin of the 2D image slice at the bottom of the cube.  
{: .notice--info} 
  
## Supported Image & Pipeline Features

Feature | Details 
---------------|---------------
Image dimensions | input: 2D, output: 3D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | yes
Number of inputs | 1
Number of outputs | 1

## Overview

The `Image2DToCubeSlice` component copies a 2D image as xy-slice into a 3D output image cube. The size of the image region to be copied is defined by the `OuputIndex` and the `OutputSize`'s x- and y- pixel coordinates and sizes respectively. The z-axis `OutputIndex` specifies the 'target slice' the 2D input image is copied into. The z-axis `OutputSize` must be `1`. The spatial reference in map coordinates is provided by the `OuputOrigin`. Input and output image may have different pixel types.