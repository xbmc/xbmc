INCLUDES=-I. -I../../../ -I../../../linux -I../../../../guilib

SRCS=emu_dummy.cpp emu_dx8.cpp emu_kernel32.cpp exports_kernel32.cpp emu_msvcrt.cpp exports_msvcrt.cpp exports_python.cpp

OBJS=wrapper.o wrapper.def

LIB=exports.a

include ../../../../Makefile.include
-include Makefile.depend

wrapper.def : wrapper.c
	(echo -n "-Wl"; grep __wrap wrapper.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$$0);}') > wrapper.def
