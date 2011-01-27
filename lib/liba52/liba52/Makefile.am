CFLAGS = @CFLAGS@ @LIBA52_CFLAGS@

lib_LTLIBRARIES = liba52.la

liba52_la_SOURCES = bitstream.c imdct.c bit_allocate.c parse.c downmix.c
liba52_la_LIBADD = @LIBA52_LIBS@ -lm
liba52_la_LDFLAGS = -no-undefined

EXTRA_DIST = configure.incl a52_internal.h bitstream.h tables.h
