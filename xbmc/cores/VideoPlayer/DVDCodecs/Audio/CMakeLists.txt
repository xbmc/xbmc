set(SOURCES DVDAudioCodecAndroidMediaCodec.cpp
            DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

set(HEADERS DVDAudioCodec.h
            DVDAudioCodecAndroidMediaCodec.h
            DVDAudioCodecFFmpeg.h
            DVDAudioCodecPassthrough.h)

core_add_library(dvdaudiocodecs)
