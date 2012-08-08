SRCS=Atomics.cpp \
     Event.cpp \
     LockFree.cpp \
     Thread.cpp \
     Timer.cpp \
     SystemClock.cpp \
     platform/Implementation.cpp

LIB=threads.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
