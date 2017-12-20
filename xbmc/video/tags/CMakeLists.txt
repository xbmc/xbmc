set(SOURCES VideoInfoTagLoaderFactory.cpp
            VideoTagLoaderFFmpeg.cpp
            VideoTagLoaderNFO.cpp)

set(HEADERS IVideoInfoTagLoader.h
            VideoInfoTagLoaderFactory.h
            VideoTagLoaderFFmpeg.h
            VideoTagLoaderNFO.h)

core_add_library(video_tags)
