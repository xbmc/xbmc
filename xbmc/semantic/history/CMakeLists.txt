# Semantic Search History Module
# Search history persistence and autocomplete suggestions

set(SOURCES SearchHistory.cpp
            SearchSuggestions.cpp)
set(HEADERS SearchHistory.h
            SearchSuggestions.h)

# No library target - these sources will be included in parent CMakeLists.txt
