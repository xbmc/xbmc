add_subdirectory(DVDCodecs)
add_subdirectory(DVDDemuxers)
add_subdirectory(DVDInputStreams)
add_subdirectory(DVDSubtitles)

aux_source_directory(. dvdplayer_SRCS)
add_library(dvdplayer ${dvdplayer_SRCS})