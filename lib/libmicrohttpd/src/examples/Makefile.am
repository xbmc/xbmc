SUBDIRS  = .

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/daemon/https \
  @LIBGCRYPT_CFLAGS@

if USE_COVERAGE
  AM_CFLAGS = --coverage
endif

# example programs
noinst_PROGRAMS = \
authorization_example \
minimal_example \
minimal_example_comet \
querystring_example \
fileserver_example \
fileserver_example_dirs \
fileserver_example_external_select \
refuse_post_example

if ENABLE_HTTPS
noinst_PROGRAMS += https_fileserver_example 
endif

minimal_example_SOURCES = \
 minimal_example.c 
minimal_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

minimal_example_comet_SOURCES = \
 minimal_example_comet.c 
minimal_example_comet_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

authorization_example_SOURCES = \
 authorization_example.c 
authorization_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

refuse_post_example_SOURCES = \
 refuse_post_example.c 
refuse_post_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

querystring_example_SOURCES = \
 querystring_example.c 
querystring_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

fileserver_example_SOURCES = \
 fileserver_example.c 
fileserver_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

fileserver_example_dirs_SOURCES = \
 fileserver_example_dirs.c 
fileserver_example_dirs_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

fileserver_example_external_select_SOURCES = \
 fileserver_example_external_select.c 
fileserver_example_external_select_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la 

https_fileserver_example_SOURCES = \
https_fileserver_example.c
https_fileserver_example_LDADD = \
 $(top_builddir)/src/daemon/libmicrohttpd.la
