set(SOURCES AddonVideoCodec.cpp
            DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp)

set(HEADERS AddonVideoCodec.h
            DVDVideoCodec.h
            DVDVideoCodecFFmpeg.h)

if(NOT ENABLE_EXTERNAL_LIBAV)
  list(APPEND SOURCES DVDVideoPPFFmpeg.cpp)
  list(APPEND HEADERS DVDVideoPPFFmpeg.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES DXVA.cpp)
  list(APPEND HEADERS DXVA.h)
endif()

if(TARGET VDPAU::VDPAU)
  list(APPEND SOURCES VDPAU.cpp)
  list(APPEND HEADERS VDPAU.h)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES VAAPI.cpp)
  list(APPEND HEADERS VAAPI.h)
endif()

if(APPLE)
  list(APPEND SOURCES VTB.cpp)
  list(APPEND HEADERS VTB.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES DVDVideoCodecAndroidMediaCodec.cpp)
  list(APPEND HEADERS DVDVideoCodecAndroidMediaCodec.h)
endif()

if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES DVDVideoCodecDRMPRIME.cpp)
  list(APPEND HEADERS DVDVideoCodecDRMPRIME.h)
endif()

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES DVDVideoCodecStarfish.cpp)
  list(APPEND HEADERS DVDVideoCodecStarfish.h)
endif()

core_add_library(dvdvideocodecs)
