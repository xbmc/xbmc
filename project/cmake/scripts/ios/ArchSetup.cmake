if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for ios. See ${PROJECT_SOURCE_DIR}/README.md")
endif()

set(CORE_MAIN_SOURCE ${CORE_SOURCE_DIR}/xbmc/platform/darwin/ios/XBMCApplication.m)

set(ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_IOS)
set(SYSTEM_DEFINES -D_REENTRANT -D_FILE_DEFINED -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
                   -D__STDC_CONSTANT_MACROS)
set(PLATFORM_DIR linux)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL armv7 OR CPU STREQUAL arm64)
    set(ARCH arm-osx)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

find_package(CXX11 REQUIRED)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${NATIVEPREFIX})

list(APPEND DEPLIBS "-framework CoreFoundation" "-framework CoreVideo"
                    "-framework CoreAudio" "-framework AudioToolbox"
                    "-framework QuartzCore" "-framework MediaPlayer"
                    "-framework CFNetwork" "-framework CoreGraphics"
                    "-framework Foundation" "-framework UIKit"
                    "-framework CoreMedia" "-framework AVFoundation"
                    "-framework VideoToolbox")

set(ENABLE_DVDCSS OFF)
set(ENABLE_OPTICAL OFF)
