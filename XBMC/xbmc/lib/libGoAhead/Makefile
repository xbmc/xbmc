INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils
SRCS=SpyceModule.cpp WebServer.cpp XBMCConfiguration.cpp XBMChttp.cpp XBMCweb.cpp

LIB=goahead.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
