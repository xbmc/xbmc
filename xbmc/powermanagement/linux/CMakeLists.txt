set(SOURCES ConsoleUPowerSyscall.cpp
            UPowerSyscall.cpp
            LogindUPowerSyscall.cpp)

set(HEADERS ConsoleUPowerSyscall.h
            FallbackPowerSyscall.h
            LogindUPowerSyscall.h
            UPowerSyscall.h)

core_add_library(powermanagement_linux)
