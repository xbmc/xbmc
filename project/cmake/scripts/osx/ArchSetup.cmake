if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for osx. See ${PROJECT_SOURCE_DIR}/README.md")
endif()

set(CORE_MAIN_SOURCE ${CORE_SOURCE_DIR}/xbmc/platform/posix/main.cpp
                     ${CORE_SOURCE_DIR}/xbmc/platform/darwin/osx/SDLMain.mm
                     ${CORE_SOURCE_DIR}/xbmc/platform/darwin/osx/SDLMain.h)

set(ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX)
set(SYSTEM_DEFINES -D_REENTRANT -D_FILE_DEFINED -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
                   -D__STDC_CONSTANT_MACROS)
set(PLATFORM_DIR linux)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL x86_64 OR CPU STREQUAL i386)
    set(ARCH x86-osx)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

find_package(CXX11 REQUIRED)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${NATIVEPREFIX})

list(APPEND DEPLIBS "-framework DiskArbitration" "-framework IOKit"
                    "-framework IOSurface" "-framework SystemConfiguration"
                    "-framework ApplicationServices" "-framework AppKit"
                    "-framework CoreAudio" "-framework AudioToolbox"
                    "-framework CoreGraphics" "-framework CoreMedia"
                    "-framework VideoToolbox")
