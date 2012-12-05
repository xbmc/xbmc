SRCS=Album.cpp \
     Artist.cpp \
     GUIViewStateMusic.cpp \
     LastFmManager.cpp \
     MusicDatabase.cpp \
     MusicDbUrl.cpp \
     MusicInfoLoader.cpp \
     MusicThumbLoader.cpp \
     Song.cpp \
     spotyXBMC/Addon.music.spotify.cpp \
     spotyXBMC/Logger.cpp \
     spotyXBMC/Utils.cpp \
     spotyXBMC/SxSettings.cpp \
     spotyXBMC/radio/SxRadio.cpp \
     spotyXBMC/radio/RadioHandler.cpp \
     spotyXBMC/radio/RadioBackgroundLoader.cpp \
     spotyXBMC/album/SxAlbum.cpp \
     spotyXBMC/album/AlbumStore.cpp \
     spotyXBMC/album/AlbumContainer.cpp \
     spotyXBMC/artist/SxArtist.cpp \
     spotyXBMC/artist/ArtistStore.cpp \
     spotyXBMC/artist/ArtistContainer.cpp \
     spotyXBMC/search/Search.cpp \
     spotyXBMC/search/SearchHandler.cpp \
     spotyXBMC/search/SearchResultBackgroundLoader.cpp \
     spotyXBMC/player/Codec.cpp \
     spotyXBMC/player/PlayerHandler.cpp \
     spotyXBMC/playlist/TopLists.cpp \
     spotyXBMC/playlist/SxPlaylist.cpp \
     spotyXBMC/playlist/StarredList.cpp \
     spotyXBMC/playlist/PlaylistStore.cpp \
     spotyXBMC/playlist/StarredBackgroundLoader.cpp \
     spotyXBMC/session/Session.cpp \
     spotyXBMC/session/BackgroundThread.cpp \
     spotyXBMC/session/SessionCallbacks.cpp \
     spotyXBMC/thumb/SxThumb.cpp \
     spotyXBMC/thumb/ThumbStore.cpp \
     spotyXBMC/track/SxTrack.cpp \
     spotyXBMC/track/TrackStore.cpp \
     spotyXBMC/track/TrackContainer.cpp

LIB=music.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
