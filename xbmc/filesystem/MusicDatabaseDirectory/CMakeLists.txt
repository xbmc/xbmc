set(SOURCES DirectoryNodeAlbum.cpp
            DirectoryNodeAlbumRecentlyAdded.cpp
            DirectoryNodeAlbumRecentlyAddedSong.cpp
            DirectoryNodeAlbumRecentlyPlayed.cpp
            DirectoryNodeAlbumRecentlyPlayedSong.cpp
            DirectoryNodeAlbumTop100.cpp
            DirectoryNodeAlbumTop100Song.cpp
            DirectoryNodeArtist.cpp
            DirectoryNodeDiscs.cpp
            DirectoryNode.cpp
            DirectoryNodeGrouped.cpp
            DirectoryNodeOverview.cpp
            DirectoryNodeRoot.cpp
            DirectoryNodeSingles.cpp
            DirectoryNodeSong.cpp
            DirectoryNodeSongTop100.cpp
            DirectoryNodeTop100.cpp           
            QueryParams.cpp)

set(HEADERS DirectoryNode.h
            DirectoryNodeAlbum.h
            DirectoryNodeAlbumRecentlyAdded.h
            DirectoryNodeAlbumRecentlyAddedSong.h
            DirectoryNodeAlbumRecentlyPlayed.h
            DirectoryNodeAlbumRecentlyPlayedSong.h
            DirectoryNodeAlbumTop100.h
            DirectoryNodeAlbumTop100Song.h
            DirectoryNodeArtist.h
            DirectoryNodeDiscs.h
            DirectoryNodeGrouped.h
            DirectoryNodeOverview.h
            DirectoryNodeRoot.h
            DirectoryNodeSingles.h
            DirectoryNodeSong.h
            DirectoryNodeSongTop100.h
            DirectoryNodeTop100.h
            QueryParams.h)

core_add_library(musicdatabasedirectory)
