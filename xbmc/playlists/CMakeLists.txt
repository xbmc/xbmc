set(SOURCES PlayList.cpp
            PlayListASX.cpp
            PlayListB4S.cpp
            PlayListFactory.cpp
            PlayListFile.cpp
            PlayListFileItemClassify.cpp
            PlayListM3U.cpp
            PlayListPLS.cpp
            PlayListRAM.cpp
            PlayListShuffle.cpp
            PlayListURL.cpp
            PlayListWPL.cpp
            PlayListXML.cpp
            PlayListXSPF.cpp
            SmartFeed.cpp)

set(HEADERS PlayList.h
            PlayListASX.h
            PlayListB4S.h
            PlayListEntryRules.h
            PlayListFactory.h
            PlayListFeed.h
            PlayListFile.h
            PlayListFileItemClassify.h
            PlayListM3U.h
            PlayListPLS.h
            PlayListRAM.h
            PlayListShuffle.h
            PlayListTypes.h
            PlayListURL.h
            PlayListWPL.h
            PlayListXML.h
            PlayListXSPF.h
            SmartFeed.h)

core_add_library(playlists)
