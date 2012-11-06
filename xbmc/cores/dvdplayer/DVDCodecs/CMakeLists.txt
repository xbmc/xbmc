add_subdirectory(Audio)
add_subdirectory(Overlay)
add_subdirectory(Video)

find_all_sources(. DVDCodecs_SRCS)
add_sources(${DVDCodecs_SRCS})