SRCS  = DeviceKitDisksProvider.cpp
SRCS += UDisksProvider.cpp
SRCS += UDevProvider.cpp
     
LIB = storage_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
