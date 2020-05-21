set(SOURCES EpgContainer.cpp
            Epg.cpp
            EpgDatabase.cpp
            EpgInfoTag.cpp
            EpgSearchData.cpp
            EpgSearchFilter.cpp
            EpgChannelData.cpp
            EpgTagsCache.cpp
            EpgTagsContainer.cpp)

set(HEADERS Epg.h
            EpgContainer.h
            EpgDatabase.h
            EpgInfoTag.h
            EpgSearchData.h
            EpgSearchFilter.h
            EpgChannelData.h
            EpgTagsCache.h
            EpgTagsContainer.h)

core_add_library(pvr_epg)
