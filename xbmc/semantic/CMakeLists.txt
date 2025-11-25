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

# Core semantic components
add_subdirectory(embedding)
add_subdirectory(transcription)
add_subdirectory(ingest)
add_subdirectory(search)

# GUI components
add_subdirectory(dialogs)
add_subdirectory(filters)
add_subdirectory(history)
add_subdirectory(voice)

# Performance monitoring
add_subdirectory(perf)

# Tests
if(ENABLE_INTERNAL_GTEST)
  add_subdirectory(test)
endif()
