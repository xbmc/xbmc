add_subdirectory(dialogs)
add_subdirectory(windows)

find_all_sources(. video_SRCS)
add_sources(${video_SRCS})
