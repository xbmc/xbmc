project(ffmpeg)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})


set(ffmpeg_conf "")
#if(CMAKE_BUILD_TYPE STREQUAL "Release")
  #  list(APPEND ffmpeg_conf --enable-optimizations)
  #else()
  #  list(APPEND ffmpeg_conf --disable-optimizations)
  #endif()

if(CROSSCOMPILING)
  set(pkgconf "PKG_CONFIG_LIBDIR=${DEPENDS_PATH}/lib/pkgconfig")
  list(APPEND ffmpeg_conf --pkg-config=${PKG_CONFIG_EXECUTABLE} --pkg-config-flags=--static)
  list(APPEND ffmpeg_conf --enable-cross-compile --cpu=${CPU} --arch=${CPU} --target-os=${OS})
  list(APPEND ffmpeg_conf --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER} --ar=${CMAKE_AR})
  list(APPEND ffmpeg_conf --extra-cflags=${CMAKE_C_FLAGS} --extra-cxxflags=${CMAKE_CXX_FLAGS} --extra-ldflags=${CMAKE_EXE_LINKER_FLAGS})
  message(STATUS "CROSS: ${ffmpeg_conf}")
endif()

if(CPU MATCHES "arm")
  list(APPEND ffmpeg_conf --disable-armv5te --disable-armv6t2)
elseif(CPU MATCHES "mips")
  list(APPEND ffmpeg_conf --disable-mips32r2 --disable-mipsdspr1 --disable-mipsdspr2)
endif()

if(CORE_SYSTEM_NAME STREQUAL "android")
  if(CPU MATCHES "arm")
    list(APPEND ffmpeg_conf --cpu=cortex-a9)
  else()
    list(APPEND ffmpeg_conf --cpu=i686 --disable-mmx)
  endif()
  list(APPEND ffmpeg_conf --target-os=linux)
endif()

find_package(GnuTls)
if(GNUTLS_FOUND)
  list(APPEND ffmpeg_conf --enable-gnutls)
endif()

include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(PARALLEL_FLAGS -j${N})
endif()

include(ExternalProject)
externalproject_add(ffmpeg
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ${pkgconf} <SOURCE_DIR>/configure
                      --disable-muxers
                      --enable-zlib
                      --enable-bzlib
                      --enable-muxer=spdif
                      --enable-muxer=adts
                      --enable-muxer=asf
                      --enable-muxer=ipod
                      --disable-encoders
                      --enable-encoder=ac3
                      --enable-encoder=aac
                      --enable-encoder=wmav2
                      --disable-decoder=mpeg_xvmc
                      --disable-devices
                      --disable-ffprobe
                      --disable-ffplay
                      --disable-ffserver
                      --disable-ffmpeg
                      --disable-crystalhd
                      --enable-static
                      --disable-shared
                      --disable-doc
                      --enable-postproc
                      --enable-gpl
                      --enable-vdpau
                      --enable-vaapi
                      --enable-protocol=http
                      --enable-pthreads
                      --enable-runtime-cpudetect
                      --enable-pic
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      ${ffmpeg_conf}
                    BUILD_COMMAND make ${PARALLEL_FLAGS})

install(CODE "Message(Done)")

# Quell warnings
set(BUILD_SHARED_LIBS)
set(XBMC_BUILD_DIR)
set(KODI_BUILD_DIR)
