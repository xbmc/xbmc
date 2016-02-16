if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for darwin. See ${PROJECT_SOURCE_DIR}/README.md")
endif()

set(ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_DARWIN)
if(CORE_SYSTEM_NAME STREQUAL "darwin")
  list(APPEND ARCH_DEFINES -DTARGET_DARWIN_OSX)
elseif(CORE_SYSTEM_NAME STREQUAL "ios")
  list(APPEND ARCH_DEFINES -DTARGET_DARWIN_IOS)
endif()

set(SYSTEM_DEFINES -D_REENTRANT -D_FILE_DEFINED -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
                   -D__STDC_CONSTANT_MACROS)
set(PLATFORM_DIR linux)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL "x86_64")
    set(ARCH x86_64-apple-darwin)
  elseif(CPU MATCHES "i386")
    set(ARCH i386-apple-darwin)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

find_package(CXX11 REQUIRED)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${NATIVEPREFIX})

list(APPEND DEPLIBS "-framework DiskArbitration" "-framework IOKit"
                    "-framework IOSurface" "-framework SystemConfiguration"
                    "-framework ApplicationServices" "-framework AppKit"
                    "-framework CoreAudio" "-framework AudioToolbox"
                    "-framework CoreGraphics")
