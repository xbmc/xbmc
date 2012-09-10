aux_source_directory(. utils_SRCS)
list(REMOVE_ITEM utils_SRCS ./WindowsShortcut.cpp)
add_library(utils STATIC ${utils_SRCS})