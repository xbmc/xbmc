set(SOURCES PVRActionListener.cpp
            PVRGUIInfo.cpp
            PVRManager.cpp
            PVRDatabase.cpp
            PVRSettings.cpp)

set(HEADERS PVRActionListener.h
            PVRDatabase.h
            PVRGUIInfo.h
            PVRManager.h
            PVRSettings.h)

core_add_library(pvr)
add_dependencies(pvr libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(pvr ffmpeg)
endif()
