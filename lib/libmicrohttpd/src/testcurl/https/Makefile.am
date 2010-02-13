SUBDIRS = .

if USE_COVERAGE
  AM_CFLAGS = --coverage
endif

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/daemon/https \
  -I$(top_srcdir)/src/daemon/https/tls \
  -I$(top_srcdir)/src/daemon/https/lgl \
  -I$(top_srcdir)/src/daemon/https/x509 \
  -I$(top_srcdir)/src/daemon \
  $(LIBCURL_CPPFLAGS)

check_PROGRAMS = \
  tls_daemon_options_test \
  tls_authentication_test \
  mhds_multi_daemon_test \
  mhds_get_test \
  mhds_session_info_test \
  tls_thread_mode_test \
  tls_multi_thread_mode_test \
  tls_session_time_out_test \
  tls_cipher_change_test \
  tls_alert_test \
  tls_extension_test

EXTRA_DIST = cert.pem key.pem tls_test_keys.h tls_test_common.h

# tls_authentication_test currently fails for unknown reasons
TESTS = \
  tls_daemon_options_test \
  mhds_multi_daemon_test \
  mhds_get_test \
  mhds_session_info_test \
  tls_thread_mode_test \
  tls_multi_thread_mode_test \
  tls_session_time_out_test \
  tls_cipher_change_test \
  tls_alert_test \
  tls_extension_test \
  tls_authentication_test

# cURL dependent tests
tls_extension_test_SOURCES = \
  tls_extension_test.c \
  tls_test_common.c
tls_extension_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_session_time_out_test_SOURCES = \
  tls_session_time_out_test.c \
  tls_test_common.c
tls_session_time_out_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_cipher_change_test_SOURCES = \
  tls_cipher_change_test.c \
  tls_test_common.c
tls_cipher_change_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_alert_test_SOURCES = \
  tls_alert_test.c \
  tls_test_common.c
tls_alert_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_daemon_options_test_SOURCES = \
  tls_daemon_options_test.c \
  tls_test_common.c
tls_daemon_options_test_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_thread_mode_test_SOURCES = \
  tls_thread_mode_test.c \
  tls_test_common.c
tls_thread_mode_test_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_multi_thread_mode_test_SOURCES = \
  tls_multi_thread_mode_test.c \
  tls_test_common.c
tls_multi_thread_mode_test_LDADD = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

tls_authentication_test_SOURCES = \
  tls_authentication_test.c \
  tls_test_common.c
tls_authentication_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

mhds_session_info_test_SOURCES = \
  mhds_session_info_test.c \
  tls_test_common.c
mhds_session_info_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@

mhds_multi_daemon_test_SOURCES = \
  mhds_multi_daemon_test.c \
  tls_test_common.c
mhds_multi_daemon_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@  

mhds_get_test_SOURCES = \
  mhds_get_test.c \
  tls_test_common.c
mhds_get_test_LDADD  = \
  $(top_builddir)/src/testcurl/libcurl_version_check.a \
  $(top_builddir)/src/daemon/libmicrohttpd.la \
  @LIBCURL@  
