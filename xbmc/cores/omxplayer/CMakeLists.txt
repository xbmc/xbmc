set(SOURCES OMXAudio.cpp
            OMXAudioCodecOMX.cpp
            OMXHelper.cpp
            OMXImage.cpp
            OMXPlayerAudio.cpp
            OMXPlayerVideo.cpp
            OMXVideo.cpp)

set(HEADERS OMXAudio.h
            OMXAudioCodec.h
            OMXAudioCodecOMX.h
            OMXHelper.h
            OMXImage.h
            OMXPlayerAudio.h
            OMXPlayerVideo.h
            OMXVideo.h
            OMXVideoCodec.h)

core_add_library(omxplayer)
target_compile_definitions(${CORE_LIBRARY} PRIVATE -D__STDC_FORMAT_MACROS)
