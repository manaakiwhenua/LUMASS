---
title: "SummarizeZones"
permalink: "/docs/cref_sum_zones"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
ValueImagePixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the ValueImage pixels.
ZoneImagePixelType | compulsory,<br>user-defined, default: `int` | 0 | The data type of the ZoneImage pixels.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of dimensions (image axes) of the input and output images.
IgnoreNodataValue | compulsory,<br>user-defined | 1 | Whether (`IgnoreNodataValue=1`) or not (`IgnoreNodataValue=0`) nodata values (`NodataValue`) shall be ingnored in calculating zonal summary statistics.
NodataValue | compulsory,<br>user-defined, default: `empty` | 1 | The pixel value of the `ValueImage` to be ignored while calculating summary statistics for a given zone. The `NodataValue` is ignored if no `ValueImage` is specified.
HaveMaxKeyRows | compulsory,<br>user-defined | 1 | Whether (`HaveMaxKeyRows=1`) or not (`HaveMaxKeyRows=0`) the number of rows of the output raster attribute table (RAT) shall match the maximum pixel value of the `ZoneImage` (s. Inputs).
ZoneTableFileName | optional,<br>user-defined | 1 | The name of the output raster attribute table containing the summary statics for each zone. This parameter is only required if the downstream processing component does not support raster attribute tables or if input is provided to an `ImageWriter` component and the specified output image format does not support RATs, e.g. TIFF.  

## Supported Image & Pipeline Features

Feature | Details 
---------------|---------------
Image dimensions | 1D, 2D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | yes
Number of inputs | 2
Number of outputs | 1

## Inputs

Name            |          | Description
----------------|----------|---------------
ZoneImage       | required | A categorical (`integer`) image depicting discrete areas that share a specific property, e.g. land cover, soil types, adminstrative regions, etc.
ValueImage      | optional | Typically a floating point image depicting a property or phenomenon that continuously varies across space, e.g. elevation, temperature, etc.

## Overview

The `SummarizeZones` component summarises the values of the `ValueImage` within the zones of the `ZoneImage`. A [summary statistic]({{ "docs/cref_sum_zones#summary-statistics" | relative_url }}) is created for each zone and provided as raster attribute table (RAT) of the output image. If no `ValueImage` is provided, a summary of the `ZoneImage` is calculated instead.

**Info**: The `SummarizeZones` component recognizes the `UserID` of its input components as input image names for distinguishing between the `ZoneImage` and the `ValueImage`.
{: .notice--info}   

## Summary Statistics

Attribute | Description
----------|--------------
rowidx    | The unique zone values detected in the `ZoneImage`.
zone_id   | A 0-based contiguous identifier for each zone.
count     | The number of pixels within a given zone.
min       | The minimum observed value within a given zone.
max       | The maximum observed value within a given zone.
mean      | The average value within a given zone.
stddev    | The standard deviation for all values within the given zone.
sum       | The sum of all values within a given zone.
minX      | The minimum observed pixel coordinate along the x-axis (column) for the given zone.
minY      | The minimum observed pixel coordinate along the y-axis (row) for the given zone.
maxX      | The maximum observed pixel coordinate along the x-axis (column) for the given zone.
maxY      | The minimum observed pixel coordinate along the y-axis (row) for the given zone.

**Important**: All pixel values are internally cast to `double` before being summarised. Information loss may occur, if the precision and/or scale of the `ValueImage` (or `ZoneImage`) data type is larger than that of `double`.
{: .notice--warning}