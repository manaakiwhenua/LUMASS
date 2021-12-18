---
title: "ExtractImageRegion"
permalink: "/docs/cref_extract_image_region"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
PixelType | compulsory,<br>user-defined, default: `unknown` | 0 | The data type of the input image pixel.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of image dimensions (axes) of input and output image.
ROIIndex | compulsory,<br>user-defined, default: `empty` | 2 | The pixel coordinates (index) of the top left pixel of the image region to be extracted. 
ROISize | compulsory,<br>user-defined, default: `empty` | 2 | The size in pixel along the image axes of the image region to be extracted.
ROIOrigin | compulsory,<br>user-defined, default: `empty` | 2 | The (real-world) map coordinates of the top left pixel centre of the image region to be extracted.
ROILength | compulsory,<br>user-defined, default: `empty` | 2 | The length in map units along the image axes of the region to be extracted. 


## Supported Image & Pipeline Features

Characteristic | Details 
---------------|---------------------
Image dimensions | 1D, 2D, 3D 
Multi-band images | no 
Pipeline role | process 
Sequential processing | yes 
Parallel processing | yes

## Overview

This component extracts a portion of an image that is specified either in pixel coordinates (`ROIIndex, ROISize`) or in (real-world) map coordinates (`ROIOrigin, ROILength`). 