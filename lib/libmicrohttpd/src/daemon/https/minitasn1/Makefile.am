AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/daemon/https/lgl \
  -I$(top_srcdir)/src/daemon/https/tls

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif

noinst_LTLIBRARIES = libasn1.la

libasn1_la_SOURCES = \
libtasn1.h \
mem.h \
gstr.h \
int.h \
parser_aux.h structure.h element.h decoding.c gstr.c \
parser_aux.c structure.c element.c coding.c
