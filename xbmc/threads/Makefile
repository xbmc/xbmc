SRCS=Atomics.cpp \
     Condition.cpp \
     Event.cpp \
     Interruptible.cpp \
     LockFree.cpp \
     Semaphore.cpp \
     Thread.cpp \
     ThreadLocal.cpp \

LIB=threads.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
