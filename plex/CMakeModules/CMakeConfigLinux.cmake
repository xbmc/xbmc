# vim: setlocal syntax=cmake:

if(UNIX)
  set(CMAKE_REQUIRED_FLAGS "-D__LINUX_USER__")
endif()

include(CheckSystemIncludes)
include(CheckSystemFunctions)

include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckCSourceCompiles)

set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_MODULE_PATH})


macro(plex_get_soname library soname)
      # split out the library name
      get_filename_component(soname ${library} NAME)
endmacro()

macro(plex_find_package package required addtolinklist)
  if(${required})
    find_package(${package} REQUIRED)
  else(${required})
    find_package(${package})
  endif(${required})
  
  string(TOUPPER ${package} PKG_UPPER_NAME)
  string(REPLACE "_" "" PKG_NAME ${PKG_UPPER_NAME})
  
  if(${PKG_NAME}_FOUND)
    if(${PKG_NAME}_INCLUDE_DIR)
      get_property(PKG_INC VARIABLE PROPERTY ${PKG_NAME}_INCLUDE_DIR)
    else()
      get_property(PKG_INC VARIABLE PROPERTY ${PKG_NAME}_INCLUDE_DIRS)
    endif()

    include_directories(${PKG_INC})

    if(${PKG_NAME}_LIBRARY)
      get_property(PKG_LIB VARIABLE PROPERTY ${PKG_NAME}_LIBRARY)
    else()
      get_property(PKG_LIB VARIABLE PROPERTY ${PKG_NAME}_LIBRARIES)
    endif()

    set(CONFIG_LIBRARY_${PKG_UPPER_NAME} ${PKG_LIB})

    if(${addtolinklist})
      list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${PKG_LIB})
    else()
      list(APPEND CONFIG_PLEX_INSTALL_LIBRARIES ${PKG_LIB})
    endif()

    set(HAVE_LIB${PKG_UPPER_NAME} 1 CACHE string "if this lib is around or not")
  endif()
endmacro()

set(LINK_PKG
  FFmpeg
  Freetype
  SDL
  SDL_image
  SDL_mixer
  OpenGL
  ZLIB
  JPEG
  X11
  SQLite3
  LibSSH
  PCRE
  Lzo2
  FriBiDi
  Fontconfig
  Samplerate
  YAJL
  microhttpd
  Crypto
  TinyXML
  GLEW
  Iconv
)

foreach(l ${LINK_PKG})
  plex_find_package(${l} 1 1)
endforeach()

include(CheckFFmpegIncludes)
  
find_package(Boost COMPONENTS thread system REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${Boost_LIBRARIES})
  set(HAVE_BOOST 1)
endif()


### install libs
set(INSTALL_LIB
  CURL
  PNG
  TIFF
)

foreach(l ${INSTALL_LIB})
  plex_find_package(${l} 1 0)
endforeach()

plex_find_package(PulseAudio 0 1)
if(PULSEAUDIO_FOUND)
  include_directories(${PULSEAUDIO_INCLUDE_DIR})
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${PULSEAUDIO_LIBRARY})
  set(HAVE_LIBPULSE 1)
endif()

# Save some SONAMES we need
plex_get_soname(CURL_SONAME ${CURL_LIBRARIES})

set(non_link_libs
  rtmp
  plist
  shairport
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
  cec
)

set(CONFIG_INTERNAL_LIBS
  lib_hts
  lib_squish
  lib_upnp
  lib_dllsymbols
)

####
if(DEFINED X11_FOUND)
  set(HAVE_X11 1)
endif()

if(DEFINED SDL_FOUND)
  set(HAVE_SDL 1)
endif()

set(USE_UPNP 1)

#### check FFMPEG member name
if(DEFINED HAVE_LIBAVFILTER_AVFILTER_H)
  set(AVFILTER_INC "#include <libavfilter/avfilter.h>")
else()
  set(AVFILTER_INC "#include <ffmpeg/avfilter.h>")
endif()
CHECK_C_SOURCE_COMPILES("
  ${AVFILTER_INC}
  int main(int argc, char *argv[])
  { 
    static AVFilterBufferRefVideoProps test;
    if(sizeof(test.sample_aspect_ratio))
      return 0;
    return 0;
  }
"
HAVE_AVFILTERBUFFERREFVIDEOPROPS_SAMPLE_ASPECT_RATIO)

if(DEFINED HAVE_LIBCRYSTALHD_LIBCRYSTALHD_IF_H)
  CHECK_C_SOURCE_COMPILES("
    #include <libcrystalhd/bc_dts_types.h>
    #include <libcrystalhd/bc_dts_defs.h>
    PBC_INFO_CRYSTAL bCrystalInfo;
    int main() {}
  " CHECK_CRYSTALHD_VERSION)
  if(CHECK_CRYSTALHD_VERSION)
    set(HAVE_LIBCRYSTALHD 2)
  else()
    set(HAVE_LIBCRYSTALHD 1)
  endif()
endif()

#### default lircdevice
set(LIRC_DEVICE "/dev/lircd")

#### on linux we want to use a "easy" name
set(EXECUTABLE_NAME "plexhometheater")
set(PLEX_TARGET_NAME PlexHomeTheater)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(ARCH "x86_64-linux")
else()
  set(ARCH "i486-linux")
endif()

set(LIBPATH bin)
set(BINPATH bin)
set(RESOURCEPATH share)

configure_file(${root}/xbmc/DllPaths_generated.h.in ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h)
configure_file(${plexdir}/config.h.in ${CMAKE_BINARY_DIR}/xbmc/config.h)
