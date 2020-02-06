set(SOURCES CPUInfoLinux.cpp
            MemUtils.cpp
            OptionalsReg.cpp
            SysfsPath.cpp
            TimeUtils.cpp)

set(HEADERS CPUInfoLinux.h
            OptionalsReg.h
            SysfsPath.h
            TimeUtils.h)

if(ALSA_FOUND)
  list(APPEND SOURCES FDEventMonitor.cpp)
  list(APPEND HEADERS FDEventMonitor.h)
endif()

if(DBUS_FOUND)
  list(APPEND SOURCES DBusMessage.cpp
                      DBusReserve.cpp
                      DBusUtil.cpp)
  list(APPEND HEADERS DBusMessage.h
                      DBusReserve.h
                      DBusUtil.h)
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  list(APPEND SOURCES RBP.cpp
                      OMXCore.cpp
                      ScreenshotSurfaceRBP.cpp)
  list(APPEND HEADERS RBP.h
                      DllBCM.h
                      DllOMX.h
                      OMXCore.h
                      ScreenshotSurfaceRBP.h)
endif()

core_add_library(linuxsupport)
