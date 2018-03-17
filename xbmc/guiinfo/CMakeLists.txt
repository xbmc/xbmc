set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            VideoGUIInfo.cpp)

set(HEADERS GUIInfo.h
            GUIInfoHelper.h
            GUIInfoLabels.h
            GUIInfoProviders.h
            IGUIInfoProvider.h
            VideoGUIInfo.h)

core_add_library(guiinfo)
