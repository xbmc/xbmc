set(SOURCES AppEnvironment.cpp
            AppInboundProtocol.cpp
            Application.cpp
            ApplicationActionListeners.cpp
            ApplicationContentGeometry.cpp
            ApplicationMessageHandling.cpp
            ApplicationPlay.cpp
            ApplicationPlayer.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPlayLists.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationStackHelper.cpp
            ApplicationVolumeHandling.cpp
            PlaybackAnnouncer.cpp
            PlayListsGUIListener.cpp
            PlayListsMessageHandler.cpp
            PlayListsPlayer.cpp
            AppParamParser.cpp
            AppParams.cpp)

set(HEADERS AppEnvironment.h
            AppInboundProtocol.h
            Application.h
            ApplicationActionListeners.h
            ApplicationContentGeometry.h
            ApplicationEnums.h
            ApplicationMessageHandling.h
            ApplicationPlay.h
            ApplicationPlayer.h
            ApplicationPlayerCallback.h
            ApplicationPlayLists.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationStackHelper.h
            ApplicationVolumeHandling.h
            PlaybackAnnouncer.h
            PlayListsGUIListener.h
            PlayListsMessageHandler.h
            PlayListsPlayer.h
            PlaybackOptions.h
            AppParamParser.h
            AppParams.h)

core_add_library(application)
