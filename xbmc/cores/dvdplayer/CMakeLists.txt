add_subdirectory(DVDCodecs)
add_subdirectory(DVDDemuxers)
add_subdirectory(DVDInputStreams)
add_subdirectory(DVDSubtitles)

find_all_sources(. dvdplayer_SRCS)
add_sources(${dvdplayer_SRCS})