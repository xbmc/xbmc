set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            AddonsGUIInfo.cpp
            GamesGUIInfo.cpp
            MusicGUIInfo.cpp
            PlayerGUIInfo.cpp
            SystemGUIInfo.cpp
            VideoGUIInfo.cpp
            WeatherGUIInfo.cpp)

set(HEADERS GUIInfo.h
            GUIInfoHelper.h
            GUIInfoLabels.h
            GUIInfoProvider.h
            GUIInfoProviders.h
            IGUIInfoProvider.h
            AddonsGUIInfo.h
            GamesGUIInfo.h
            MusicGUIInfo.h
            PlayerGUIInfo.h
            SystemGUIInfo.h
            VideoGUIInfo.h
            WeatherGUIInfo.h)

core_add_library(guiinfo)
