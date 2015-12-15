.SUFFIXES : .m

SRCS=SimplePing.m \
     darwinICMPPing.m \
     ZeroconfBrowserOSX.cpp \
     ZeroconfOSX.cpp

LIB=network.a

include ../../../Makefile.include
-include $(patsubst %.m,%.P,$(patsubst %.mm,%.P,$(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))))
