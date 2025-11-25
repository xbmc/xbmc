# Semantic Query Expansion Module
# Query expansion strategies for improved search recall

set(SOURCES QueryExpander.cpp
            SynonymDatabase.cpp)
set(HEADERS QueryExpander.h
            SynonymDatabase.h)

# No library target - these sources will be included in parent CMakeLists.txt
