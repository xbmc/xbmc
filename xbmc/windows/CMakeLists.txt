find_all_sources(. windows_SRCS)
list(REMOVE_ITEM windows_SRCS ./GUIWindowHome.cpp ./GUIWindowStartup.cpp)
add_sources(${windows_SRCS})
