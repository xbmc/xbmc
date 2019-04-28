if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for ios/tvos. See ${CMAKE_SOURCE_DIR}/cmake/README.md")
endif()


if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/MainApplication.mm)
  set(ARCH_DEFINES -DTARGET_DARWIN_TVOS)
  set(PLATFORM_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/Info.plist.in)
else()
  set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/ios/XBMCApplication.mm)
  set(ARCH_DEFINES -DTARGET_DARWIN_IOS)
  set(PLATFORM_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/ios/Info.plist.in)
endif()
list(APPEND ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_EMBEDDED)
set(SYSTEM_DEFINES -D_REENTRANT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
                   -D__STDC_CONSTANT_MACROS)
set(PLATFORM_DIR platform/linux)
set(CMAKE_SYSTEM_NAME Darwin)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL armv7)
    set(CMAKE_OSX_ARCHITECTURES ${CPU})
    set(ARCH arm)
    set(NEON True)
  elseif(CPU STREQUAL arm64)
    set(CMAKE_OSX_ARCHITECTURES ${CPU})
    set(ARCH aarch64)
    set(NEON True)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

# Additional SYSTEM_DEFINES
list(APPEND SYSTEM_DEFINES -DHAS_LINUX_NETWORK -DHAS_ZEROCONF)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${NATIVEPREFIX})

list(APPEND DEPLIBS "-framework CoreFoundation" "-framework CoreVideo"
                    "-framework CoreAudio" "-framework AudioToolbox"
                    "-framework QuartzCore" "-framework MediaPlayer"
                    "-framework CFNetwork" "-framework CoreGraphics"
                    "-framework Foundation" "-framework UIKit"
                    "-framework CoreMedia" "-framework AVFoundation"
                    "-framework VideoToolbox")

set(ENABLE_OPTICAL OFF CACHE BOOL "" FORCE)

set(CMAKE_XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET "11.0")
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "9.0")
if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3")
else()
  set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
endif()

set(CMAKE_XCODE_ATTRIBUTE_INLINES_ARE_PRIVATE_EXTERN OFF)
set(CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN OFF)
set(CMAKE_XCODE_ATTRIBUTE_COPY_PHASE_STRIP OFF)

# Xcode strips dead code by default which breaks wrapping
set(CMAKE_XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING OFF)

# Unify output directories for iOS/tvOS packaging scripts
if(NOT CMAKE_GENERATOR MATCHES Xcode)
  if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
    set(CORE_BUILD_CONFIG "${CORE_BUILD_CONFIG}-appletvos")
  else()
    set(CORE_BUILD_CONFIG "${CORE_BUILD_CONFIG}-iphoneos")
  endif()
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CORE_BUILD_DIR}/${CORE_BUILD_CONFIG})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CORE_BUILD_DIR}/${CORE_BUILD_CONFIG})
endforeach()
