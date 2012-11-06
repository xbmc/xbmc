find_all_sources(. settings_SRCS)
list(REMOVE_ITEM settings_SRCS ./GUISettings.cpp)
add_sources(${settings_SRCS})
