set(SOURCES CDDARipJob.cpp
            CDDARipper.cpp
            Encoder.cpp
            EncoderFFmpeg.cpp)

set(HEADERS CDDARipJob.h
            CDDARipper.h
            Encoder.h
            EncoderFFmpeg.h
            IEncoder.h)

core_add_library(cdrip)
add_dependencies(cdrip libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(cdrip ffmpeg)
endif()
