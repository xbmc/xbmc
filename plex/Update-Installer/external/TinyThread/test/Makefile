#-----------------------------------------------------------------------------------------
# Makefile for GCC & gmake (Linux, Windows/MinGW, OpenSolaris, etc).
#-----------------------------------------------------------------------------------------
# Copyright (c) 2010 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#     1. The origin of this software must not be misrepresented; you must not
#     claim that you wrote the original software. If you use this software
#     in a product, an acknowledgment in the product documentation would be
#     appreciated but is not required.
#
#     2. Altered source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
#
#     3. This notice may not be removed or altered from any source
#     distribution.
#-----------------------------------------------------------------------------------------

# A simple hack to check if we are on Windows or not (i.e. are we using mingw32-make?)
ifeq ($(findstring mingw32, $(MAKE)), mingw32)
WINDOWS=1
endif

# Compiler settings
CPP = g++
CPPFLAGS = -W -O3 -c -I../source
LFLAGS =
LIBS =

# Non-windows systems need pthread
ifndef WINDOWS
LIBS += -lpthread
endif

# MinGW32 GCC 4.5 link problem fix
ifdef WINDOWS
ifeq ($(findstring 4.5.,$(shell g++ -dumpversion)), 4.5.)
LFLAGS += -static-libstdc++ -static-libgcc
endif
endif

# Misc. system commands
ifdef WINDOWS
RM = del /Q
else
RM = rm -f
endif

# File endings
ifdef WINDOWS
EXE = .exe
else
EXE =
endif

# Object files for the hello program
HELLO_OBJS = hello.o

# Object files for the test program
TEST_OBJS = test.o

# Object files for the hello program
FRACAL_OBJS = fractal.o

# TinyThread++ object files
TINYTHREAD_OBJS = tinythread.o

all: hello$(EXE) test$(EXE) fractal$(EXE)

clean:
	$(RM) hello$(EXE) test$(EXE) fractal$(EXE) $(HELLO_OBJS) $(TEST_OBJS) $(FRACAL_OBJS) $(TINYTHREAD_OBJS)


test$(EXE): $(TEST_OBJS) $(TINYTHREAD_OBJS)
	$(CPP) $(LFLAGS) -o $@ $(TEST_OBJS) $(TINYTHREAD_OBJS) $(LIBS)

hello$(EXE): $(HELLO_OBJS) $(TINYTHREAD_OBJS)
	$(CPP) $(LFLAGS) -o $@ $(HELLO_OBJS) $(TINYTHREAD_OBJS) $(LIBS)

fractal$(EXE): $(FRACAL_OBJS) $(TINYTHREAD_OBJS)
	$(CPP) $(LFLAGS) -o $@ $(FRACAL_OBJS) $(TINYTHREAD_OBJS) $(LIBS)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $<

%.o: ../source/%.cpp
	$(CPP) $(CPPFLAGS) $<

# Dependencies
hello.o: hello.cpp ../source/tinythread.h
test.o: test.cpp ../source/tinythread.h ../source/fast_mutex.h
fractal.o: fractal.cpp ../source/tinythread.h
tinythread.o: ../source/tinythread.cpp ../source/tinythread.h
