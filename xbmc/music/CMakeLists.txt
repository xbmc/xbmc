add_subdirectory(dialogs)
add_subdirectory(infoscanner)
add_subdirectory(karaoke)
add_subdirectory(tags)
add_subdirectory(windows)

find_all_sources(. music_SRCS)
list(REMOVE_ITEM music_SRCS ./MusicThumbLoader.cpp)
add_sources(${music_SRCS})
