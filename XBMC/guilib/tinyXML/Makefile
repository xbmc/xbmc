INCLUDES=-I. -I../ -I../../xbmc -I../../xbmc/linux -I../../xbmc/utils
SRCS=tinyxmlerror.cpp tinyxmlparser.cpp tinystr.cpp tinyxml.cpp
OBJS=tinyxmlerror.o  tinyxmlparser.o tinystr.o  tinyxml.o
LIB=tinyxml.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
