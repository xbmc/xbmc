bin_PROGRAMS = adplugdb

adplugdb_SOURCES = adplugdb.cpp

EXTRA_adplugdb_SOURCES = getopt.c mygetopt.h

AM_LDFLAGS = $(top_builddir)/src/.libs/libadplug.la $(libbinio_LIBS) \
	$(GETOPT_SOURCES)

adplugdb_DEPENDENCIES = $(GETOPT_SOURCES)

adplug_data_dir = $(sharedstatedir)/adplug

AM_CPPFLAGS = -DADPLUG_DATA_DIR=\"$(adplug_data_dir)\" $(libbinio_CFLAGS)
