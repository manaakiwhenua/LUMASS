---
title: "RAMFlowAcc"
permalink: "/docs/cref_ram_flowacc"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
PixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the input image pixel.
Nodata    | compulsory,<br>user-defined, default: `0` | 1 | Nodata value
Algorithm | compulsory,<br>user-defined, default: `MFD` | 0 | The flow accumulation algorithm {`Dinf`, `MFD`, `MFDw`}.
FlowExponent | optional,<br>user-defined, default: `4` | 0 | Flow exponent for `MFDw` algorithm.
FlowLength | compulsory,<br>user-defined, default: `NO_FLOWLENGTH` | 0 | Parameter that specifies whether flow length or flow accumulation (`NO_FLOWLENGTH`) is calculated. Either `UPSTREAM` or `DOWNSTREAM` must be selected for flow-length calculations.

## Supported Image & Pipeline Features

Feature | Details | Comments
---------------|---------------
Image dimensions | 2D |
Multi-band images | no |
Pipeline role | process |
Sequential processing | no |
Parallel processing | no |
Number of inputs | 2 | Second input (`FlowWeight`, s. below) is optional
Number of outputs | 1 |

## Inputs

Name            |          | Description
----------------|----------|---------------
DEM             | required | A digital elevation model that represents the height of the land surface.
FlowWeight      | optional | A weight surface representing the proportion of flow that is able to pass a given pixel downhill.

## Overview

The `RAMFlowAcc` component calculates flow accumulation (`FlowLength=NO_FLOWLENGTH`) or flow length (`FlowLength=(DOWNSTREAM | UPSTREAM)`) of individual pixels according to the specified (`Algorithm`). The output image units are given in pixels. Flow accumulation denotes the nubmer of pixels flowing into a given pixel (including the pixel itself). Flow length represents the `DOWNSTREAM` or `UPSTREAM` distance from the given pixel to the flow outlet or flow initiation point (local maximum) respectively.

**Important**: As indicated by its name, the `RAMFlowAcc` component does not support sequential processing of large images. Therefore, it can only be part of a non-streaming pipeline, i.e. a pipeline ending in a [`DataBuffer`]({{ "docs/cref_databuffer.md" | relative_url }}) component or an [`ImageWriter`]({{ "docs/cref_image_writer#properties" | relative_url }}) component with its `StreamingMethod` set to `NO_STREAMING`.
{: .notice--warning}


## Algorithm

- `Dinf` - D-infinity flow accumulation algorithm (Tarboton 1997).
- `MFD` - Multiple flow accumulation algorithm (Quint et al. 1991).
- `MFDw` - Weighted multiple flow algorithm using the FlowExponent (Holmgren 1994).

## References

Holmgeren P 1994. Multiple flow direction algorithms for runoff modelling in grid-based elevation models: an empirical evaluation. Hydrological Processes 8: 327-334. [Paper](https://doi.org/10.1002/hyp.3360080405)<br>

Tarboton DG 1997. A new method for the determination of flow directions and upslope areas in grid digital elevation models. Water Resources Research 33: 309-319. [Paper](https://doi.org/10.1029/96WR03137)<br>

Quinn P, Beven K, Chevallier P, Planchon O 1991. The prediction of hillslope flow paths for distributed hydrological modelling using digital terrain models. Hydrological Processes 5: 59-79. [Paper](https://doi.org/10.1002/hyp.3360050106)