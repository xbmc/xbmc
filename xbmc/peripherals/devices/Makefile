SRCS=Peripheral.cpp \
     PeripheralBluetooth.cpp \
     PeripheralCecAdapter.cpp \
     PeripheralDisk.cpp \
     PeripheralHID.cpp \
     PeripheralNIC.cpp \
     PeripheralNyxboard.cpp \
     PeripheralTuner.cpp
     
LIB=peripheral-devices.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
