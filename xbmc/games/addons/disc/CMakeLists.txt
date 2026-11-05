set(SOURCES GameClientDiscM3U.cpp
            GameClientDiscModel.cpp
            GameClientDiscMergeUtils.cpp
            GameClientDiscs.cpp
            GameClientDiscTransport.cpp
)

set(HEADERS GameClientDiscM3U.h
            GameClientDiscModel.h
            GameClientDiscState.h
            GameClientDiscMergeUtils.h
            GameClientDiscs.h
            GameClientDiscTransport.h
)

core_add_library(gamediscs)
