SRCS=Atomics.cpp \
     Event.cpp \
     LockFree.cpp \
     Thread.cpp \
     ThreadLocal.cpp \

LIB=threads.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
