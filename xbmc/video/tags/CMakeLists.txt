set(SOURCES VideoInfoTagLoaderFactory.cpp
            VideoTagLoaderFFmpeg.cpp
            VideoTagLoaderNFO.cpp
            VideoTagLoaderPlugin.cpp)

set(HEADERS IVideoInfoTagLoader.h
            VideoInfoTagLoaderFactory.h
            VideoTagLoaderFFmpeg.h
            VideoTagLoaderNFO.h
            VideoTagLoaderPlugin.h)

core_add_library(video_tags)
