# vim: setlocal syntax=cmake:

set(CMAKE_REQUIRED_INCLUDES ${dependdir}/include ${root}/lib/ffmpeg)
set(CMAKE_REQUIRED_FLAGS "-D__LINUX_USER__")

######################### CHECK LIBRARIES / FRAMEWORKS
#### Frameworks for MacOSX
set(osx_frameworks
  AudioToolbox
  AudioUnit
  Cocoa
  CoreAudio
  CoreServices
  Foundation
  OpenGL
  AppKit
  ApplicationServices
  IOKit
  QuickTime
  Carbon
  DiskArbitration
  QuartzCore
  SystemConfiguration
  IOSurface
)

set(external_libs
  lzo2
  pcre
  pcrecpp
  fribidi
  cdio
  freetype
  fontconfig
  sqlite3
  samplerate
  microhttpd
  yajl
  jpeg
  crypto
  SDL
  SDL_mixer
  SDL_image
  tinyxml
  boost_thread
  boost_system
  z
  GLEW
  
  # ffmpeg libraries
  avcodec
  avutil
  avformat
  avfilter
  avdevice
  postproc
  swscale
  swresample 
)

set(non_link_libs
  rtmp
  plist
  shairport
  curl
  FLAC
  modplug
  vorbis
  vorbisfile
  vorbisenc
  ogg
  ass
  mad
  mpeg2
  bluray
  png
  tiff
  cec
)

set(system_libs iconv stdc++)  

# go through all the libs we need and find them plus set some good variables
foreach(lib ${external_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 1)
endforeach()

foreach(lib ${non_link_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 0)
endforeach()

# find our system libs
foreach(lib ${system_libs})
  plex_find_library(${lib} 0 0 "" 1)
endforeach()

foreach(lib ${osx_frameworks})
  plex_find_library(${lib} 1 0 ${dependdir}/Frameworks 1)
endforeach()

#### Deal with some generated files
set(EXECUTABLE_NAME "Plex Home Theater")

set(ARCH "x86-osx")

set(LIBPATH "${EXECUTABLE_NAME}.app/Contents/Frameworks")
set(BINPATH "${EXECUTABLE_NAME}.app/Contents/MacOSX")
set(RESOURCEPATH "${EXECUTABLE_NAME}.app/Contents/Resources/XBMC")
set(FFMPEG_INCLUDE_DIRS ${dependdir}/include)

set(HAVE_LIBVDADECODER 1)
set(AC_APPLE_UNIVERSAL_BUILD 0)
