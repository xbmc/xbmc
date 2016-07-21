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

core_add_library(paplayer)
