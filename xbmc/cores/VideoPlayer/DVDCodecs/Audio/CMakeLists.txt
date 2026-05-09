set(SOURCES AddonAudioCodec.cpp
            DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

set(HEADERS AddonAudioCodec.h
            DVDAudioCodec.h
            DVDAudioCodecFFmpeg.h
            DVDAudioCodecPassthrough.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES DVDAudioCodecAndroidMediaCodec.cpp)
  list(APPEND HEADERS DVDAudioCodecAndroidMediaCodec.h)
endif()

core_add_library(dvdaudiocodecs)
