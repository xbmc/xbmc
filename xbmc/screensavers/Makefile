INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

SRCS=ScreenSaver.cpp ScreenSaverFactory.cpp
DIRS=rsxs-0.9 rsxs-0.9/src/plasma rsxs-0.9/src/solarwinds

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

lib: $(OBJS)
	make -C rsxs-0.9
