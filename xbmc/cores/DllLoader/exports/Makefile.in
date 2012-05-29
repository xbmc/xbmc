SRCS=emu_dummy.cpp \
     emu_kernel32.cpp \
     emu_msvcrt.cpp \
     wrapper.c

LIB=exports.a

CLEAN_FILES=wrapper.def

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

wrapper.o : wrapper.c
	$(CC) $(CFLAGS) $(DEFINES) -c $(INCLUDES) $< -o $@

exports_python_linux.o : exports_python_linux.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -c $(INCLUDES) $< -o $@

wrapper.def : wrapper.o
	nm wrapper.o | grep __wrap | awk '{printf("%s ", $$3);}' | sed "s/___wrap_/__wrap_/g" | sed "s/__wrap_/-Wl,-wrap,/g" > $@
