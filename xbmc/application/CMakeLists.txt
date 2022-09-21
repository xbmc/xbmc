set(SOURCES AppEnvironment.cpp
            ApplicationActionListeners.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationVolumeHandling.cpp)

set(HEADERS AppEnvironment.h
            ApplicationActionListeners.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationVolumeHandling.h)

core_add_library(application)
