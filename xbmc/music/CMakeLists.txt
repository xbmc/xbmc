set(SOURCES Album.cpp
            Artist.cpp
            CueInfoLoader.cpp
            GUIViewStateMusic.cpp
            MusicDatabase.cpp
            MusicDbUrl.cpp
            MusicInfoLoader.cpp
            MusicThumbLoader.cpp
            Song.cpp)

set(HEADERS Album.h
            Artist.h
            CueInfoLoader.h
            EmbeddedArt.h
            GUIViewStateMusic.h
            MusicDatabase.h
            MusicDbUrl.h
            MusicInfoLoader.h
            MusicThumbLoader.h
            Song.h)

core_add_library(music)
add_dependencies(music libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(music ffmpeg)
endif()
