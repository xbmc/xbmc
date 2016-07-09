set(SOURCES EpgContainer.cpp
            Epg.cpp
            EpgDatabase.cpp
            EpgInfoTag.cpp
            EpgSearchFilter.cpp
            GUIEPGGridContainer.cpp
            GUIEPGGridContainerModel.cpp)

set(HEADERS Epg.h
            EpgContainer.h
            EpgDatabase.h
            EpgInfoTag.h
            EpgSearchFilter.h
            GUIEPGGridContainer.h
            GUIEPGGridContainerModel.h)

core_add_library(epg)
