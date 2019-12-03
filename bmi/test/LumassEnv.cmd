@echo off
rem set local path
SET BATPATH=%~dp0..
SET GDAL_DRIVER_PATH=%BATPATH%\gdalplugins;%BATPATH%\utils\bin
SET PATH=%BATPATH%\utils\bin\otb;%BATPATH%\utils\bin\vtk;%BATPATH%\bin;%GDAL_DRIVER_PATH%;%BATPATH%\winsys\bin;%BATPATH%\utils\bin
rem %SystemRoot%\system32\cmd.exe
@echo ON
