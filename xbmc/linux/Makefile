INCLUDES=-I. -I/usr/include/SDL -I.. -I../../guilib -I../utils
CFLAGS+=-DHAS_SDL

SRCS=ConvUtils.cpp XEventUtils.cpp XFileUtils.cpp XHandle.cpp XSyncUtils.cpp XTimeUtils.cpp XMemUtils.cpp XThreadUtils.cpp NetworkLinux.cpp NetworkLinux.cpp

LIB=linux.a

include ../../Makefile.include

-include Makefile.depend

x: NetworkLinux.o
	g++ -o x NetworkLinux.o
