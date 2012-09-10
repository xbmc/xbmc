aux_source_directory(. exports_SRCS)
aux_source_directory(util exports_util_SRCS)
list(REMOVE_ITEM exports_SRCS ./win32-dirent.cpp)
add_library(exports STATIC ${exports_SRCS} ${exports_util_SRCS})
