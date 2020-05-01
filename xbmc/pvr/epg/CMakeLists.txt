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

set(UNITY_EXCLUDES EpgTagsContainer.cpp)
if(CORE_PLATFORM_NAME_LC STREQUAL tvos OR CORE_PLATFORM_NAME_LC STREQUAL ios)
  list(APPEND UNITY_EXCLUDES EpgInfoTag.cpp
                             EpgDatabase.cpp)
endif()


core_add_library(pvr_epg)
