set(SOURCES EpgContainer.cpp
            Epg.cpp
            EpgDatabase.cpp
            EpgInfoTag.cpp
            EpgSearchFilter.cpp
            EpgChannelData.cpp
            EpgTagsContainer.cpp)

set(HEADERS Epg.h
            EpgContainer.h
            EpgDatabase.h
            EpgInfoTag.h
            EpgSearchFilter.h
            EpgChannelData.h
            EpgTagsContainer.h)

core_add_library(pvr_epg)
