set(SOURCES GUIInfoProviders.cpp)

set(HEADERS GUIInfo.h
            GUIInfoLabels.h
            GUIInfoProviders.h
            IGUIInfoProvider.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(guiinfo)
endif()
