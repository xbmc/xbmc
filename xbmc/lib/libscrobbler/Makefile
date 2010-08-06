INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils -I../../FileSystem
INCLUDES+=-I../../cores
SRCS=scrobbler.cpp lastfmscrobbler.cpp librefmscrobbler.cpp

LIB=scrobbler.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
