lib_LTLIBRARIES = libmp4ff.la

include_HEADERS = mp4ff.h

libmp4ff_la_SOURCES = mp4ff.c mp4atom.c mp4meta.c mp4sample.c mp4util.c \
   mp4tagupdate.c mp4ff.h mp4ffint.h mp4ff_int_types.h \
   drms.h drms.c drmstables.h

AM_CFLAGS = -O2 -g -DUSE_TAGGING=1
LIBTOOL_DEPS =
LIBS =

