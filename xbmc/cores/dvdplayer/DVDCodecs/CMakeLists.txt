add_subdirectory(Audio)
add_subdirectory(Overlay)
add_subdirectory(Video)

aux_source_directory(. DVDCodecs_SRCS)
add_sources(${DVDCodecs_SRCS})