add_subdirectory(addons)
add_subdirectory(channels)
add_subdirectory(dialogs)
add_subdirectory(recordings)
add_subdirectory(timers)
add_subdirectory(windows)

find_all_sources(. pvr_SRCS)
add_sources(${pvr_SRCS})
