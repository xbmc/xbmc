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
  Avahi
  Xrandr
  LibDl
  LibRt
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
  Vorbis
  LibMad
  Mpeg2
  Ass
  RTMP
  PLIST
  ShairPort
  CEC
  VAAPI
  VDPAU
)

foreach(l ${INSTALL_LIB})
  plex_find_package(${l} 1 0)
endforeach()

plex_find_package(Threads 1 0)
if(CMAKE_USE_PTHREADS_INIT)
  message(STATUS "Using pthreads: ${CMAKE_THREAD_LIBS_INIT}")
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
  set(HAVE_LIBPTHREAD 1)
endif()

plex_find_package(PulseAudio 0 1)
plex_find_package(Alsa 0 1)

if(VAAPI_FOUND)
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${VAAPI_LIBRARIES})
  include_directories(${VAAPI_INCLUDE_DIR})
  set(HAVE_LIBVA 1)
endif()

plex_get_soname(CURL_SONAME ${CURL_LIBRARY})

list(APPEND CONFIG_INTERNAL_LIBS lib_dllsymbols)

####
if(DEFINED X11_FOUND)
  set(HAVE_X11 1)
endif()

if(DEFINED OPENGL_FOUND)
  set(HAVE_LIBGL 1)
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
set(RESOURCEPATH share/XBMC)


set(PLEX_LINK_WHOLEARCHIVE -Wl,--whole-archive)
set(PLEX_LINK_NOWHOLEARCHIVE -Wl,--no-whole-archive)

