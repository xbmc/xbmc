set(SOURCES Encoder.cpp
            EncoderAddon.cpp
            EncoderFFmpeg.cpp)

set(HEADERS Encoder.h
            EncoderAddon.h
            EncoderFFmpeg.h
            IEncoder.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES CDDARipJob.cpp
                      CDDARipper.cpp)
  list(APPEND HEADERS CDDARipJob.h
                      CDDARipper.h)
endif()

core_add_library(cdrip)
