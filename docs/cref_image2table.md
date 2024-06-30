---
title: "Image2Table"
permalink: "/docs/cref_image2table"
---
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description
----------|----------------|:-----------:|-------------
TableFileName | optional,<br>user-defined | 1 |  The absolute filename of the SQLite database in which to store the output table (`TableName`). The database will be created, if it does not exist yet.
TableName   | optional,<br>user-defined| 1 | The name of the output table the input image variable and associated variables are written to. If the table does not exist, it will be created.
ImageVarName | optional,<br>user-defined | 1 | The name of the column in the output table the input image variable is written into. If `AuxVarNames` is specified, this property must name the input image variable stored in `<NcImageContainer>/<NcGroupName>`.
UpdateMode | optional,<br>user-defined, default: `0` | 1 | If `UpdateMode = 0`, the specified variables (`ImageVarName`, `AuxVarNames`) are going to be inserted into the table (`TableName`). If `UpdateMode = 1`, previously inserted variable values are going to be updated (overwritten).
NcImageContainer | optional,<br>user-defined | 1 | Absolute filename of a NetCDF file (*.nc) containing the associated auxillary variables to be written into the output table alongside the input image (variable). Only required in conjuction with `AuxVarNames`.
NcGroupName | optional,<br>user-defined | 1 | NetCDF group in `NcImageContainer` holding variables (`AuxVarNames`) associated with the input image (NetCDF variable) `ImageVarName`. Only required in conjunction with `AuxVarNames`. Only specify if the variables are **not** stored in the root group.
StartIndex | optional,<br>user-defined, default: image origin | 2 | The pixel coordinates of the start point (top left pixel) of the input image (and associated variables, if applicable) region to be copied to the output table. 
Size | optional,<br>user-defined, default: image size | 2 | The size of the input image region in pixels to be copied to the output table.
DimVarNames | optional,<br>user-defined | 2 | The names of the (integer) output columns the pixel coordinates are written into. If specified at all, a parameter for each image dimension must be specified. However, image dimensions whose pixel coordinates shall not be written into the output table, can be indicated by an empty parameter value (s. note below). All specified (non-empty) names are suffixed by `_id`.
AuxVarNames | optional,<br>user-defined | 2 | Names of NetCDF variables associated with the input image (variable `ImageVarName`) that are stored in the same group (`NcGroupName`), share at least one dimension, and are to be copied alongside it into the output table. 

## Supported Image & Pipeline Features

Feature | Details 
---------------|---------------
Image dimensions | 1D, 2D, 3D
Multi-band images | no
Pipeline role | process
Sequential processing | yes
Parallel processing | no
Number of inputs | 1
Number of outputs | 1

## Overview

The `Image2Table` component copies (flattens) an input image variable from a NetCDF file and writes it into an SQLite database (`TableFileName`) table (`TablenName`). If the output database file and table does not exist already, it will be created in the process. The structure of the 'image table' (`TableName`), i.e. `DimVarNames` and `AuxVarNames`, cannot be changed by this component after the table has been created. However, additional records representing additional image areas or slices, can be added to the table, or existing records representing image areas that have changed, can be updated (`UpdateMode=1`).  

Since the `Image2Table` is not a *sink* component it needs to be connected as an input to an [`ImageWriter`]({{ "docs/cref_image_writer" | relative_url }}) that needs to be configured as 'virtual writer', i.e. its `FileNames` and `InputTables` properties should be empty and `WriteImage` as well as `WriteTable` should be set to `False`. However, `StreamingMethod` should be set to either `STRIPPED` or `TILED`.
