set(SOURCES VideoDatabase.cpp
            VideoDatabaseArt.cpp
            VideoDatabaseBookmarks.cpp
            VideoDatabaseEpisodes.cpp
            VideoDatabaseExportImport.cpp
            VideoDatabaseMovies.cpp
            VideoDatabaseMusicVideos.cpp
            VideoDatabasePlayCount.cpp
            VideoDatabaseRatings.cpp
            VideoDatabaseSeasons.cpp
            VideoDatabaseSets.cpp
            VideoDatabaseSettings.cpp
            VideoDatabaseStreamDetails.cpp
            VideoDatabaseTags.cpp
            VideoDatabaseTvShows.cpp)

set(HEADERS VideoDatabase.h)

core_add_library(video_database)
