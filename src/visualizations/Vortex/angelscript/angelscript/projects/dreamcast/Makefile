#
# GNU make Makefile for Angelscript
#

TARGET = libangelscript.a

CC_FILES   := $(shell ls -1 as_*.cpp)
OBJS = $(patsubst %.cpp, %.o, $(CC_FILES))

all: ../include/memory.h defaultall

../include/memory.h:
	echo "#include <string.h>" >> ../include/memory.h

include $(KOS_BASE)/addons/Makefile.prefab

KOS_CFLAGS += -Dmemory=string -D__dreamcast__ -I../include -I.

clean:
	rm -f *~ *.o *.a


