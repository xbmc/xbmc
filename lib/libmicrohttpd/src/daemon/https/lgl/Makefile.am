AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include @LIBGCRYPT_CFLAGS@

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif

noinst_LTLIBRARIES = liblgl.la

liblgl_la_SOURCES = \
  gc-libgcrypt.c \
  gc.h
liblgl_la_LIBADD = @LIBGCRYPT_LIBS@
