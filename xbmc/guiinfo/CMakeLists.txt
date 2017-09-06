set(HEADERS GUIInfoLabels.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(guiinfo)
endif()
