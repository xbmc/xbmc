# Semantic Search Module
# High-level search API and vector similarity search

set(SOURCES VectorSearcher.cpp
            SemanticSearch.cpp
            ResultRanker.cpp
            HybridSearchEngine.cpp)
set(HEADERS VectorSearcher.h
            SemanticSearch.h
            ResultRanker.h
            HybridSearchEngine.h)

# No library target - these sources will be included in parent CMakeLists.txt
