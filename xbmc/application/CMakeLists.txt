set(SOURCES AppEnvironment.cpp
            AppInboundProtocol.cpp
            ApplicationActionListeners.cpp
            ApplicationPlayer.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationVolumeHandling.cpp
            AppParamParser.cpp
            AppParams.cpp)

set(HEADERS AppEnvironment.h
            AppInboundProtocol.h
            ApplicationActionListeners.h
            ApplicationPlayer.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationVolumeHandling.h
            AppParamParser.h
            AppParams.h)

core_add_library(application)
