set(SOURCES PlayListB4S.cpp
            PlayList.cpp
            PlayListFactory.cpp
            PlayListM3U.cpp
            PlayListPLS.cpp
            PlayListURL.cpp
            PlayListWPL.cpp
            PlayListXML.cpp
            PlayListXSPF.cpp
            SmartPlayList.cpp
            SmartPlaylistFileItemListModifier.cpp)

set(HEADERS PlayList.h
            PlayListB4S.h
            PlayListFactory.h
            PlayListM3U.h
            PlayListPLS.h
            PlayListTypes.h
            PlayListURL.h
            PlayListWPL.h
            PlayListXML.h
            PlayListXSPF.h
            SmartPlayList.h
            SmartPlaylistFileItemListModifier.h)

core_add_library(playlists)
