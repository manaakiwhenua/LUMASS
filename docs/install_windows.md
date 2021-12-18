---
title: "Windows installation"
permalink: "/docs/install_windows"
---

1. [Download](https://github.com/manaakiwhenua/LUMASS/releases) the most recent LUMASS for Windows release

2. Extract the archive into an arbitrary folder on your hard drive or usb stick.

3. To start the LUMASS desktop application, change into the LUMASS folder (you've just extracted) and double click the file `lumass.bat`.

   To run the lumassengine on the conmmandline, use the `lumassengine.bat` and provide appropriate commandline arguments:

   ```
   Usage: lumassengine.bat --moso <settings file (*.los)> | --model 
   <LUMASS model file (*.lmx | *.yaml)> [--workspace <absolute 
   directory path for '$[LUMASS:Workspace]$'>] [--logfile <file 
   name>] [--logprov]
   ```
