aux_source_directory(. settings_SRCS)
list(REMOVE_ITEM settings_SRCS ./GUISettings.cpp)
add_library(settings STATIC ${settings_SRCS})
