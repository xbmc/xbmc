set(SOURCES ApplicationActionListeners.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationVolumeHandling.cpp)

set(HEADERS ApplicationActionListeners.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSkinHandling.h
            ApplicationVolumeHandling.h)

core_add_library(application)
