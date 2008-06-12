bin_PROGRAMS = faad

faad_SOURCES = main.c audio.c $(top_srcdir)/common/faad/getopt.c \
   audio.h

AM_CFLAGS = -O2
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/common/faad \
   -I$(top_srcdir)/common/mp4ff
LDADD = $(top_builddir)/libfaad/libfaad.la \
   $(top_builddir)/common/mp4ff/libmp4ff.la


CCLD = $(CXX)
