---
title: "ResampleImage"
permalink: "/docs/cref_resample_image"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixels.
OutputPixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the output image pixels.
InterpolationMethod | compulsory,<br>user-defined | 1 | The [`InterpolationMethod`]({{ "/docs/cref_resample_image#InterpolationMethod" | relative_url }}) used for resampling the input image. 
DefaultPixelValue | compulsory,<br>user-defined | 1 | The default pixel value written into the output image for pixel that don't overlap with the input image.
OutputSpacing | compulsory,<br>user-defined | 2 | The image pixel size in map units along the image's `X` and `Y` axis.
OutputOrigin | compulsory,<br>user-defined | 2 | The map coordinates of the top left pixel centre, i.e. `X` and `Y` coordinates. 
OutputSize | compulsory,<br>user-defined | 2 | The size of the output image in pixel along its `X` and `Y` axis. 

## Supported Image & Pipeline Features

Feature | Details
---------------|---------------
Image dimensions | 2D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | yes
Number of inputs | 1
Number of outputs | 1

## Overview

The `ResampleImage` component can be used to resample an arbitrary area of a given input image using a given `InterpolationMethod`. The output area does not have to align with the current image area (or pixels) and may also only partly overlap with the input image or may be larger than the input image. In other words, the component can be used to decrease (cookie-cut) or increase (blow-up) the spatial extent of the input image,  re-align the input image's pixel (location), change the input image's spatial resolution, or do everything at the same time.


## InterpolationMethod

- NearestNeighbour
- Linear
- BSpline0
- BSpline1
- BSpline2
- BSpline3
- BSpline4
- BSpline5
- Mean
- Median