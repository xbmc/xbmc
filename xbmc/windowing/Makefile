SRCS=WinEventsSDL.cpp \
     WinEventsLinux.cpp \
     WinSystem.cpp \
     WinEventsX11.cpp \
     
LIB=windowing.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
