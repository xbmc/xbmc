set(SOURCES SetInfoTagLoaderFactory.cpp
            SetTagLoaderNFO.cpp
            VideoTagExtractionHelper.cpp
            VideoInfoTagLoaderFactory.cpp
            VideoTagLoaderFFmpeg.cpp
            VideoTagLoaderNFO.cpp
            VideoTagLoaderPlugin.cpp)

set(HEADERS ISetInfoTagLoader.h
            IVideoInfoTagLoader.h
            SetInfoTagLoaderFactory.h
            SetTagLoaderNFO.h
            VideoTagExtractionHelper.h
            VideoInfoTagLoaderFactory.h
            VideoTagLoaderFFmpeg.h
            VideoTagLoaderNFO.h
            VideoTagLoaderPlugin.h)

core_add_library(video_tags)
