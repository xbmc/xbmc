# tries to find the latest OSX or iPhone SDK
# defines HAVE_OSX_SDK or HAVE_IPHONEOS_SDK
# defines OSX_SDK_VERSION or IPHONEOS_SDK_VERSION
# defines OSX_SDK_PATH IPHONEOS_SDK_PATH

if(HAVE_OSX_SDK OR HAVE_IPHONEOS_SDK)
  set(OSX_SDK_QUIET 1)
endif()

find_program(XCODE_SELECT xcode-select HINTS /usr/bin)
if(NOT XCODE_SELECT MATCHES "-NOTFOUND")
  execute_process(
    COMMAND ${XCODE_SELECT} -print-path
    OUTPUT_VARIABLE XCODE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT OSX_SDK_QUIET)
    message(STATUS "XCode resides in ${XCODE_PATH}")
  endif()
  
  find_program(XCODE_BUILD xcodebuild HINTS ${XCODE_PATH}/usr/bin)
  if(XCODE_BUILD MATCHES "NOTFOUND")
    message(FATAL_ERROR "No xcodebuild found in ${XCODE_PATH}/usr/bin")
  endif()
  
  execute_process(
    COMMAND ${XCODE_BUILD} -showsdks
    COMMAND grep macosx
    COMMAND sort
    COMMAND tail -n 1
    COMMAND grep -oE "macosx[0-9.0-9]+"
    COMMAND cut -c 7-
    OUTPUT_VARIABLE _OSX_SDK_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT _OSX_SDK_VERSION MATCHES "1[01].[0-9]+")
    message(FATAL_ERROR "Version ${_OSX_SDK_VERSION} is not parsable")
  endif()

  set(HAVE_OSX_SDK 1 CACHE BOOL "Have OSX SDK")
  set(OSX_SDK_VERSION ${_OSX_SDK_VERSION} CACHE STRING "Version of OSX SDK")
  set(OSX_SDK_PATH ${XCODE_PATH}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk)
  
  if(NOT OSX_SDK_QUIET)
    message(STATUS "Found OSX SDK version ${_OSX_SDK_VERSION} (${OSX_SDK_PATH})")
  endif()
  
  
  execute_process(
    COMMAND ${XCODE_BUILD} -showsdks
    COMMAND grep iphoneos
    COMMAND sort
    COMMAND tail -n 1
    COMMAND awk "{ print $2}"
    OUTPUT_VARIABLE _IPHONEOS_SDK_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT _IPHONEOS_SDK_VERSION MATCHES "[0-9].[0-9]+")
    message(FATAL_ERROR "Version ${_IPHONEOS_SDK_VERSION} is not parsable")
  endif()
  
  if(NOT OSX_SDK_QUIET)
    message(STATUS "Found iOS SDK version ${_IPHONEOS_SDK_VERSION}")
  endif()
  
  set(HAVE_IPHONEOS_SDK 1 CACHE BOOL "Have iPhoneOS SDK")
  set(IPHONEOS_SDK_VERISON ${_IPHONEOS_SDK_VERSION} CACHE STRING "Version of IPHONEOS SDK")
else()
  message(FATAL_ERROR "No xcode-select found!")
endif()

