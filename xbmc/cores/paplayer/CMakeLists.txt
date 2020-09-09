set(SOURCES AudioDecoder.cpp
            CodecFactory.cpp
            PAPlayer.cpp
            VideoPlayerCodec.cpp)

set(HEADERS AudioDecoder.h
            CachingCodec.h
            CodecFactory.h
            ICodec.h
            PAPlayer.h
            VideoPlayerCodec.h)

set(UNITY_EXCLUDES CodecFactory.cpp)

core_add_library(paplayer)
