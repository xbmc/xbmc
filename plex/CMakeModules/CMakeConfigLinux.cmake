# vim: setlocal syntax=cmake:

if(UNIX)
  set(CMAKE_REQUIRED_FLAGS "-D__LINUX_USER__")
endif()

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

#### default lircdevice
set(LIRC_DEVICE "/dev/lircd")

#### on linux we want to use a "easy" name
set(EXECUTABLE_NAME "plexhometheater")

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(ARCH "x86_64-linux")
else()
  set(ARCH "i486-linux")
endif()

set(LIBPATH bin)
set(BINPATH bin)
set(RESOURCEPATH share)

