SUBDIRS  = .

if USE_COVERAGE
  AM_CFLAGS = -fprofile-arcs -ftest-coverage
endif

if ENABLE_HTTPS
if MHD_DEBUG_TLS
  SUBDIRS += https
endif
endif

AM_CPPFLAGS = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/daemon \
-I$(top_srcdir)/src/include \
$(LIBCURL_CPPFLAGS)

check_PROGRAMS = \
  daemontest_get \
  daemontest_post \
  daemontest_postform \
  daemontest_post_loop \
  daemontest_put \
  daemontest_process_headers \
  daemontest_process_arguments \
  daemontest_parse_cookies \
  daemontest_large_put \
  daemontest_get11 \
  daemontest_post11 \
  daemontest_postform11 \
  daemontest_post_loop11 \
  daemontest_put11 \
  daemontest_large_put11 \
  daemontest_long_header \
  daemontest_long_header11 \
  daemontest_get_chunked \
  daemontest_put_chunked \
  daemontest_iplimit11 \
  daemontest_termination
noinst_PROGRAMS = \
  daemon_options_test

TESTS = $(check_PROGRAMS)

noinst_LIBRARIES = libcurl_version_check.a

libcurl_version_check_a_SOURCES = \
  curl_version_check.c

daemon_options_test_SOURCES = \
  daemon_options_test.c
daemon_options_test_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la

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

daemontest_process_headers_SOURCES = \
  daemontest_process_headers.c
daemontest_process_headers_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_parse_cookies_SOURCES = \
  daemontest_parse_cookies.c
daemontest_parse_cookies_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_process_arguments_SOURCES = \
  daemontest_process_arguments.c
daemontest_process_arguments_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_postform_SOURCES = \
  daemontest_postform.c
daemontest_postform_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_post_loop_SOURCES = \
  daemontest_post_loop.c
daemontest_post_loop_LDADD = \
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

daemontest_post_loop11_SOURCES = \
  daemontest_post_loop.c
daemontest_post_loop11_LDADD = \
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

daemontest_long_header11_SOURCES = \
  daemontest_long_header.c
daemontest_long_header11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_iplimit11_SOURCES = \
  daemontest_iplimit.c
daemontest_iplimit11_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

daemontest_termination_SOURCES = \
  daemontest_termination.c
daemontest_termination_LDADD = \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@ 

