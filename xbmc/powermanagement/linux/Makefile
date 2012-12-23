SRCS  = ConsoleDeviceKitPowerSyscall.cpp
SRCS += ConsoleUPowerSyscall.cpp
SRCS += HALPowerSyscall.cpp
SRCS += UPowerSyscall.cpp
SRCS += SystemdUPowerSyscall.cpp
SRCS += PivosPowerSyscall.cpp

LIB   = powermanagement_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
