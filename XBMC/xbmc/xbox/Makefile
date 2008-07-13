INCLUDES=-I. -I../ -I../linux -I../../guilib

SRCS=IoSupport.cpp XKGeneral.cpp

LIB=xbox.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
