---
title: "ExternalExec"
permalink: "/docs/cref_external_exec"
--- 
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## Properties

 Property | Characteristic | Dimension | Description 
----------|----------------|:-----------:|-------------
Command | compulsory,<br>user-defined, default: `empty` | 1 | The command to be executed at a given iteration. 
Environment | compulsory<br>user-defined, default: `empty` | 2 | The set of environment variable definitions as required by the specified command for the given iteration.  

## Supported Image & Pipeline Features

Characteristic | Details
---------------|---------------
Image dimensions | n/a
Multi-band images | n/a
Pipeline role | stand alone
Sequential processing | n/a
Parallel processing | n/a

## Overview

`ExternalExec` enables the integration of commandline-based programs or scripts into a LUMASS model (workflow). When `ExternalExec` is executed, LUMASS launches a separate process, establishes the specified `Environment` and executes the specified `Command`. `Comannd` output is forwareded to either the model's log file or the [Notifications]({{ "docs/gui" | relative_url }}) window, depending on whether the model is run by the `lumassengine` or the LUMASS desktop (GUI) program.  

## Command Specification

The `Command` property can be used to specify any commandline-based program or script including its arguments that runs on the computer LUMASS is being executed on. The example below shows how LUMASS utilises a small utility script to run the [`gdal_rasterize`](https://gdal.org/programs/gdal_rasterize.html) command.

[![ExternalExecCommand]({{ "/assets/images/docs/external_command.png" | relative_url }})]({{ "/assets/images/docs/external_command.png" | relative_url }})<br>
**`ExternalExec`'s `Command` property**


## Environment Variable Definition

Environment variables for a given command for a given iteration are specified as `<var_name>=<value>`, e.g.

```bash
MY_OUTPUTDIR="/home/geoprocessor/outputs/scenario_1"
```

[![EnvVarDefinition]({{ "/assets/images/docs/env_var_definition.png" | relative_url }}){: width="80%", height="80%"}]({{ "/assets/images/docs/env_var_definition.png" | relative_url }})<br>
**Evironment variable definition**


## External Utility Scripts

 LUMASS ships with a small number of external utility scripts that execute certain externally provided functionality, such as GDAL applications or operating system commands. All scripts carry the `*.bat` file extension to enable model portability across Windows and Linux platforms. The scripts can be found in the `usr/utils` subidrectory of the LUMASS AppImage mountpoint, e.g. `/tmp/.mount_lumassgWSfhy`, on Linux or in the `utils` subdirectory of the main LUMASS folder, e.g. `lumass-0.9.66`, on Windows. On both systems, the path can be accessed from within a model with the LUMASS expression `$[LUMASS:LUMASSPath]$/../utils` (cf. [ExternalExec's Command property]({{ "/assets/images/docs/external_command.png" | relative_url }})).

Script | Functionality | Synopsis
-------|---------------|-----------
del.bat | Deletion of files | `del.bat <path> <file>`
gdal_polygonize.bat | Vectorisation of images (raster) | s. [gdal.org](https://gdal.org/programs/gdal_polygonize.html)
gdal_rasterize.bat | Rasterisation of vectors (shape files) | s. [gdal.org](https://gdal.org/programs/gdal_rasterize.html)
ogr2ogr.bat | Vector file format conversion | s. [gdal.org](https://gdal.org/programs/ogr2ogr.html)

**Important** While LUMASS for Windows provides all necessary libraries and programs to successfully execute these scripts, Linux users need to [install]({{ "docs/install_linux" | relative_url }}) GDAL to utilise its functionality. 
{: .notice--warning}
