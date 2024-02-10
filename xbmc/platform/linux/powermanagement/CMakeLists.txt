set(SOURCES LinuxPowerSyscall.cpp)

set(HEADERS FallbackPowerSyscall.h
            LinuxPowerSyscall.h)

if(TARGET DBus::DBus)
  list(APPEND SOURCES ConsoleUPowerSyscall.cpp
                      LogindUPowerSyscall.cpp
                      UPowerSyscall.cpp)
  list(APPEND HEADERS ConsoleUPowerSyscall.h
                      LogindUPowerSyscall.h
                      UPowerSyscall.h)
endif()

if (TARGET_WEBOS)
  list(APPEND SOURCES LunaPowerManagement.cpp)
  list(APPEND HEADERS LunaPowerManagement.h)
endif ()

if(SOURCES)
  core_add_library(platform_linux_powermanagement)
endif()
