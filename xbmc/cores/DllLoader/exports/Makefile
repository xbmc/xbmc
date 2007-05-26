INCLUDES=-I. -I../../../ -I../../../linux -I../../../../guilib

SRCS=emu_dummy.cpp emu_dx8.cpp emu_kernel32.cpp exports_kernel32.cpp emu_msvcrt.cpp exports_msvcrt.cpp exports_python_linux.cpp

OBJS=wrapper.o wrapper.def

LIB=exports.a

include ../../../../Makefile.include
-include Makefile.depend

exports_python_linux.o : exports_python_linux.cpp
	g++-4.1 -c -g  -D_LINUX -I. -I../../../ -I../../../linux -I../../../../guilib exports_python_linux.cpp -o exports_python_linux.o

wrapper.def : wrapper.c
	(echo -n "-Wl"; grep __wrap wrapper.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$$0);}') > wrapper.def
