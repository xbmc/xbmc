project(ffmpeg)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(ENABLE_CCACHE AND CCACHE_PROGRAM)
  set(ffmpeg_conf "--cc=${CCACHE_PROGRAM} ${CMAKE_C_COMPILER}" "--cxx=${CCACHE_PROGRAM} ${CMAKE_CXX_COMPILER}")
else()
  set(ffmpeg_conf --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER})
endif()

if(CROSSCOMPILING)
  set(pkgconf "PKG_CONFIG_LIBDIR=${DEPENDS_PATH}/lib/pkgconfig")
  list(APPEND ffmpeg_conf --pkg-config=${PKG_CONFIG_EXECUTABLE} --pkg-config-flags=--static)
  list(APPEND ffmpeg_conf --enable-cross-compile --cpu=${CPU} --arch=${CPU} --target-os=${OS})
  list(APPEND ffmpeg_conf --ar=${CMAKE_AR} --strip=${CMAKE_STRIP})
  message(STATUS "CROSS: ${ffmpeg_conf}")
endif()

if(CMAKE_C_FLAGS)
  list(APPEND ffmpeg_conf --extra-cflags=${CMAKE_C_FLAGS})
endif()

if(CMAKE_CXX_FLAGS)
  list(APPEND ffmpeg_conf --extra-cxxflags=${CMAKE_CXX_FLAGS})
endif()

if(CMAKE_EXE_LINKER_FLAGS)
  list(APPEND ffmpeg_conf --extra-ldflags=${CMAKE_EXE_LINKER_FLAGS})
endif()

if(ENABLE_NEON)
  list(APPEND ffmpeg_conf --enable-neon)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
  list(APPEND ffmpeg_conf --disable-debug)
endif()

if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
  if(CORE_PLATFORM_NAME STREQUAL rbpi)
    list(APPEND ffmpeg_conf --cpu=${CPU} --disable-vaapi --disable-vdpau)
  else()
    list(APPEND ffmpeg_conf --enable-pic)
    if(ENABLE_VAAPI)
      list(APPEND ffmpeg_conf --enable-vaapi)
    else()
      list(APPEND ffmpeg_conf --disable-vaapi)
    endif()
    if(ENABLE_VDPAU)
      list(APPEND ffmpeg_conf --enable-vdpau)
    else()
      list(APPEND ffmpeg_conf --disable-vdpau)
    endif()
  endif()
elseif(CORE_SYSTEM_NAME STREQUAL android)
  if(CPU MATCHES arm64)
    list(APPEND ffmpeg_conf --cpu=cortex-a53 --arch=aarch64)
  elseif(CPU MATCHES arm)
    list(APPEND ffmpeg_conf --cpu=cortex-a9)
  else()
    list(APPEND ffmpeg_conf --cpu=i686 --disable-mmx)
  endif()
  list(APPEND ffmpeg_conf --target-os=linux --extra-libs=-liconv --disable-linux-perf)
elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  if(NOT CPU MATCHES arm64)
    list(APPEND ffmpeg_conf --cpu=cortex-a8)
  else()
    list(APPEND ffmpeg_conf "--as=${NATIVEPREFIX}/bin/gas-preprocessor.pl -arch aarch64 -- ${CMAKE_C_COMPILER}")
  endif()
  list(APPEND ffmpeg_conf --disable-decoder=mpeg_xvmc --disable-crystalhd --enable-videotoolbox
                          --target-os=darwin)
elseif(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND ffmpeg_conf --disable-decoder=mpeg_xvmc --disable-crystalhd --enable-videotoolbox
                          --target-os=darwin
                          --disable-securetransport)
endif()

if(CPU MATCHES arm OR CORE_PLATFORM_NAME STREQUAL rbpi)
  list(APPEND ffmpeg_conf --enable-pic --disable-armv5te --disable-armv6t2)
elseif(CPU MATCHES mips)
  list(APPEND ffmpeg_conf --disable-mips32r2 --disable-mipsdsp --disable-mipsdspr2)
endif()

find_package(GnuTls)
if(GNUTLS_FOUND)
  list(APPEND ffmpeg_conf --enable-gnutls)
endif()

message(STATUS "FFMPEG_CONF: ${ffmpeg_conf}")

include(ExternalProject)
externalproject_add(ffmpeg
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ${pkgconf} <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --extra-version="kodi-${FFMPEG_VER}"
                      --disable-devices
                      --disable-doc
                      --disable-ffplay
                      --disable-ffmpeg
                      --disable-ffprobe
                      --enable-gpl
                      --enable-runtime-cpudetect
                      --enable-postproc
                      --enable-pthreads
                      --enable-muxer=spdif
                      --enable-muxer=adts
                      --enable-muxer=asf
                      --enable-muxer=ipod
                      --enable-encoder=ac3
                      --enable-encoder=aac
                      --enable-encoder=wmav2
                      --enable-protocol=http
                      --enable-encoder=png
                      --enable-encoder=mjpeg
                      ${ffmpeg_conf})

install(CODE "Message(Done)")

# Quell warnings
set(BUILD_SHARED_LIBS)
set(XBMC_BUILD_DIR)
set(KODI_BUILD_DIR)
