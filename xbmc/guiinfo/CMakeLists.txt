set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            MusicGUIInfo.cpp
            VideoGUIInfo.cpp)

set(HEADERS GUIInfo.h
            GUIInfoHelper.h
            GUIInfoLabels.h
            GUIInfoProvider.h
            GUIInfoProviders.h
            IGUIInfoProvider.h
            MusicGUIInfo.h
            VideoGUIInfo.h)

core_add_library(guiinfo)
