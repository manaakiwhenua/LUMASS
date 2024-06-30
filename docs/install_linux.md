---
title: "Linux installation"
permalink: "/docs/install_linux"
---
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

LUMASS is provided as an [AppImage](https://appimage.org/) based on Ubuntu 20.04. It ships with most of its dependencies. However, a number of low-level system libraries are not distributed. For Ubuntu 20.04 those libraries are:

```libc6 libcom-err2 libexpat1 libfontconfig libglib2.0-0 libglx0 libgmp10 libgpg-error0 libopengl0 libp11-kit0 libstdc++6 libx11-6 libxcb1 zlib1g``` 

If you encounter trouble running lumass, please make sure you've got the above mentioned libraries installed on your system. Note that the names of those libraries are likely
to be different for other distributions. To 'install' LUMASS on your system, follow the steps below.

1. Install gdal and OpenGL using your package manager

   *Ubuntu 20.04*

   ```
   $ sudo apt-get install gdal-bin libgdal26 libopengl0
   ```

2. Download the most recent lumass AppImage to a directory you've got write permissions for.

   [https://github.com/manaakiwhenua/LUMASS/releases](https://github.com/manaakiwhenua/LUMASS/releases)

3. To 'install' lumass, just make the AppImage executable

   ```
   $ chmod +x ./lumass-0.9.65-rc4-x86_64.AppImage
    ```

4. Run lumass by executing the AppImage. To run models with the lumassengine, just provide appropriate commandline arguments.

   *LUMASS GUI*

   Just execute or double click the AppImage to run the lumass GUI:

     ```
     $ ./lumass-0.9.65-rc4-x86_64.AppImage
     ```

   *lumassengine*
   
   To run lumass on the commandline, just provide appropriate commandline paramters to the AppImage. To get an overview of the commandline arguments the lumassengine accepts, just pass `--model` or `--moso`:

   ```
   $ ./lumass-0.9.65-rc4-x86_64.AppImage --model
   ERROR - LUMASS_engine::isFileAccessible, l. 762: No settings
   file has been specified!

   LUMASS (lumassengine) 0.9.65

   Usage: lumassengine --moso <settings file (*.los)> | --model
   <LUMASS model file (*.lmx | *.yaml)> [--workspace <absolute
   directory path for '$[LUMASS:Workspace]$'>] [--logfile <file
   name>] [--logprov]
   ```