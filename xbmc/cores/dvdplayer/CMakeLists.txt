add_subdirectory(DVDCodecs)
add_subdirectory(DVDDemuxers)
add_subdirectory(DVDInputStreams)
add_subdirectory(DVDSubtitles)

find_all_sources(. dvdplayer_SRCS)
list(REMOVE_ITEM dvdplayer_SRCS ./DVDMessageTracker.cpp)
add_sources(${dvdplayer_SRCS})