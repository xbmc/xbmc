if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for rbpi. See ${PROJECT_SOURCE_DIR}/README.md")
endif()

set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_ARMEL -DTARGET_RASPBERRY_PI
                 -DHAS_OMXPLAYER -DHAVE_OMXLIB)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)

string(REGEX REPLACE "[ ]+" ";" SYSTEM_LDFLAGS $ENV{LDFLAGS})
set(CMAKE_SYSTEM_NAME Linux)

if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL arm1176jzf-s)
    set(ARCH arm-linux-gnueabihf)
  elseif(CPU MATCHES "cortex-a7")
    set(ARCH arm-linux-gnueabihf)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

find_package(CXX11 REQUIRED)

set(LIRC_DEVICE          "\"/dev/lircd\"" CACHE STRING "LIRC device to use")
set(DEP_DEFINES -DLIRC_DEVICE=${LIRC_DEVICE})
set(MMAL_FOUND 1 CACHE INTERNAL "MMAL")
set(OMX_FOUND 1 CACHE INTERNAL "OMX")
set(OMXLIB_FOUND 1 CACHE INTERNAL "OMX")
