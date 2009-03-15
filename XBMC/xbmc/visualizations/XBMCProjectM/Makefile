INCLUDES=-I. -I.. -I../../linux -I../../ -I../../../guilib -I../../../visualisations
SRCS=Main.cpp
LIB=../../../visualisations/ProjectM.vis
CFLAGS=-O3 -g -fPIC
DIRS=libprojectM

include ../../../Makefile.include
-include $(OBJS:.o=.P)

.PHONY: $(DIRS)
.DEFAULT_GOAL:= $(LIB)

ifeq ($(findstring osx,$(ARCH)), osx)
libprojectM/libprojectM.dylib: libprojectM;
libprojectM:
	$(MAKE) -C libprojectM

$(LIB): $(OBJS) libprojectM/libprojectM.dylib
	$(CXX) -bundle -shared -undefined suppress -flat_namespace $(CFLAGS) -o $(LIB) \
		./libprojectM/CMakeFiles/projectM.dir/*.o \
		$(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@

projectm:

else
libprojectM/libprojectM.so: libprojectM;
libprojectM:
	$(MAKE) -C libprojectM

$(LIB): $(OBJS) libprojectM/libprojectM.so
	$(CXX) ./libprojectM/CMakeFiles/projectM.dir/*.o -fPIC -shared $(CFLAGS) -o $(LIB) $(OBJS) `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o

endif

