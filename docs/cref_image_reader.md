---
title: "ImageReader"
permalink: "/docs/cref_image_reader"
--- 

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
PixelType | compulsory,<br>user-defined, default: `unknown`  | 0 | The data type of the image pixels or pixel components (image bands) of the output image. If the specified pixel type differs from the original type, LUMASS casts the orignal type to the specified ouptut type and data loss my occur if the precision and/or scale of the ouptut type is smaller than that of the original type.
NumDimensions | compulsory,<br>user-defined, default: `2` | 0 | The number of dimensions (image axes) of the image to be read. LUMASS supports 1, 2, and 3 dimensional images.
NumBands | compulsory<br>user-defined, default: `1` | 0 | The number of image bands or pixel components of the image to be read. 
FileNames | compulsory,<br>user-defined | 1 | The absolute path to the image/RAT to be read. Please use forward slashes `/` as path separator on all platforms.
RATType   | compulsory,<br>user-defined, default: `ATTABLE_TYPE_RAM` | 0 | the type of attribute table (RAM or SQLite-based) to be provided to downstream components
DbRATReadOnly | compulsory,<br>user-defined, default: `False` | 0 | if RATType is set to ATTABLE_TYPE_SQLITE, whether or not the database should be opened in read-only mode
RGBMode | compulsory,<br>user-defined, default: `False` | 0 | If `NumBands=3` and `RGBMode=True`, LUMASS interpretes the first three bands of the image or the bands specified in `BandList` as RGB-coloured image.
BandList | optional,<br>user-defined | 2 | List of 1-based band numbers to be read from a multi-band image.

## Supported Image & Pipeline Features

Characteristic | Details 
---------------|---------------
Image dimensions | 1D, 2D, 3D  
Multi-band images | yes
Pipeline role | source
Sequential processing | yes
Parallel processing | no

## Overview

The `ImageReader` process component reads an image (raster) and its associated raster attribute table (RAT), if available. If a RAT is available, it is automatically read/accessed and passed on to the next downstream process component together with the image. LUMASS recoginses two types of RATs: i) image format specific internal RATs, and ii) external [SQLite](https://www.sqlite.org)-based LUMASS RATs. The type of table that is going to be passed on downstream, is specified by the `RATType` parameter. Please refer to [RATType]({{ "docs/cref_image_reader#rattype" | relative_url }}) for more information on RATs. 

## Supported Image Formats

LUMASS uses the Geospatial Data Abstraction Library ([GDAL](https://gdal.org/index.html)) to read and write 2D images. However, LUMASS does not support all image formats that are supported by GDAL. Image formats that are (most likely) supported by LUMASS, are single dataset formats residing on local or network storage and that are referenced by conventional filenames. Additionally, LUMASS provides read/write access to (1D,2D,3D) datasets stored in [netCDF-4](https://www.unidata.ucar.edu/software/netcdf/documentation/historic/netcdf/NetCDF_002d4-Format.html#NetCDF_002d4-Format) files. 

## FileNames

LUMASS expects a filename to represent the absolute (fully qualified) pathname to the image to be read. On all platforms, please use forward slashes `/` as path separator. 

*Windows*

```
D:/mydata/Awesome Project/land-use.img
```

*Linux*

```
/home/lumass/Awesome Project/land-use.kea
```

LUMASS supports multi-dataset netCDF-4 files that organises images in groups. Group names are appended with `:` to the filename. The root group of a netCDF-4 file is named `/` and nested group names are separated also by `/`. For example the netCDF-variable `swc` in the root group of the netCDF `soilwater2007-2009.nc` file would be referenced in LUMASS like this:

```
/home/lumass/Awesome Project/soilwater2007-2009.nc:/swc
```

The variable `drg` stored in group `/regionA/scenarioB` would be referenced in LUMASS like this:

```
/home/lumass/Awesome Project/soilwater2007-2009.nc:/regionA/scenarioB/drg
```

## RATType

### ATTABLE_TYPE_RAM

The `ATTABLE_TYPE_RAM` parameter instructs LUMASS to access the RAT that is stored within the image file itself, regardless of whether an external RAT is available or not. LUMASS reads the entire RAT into memory (RAM). This provides very fast access to invididual table values and is the recommended way of passing RATs to the [`MapAlgebra`]({{ "docs/cref_map_algebra" | relative_url }}) component for pixel-based image processing. Image formats that support RATs are, for example [Erdas Imagine](https://gdal.org/drivers/raster/hfa.html) (`*.img`) or [KEA](https://gdal.org/drivers/raster/kea.html) (`*.kea`).

### ATTABLE_TYPE_SQLITE

This parameter instructs LUMASS to access the external SQLite-based RAT of the specified image file (s. `FileNames`). If an external RAT does not exist, but the image provides an internal (`ATTABLE_TYPE_RAM`) RAT, LUMASS creates the external RAT from the internal RAT. The external RAT is stored in a SQLite database that shares the same file basename as the image and carries the `*.ldb` extension. Instead of reading the entire RAT into main memory, LUMASS will only pass on the database connection to the downstream processing component. This makes external RATs RAM efficient and provides full SQLite processing capabilities that can be utilised by the [`SQLProcessor`]({{ "docs/cref_sql_processor" | relative_url }}) component.

**Important**: If `RATType=ATTABLE_TYPE_SQLITE` is specified and a SQLite-based LUMASS RAT (\*.ldb) is available alongside the image file, LUMASS **ignores** any RAT stored inside the actual image (e.g. *.img, *.kea) itself! 
{: .notice--warning}
