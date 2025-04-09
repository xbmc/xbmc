set(SOURCES VideoActionProcessorBase.cpp
            VideoGUIUtils.cpp
            VideoPlayActionProcessor.cpp
            VideoSelectActionProcessor.cpp
            VideoStreamSelectHelper.cpp)

set(HEADERS VideoAction.h
            VideoActionProcessorBase.h
            VideoGUIUtils.h
            VideoPlayActionProcessor.h
            VideoSelectActionProcessor.h
            VideoStreamSelectHelper.h)

core_add_library(video_guilib)
