INCLUDES+=-I/home/dc4/xbmc_holder/xbmc/xbmc/cores/VideoPlayer

SRCS  = ProcessInfo.cpp overrides/rbp/ProcessInfoPi.cpp
CXXFLAGS += -DPLATFORM_OVERRIDE

LIB = Process.a

include /home/dc4/xbmc_holder/xbmc/Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
