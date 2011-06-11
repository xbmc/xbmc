SRCS=Atomics.cpp \
     Event.cpp \
     Mutex.cpp \
     LockFree.cpp \
     Semaphore.cpp \
     Thread.cpp \
     XBMC_mutex.cpp \

LIB=threads.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
