if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for osx. See ${CMAKE_SOURCE_DIR}/cmake/README.md")
endif()

list(APPEND CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/osx/XBMCApplication.h)
set(PLATFORM_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/${CORE_PLATFORM_NAME_LC}/Info.plist.in)

set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX)
list(APPEND SYSTEM_DEFINES -D_REENTRANT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
                           -D__STDC_CONSTANT_MACROS)
set(PLATFORM_DIR platform/darwin)
set(PLATFORMDEFS_DIR platform/posix)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL x86_64 OR CPU STREQUAL i386)
    set(ARCH x86-osx)
    set(NEON False)
  elseif(CPU STREQUAL arm64)
    set(ARCH aarch64)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

# xbmchelper (old apple IR remotes) only make sense for x86
# last macs featuring the IR receiver are those of mid 2012
# which are still able to run Mojave (10.14). Drop all together
# when the sdk requirement is bumped.
if(CPU STREQUAL arm64)
  set(ENABLE_XBMCHELPER OFF)
else()
  set(ENABLE_XBMCHELPER ON)
  list(APPEND SYSTEM_DEFINES -DHAS_XBMCHELPER)
endif()

# m1 macs can execute x86_64 code via rosetta
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" AND
   CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(HOST_CAN_EXECUTE_TARGET TRUE)
endif()

set(CMAKE_OSX_ARCHITECTURES ${CPU})

# Additional SYSTEM_DEFINES
list(APPEND SYSTEM_DEFINES -DHAS_POSIX_NETWORK -DHAS_OSX_NETWORK -DHAS_ZEROCONF)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${NATIVEPREFIX})

list(APPEND DEPLIBS "-framework DiskArbitration" "-framework IOKit"
                    "-framework IOSurface" "-framework SystemConfiguration"
                    "-framework ApplicationServices" "-framework AppKit"
                    "-framework CoreAudio" "-framework AudioToolbox"
                    "-framework CoreGraphics" "-framework CoreMedia"
                    "-framework VideoToolbox" "-framework Security"
                    "-framework GameController" "-framework Speech"
                    "-framework AVFoundation")

if(ARCH STREQUAL aarch64)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
else()
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
endif()
set(CMAKE_XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME OFF)

include(cmake/scripts/darwin/Macros.cmake)
enable_arc()
