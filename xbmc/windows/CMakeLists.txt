aux_source_directory(. windows_SRCS)
list(REMOVE_ITEM windows_SRCS ./GUIWindowHome.cpp)
add_sources(${windows_SRCS})
