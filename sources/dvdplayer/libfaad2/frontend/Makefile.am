bin_PROGRAMS = faad

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/common/faad \
   -I$(top_srcdir)/common/mp4ff

faad_LDADD = $(top_builddir)/libfaad/libfaad.la \
	$(top_builddir)/common/mp4ff/libmp4ff.la

faad_SOURCES = main.c \
	audio.c audio.h \
	$(top_srcdir)/common/faad/getopt.c
