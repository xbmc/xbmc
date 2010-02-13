SUBDIRS  = .

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif

INCLUDES = -I$(top_srcdir)/src/include

EXTRA_DIST = README socat.c

check_PROGRAMS = \
  daemontest_get \
  daemontest_post \
  daemontest_postform \
  daemontest_put \
  daemontest_large_put \
  daemontest_get11 \
  daemontest_post11 \
  daemontest_postform11 \
  daemontest_put11 \
  daemontest_large_put11 \
  daemontest_long_header \
  daemontest_get_chunked \
  daemontest_put_chunked 

TESTS = $(check_PROGRAMS)

daemontest_get_SOURCES = \
  daemontest_get.c
daemontest_get_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_get_chunked_SOURCES = \
  daemontest_get_chunked.c
daemontest_get_chunked_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_post_SOURCES = \
  daemontest_post.c
daemontest_post_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_postform_SOURCES = \
  daemontest_postform.c
daemontest_postform_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_put_SOURCES = \
  daemontest_put.c
daemontest_put_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_put_chunked_SOURCES = \
  daemontest_put_chunked.c
daemontest_put_chunked_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_get11_SOURCES = \
  daemontest_get.c
daemontest_get11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_post11_SOURCES = \
  daemontest_post.c
daemontest_post11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_postform11_SOURCES = \
  daemontest_postform.c
daemontest_postform11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_put11_SOURCES = \
  daemontest_put.c
daemontest_put11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 


daemontest_large_put_SOURCES = \
  daemontest_large_put.c
daemontest_large_put_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_large_put11_SOURCES = \
  daemontest_large_put.c
daemontest_large_put11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_long_header_SOURCES = \
  daemontest_long_header.c
daemontest_long_header_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 
