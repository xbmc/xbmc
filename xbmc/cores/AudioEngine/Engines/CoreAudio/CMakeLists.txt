find_all_sources(. ae_SRCS)
list(REMOVE_ITEM ae_SRCS ./CoreAudioAEHALIOS.cpp)
add_sources(${ae_SRCS})
