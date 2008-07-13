INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils

SRCS=ScreenSaver.cpp ScreenSaverFactory.cpp

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
