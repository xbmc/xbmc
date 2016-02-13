set(SOURCES DirectoryNode.cpp
            DirectoryNodeEpisodes.cpp
            DirectoryNodeGrouped.cpp
            DirectoryNodeInProgressTvShows.cpp
            DirectoryNodeMoviesOverview.cpp
            DirectoryNodeMusicVideosOverview.cpp
            DirectoryNodeOverview.cpp
            DirectoryNodeRecentlyAddedEpisodes.cpp
            DirectoryNodeRecentlyAddedMovies.cpp
            DirectoryNodeRecentlyAddedMusicVideos.cpp
            DirectoryNodeRoot.cpp
            DirectoryNodeSeasons.cpp
            DirectoryNodeTitleMovies.cpp
            DirectoryNodeTitleMusicVideos.cpp
            DirectoryNodeTitleTvShows.cpp
            DirectoryNodeTvShowsOverview.cpp
            QueryParams.cpp)

set(HEADERS DirectoryNode.h
            DirectoryNodeEpisodes.h
            DirectoryNodeGrouped.h
            DirectoryNodeInProgressTvShows.h
            DirectoryNodeMoviesOverview.h
            DirectoryNodeMusicVideosOverview.h
            DirectoryNodeOverview.h
            DirectoryNodeRecentlyAddedEpisodes.h
            DirectoryNodeRecentlyAddedMovies.h
            DirectoryNodeRecentlyAddedMusicVideos.h
            DirectoryNodeRoot.h
            DirectoryNodeSeasons.h
            DirectoryNodeTitleMovies.h
            DirectoryNodeTitleMusicVideos.h
            DirectoryNodeTitleTvShows.h
            DirectoryNodeTvShowsOverview.h
            QueryParams.h)

core_add_library(videodatabasedirectory)
