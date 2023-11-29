set(SOURCES VideoPlayActionProcessor.cpp
            VideoSelectActionProcessor.cpp
            VideoActionProcessorHelper.cpp)

set(HEADERS VideoPlayAction.h
            VideoPlayActionProcessor.h
            VideoSelectAction.h
            VideoSelectActionProcessor.h
            VideoActionProcessorHelper.h)

core_add_library(video_guilib)
