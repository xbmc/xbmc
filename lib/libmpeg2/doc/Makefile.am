AM_CFLAGS = $(MPEG2DEC_CFLAGS)

libmpeg2 = $(top_builddir)/libmpeg2/libmpeg2.la
libmpeg2convert = $(top_builddir)/libmpeg2/convert/libmpeg2convert.la

noinst_PROGRAMS = sample1 sample2 sample3 sample4 sample5 sample6
sample1_SOURCES = sample1.c
sample1_LDADD = $(libmpeg2)
sample2_SOURCES = sample2.c
sample2_LDADD = $(libmpeg2) $(libmpeg2convert)
sample3_SOURCES = sample3.c
sample3_LDADD = $(libmpeg2)
sample4_SOURCES = sample4.c
sample4_LDADD = $(libmpeg2) $(libmpeg2convert)
sample5_SOURCES = sample5.c
sample5_LDADD = $(libmpeg2)
sample6_SOURCES = sample6.c
sample6_LDADD = $(libmpeg2) $(libmpeg2convert)

EXTRA_DIST = libmpeg2.txt
