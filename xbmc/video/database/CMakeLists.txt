set(SOURCES VideoDatabase.cpp
            VideoDatabaseBookmarks.cpp
            VideoDatabaseExportImport.cpp
            VideoDatabaseMovies.cpp
            VideoDatabaseMusicVideos.cpp
            VideoDatabasePlayCount.cpp
            VideoDatabaseRatings.cpp
            VideoDatabaseSeasons.cpp
            VideoDatabaseSets.cpp
            VideoDatabaseSettings.cpp
            VideoDatabaseTags.cpp)

set(HEADERS VideoDatabase.h)

core_add_library(video_database)
