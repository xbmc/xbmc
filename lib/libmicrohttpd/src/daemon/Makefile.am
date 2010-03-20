AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/daemon \
  -I$(top_srcdir)/src/daemon/https/lgl \
  -I$(top_srcdir)/src/daemon/https/x509 \
  -I$(top_srcdir)/src/daemon/https/minitasn1 \
  -I$(top_srcdir)/src/daemon/https/tls \
  -I$(top_srcdir)/src/daemon/https \
  @LIBGCRYPT_CFLAGS@ 

EXTRA_DIST = EXPORT.sym

lib_LTLIBRARIES = \
  libmicrohttpd.la

libmicrohttpd_la_SOURCES = \
  connection.c connection.h \
  reason_phrase.c reason_phrase.h \
  daemon.c  \
  internal.c internal.h \
  memorypool.c memorypool.h \
  postprocessor.c \
  response.c response.h
libmicrohttpd_la_LDFLAGS = \
  $(MHD_LIB_LDFLAGS) \
  -version-info @LIB_VERSION_CURRENT@:@LIB_VERSION_REVISION@:@LIB_VERSION_AGE@

if USE_COVERAGE
  AM_CFLAGS = --coverage
endif


if ENABLE_HTTPS
SUBDIRS = https .
libmicrohttpd_la_SOURCES += \
  connection_https.c connection_https.h
libmicrohttpd_la_LIBADD = \
  https/lgl/liblgl.la \
  https/x509/libx509.la \
  https/tls/libtls.la \
  https/minitasn1/libasn1.la 
endif

check_PROGRAMS = \
  postprocessor_test \
  postprocessor_large_test \
  daemon_test 

TESTS = $(check_PROGRAMS)

daemon_test_SOURCES = \
  daemon_test.c
daemon_test_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la 

postprocessor_test_SOURCES = \
  postprocessor_test.c
postprocessor_test_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la 

postprocessor_large_test_SOURCES = \
  postprocessor_large_test.c
postprocessor_large_test_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la 
