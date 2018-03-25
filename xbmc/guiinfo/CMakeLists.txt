set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            MusicGUIInfo.cpp
            PlayerGUIInfo.cpp
            SystemGUIInfo.cpp
            VideoGUIInfo.cpp)

set(HEADERS GUIInfo.h
            GUIInfoHelper.h
            GUIInfoLabels.h
            GUIInfoProvider.h
            GUIInfoProviders.h
            IGUIInfoProvider.h
            MusicGUIInfo.h
            PlayerGUIInfo.h
            SystemGUIInfo.h
            VideoGUIInfo.h)

core_add_library(guiinfo)
