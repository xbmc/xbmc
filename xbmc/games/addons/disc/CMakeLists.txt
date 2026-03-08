set(SOURCES GameClientDiscM3U.cpp
            GameClientDiscPlaylist.cpp
            GameClientDiscModel.cpp
            GameClientDiscMergeUtils.cpp
            GameClientDiscs.cpp
            GameClientDiscTransport.cpp
            GameClientDiscXML.cpp
)

set(HEADERS GameClientDiscM3U.h
            GameClientDiscPlaylist.h
            GameClientDiscModel.h
            GameClientDiscMergeUtils.h
            GameClientDiscs.h
            GameClientDiscTransport.h
            GameClientDiscXML.h
)

core_add_library(gamediscs)
