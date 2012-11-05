find_all_sources(. osx_SRCS)
list(REMOVE_ITEM osx_SRCS ./CoreAudioSoundManager.cpp)
list(REMOVE_ITEM osx_SRCS ./IOSEAGLView.mm ./IOSExternalTouchController.mm ./IOSScreenManager.mm)
add_sources(${osx_SRCS})
