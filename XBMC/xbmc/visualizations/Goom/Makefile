INCLUDES=-I. -I.. -I../../linux -I../../ -I../../../guilib -I../../../visualisations -Igoom2k4-0/src
SRCS=Main.cpp
LIB=../../../visualisations/Goom.vis
CFLAGS=-g -O3 -fPIC
DIRS=goom2k4-0

include ../../../Makefile.include
-include $(OBJS:.o=.P)

ifeq ($(ARCH), osx)
.PHONY: $(DIRS)
.DEFAULT_GOAL:= $(LIB)

goom2k4-0/src/.libs/libgoom2.a:
	$(MAKE) -C goom2k4-0

$(LIB): $(OBJS) goom2k4-0/src/.libs/libgoom2.a
	$(CXX) -fPIC -bundle -shared -undefined suppress -flat_namespace goom2k4-0/src/.libs/libgoom2.a $(CFLAGS) -o $(LIB) $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@

else
.PHONY: $(DIRS)
.DEFAULT_GOAL:= $(LIB)

goom2k4-0/src/.libs/libgoom2.a:
	$(MAKE) -C goom2k4-0
#goom2k4-0:
#	./configure --enable-static --disable-shared
#	$(MAKE)

$(LIB): $(OBJS) goom2k4-0/src/.libs/libgoom2.a
	$(CXX) -fPIC goom2k4-0/src/*.o -shared $(CFLAGS) -o $(LIB) $(OBJS) `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o

endif

