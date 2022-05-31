set(SOURCES ApplicationActionListeners.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationVolumeHandling.cpp)

set(HEADERS ApplicationActionListeners.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationVolumeHandling.h)

core_add_library(application)
