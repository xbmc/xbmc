# vim: setlocal syntax=cmake:

######################### Compiler CFLAGS
set(EXTRA_CFLAGS "-fPIC -DPIC")

######################### CHECK LIBRARIES / FRAMEWORKS
if(UNIX)
  set(CMAKE_REQUIRED_FLAGS "-D__LINUX_USER__")
endif()

option(USE_INTERNAL_FFMPEG "" ON)

set(LINK_PKG
  Freetype
  SDL
  SDL_image
  SDL_mixer
  OpenGL
  ZLIB
  JPEG
  X11
  SQLite3
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
  FLAC
  DBUS
)

if(NOT USE_INTERNAL_FFMPEG)
  list(APPEND LINK_PKG FFmpeg)
else()
  set(FFMPEG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/lib/ffmpeg ${CMAKE_BINARY_DIR}/lib/ffmpeg/ffmpeg/src/ffmpeg-build)
endif()

if(ENABLE_PYTHON)
   list(APPEND LINK_PKG Python)
endif(ENABLE_PYTHON)

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
  VAAPI
  VDPAU
)

foreach(l ${INSTALL_LIB})
  plex_find_package(${l} 1 0)
endforeach()

if(NOT DISABLE_CEC)
  plex_find_package(CEC 0 0)
endif(DISABLE_CEC)

plex_find_package(Threads 1 0)
if(CMAKE_USE_PTHREADS_INIT)
  message(STATUS "Using pthreads: ${CMAKE_THREAD_LIBS_INIT}")
  list(APPEND CONFIG_PLEX_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
  set(HAVE_LIBPTHREAD 1)
endif()

plex_find_package(PulseAudio 0 1)
if(HAVE_LIBPULSEAUDIO)
  set(HAVE_LIBPULSE 1)
endif()
plex_find_package(Alsa 0 1)

plex_find_package(LibUSB 0 1)
plex_find_package(LibUDEV 0 1)

if(ENABLE_DVD_DRIVE)
  plex_find_package(CDIO 1 1)
endif(ENABLE_DVD_DRIVE)

if(NOT LIBUSB_FOUND AND NOT LIBUDEV_FOUND)
  message(WARNING "No USB support")
endif()

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

set(PLEX_LINK_WRAPPED "-Wl,--unresolved-symbols=ignore-all -Wl,-wrap,_IO_getc -Wl,-wrap,_IO_getc_unlocked -Wl,-wrap,_IO_putc -Wl,-wrap,__fgets_chk -Wl,-wrap,__fprintf_chk -Wl,-wrap,__fread_chk -Wl,-wrap,__fxstat64 -Wl,-wrap,__lxstat64 -Wl,-wrap,__printf_chk -Wl,-wrap,__read_chk -Wl,-wrap,__vfprintf_chk -Wl,-wrap,__xstat64 -Wl,-wrap,_stat -Wl,-wrap,calloc -Wl,-wrap,clearerr -Wl,-wrap,close -Wl,-wrap,closedir -Wl,-wrap,dlopen -Wl,-wrap,fclose -Wl,-wrap,fdopen -Wl,-wrap,feof -Wl,-wrap,ferror -Wl,-wrap,fflush -Wl,-wrap,fgetc -Wl,-wrap,fgetpos -Wl,-wrap,fgetpos64 -Wl,-wrap,fgets -Wl,-wrap,fileno -Wl,-wrap,flockfile -Wl,-wrap,fopen -Wl,-wrap,fopen64 -Wl,-wrap,fprintf -Wl,-wrap,fputc -Wl,-wrap,fputs -Wl,-wrap,fread -Wl,-wrap,free -Wl,-wrap,freopen -Wl,-wrap,fseek -Wl,-wrap,fseeko64 -Wl,-wrap,fsetpos -Wl,-wrap,fsetpos64 -Wl,-wrap,fstat -Wl,-wrap,ftell -Wl,-wrap,ftello64 -Wl,-wrap,ftrylockfile -Wl,-wrap,funlockfile -Wl,-wrap,fwrite -Wl,-wrap,getc -Wl,-wrap,getc_unlocked -Wl,-wrap,getmntent -Wl,-wrap,ioctl -Wl,-wrap,lseek -Wl,-wrap,lseek64 -Wl,-wrap,malloc -Wl,-wrap,open -Wl,-wrap,open64 -Wl,-wrap,opendir -Wl,-wrap,popen -Wl,-wrap,printf -Wl,-wrap,read -Wl,-wrap,readdir -Wl,-wrap,readdir64 -Wl,-wrap,realloc -Wl,-wrap,rewind -Wl,-wrap,rewinddir -Wl,-wrap,setvbuf -Wl,-wrap,ungetc -Wl,-wrap,vfprintf -Wl,-wrap,write")

set(PLEX_LINK_WHOLEARCHIVE -Wl,--whole-archive)
set(PLEX_LINK_NOWHOLEARCHIVE -Wl,--no-whole-archive)

option(OPENELEC "Are we building OpenELEC dist?" OFF)
if(OPENELEC)
  add_definitions(-DOPENELEC)
endif(OPENELEC)

############ Add our definitions
add_definitions(-DTARGET_LINUX -D_LINUX)
