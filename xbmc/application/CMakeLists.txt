set(SOURCES AppEnvironment.cpp
            ApplicationActionListeners.cpp
            ApplicationPlayerCallback.cpp
            ApplicationPowerHandling.cpp
            ApplicationSettingsHandling.cpp
            ApplicationSkinHandling.cpp
            ApplicationVolumeHandling.cpp
            AppParamParser.cpp
            AppParams.cpp)

set(HEADERS AppEnvironment.h
            ApplicationActionListeners.h
            ApplicationPlayerCallback.h
            ApplicationPowerHandling.h
            ApplicationSettingsHandling.h
            ApplicationSkinHandling.h
            ApplicationVolumeHandling.h
            AppParamParser.h
            AppParams.h)

core_add_library(application)
