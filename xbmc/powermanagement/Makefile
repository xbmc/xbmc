SRCS=DPMSSupport.cpp \
     PowerManager.cpp \
     IPowerSyscall.cpp \

LIB=powermanagement.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
