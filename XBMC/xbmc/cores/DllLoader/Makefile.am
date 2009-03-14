CPPFLAGS	+=	-I$(top_srcdir) -I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib
CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/utils
CFLAGS		+=	-DAPI_DEBUG

SUBDIRS	=	exports

noinst_LIBRARIES	=	libdllloader.a
libdllloader_a_SOURCES	=	DllLoader.cpp \
				DllLoaderContainer.cpp \
				dll.cpp \
				coff.cpp \
				dll_tracker.cpp \
				dll_tracker_library.cpp \
				dll_tracker_file.cpp \
				dll_tracker_memory.cpp \
				dll_tracker_critical_section.cpp \
				dll_util.cpp \
				SoLoader.cpp \
				LibraryLoader.cpp

# vim: ts=8:sw=8:noexpandtab
