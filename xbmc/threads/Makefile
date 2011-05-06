SRCS=Atomics.cpp \
     CriticalSection.cpp \
     Event.cpp \
     LockFree.cpp \
     Mutex.cpp \
     Semaphore.cpp \
     SharedSection.cpp \
     SingleLock.cpp \
     Thread.cpp \
     XBMC_mutex.cpp \

LIB=threads.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
