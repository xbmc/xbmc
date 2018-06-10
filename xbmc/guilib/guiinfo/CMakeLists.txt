set(SOURCES GUIInfo.cpp
            GUIInfoHelper.cpp
            GUIInfoProviders.cpp
            GUIInfoLabel.cpp
            GUIInfoBool.cpp
            GUIInfoColor.cpp
            AddonsGUIInfo.cpp
            GamesGUIInfo.cpp
            GUIControlsGUIInfo.cpp
            LibraryGUIInfo.cpp
            MusicGUIInfo.cpp
            PicturesGUIInfo.cpp
            PlayerGUIInfo.cpp
            SkinGUIInfo.cpp
            SystemGUIInfo.cpp
            VideoGUIInfo.cpp
            VisualisationGUIInfo.cpp
            WeatherGUIInfo.cpp)

set(HEADERS GUIInfo.h
            GUIInfoHelper.h
            GUIInfoLabels.h
            GUIInfoProvider.h
            GUIInfoProviders.h
            GUIInfoLabel.h
            GUIInfoBool.h
            GUIInfoColor.h
            IGUIInfoProvider.h
            AddonsGUIInfo.h
            GamesGUIInfo.h
            GUIControlsGUIInfo.h
            LibraryGUIInfo.h
            MusicGUIInfo.h
            PicturesGUIInfo.h
            PlayerGUIInfo.h
            SkinGUIInfo.h
            SystemGUIInfo.h
            VideoGUIInfo.h
            VisualisationGUIInfo.h
            WeatherGUIInfo.h)

core_add_library(guilib_guiinfo)
