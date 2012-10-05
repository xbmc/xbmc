add_subdirectory(dialogs)
add_subdirectory(infoscanner)
add_subdirectory(karaoke)
add_subdirectory(tags)
add_subdirectory(windows)

find_all_sources(. music_SRCS)
add_sources(${music_SRCS})
