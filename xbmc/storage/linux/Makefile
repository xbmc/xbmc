SRCS=DeviceKitDisksProvider.cpp \
     HALProvider.cpp \
     UDisksProvider.cpp \
     
LIB=storage_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
