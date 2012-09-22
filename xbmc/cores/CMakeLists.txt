add_subdirectory(DllLoader)
add_subdirectory(ExternalPlayer)
add_subdirectory(dvdplayer)
add_subdirectory(paplayer)
add_subdirectory(playercorefactory)
add_subdirectory(VideoRenderers)

aux_source_directory(. core_SRCS)
add_sources(${core_SRCS})