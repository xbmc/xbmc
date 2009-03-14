CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libPython/Python -I$(top_srcdir)/guilib

SUBDIRS	=	util

noinst_LIBRARIES	=	libexports.a
libexports_a_SOURCES	=	emu_dummy.cpp \
				emu_dx8.cpp \
				emu_kernel32.cpp \
				exports_kernel32.cpp \
				emu_msvcrt.cpp \
				exports_msvcrt.cpp \
				exports_python_linux.cpp \
				wrapper.c

wrapper.def : wrapper.c
	(echo -n "-Wl"; grep __wrap wrapper.c | grep -v bash | sed "s/.*__wrap_//g" | sed "s/(.*//g" | awk '{printf(",-wrap,%s",$$0);}') > wrapper.def

# vim: ts=8:sw=8:noexpandtab
