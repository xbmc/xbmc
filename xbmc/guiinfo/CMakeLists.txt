set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            AddonsGUIInfo.cpp
            GamesGUIInfo.cpp
            GUIControlsGUIInfo.cpp
            LibraryGUIInfo.cpp
            MusicGUIInfo.cpp
            PicturesGUIInfo.cpp
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
            GUIControlsGUIInfo.h
            LibraryGUIInfo.h
            MusicGUIInfo.h
            PicturesGUIInfo.h
            PlayerGUIInfo.h
            SystemGUIInfo.h
            VideoGUIInfo.h
            WeatherGUIInfo.h)

core_add_library(guiinfo)
