set(SOURCES DPMSSupport.cpp
            IPowerSyscall.cpp
            PowerManager.cpp)

set(HEADERS DPMSSupport.h
            IPowerSyscall.h
            PowerManager.h)

if(CORE_SYSTEM_NAME MATCHES windows)
  list(APPEND HEADERS WinIdleTimer.h)
endif()

core_add_library(powermanagement)
