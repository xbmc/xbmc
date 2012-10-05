find_all_sources(. osx_SRCS)
list(REMOVE_ITEM osx_SRCS ./CoreAudioSoundManager.cpp)
add_sources(${osx_SRCS})
