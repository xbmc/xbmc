aux_source_directory(. osx_SRCS)
list(REMOVE_ITEM osx_SRCS ./CoreAudioSoundManager.cpp)
add_sources(${osx_SRCS})
