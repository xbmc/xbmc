project(ffmpeg)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set(ffmpeg_conf "")

if(CROSSCOMPILING)
  set(pkgconf "PKG_CONFIG_LIBDIR=${DEPENDS_PATH}/lib/pkgconfig")
  list(APPEND ffmpeg_conf --pkg-config=${PKG_CONFIG_EXECUTABLE} --pkg-config-flags=--static)
  list(APPEND ffmpeg_conf --enable-cross-compile --target-os=${OS})
  list(APPEND ffmpeg_conf --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER} --ar=${CMAKE_AR})
  list(APPEND ffmpeg_conf --extra-cflags=${CMAKE_C_FLAGS} --extra-cxxflags=${CMAKE_CXX_FLAGS} --extra-ldflags=${CMAKE_EXE_LINKER_FLAGS})
  message(STATUS "CROSS: ${ffmpeg_conf}")
endif()

list(APPEND ffmpeg_conf --arch=${ARCH})

if(CPU)
  list(APPEND ffmpeg_conf --cpu=${CPU})
endif()

if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
  list(APPEND ffmpeg_conf --enable-vdpau --enable-vaapi --enable-pic)
elseif(CORE_SYSTEM_NAME STREQUAL android)
  if(ARCH MATCHES "x86.*")
    list(APPEND ffmpeg_conf --disable-mmx)
  endif()
  list(APPEND ffmpeg_conf --extra-libs=-liconv)
elseif(CORE_SYSTEM_NAME STREQUAL ios)
  list(APPEND ffmpeg_conf --disable-decoder=mpeg_xvmc --disable-vda --disable-crystalhd --enable-videotoolbox)
elseif(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND ffmpeg_conf --disable-outdev=sdl
                          --disable-decoder=mpeg_xvmc --disable-vda --disable-crystalhd --enable-videotoolbox
                          --disable-securetransport)
endif()

if(ARCH STREQUAL arm)
  list(APPEND ffmpeg_conf --enable-pic --disable-armv5te --disable-armv6t2)
elseif(ARCH MATCHES mips)
  list(APPEND ffmpeg_conf --disable-mips32r2 --disable-mipsdspr1 --disable-mipsdspr2)
endif()

if(NEON)
  list(APPEND ffmpeg_conf --enable-neon)
endif()


find_package(GnuTls)
if(GNUTLS_FOUND)
  list(APPEND ffmpeg_conf --enable-gnutls)
endif()

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
                      --disable-ffserver
                      --disable-sdl
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
