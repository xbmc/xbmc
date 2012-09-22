add_subdirectory(dialogs)
add_subdirectory(windows)

aux_source_directory(. video_SRCS)
add_sources(${video_SRCS})
