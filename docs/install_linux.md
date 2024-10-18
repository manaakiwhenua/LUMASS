---
title: "Linux installation"
permalink: "/docs/install_linux"
---
<link rel="shortcut icon" type="image/x-icon" href="../LUMASS_icon_64.ico">

## AppImage

LUMASS is provided as an [AppImage](https://appimage.org/) based on Ubuntu 20.04. It ships with most of its dependencies. However, a number of low-level system libraries are not distributed. For Ubuntu 20.04 those libraries are:

```libc6 libcom-err2 libexpat1 libfontconfig libglib2.0-0 libglx0 libgmp10 libgpg-error0 libopengl0 libp11-kit0 libstdc++6 libx11-6 libxcb1 zlib1g``` 

If you encounter trouble running lumass, please make sure you've got the above mentioned libraries installed on your system. Note that the names of those libraries are likely
to be different for other distributions. To 'install' LUMASS on your system, follow the steps below.

1. Install gdal and OpenGL using your package manager

   *Ubuntu 20.04*

   ```
   sudo apt-get install gdal-bin libgdal26 libopengl0
   ```

2. Download the most recent lumass AppImage to a directory you've got write permissions for.

   [https://github.com/manaakiwhenua/LUMASS/releases](https://github.com/manaakiwhenua/LUMASS/releases)

3. To 'install' lumass, just make the AppImage executable

   ```
   chmod +x ./lumass-0.9.65-rc4-x86_64.AppImage
   ```

4. Run lumass by executing the AppImage. To run models with the lumassengine, just provide appropriate commandline arguments.

   *LUMASS GUI*

   Just execute or double click the AppImage to run the lumass GUI:

     ```
     ./lumass-0.9.65-rc4-x86_64.AppImage
     ```

   *lumassengine*
   
   To run lumass on the commandline, just provide appropriate commandline paramters to the AppImage. To get an overview of the commandline arguments the lumassengine accepts, just pass `--model` or `--moso`:

   ```
   ./lumass-0.9.65-rc4-x86_64.AppImage --model
   ERROR - LUMASS_engine::isFileAccessible, l. 762: No settings
   file has been specified!

   LUMASS (lumassengine) 0.9.65

   Usage: lumassengine --moso <settings file (*.los)> | --model
   <LUMASS model file (*.lmx | *.yaml)> [--workspace <absolute
   directory path for '$[LUMASS:Workspace]$'>] [--logfile <file
   name>] [--logprov]
   ```

## Build LUMASS and AppImage from source

**Info:** the build scripts used in the below guide were developed for *Ubuntu 22.04*. You may need to adapt both scripts in order to successfully build LUMASS from source on other Linux distributions and Ubuntu versions.
{: .notice--info}

### Prerequisites 
- Super user access to your system to install ubuntu packages to satisfy the build requirements (`lumass_install_deb_packages.sh`). If the required libraries are already installed, this step can be skipped.
- Make sure you have about 15 GB of free disk space available for additional LUMASS dependencies that are being built as part of the process.

### Build steps

1. Download the build scripts
```
wget https://raw.githubusercontent.com/manaakiwhenua/LUMASS/develop/utils/build/lumass_install_deb_packages.sh
wget https://raw.githubusercontent.com/manaakiwhenua/LUMASS/develop/utils/build/lumass_build.sh
```

2. Make sure the build scripts are executable
```
chmod u+x lumass_*.sh
```

3. Install packages required for building LUMASS and additional dependencies from source
```
sudo ./lumass_install_dep_packages.sh
```

4. Build LUMASS dependencies and LUMASS from source, download AppImage tools, generate AppImage
```
./lumass_build.sh
```

   The build script will generate the following directory structure in your home directory:
   ```
   $HOME
   |-garage
         |-apps
         |   |-<AppImage tools>
         |-cpp
         |   |-<source code of dependencies and LUMASS>
         |-build
             |-<binaries of dependencies and LUMASS>
   ```
   After the script has successfully finished, the LUMASS binaries will be installed in 
      ```
      $HOME/garage/build/lumass-build/lumass-0.9.<rev>
      ```

   and the AppImage will be available in

      ```
      $HOME/garage/build/lumass-build/lumass-x86_64.AppImage
      ```

### Running LUMASS from the install tree

Starting the LUMASS GUI
```
$HOME/garage/build/lumass-build/lumass-0.9.<rev>/usr/bin/lumass
```

Running the lumassengine
```
$HOME/garage/build/lumass-build/lumass-0.9.<rev>/usr/bin/lumassengine
```

### Using the LUMASS AppImage

**Info:** The AppImage is portable and can be copied to different folders or Linux distributions that are as or more recent than the system it was generated on (here: *Ubuntu 22.04*). For more detailed information on AppImages in general, please refer to the [AppImage](https://www.appimage.org) website.
{: .notice--info}


Ensure the LUMASS `*.AppImage` is executable
```
chmod u+x $HOME/garage/build/lumass-build/lumass-x86_64.AppImage
```

Starting the LUMASS GUI
```
$HOME/garage/build/lumass-build/lumass-x86_64.AppImage
```

To invoke the lumassengine from the AppImage, just pass a valid parameter and argument to the AppImage, e.g.
```
$HOME/garage/build/lumass-build/lumass-x86_64.AppImage --model myModel_config.yaml
```
