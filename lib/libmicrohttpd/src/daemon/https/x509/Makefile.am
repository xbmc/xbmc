AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/daemon/https \
  -I$(top_srcdir)/src/daemon/https/minitasn1 \
  -I$(top_srcdir)/src/daemon/https/lgl \
  -I$(top_srcdir)/src/daemon/https/tls \
  -I$(top_srcdir)/src/daemon/https/x509 \
  @LIBGCRYPT_CFLAGS@

noinst_LTLIBRARIES = libx509.la

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif


libx509_la_SOURCES = \
  common.c common.h \
  extensions.c extensions.h \
  mpi.c mpi.h \
  x509_privkey.c privkey.h \
  x509.c x509.h
libx509_la_LIBADD = @LIBGCRYPT_LIBS@

