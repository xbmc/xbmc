set(SOURCES AppEnvironment.cpp
            AppInboundProtocol.cpp
            Application.cpp
            ApplicationActionListeners.cpp
            ApplicationPlayer.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationStackHelper.cpp
            ApplicationVolumeHandling.cpp
            AppParamParser.cpp
            AppParams.cpp)

set(HEADERS AppEnvironment.h
            AppInboundProtocol.h
            Application.h
            ApplicationActionListeners.h
            ApplicationPlayer.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationStackHelper.h
            ApplicationVolumeHandling.h
            AppParamParser.h
            AppParams.h)

core_add_library(application)
