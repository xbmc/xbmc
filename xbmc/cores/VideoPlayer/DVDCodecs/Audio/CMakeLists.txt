set(SOURCES DVDAudioCodecFFmpeg.cpp
            DVDAudioCodecPassthrough.cpp)

set(HEADERS DVDAudioCodec.h
            DVDAudioCodecFFmpeg.h
            DVDAudioCodecPassthrough.h)

core_add_library(dvdaudiocodecs)
