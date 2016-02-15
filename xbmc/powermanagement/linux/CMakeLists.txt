set(SOURCES ConsoleDeviceKitPowerSyscall.cpp
            ConsoleUPowerSyscall.cpp
            UPowerSyscall.cpp
            LogindUPowerSyscall.cpp)

set(HEADERS ConsoleDeviceKitPowerSyscall.h
            ConsoleUPowerSyscall.h
            FallbackPowerSyscall.h
            LogindUPowerSyscall.h
            UPowerSyscall.h)

core_add_library(powermanagement_linux)
