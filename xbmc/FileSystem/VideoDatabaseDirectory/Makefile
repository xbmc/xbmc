INCLUDES=-I. -I../../ -I../../linux -I../../../guilib

SRCS=DirectoryNodeActor.cpp \
     DirectoryNode.cpp \
     DirectoryNodeDirector.cpp \
     DirectoryNodeEpisodes.cpp \
     DirectoryNodeGenre.cpp \
     DirectoryNodeCountry.cpp \
     DirectoryNodeSets.cpp \
     DirectoryNodeMoviesOverview.cpp \
     DirectoryNodeOverview.cpp \
     DirectoryNodeRoot.cpp \
     DirectoryNodeSeasons.cpp \
     DirectoryNodeTitleMovies.cpp \
     DirectoryNodeTitleTvShows.cpp \
     DirectoryNodeTvShowsOverview.cpp \
     DirectoryNodeYear.cpp \
     QueryParams.cpp \
     DirectoryNodeRecentlyAddedMovies.cpp \
     DirectoryNodeRecentlyAddedEpisodes.cpp \
     DirectoryNodeStudio.cpp \
     DirectoryNodeMusicVideosOverview.cpp \
     DirectoryNodeRecentlyAddedMusicVideos.cpp \
     DirectoryNodeTitleMusicVideos.cpp \
     DirectoryNodeMusicVideoAlbum.cpp \

LIB=videodatabasedirectory.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
