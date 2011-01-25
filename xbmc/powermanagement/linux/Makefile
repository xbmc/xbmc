SRCS=ConsoleDeviceKitPowerSyscall.cpp \
     ConsoleUPowerSyscall.cpp \
     HALPowerSyscall.cpp \
     
LIB=powermanagement_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
