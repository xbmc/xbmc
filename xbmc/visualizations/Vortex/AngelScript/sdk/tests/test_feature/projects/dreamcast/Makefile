#
# GNU make Makefile for Angelscript
#

CC_FILES   := $(shell ls -1 *.cpp)
TARGET := ../bin/test.elf

CC         := kos-c++
OBJS       := $(patsubst %.cpp, %.o, $(CC_FILES)) scriptstring.o
TESTDIR    := /pc$(shell pwd | sed -e s/source/bin/g)
CC_FLAGS   := -I../../../angelscript/include -D__dreamcast__ -DasTestDir="$(TESTDIR)"
LIBS       := -langelscript -lstdc++ -lm -lgcc -lc -L../../../angelscript/lib


$(TARGET): $(OBJS) main.o
	kos-cc -o $(TARGET) $(OBJS) $(LIBS)
	@ echo "Done. Successfully created AngelScript library $(TARGET_LIB)"

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules
KOS_CFLAGS += $(CC_FLAGS)

scriptstring.o: ../../../add_on/scriptstring/scriptstring.cpp
	$(CC) -o $@ -c $<
	
clean:
	rm -rf *~ .?*.* .??* *.o AS_DEBUG
	rm -f $(TARGET)
