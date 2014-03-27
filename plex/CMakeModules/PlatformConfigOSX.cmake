# vim: setlocal syntax=cmake:

if(NOT DEFINED OSX_ARCH)
  set(OSX_ARCH x86_64)
endif()

if(NOT OSX_ARCH STREQUAL "i386" AND NOT OSX_ARCH STREQUAL "x86_64")
  message(FATAL_ERROR "Architecture ${OSX_ARCH} is not supported")
endif()

find_package(OSXSDK)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch ${OSX_ARCH}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch ${OSX_ARCH}")

# we will test that our compiler handles the arch
if(NOT CMAKE_C_FLAGS STREQUAL BASIC_COMPILE_TEST_FLAGS)
  unset(BASIC_COMPILE_TEST CACHE)
  unset(BASIC_COMPILE_TEST_FLAGS CACHE)
endif()

include(CheckCSourceCompiles)
CHECK_C_SOURCE_COMPILES("
  int main(int argc, char *argv[])
  { 
    return 0;
  }
" BASIC_COMPILE_TEST)

if(NOT BASIC_COMPILE_TEST)
  message(FATAL_ERROR "Compiler failed even the most basic compile test...")
else()
  set(BASIC_COMPILE_TEST_FLAGS ${CMAKE_C_FLAGS} CACHE STRING "CFLAGS for tests")
endif()


# MUST BE ADDED FIRST :)
# This will download our dependency tree
add_subdirectory(plex/Dependencies)

set(CMAKE_REQUIRED_INCLUDES ${dependdir}/include ${root}/lib/ffmpeg)
set(CMAKE_REQUIRED_FLAGS "-D__LINUX_USER__")

######################### Check if we are running within XCode
if(DEFINED XCODE_VERSION)
  message("Building with XCode Generator")
  set(USING_XCODE 1)
endif()

######################### Compiler CFLAGS
set(EXTRA_CFLAGS "-Qunused-arguments -mmacosx-version-min=10.6 -isysroot ${OSX_SDK_PATH}")

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
  Carbon
  DiskArbitration
  QuartzCore
  SystemConfiguration
  IOSurface
  bz2
  z
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
  tinyxml
  boost_thread
  boost_system
  GLEW
  vorbis
  vorbisenc
)

set(ffmpeg_libs
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

if(ENABLE_PYTHON)
  plex_find_package(Python 1 1)
endif()

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
  SDL_image
)

set(system_libs iconv stdc++)  

# go through all the libs we need and find them plus set some good variables
foreach(lib ${external_libs})
  plex_find_library(${lib} 0 1 ${dependdir}/lib 1)
endforeach()

# find ffmpeg libs
foreach(lib ${ffmpeg_libs})
  plex_find_library(${lib} 0 1 ${ffmpegdir}/lib 1)
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
set(FFMPEG_INCLUDE_DIRS ${ffmpegdir}/include)

set(PLEX_LINK_WRAPPED "-arch ${OSX_ARCH} -undefined dynamic_lookup -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias")
if(OSX_ARCH STREQUAL "i386")
  set(PLEX_LINK_WRAPPED "${PLEX_LINK_WRAPPED} -read_only_relocs suppress")
endif(OSX_ARCH STREQUAL "i386")

set(HAVE_LIBVDADECODER 1)
set(AC_APPLE_UNIVERSAL_BUILD 0)

################## Definitions
add_definitions(-DTARGET_DARWIN -DTARGET_DARWIN_OSX -DUSE_EXTERNAL_FFMPEG)
