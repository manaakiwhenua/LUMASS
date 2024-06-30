---
title: "ImageWriter"
permalink: "/docs/cref_image_writer"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
PixelType | compulsory,<br>user-defined, default: `float` | 0 | The data type of the image pixels to be written. For RGB or multi-band (vector) images, `PixelType` denotes the data type of the individual image bands, i.e. pixel components.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 |  The number of dimensions of the image to be written. LUMASS currently only supports writing 3D images as (into) NetCDF (*.nc) files.
NumBands | compulsory,<br>user-defined, default: `1` | 0 | The number of bands of the image to be written. This parameter is required to determine the pixel type of the output image, i.e. scalar (`=1`), RGB (`=3` and `RGBMode=True`), and multi-band (vector) for all other values. 
FileNames | compulsory,<br>user-defined, default: `empty` | 1 | Absolute path(s) to the image(s) to be written; uses forward slash '/' as path separator
InputTables | optional,<br>user-defined, default: `empty` | 1 | List of component output specifications (e.g. `TableReader:0`) referring to tables to be written into the specified images (`FileNames`).
RGBMode | compulsory,<br>user-defined, default: `False` | 0 | If this parameter is selected, i.e. `RGBMode=True`, and `NumBands=3`, then LUMASS writes an RGB image.
WriteImage | compulsory,<br>user-defined, default: `True` | 0 | When selected, i.e. `True`, the image(s) is (are) written or updated. When deselected, i.e. `False`, the image(s) is (are) not written or updated.
WriteTable | compulsory,<br>user-defined, default: `True` | 0 | When selected and a raster attribute table is provided as part of the input image or specified by the `InputTables` parameter, the raster attritubte table(s) are written into the corresponding image. 
StreamingMethod | compulsory,<br>user-defined, default: `STRIPPED` | 0 | Specifies the streaming method to be applied for sequential image processing by the processing-pipeline. If `NO_STREAMING` is selected, no sequential image processing is applied by this processing pieline.
PipelineMemoryFootprint | compulsory,<br>user-defined, default: `512` | 0 | The maximum amount of main memory in megabyte available to this processing pipeline. Note that this value is only an estimation and the actual memory usage may differ from the specified value.
PyramidResamplingType | compulsory,<br>user-defined, default: `NEAREST` | 0 | The resampling algorithm to be used for generating image pyramids for fast display with LUMASS or GIS-type applications.

## Supported Image & Pipeline Features

Characteristic | Details | Comments
---------------|---------------
Image dimensions | 1D, 2D, 3D  | 
Multi-band images | yes |
Pipeline role | sink |
Sequential processing | yes |
Parallel processing | no |
Number of inputs | 1..multiple | provided by a single input component

## Overview

The `ImageWriter` writes images and/or their associated raster attribute tables (RAT) to disk (`WriteImage`, `WriteTable`). However, `ImageWriter` can only write RAT into the specified image file. If you want to write a stand-alone (SQLite-based) table as RAT into the specified image file, please use the `TableFileNames` property to specify the SQLite database filename.  

To create SQLite-based RAT (*.ldb), please refer to the [`SummarizeZones`]({{ "docs/cref_sum_zones" | relative_url}}) component. To create stand-alone SQLite-based tables, please refer to the [`TableReader`]({{ "docs/cref_table_reader" | relative_url}}) or the [`ParameterTable`]({{ "docs/cref_param_table" | relative_url }}) component.

Please refer to the `ImageReader` component reference for supported image [formats]({{ "docs/cref_image_reader#supported-image-formats" | relative_url}}) and [filenames]({{ "docs/cref_image_reader#filenames" | relative_url}}). 

**Note**: If you're working with large images, i.e. > 2 GiB, we recommend the [`KEA`](https://gdal.org/drivers/raster/kea.html#raster-kea) image format. It keeps image sizes small and also  supports RAT for large images!
{: .notice--info}

## Sequential Processing and Memory Footprint  

Since the `ImageWriter` is a sink component, it sits at the bottom (downstream) end of a processing pipeline. It enables the sequential processing (`StreamingMethod`) of large images as chunks of image tiles (`TILED`) or as chunks of rows (`STRIPPED`). The approximate maximum memory footprint of a pipeline can be configured with the `PipelineMemoryFootprint` property.

## Image Pyramids

To speed up display in LUMASS or GIS, e.g. [QGIS](https://qgis.org), the `ImageWriter` automatically creates iamge pyramids. The resampling type used to create image pyramids can be configured using the `PyramidResamplingType` property. When writing into an image cube that is supposed to be extended further by subsequent write operations, set the `PyramidResamplingType` to `NONE` to prevent premature image pyramid generation.

## Writing Multiple Images

While this writer can only be connected to one input component, it can write multiple images at a time (cf. [Multiple Inputs and Outputs]({{ "docs/mod_multiple_inoutputs" | relative_url }}) if the input component produces multiple output images (e.g. [`MapAlgebra`]({{ "/docs/../../cref_map_algebra" | relative_url }}) or [`BMIModel`]({{ "/docs/cref_bmi_model" | relative_url }})). It writes only as many output images as filenames are provided. The order of filenames should reflect the order of `Inputs` configured for the writer. Images provided in excess of the number of filenames specified, are ignored. 

**Important**: Because `FileNames` is a [1D property]({{ "docs/mod_structure#property-dimensions" | relative_url }}), you can only specify a set of filenames for a single iteration when writing multiple images. However, by using [parameter expressions]({{ "docs/mod_dynamic_parameters#parameter-expressions" | relative_url}}), you can still dynamically change the set of filenames to support multiple iterations.
{: .notice--warning}
