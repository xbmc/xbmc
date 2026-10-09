set(SOURCES PlayList.cpp
            PlayListASX.cpp
            PlayListB4S.cpp
            PlayListFactory.cpp
            PlayListFileItemClassify.cpp
            PlayListM3U.cpp
            PlayListPLS.cpp
            PlayListRAM.cpp
            PlayListURL.cpp
            PlayListWPL.cpp
            PlayListXML.cpp
            PlayListXSPF.cpp
            SmartFeed.cpp
            SmartPlayList.cpp
            SmartPlaylistFileItemListModifier.cpp)

set(HEADERS PlayList.h
            PlayListASX.h
            PlayListB4S.h
            PlayListFactory.h
            PlayListFeed.h
            PlayListFileItemClassify.h
            PlayListM3U.h
            PlayListPLS.h
            PlayListRAM.h
            PlayListTypes.h
            PlayListURL.h
            PlayListWPL.h
            PlayListXML.h
            PlayListXSPF.h
            SmartFeed.h
            SmartPlayList.h
            SmartPlaylistFileItemListModifier.h)

core_add_library(playlists)
