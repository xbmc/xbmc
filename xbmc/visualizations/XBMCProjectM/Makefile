INCLUDES=-I. -I.. -I../../linux -I../../ -I../../../guilib -I../../../visualisations
SRCS=Main.cpp
LIB=ProjectM.vis
CFLAGS=-O3 -g -fPIC

default: vis
DIRS=libprojectM

include ../../../Makefile.include

lib: vis

vis: Main.o
	$(MAKE) -C libprojectM
	$(CXX) ./libprojectM/CMakeFiles/projectM.dir/*.o -fPIC -shared $(CFLAGS) -o $(LIB) $(OBJS) `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o && cp $(LIB) ../../../visualisations/
