add_subdirectory(exports)

aux_source_directory(. dllloader_SRCS)
list(REMOVE_ITEM dllloader_SRCS ./DllLoader-linux.cpp)
list(REMOVE_ITEM dllloader_SRCS ./Win32DllLoader.cpp)
add_sources(${dllloader_SRCS})
