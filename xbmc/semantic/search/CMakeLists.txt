# Semantic Search Module
# High-level search API and vector similarity search

set(SOURCES VectorSearcher.cpp
            SemanticSearch.cpp
            ResultRanker.cpp
            HybridSearchEngine.cpp
            ContextProvider.cpp
            ResultEnricher.cpp)
set(HEADERS VectorSearcher.h
            SemanticSearch.h
            ResultRanker.h
            HybridSearchEngine.h
            ContextProvider.h
            ResultEnricher.h)

# No library target - these sources will be included in parent CMakeLists.txt
