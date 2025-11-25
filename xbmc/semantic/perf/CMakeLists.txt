# Semantic Search Performance Module
# Performance monitoring, caching, and memory management

set(SOURCES PerformanceMonitor.cpp
            QueryCache.cpp
            MemoryManager.cpp)
set(HEADERS PerformanceMonitor.h
            QueryCache.h
            MemoryManager.h)

# No library target - these sources will be included in parent CMakeLists.txt
