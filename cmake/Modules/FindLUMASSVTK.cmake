# ***********************************************
# FindLUMASSVTK.cmake
#
# created by Alexander Herzig
# Copyright 2014 Landcare Reseaerch New Zealand
#
# just looks for some possible places VTK might be installed in
# to populate VTK_DIR
# ***********************************************

if (NOT VTK_DIR)
    FIND_PATH(VTK_DIR vtk-config.cmake
    PATH_SUFFIXES
        VTK-debug
        VTK-9.0.1-dbg
        VTK-bin
        VTK-9.0.1-bin
        vtk-8.2
        VTK-9.0.1
        vtk-9.0.1
        VTK-9.3.0-bin
        VTK-9.3

    PATHS
        $ENV{HOME}/build
        /opt
        /usr/local
        /usr/local/lib
        /usr/lib/cmake
        /usr/lib/x86_64-linux/cmake
        c:/opt
        c:/build
    )
endif()

message(STATUS "vtk-config.cmake found in : ${VTK_DIR}")


if (NOT VTK_DIR)
    FIND_PATH(VTK_DIR VTKConfig.cmake
    PATH_SUFFIXES
      VTK-debug
      VTK-reldebinfo
      VTK-bin
      vtk-bin
      vtk
      VTK
      vtk-6.1
      vtk-6.2
      vtk-6.3
      vtk-6.4
      VTK-9.3.0-bin
      VTK-9.3

    PATHS
      $ENV{HOME}/build
      /opt
      /usr/local
      /usr/local/lib
      /usr/local/src
      /usr/lib
      /usr/lib/cmake
      /usr/share
      c:/opt
      c:/build
      "c:/Programm Files (x86)"
      "c:/Programm Files"
    )
    message(STATUS "VTKconfig.cmake found in : ${VTK_DIR}")
endif()

if (MSVC)
  set(FFMPEG_VERSION "7.1")
  set(FFMPEG_avcodec_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_avcodec_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/avcodec.lib")
  set(FFMPEG_avdevice_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_avdevice_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/avdevice.lib")
  set(FFMPEG_avfilter_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_avfilter_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/avfilter.lib")
  set(FFMPEG_avformat_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_avformat_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/avformat.lib")
  set(FFMPEG_avutil_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_avutil_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/avutil.lib")
  set(FFMPEG_swresample_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_swresample_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/swresample.lib")
  set(FFMPEG_swscale_INCLUDE_DIR "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/include")
  set(FFMPEG_swscale_LIBRARY "C:/Program Files/WinGet/Packages/BtbN.FFmpeg.LGPL.Shared.7.1_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-n7.1.1-57-g1b48158a23-win64-lgpl-shared-7.1/lib/swscale.lib")
endif()
