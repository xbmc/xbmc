set(SOURCES AddonInterfaces.cpp
            AudioEngine.cpp
            General.cpp
            Filesystem.cpp
            Network.cpp)

set(HEADERS AddonInterfaces.h
            AudioEngine.h
            General.h
            Filesystem.h
            Network.h)

core_add_library(addonsBinaryInterfaces)
