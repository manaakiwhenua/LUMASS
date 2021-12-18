---
title: "CostDistanceBuffer"
permalink: "/docs/cref_costdistancebuffer"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
InputPixelType | compulsory,<br>user-defined, default: `int` | 0 | The data type of the input image pixels.
InputImageFileName | compulsory,<br>user-defined | 1 | The absolute path of the image containing the features the calculated distances refer to, or the buffer zones are created around.
CostImageFileName | compulsory,<br>user-defined | 1 | The absolute path of the image containing the cost surface to be accounted for in the distance calculation. 
OutputImageFileName | compulsory,<br>user-defined | 1 |The absolute path of the otuput image containing the (cost-) distance surface or the buffer-zone image respectively.
ObjectValueList | compulsory,<br>user-defined | 2 | A list of values of `InputPixelType`, indicating the objects the buffer or distance value zone is generated around.
MaxDistance | compulsory,<br>user-defined, default: `0` | 1 | The radius of the buffer or distance value zone generated around the specified objects (`ObjectValueList`).
UseImageSpacing | compulsory,<br>user-defined, default: `True` | 0 | Whether to use the input image's (`InputImageFileName`) pixel spacing information as basis for the distance value and `MaxDistance` calculation. If unselected (`UseImageSpace=False`), distances are calculated in pixel space (i.e. number of pixels).
CreateBuffer |  compulsory,<br>user-defined, default: `False` | 0 | Whether a single-valued (`BufferZoneIndicator`) (buffer) zone is to be created around the specified objects  (`ObjectValueList`). If deselected (`CreateBuffer=False`), the distance to the closest object is calcuated for each pixel within the specified radius (`MaxDistance`) around the specified objects. 
BufferZoneIndicator | compulsory,<br>user-defined | 1 |  If `CreateBuffer=True`, the value written into each pixel within the given radius around the specified objects.

## Supported Image & Pipeline Features

Feature | Details
---------------|---------------------
Image dimensions | 2D
Multi-band images | no
Pipeline role | stand alone
Sequential processing | yes
Parallel processing | no

## Overview

The `CostDistanceBuffer` component generates either distance surfaces (`CreateBuffer=false`) or buffer zones (`CreateBuffer=True`) around the specified objects (`ObjectValueList`) in the input image (`InputImageFileName`). The `MaxDistance` parameter defines the radius of the distance surface or buffer zone generated around the specified objects in the input image. Distances may either be calculated in pixel space (i.e. number of pixels) (`UseImageSpacing=False`) or geographic space (`UseImageSpacing=True`) as defined by the input image's pixel spacing information in x and y direction. This component is a stand-alone component and cannot be included in a processing pipeline. 


