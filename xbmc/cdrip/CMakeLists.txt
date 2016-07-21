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
