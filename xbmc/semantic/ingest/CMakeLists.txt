set(SOURCES
    ChunkProcessor.cpp
    ContentParserBase.cpp
    MetadataParser.cpp
    SubtitleParser.cpp
)

set(HEADERS
    ChunkProcessor.h
    ContentParserBase.h
    IContentParser.h
    MetadataParser.h
    SubtitleParser.h
)

core_add_library(semantic_ingest)

if(ENABLE_INTERNAL_GTEST)
  add_subdirectory(test)
endif()
