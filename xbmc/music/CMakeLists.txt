add_subdirectory(dialogs)
add_subdirectory(infoscanner)
add_subdirectory(karaoke)
add_subdirectory(tags)
add_subdirectory(windows)

aux_source_directory(. music_SRCS)
add_sources(${music_SRCS})
