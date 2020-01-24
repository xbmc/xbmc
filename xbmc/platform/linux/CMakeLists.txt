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

core_add_library(linuxsupport)
