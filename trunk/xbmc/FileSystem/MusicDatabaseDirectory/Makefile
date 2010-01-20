INCLUDES=-I. -I../../ -I../../linux -I../../../guilib

SRCS=DirectoryNodeAlbumCompilations.cpp DirectoryNodeAlbumCompilationsSongs.cpp DirectoryNodeAlbum.cpp DirectoryNodeAlbumRecentlyAdded.cpp DirectoryNodeAlbumRecentlyAddedSong.cpp DirectoryNodeAlbumRecentlyPlayed.cpp DirectoryNodeAlbumRecentlyPlayedSong.cpp DirectoryNodeAlbumTop100.cpp DirectoryNodeAlbumTop100Song.cpp DirectoryNodeArtist.cpp DirectoryNode.cpp DirectoryNodeGenre.cpp DirectoryNodeOverview.cpp DirectoryNodeRoot.cpp DirectoryNodeSong.cpp DirectoryNodeSongTop100.cpp DirectoryNodeTop100.cpp QueryParams.cpp DirectoryNodeYearAlbum.cpp DirectoryNodeYearSong.cpp DirectoryNodeYear.cpp DirectoryNodeSingles.cpp

LIB=musicdatabasedirectory.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
