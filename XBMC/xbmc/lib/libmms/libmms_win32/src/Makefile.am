# noinst_PROGRAMS = testconnect testdownload testswap

lib_LTLIBRARIES = libmms.la

libmms_la_SOURCES = 		\
	mmsio.h			\
	mms.c			\
	mms.h			\
	mms_config.h		\
	mmsh.c			\
	mmsh.h			\
	mmsx.c			\
	mmsx.h			\
	bswap.h			\
	uri.c			\
	uri.h			\
	asfheader.h		\
	mmsh.h

# noinst_HEADERS = 

libmms_la_LDFLAGS = -version-info 0:2:0 -no-undefined -export-dynamic
libmms_la_LIBADD = $(GLIB_LIBS)

libmmsincdir = $(includedir)/libmms

libmmsinc_HEADERS = 	\
	mms.h		\
	mms_config.h	\
	bswap.h		\
	mmsio.h		\
	mmsx.h		\
	mmsh.h

INCLUDES =				\
	-I$(top_srcdir) 		\
	-I$(top_builddir) 		\
	$(MMS_PKG_CFLAGS)		\
	$(GLIB_CFLAGS)

DEPS = $(top_builddir)/src/libmms.la
LDADD = $(top_builddir)/src/libmms.la

# libmms_test_SOURCES = testconnect.c testdownload.c testswap.c
# libmms_test_LDADD = $(LDADD)
