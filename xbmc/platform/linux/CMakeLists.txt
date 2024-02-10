set(SOURCES AppParamParserLinux.cpp
            CPUInfoLinux.cpp
            GPUInfoLinux.cpp
            MemUtils.cpp
            OptionalsReg.cpp
            PlatformLinux.cpp
            SysfsPath.cpp
            TimeUtils.cpp)

set(HEADERS AppParamParserLinux.h
            CPUInfoLinux.h
            GPUInfoLinux.h
            OptionalsReg.h
            PlatformLinux.h
            SysfsPath.h
            TimeUtils.h)

if(TARGET_WEBOS)
  list(APPEND SOURCES AppParamParserWebOS.cpp
                      PlatformWebOS.cpp)
  list(APPEND HEADERS AppParamParserWebOS.h
                      PlatformWebOS.h)
endif()

if(TARGET ALSA::ALSA)
  list(APPEND SOURCES FDEventMonitor.cpp)
  list(APPEND HEADERS FDEventMonitor.h)
endif()

if(TARGET DBus::DBus)
  list(APPEND SOURCES DBusMessage.cpp
                      DBusUtil.cpp)
  list(APPEND HEADERS DBusMessage.h
                      DBusUtil.h)
endif()

if(ADDONS_CONFIGURE_AT_STARTUP)
  add_compile_definitions(ADDONS_CONFIGURE_AT_STARTUP)
endif()

core_add_library(linuxsupport)
