CPPFLAGS	+=	-I$(top_srcdir) -I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib
CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/utils
CFLAGS		+=	-DAPI_DEBUG

SUBDIRS	=	exports

noinst_LIBRARIES	=	libdllloader.a
libdllloader_a_SOURCES	=	coff.cpp \
				DllLoader.cpp \
				DllLoaderContainer.cpp \
				dll.cpp \
				dll_tracker.cpp \
				dll_tracker_library.cpp \
				dll_tracker_file.cpp \
				dll_tracker_memory.cpp \
				dll_tracker_critical_section.cpp \
				dll_util.cpp \
				LibraryLoader.cpp \
				ldt_keeper.c \
				mmap_anon.c \
				SoLoader.cpp

# vim: ts=8:sw=8:noexpandtab
