aux_source_directory(. overlay_SRCS)
aux_source_directory(libspucc supcc_SRCS)
add_library(dvdplayer_dvdcodecs_overlay STATIC ${overlay_SRCS} ${supcc_SRCS})