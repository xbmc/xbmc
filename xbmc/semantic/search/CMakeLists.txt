# Semantic Search Module
# Phase 1: Full-text search with SQLite FTS5

set(SOURCES SemanticSearch.cpp
            ContextProvider.cpp)
set(HEADERS SemanticSearch.h
            ContextProvider.h)

core_add_library(semantic_search)
