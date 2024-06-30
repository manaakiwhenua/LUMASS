---
title: "DataBuffer & -Reference"
permalink: "/docs/cref_databuffer"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
IsStreamable | compulsory,<br>user-defined, default: `False` | 0 | Whether (`True`) or not (`False`) this `DataBuffer` can be used as an input into a streaming (sequential processing) pipeline.
DataComponentName | compulsory<br>user-defined, default: `empty` | 0 | The `ComponentName` of the `DataBuffer` this `DataBufferReference` component is referencing. (**Only available for `DataBufferReference`**)

## Supported Image & Pipeline Features

Characteristic | Details 
---------------|---------------
Image dimensions | 1D, 2D, 3D  
Multi-band images | yes
Pipeline role | source, sink
Sequential processing | yes
Parallel processing | n/a

## Overview

A `DataBuffer` holds either an entire image including RAM or SQLite-based raster attribute table (RAT) or a stand-alone (SQLite-based) table (connection) in memory. `DataBuffer` can act as source (input) or as sink component, i.e. providing or receiving data to/from processing components. The assigned data will persist until the component is reset, receives new data, or the component is removed from the modelling environment. The content of a `DataBuffer` is not stored in a model file (*.lmx, *.lmv), i.e. 
a `DataBuffer` needs to be updated (Context Menu >> Update ... ) after it has been loaded into a model, before its content is available.

If a `DataBuffer` is feeding data into a sequential (streaming) processing piepline, i.e. the sink components (`ImageWriter`) is configured to stream data through the pipeline, its `IsStreamable` property needs to be set to `True`. Otherwise, the `DataBuffer` component will deliver the correct image region requested by the downstream pipeline component.

A `DataBufferReference` is, as the name suggests, a reference to a (proper) `DataBuffer`. It does not hold any data itself and merely works as another means of accessing the data ('storage') held by the `DataBuffer` it references. Hence, assigning new data to a `DataBufferReference` is equivalent to assigning that data to the referenced `DataBuffer` component directly. In short, a `DataBufferReference` is just a convenient way of accessing the same 'cached' data at different places in a model.