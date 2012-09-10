aux_source_directory(. osx_SRCS)
list(REMOVE_ITEM osx_SRCS ./CoreAudioSoundManager.cpp)
add_library(osx STATIC ${osx_SRCS})