set(SOURCES
    SemanticDatabase.cpp
    SemanticIndexService.cpp
)

set(HEADERS
    SemanticDatabase.h
    SemanticIndexService.h
    SemanticTypes.h
)

core_add_library(semantic)

# Core semantic components (Phase 1: Content Text Index)
add_subdirectory(transcription)
add_subdirectory(ingest)
add_subdirectory(search)

# Tests
if(ENABLE_INTERNAL_GTEST)
  add_subdirectory(test)
endif()
