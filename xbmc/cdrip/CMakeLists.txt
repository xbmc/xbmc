find_all_sources(. cdrip_SRCS)
list(REMOVE_ITEM cdrip_SRCS ./EncoderFFmpeg.cpp)
list(REMOVE_ITEM cdrip_SRCS ./EncoderFFmpeg.h)
add_sources(${cdrip_SRCS})
