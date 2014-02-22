if(TARGET_COMMON_LINUX)
  add_subdirectory(linux)
elseif(TARGET_WIN32)
  add_subdirectory(windows)
endif(TARGET_COMMON_LINUX)

find_all_sources(. input_SRCS)
if(TARGET_WIN32)
  list(REMOVE_ITEM input_SRCS ./SDLJoystick.cpp)
endif(TARGET_WIN32)

if(TARGET_RPI)
  list(REMOVE_ITEM input_SRCS ./SDLJoystick.cpp)
endif(TARGET_RPI)


add_sources(${input_SRCS})
