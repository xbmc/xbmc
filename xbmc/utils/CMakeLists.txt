find_all_sources(. utils_SRCS)
list(REMOVE_ITEM utils_SRCS ./WindowsShortcut.cpp)
add_sources(${utils_SRCS})
