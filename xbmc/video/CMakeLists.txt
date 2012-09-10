add_subdirectory(dialogs)
add_subdirectory(windows)

aux_source_directory(. video_SRCS)
add_library(video STATIC ${video_SRCS})