INCLUDES=-I. -I.. -I../../../XBMC/xbmc/linux -I../../../XBMC/xbmc -I../../../XBMC/guilib
SRCS=Main.cpp
LIB=ProjectM.vis
CFLAGS=-O3 -g

default: vis

include ../../../XBMC/Makefile.include

vis: Main.o
	(unset CC; unset CXX; cd libprojectM; cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D USE_FTGL:BOOL=OFF .;)
	$(MAKE) -C libprojectM
	$(CXX) ./libprojectM/CMakeFiles/projectM.dir/*.o -fPIC -shared $(CFLAGS) -o $(LIB) $(OBJS) `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o
