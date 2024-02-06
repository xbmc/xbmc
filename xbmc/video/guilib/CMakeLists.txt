set(SOURCES VideoPlayActionProcessor.cpp
            VideoSelectActionProcessor.cpp
            VideoAssetHelper.cpp)

set(HEADERS VideoAction.h
            VideoPlayActionProcessor.h
            VideoSelectActionProcessor.h
            VideoAssetHelper.h)

core_add_library(video_guilib)
