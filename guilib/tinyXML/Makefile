INCLUDES=-I. -I../ -I../../xbmc -I../../xbmc/linux -I../../xbmc/utils
SRCS=tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp
LIB=tinyxml.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
