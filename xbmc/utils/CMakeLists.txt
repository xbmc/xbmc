find_all_sources(. utils_SRCS)

list(REMOVE_ITEM utils_SRCS ./WindowsShortcut.cpp)
if(WIN32)
  list(REMOVE_ITEM utils_SRCS ./CryptThreading.cpp)
  list(REMOVE_ITEM utils_SRCS ./GLUtils.cpp)
  list(REMOVE_ITEM utils_SRCS ./LCDFactory.cpp)
endif()
add_sources(${utils_SRCS})
