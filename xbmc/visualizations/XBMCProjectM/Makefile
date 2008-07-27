INCLUDES=-I. -I.. -I../../linux -I../../ -I../../../guilib -I../../../visualisations
SRCS=Main.cpp
LIB=../../../visualisations/ProjectM.vis
CFLAGS=-O3 -g -fPIC
DIRS=libprojectM

include ../../../Makefile.include
-include $(OBJS:.o=.P)

.PHONY: $(DIRS)
.DEFAULT_GOAL:= $(LIB)

libprojectM/libprojectM.so: libprojectM;
libprojectM:
	$(MAKE) -C libprojectM

$(LIB): $(OBJS) libprojectM/libprojectM.so
	$(CXX) ./libprojectM/CMakeFiles/projectM.dir/*.o -fPIC -shared $(CFLAGS) -o $(LIB) $(OBJS) `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o


