set(SOURCES CDDARipJob.cpp
            Encoder.cpp
            EncoderFFmpeg.cpp)

set(HEADERS CDDARipJob.h
            Encoder.h
            EncoderFFmpeg.h
            IEncoder.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES CDDARipper.cpp)
  list(APPEND HEADERS CDDARipper.h)
endif()

core_add_library(cdrip)
