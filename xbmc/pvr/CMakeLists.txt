set(SOURCES PVRActionListener.cpp
            PVRGUIInfo.cpp
            PVRManager.cpp
            PVRDatabase.cpp
            PVRSettings.cpp)

set(HEADERS PVRActionListener.h
            PVRDatabase.h
            PVREvent.h
            PVRGUIInfo.h
            PVRManager.h
            PVRSettings.h)

core_add_library(pvr)
