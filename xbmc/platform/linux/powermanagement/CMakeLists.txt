set(SOURCES LinuxPowerSyscall.cpp)

set(HEADERS FallbackPowerSyscall.h
            LinuxPowerSyscall.h)

if(DBUS_FOUND)
  list(APPEND SOURCES LogindUPowerSyscall.cpp
                      UPowerSyscall.cpp)
  list(APPEND HEADERS LogindUPowerSyscall.h
                      UPowerSyscall.h)
endif()

if(SOURCES)
  core_add_library(platform_linux_powermanagement)
endif()
