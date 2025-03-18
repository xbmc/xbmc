set(SOURCES VideoActionProcessorBase.cpp
            VideoGUIUtils.cpp
            VideoPlayActionProcessor.cpp
            VideoSelectActionProcessor.cpp
            VideoStreamSelectHelper.cpp
            VideoVersionHelper.cpp)

set(HEADERS VideoAction.h
            VideoActionProcessorBase.h
            VideoGUIUtils.h
            VideoPlayActionProcessor.h
            VideoSelectActionProcessor.h
            VideoStreamSelectHelper.h
            VideoVersionHelper.h)

core_add_library(video_guilib)
