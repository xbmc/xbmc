# Semantic Search Module
# High-level search API and vector similarity search

set(SOURCES VectorSearcher.cpp
            SemanticSearch.cpp)
set(HEADERS VectorSearcher.h
            SemanticSearch.h)

# No library target - these sources will be included in parent CMakeLists.txt
