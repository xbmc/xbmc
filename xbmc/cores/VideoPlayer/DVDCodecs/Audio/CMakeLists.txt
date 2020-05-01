set(SOURCES DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

set(HEADERS DVDAudioCodec.h
            DVDAudioCodecFFmpeg.h
            DVDAudioCodecPassthrough.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES DVDAudioCodecAndroidMediaCodec.cpp)
  list(APPEND HEADERS DVDAudioCodecAndroidMediaCodec.h)
elseif(CORE_PLATFORM_NAME_LC STREQUAL tvos OR CORE_PLATFORM_NAME_LC STREQUAL ios)
  set(UNITY_EXCLUDES DVDAudioCodecFFmpeg.cpp)
endif()

core_add_library(dvdaudiocodecs)
